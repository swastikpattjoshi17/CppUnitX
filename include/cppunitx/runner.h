#pragma once

#include "registry.h"
#include "assertions.h"
#include "reporter.h"

#include <chrono>
#include <string>
#include <vector>
#include <iostream>

// POSIX headers for crash isolation & timeout
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

namespace cppunitx {

// ─────────────────────────────────────────────
// RunnerOptions: parsed from CLI arguments
// ─────────────────────────────────────────────
struct RunnerOptions {
    std::string filter  = "";      // e.g. "Math.*" or "Math.Add"
    std::string tag     = "";      // e.g. "slow"
    bool        no_fork = false;   // disable crash isolation (for debugging)
    std::string xml_out = "";      // path to write JUnit XML report
    std::string json_out= "";      // path to write JSON report
    bool        verbose = false;
    bool        list    = false;   // list tests and exit

    static RunnerOptions parse(int argc, char* argv[]) {
        RunnerOptions opts;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.rfind("--filter=", 0) == 0)
                opts.filter  = arg.substr(9);
            else if (arg.rfind("--tag=", 0) == 0)
                opts.tag     = arg.substr(6);
            else if (arg.rfind("--xml=", 0) == 0)
                opts.xml_out = arg.substr(6);
            else if (arg.rfind("--json=", 0) == 0)
                opts.json_out= arg.substr(7);
            else if (arg == "--no-fork")
                opts.no_fork = true;
            else if (arg == "--verbose" || arg == "-v")
                opts.verbose = true;
            else if (arg == "--list")
                opts.list    = true;
        }
        return opts;
    }
};

// ─────────────────────────────────────────────
// Runner: executes tests and collects results
// ─────────────────────────────────────────────
class Runner {
public:
    static int run(int argc, char* argv[]) {
        auto opts = RunnerOptions::parse(argc, argv);
        return Runner(opts).execute();
    }

private:
    RunnerOptions opts_;
    Reporter      reporter_;

    explicit Runner(RunnerOptions opts)
        : opts_(std::move(opts)), reporter_(opts_.verbose) {}

    int execute() {
        auto tests = Registry::instance().get_tests(opts_.filter, opts_.tag);

        if (opts_.list) {
            for (auto* tc : tests)
                std::cout << tc->full_name() << "\n";
            return 0;
        }

        reporter_.on_suite_start(tests.size());

        std::vector<TestResult> results;
        results.reserve(tests.size());

        for (auto* tc : tests) {
            TestResult r = opts_.no_fork
                           ? run_in_process(*tc)
                           : run_isolated(*tc);
            reporter_.on_test_done(r);
            results.push_back(std::move(r));
        }

        reporter_.on_suite_done(results);

        // Write reports
        if (!opts_.xml_out.empty())
            Reporter::write_xml(results, opts_.xml_out);
        if (!opts_.json_out.empty())
            Reporter::write_json(results, opts_.json_out);

        // Return non-zero exit code if any test failed
        for (const auto& r : results)
            if (r.status == Status::FAIL  ||
                r.status == Status::CRASH ||
                r.status == Status::TIMEOUT)
                return 1;
        return 0;
    }

    // ── In-process execution (no isolation) ───────────────────────────────
    TestResult run_in_process(const TestCase& tc) {
        TestResult r;
        r.suite_name = tc.suite_name;
        r.test_name  = tc.test_name;

        auto start = std::chrono::high_resolution_clock::now();
        try {
            tc.fn();
            r.status = Status::PASS;
        }
        catch (const SkipException& e) {
            r.status      = Status::SKIP;
            r.failure_msg = e.msg;
        }
        catch (const AssertionFailure& e) {
            r.status      = Status::FAIL;
            r.failure_msg = e.what();
            r.file        = e.file();
            r.line        = e.line();
        }
        catch (const std::exception& e) {
            r.status      = Status::FAIL;
            r.failure_msg = std::string("Unexpected exception: ") + e.what();
        }
        catch (...) {
            r.status      = Status::FAIL;
            r.failure_msg = "Unknown exception thrown";
        }
        auto end = std::chrono::high_resolution_clock::now();
        r.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        return r;
    }

    // ── Crash-isolated execution via fork() + SIGALRM timeout ─────────────
    //
    // Each test runs in a child process. The parent waits up to timeout_ms
    // and reads the result via a pipe. If the child crashes (SIGSEGV etc.)
    // or times out, the parent captures that and marks accordingly.
    //
    TestResult run_isolated(const TestCase& tc) {
        TestResult r;
        r.suite_name = tc.suite_name;
        r.test_name  = tc.test_name;

        // Pipe: child writes serialized result, parent reads
        int pipe_fds[2];
        if (pipe(pipe_fds) == -1) {
            r.status      = Status::FAIL;
            r.failure_msg = "pipe() failed — cannot isolate test";
            return r;
        }

        auto start = std::chrono::high_resolution_clock::now();
        pid_t child = fork();

        if (child == -1) {
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            r.status      = Status::FAIL;
            r.failure_msg = "fork() failed";
            return r;
        }

        if (child == 0) {
            // ── Child process ─────────────────────────────────────────
            close(pipe_fds[0]);  // close read end

            // Set timeout via SIGALRM
            int timeout_sec = std::max(1, tc.timeout_ms / 1000);
            alarm(static_cast<unsigned>(timeout_sec));

            // Run the test
            TestResult cr = run_in_process(tc);

            // Serialize minimal result through pipe
            // Format: status(1) | msg_len(4) | msg
            uint8_t status_byte = static_cast<uint8_t>(cr.status);
            write(pipe_fds[1], &status_byte, 1);

            uint32_t msg_len = static_cast<uint32_t>(cr.failure_msg.size());
            write(pipe_fds[1], &msg_len, 4);
            if (msg_len > 0)
                write(pipe_fds[1], cr.failure_msg.data(), msg_len);

            // Duration
            write(pipe_fds[1], &cr.duration_ms, sizeof(double));

            close(pipe_fds[1]);
            _exit(0);
        }

        // ── Parent process ─────────────────────────────────────────────
        close(pipe_fds[1]);  // close write end

        int wstatus = 0;
        pid_t waited = waitpid(child, &wstatus, 0);

        auto end = std::chrono::high_resolution_clock::now();
        r.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

        if (waited == -1) {
            r.status      = Status::FAIL;
            r.failure_msg = "waitpid() error";
            close(pipe_fds[0]);
            return r;
        }

        if (WIFSIGNALED(wstatus)) {
            int sig = WTERMSIG(wstatus);
            r.status      = (sig == SIGALRM) ? Status::TIMEOUT : Status::CRASH;
            r.failure_msg = (sig == SIGALRM)
                ? "Test timed out after " + std::to_string(tc.timeout_ms) + "ms"
                : "Test crashed with signal " + std::to_string(sig) +
                  " (" + signal_name(sig) + ")";
            close(pipe_fds[0]);
            return r;
        }

        // Child exited normally — read result from pipe
        uint8_t status_byte = 0;
        if (read(pipe_fds[0], &status_byte, 1) == 1) {
            r.status = static_cast<Status>(status_byte);

            uint32_t msg_len = 0;
            if (read(pipe_fds[0], &msg_len, 4) == 4 && msg_len > 0) {
                r.failure_msg.resize(msg_len);
                read(pipe_fds[0], r.failure_msg.data(), msg_len);
            }

            double child_dur = 0.0;
            if (read(pipe_fds[0], &child_dur, sizeof(double)) == sizeof(double))
                r.duration_ms = child_dur;
        }

        close(pipe_fds[0]);
        return r;
    }

    static const char* signal_name(int sig) {
        switch (sig) {
            case SIGSEGV: return "SIGSEGV - Segmentation Fault";
            case SIGABRT: return "SIGABRT - Abort";
            case SIGFPE:  return "SIGFPE - Floating Point Exception";
            case SIGBUS:  return "SIGBUS - Bus Error";
            case SIGILL:  return "SIGILL - Illegal Instruction";
            case SIGALRM: return "SIGALRM - Timeout";
            default:      return "Unknown Signal";
        }
    }
};

} // namespace cppunitx
