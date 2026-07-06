#pragma once

#include "registry.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <ctime>

namespace cppunitx {

// ─────────────────────────────────────────────
// ANSI color codes (disabled on non-TTY)
// ─────────────────────────────────────────────
namespace color {
    inline bool enabled = true;
    inline const char* RESET  = "\033[0m";
    inline const char* GREEN  = "\033[32m";
    inline const char* RED    = "\033[31m";
    inline const char* YELLOW = "\033[33m";
    inline const char* CYAN   = "\033[36m";
    inline const char* BOLD   = "\033[1m";
    inline const char* DIM    = "\033[2m";

    inline std::string apply(const char* code, const std::string& s) {
        if (!enabled) return s;
        return std::string(code) + s + RESET;
    }
}

// ─────────────────────────────────────────────
// Reporter: handles all terminal and file output
// ─────────────────────────────────────────────
class Reporter {
public:
    explicit Reporter(bool verbose = false) : verbose_(verbose) {
        // Disable colors if stdout is not a TTY
        color::enabled = isatty(STDOUT_FILENO);
    }

    void on_suite_start(size_t total) {
        std::cout << color::apply(color::BOLD, "\n══════════════════════════════════════\n");
        std::cout << color::apply(color::BOLD, "  CppUnit-X Test Runner\n");
        std::cout << color::apply(color::BOLD, "══════════════════════════════════════\n");
        std::cout << color::apply(color::DIM,
            "  Running " + std::to_string(total) + " test(s)\n\n");
    }

    void on_test_done(const TestResult& r) {
        std::string badge, name_colored;

        switch (r.status) {
            case Status::PASS:
                badge = color::apply(color::GREEN, "[ PASS ]");
                break;
            case Status::FAIL:
                badge = color::apply(color::RED,   "[ FAIL ]");
                break;
            case Status::SKIP:
                badge = color::apply(color::YELLOW,"[ SKIP ]");
                break;
            case Status::TIMEOUT:
                badge = color::apply(color::RED,   "[ TIME ]");
                break;
            case Status::CRASH:
                badge = color::apply(color::RED,   "[CRASH ]");
                break;
        }

        std::ostringstream line;
        line << badge << "  "
             << color::apply(color::CYAN, r.suite_name + "." + r.test_name)
             << "  "
             << color::apply(color::DIM,
                             "(" + format_duration(r.duration_ms) + ")");
        std::cout << line.str() << "\n";

        if (r.status != Status::PASS && !r.failure_msg.empty()) {
            // Indent failure message
            std::istringstream ss(r.failure_msg);
            std::string msg_line;
            while (std::getline(ss, msg_line)) {
                std::cout << "         "
                          << color::apply(color::RED, msg_line) << "\n";
            }
            std::cout << "\n";
        }
    }

    void on_suite_done(const std::vector<TestResult>& results) {
        int pass = 0, fail = 0, skip = 0, crash = 0, timeout = 0;
        double total_ms = 0.0;
        for (const auto& r : results) {
            total_ms += r.duration_ms;
            switch (r.status) {
                case Status::PASS:    ++pass;    break;
                case Status::FAIL:    ++fail;    break;
                case Status::SKIP:    ++skip;    break;
                case Status::CRASH:   ++crash;   break;
                case Status::TIMEOUT: ++timeout; break;
            }
        }

        std::cout << "\n";
        std::cout << color::apply(color::BOLD,
            "══════════════════════════════════════\n");
        std::cout << "  Results:  ";
        std::cout << color::apply(color::GREEN,
            std::to_string(pass) + " passed");
        std::cout << "  |  ";
        if (fail + crash + timeout > 0)
            std::cout << color::apply(color::RED,
                std::to_string(fail + crash + timeout) + " failed");
        else
            std::cout << color::apply(color::DIM, "0 failed");
        if (skip > 0)
            std::cout << "  |  "
                      << color::apply(color::YELLOW,
                          std::to_string(skip) + " skipped");
        std::cout << "\n";
        std::cout << "  Total time: "
                  << color::apply(color::DIM, format_duration(total_ms))
                  << "\n";
        std::cout << color::apply(color::BOLD,
            "══════════════════════════════════════\n\n");

        // Print failures summary at end
        bool any_fail = false;
        for (const auto& r : results) {
            if (r.status == Status::FAIL ||
                r.status == Status::CRASH ||
                r.status == Status::TIMEOUT) {
                if (!any_fail) {
                    std::cout << color::apply(color::RED | 0,
                        "Failed tests:\n");
                    any_fail = true;
                }
                std::cout << "  ✗ " << r.suite_name << "." << r.test_name << "\n";
                if (!r.failure_msg.empty())
                    std::cout << "    " << r.failure_msg << "\n";
            }
        }
    }

    // ── JUnit-compatible XML (for CI/CD: GitHub Actions, Jenkins) ──────────
    static void write_xml(const std::vector<TestResult>& results,
                          const std::string& path)
    {
        std::ofstream f(path);
        if (!f) {
            std::cerr << "[CppUnit-X] Warning: cannot write XML to " << path << "\n";
            return;
        }

        int failures = 0, errors = 0, skipped = 0;
        double total_time = 0.0;
        for (const auto& r : results) {
            if (r.status == Status::FAIL) ++failures;
            if (r.status == Status::CRASH || r.status == Status::TIMEOUT) ++errors;
            if (r.status == Status::SKIP) ++skipped;
            total_time += r.duration_ms;
        }

        f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        f << "<testsuites tests=\"" << results.size()
          << "\" failures=\"" << failures
          << "\" errors=\"" << errors
          << "\" skipped=\"" << skipped
          << "\" time=\"" << std::fixed << std::setprecision(3)
          << total_time / 1000.0 << "\">\n";

        // Group by suite
        std::string current_suite = "";
        for (const auto& r : results) {
            if (r.suite_name != current_suite) {
                if (!current_suite.empty()) f << "  </testsuite>\n";
                current_suite = r.suite_name;
                f << "  <testsuite name=\"" << xml_escape(r.suite_name) << "\">\n";
            }

            f << "    <testcase name=\"" << xml_escape(r.test_name)
              << "\" classname=\"" << xml_escape(r.suite_name)
              << "\" time=\"" << std::fixed << std::setprecision(3)
              << r.duration_ms / 1000.0 << "\"";

            if (r.status == Status::PASS) {
                f << "/>\n";
            } else if (r.status == Status::SKIP) {
                f << ">\n      <skipped message=\"" << xml_escape(r.failure_msg)
                  << "\"/>\n    </testcase>\n";
            } else if (r.status == Status::CRASH || r.status == Status::TIMEOUT) {
                f << ">\n      <error message=\"" << xml_escape(r.failure_msg)
                  << "\"/>\n    </testcase>\n";
            } else {
                f << ">\n      <failure message=\"" << xml_escape(r.failure_msg)
                  << "\"/>\n    </testcase>\n";
            }
        }
        if (!current_suite.empty()) f << "  </testsuite>\n";
        f << "</testsuites>\n";

        std::cout << "[CppUnit-X] JUnit XML written to: " << path << "\n";
    }

    // ── JSON report ─────────────────────────────────────────────────────────
    static void write_json(const std::vector<TestResult>& results,
                           const std::string& path)
    {
        std::ofstream f(path);
        if (!f) {
            std::cerr << "[CppUnit-X] Warning: cannot write JSON to " << path << "\n";
            return;
        }

        auto status_str = [](Status s) -> std::string {
            switch (s) {
                case Status::PASS:    return "PASS";
                case Status::FAIL:    return "FAIL";
                case Status::SKIP:    return "SKIP";
                case Status::TIMEOUT: return "TIMEOUT";
                case Status::CRASH:   return "CRASH";
            }
            return "UNKNOWN";
        };

        f << "{\n  \"tests\": [\n";
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& r = results[i];
            f << "    {\n"
              << "      \"suite\": \""   << json_escape(r.suite_name)  << "\",\n"
              << "      \"name\": \""    << json_escape(r.test_name)   << "\",\n"
              << "      \"status\": \""  << status_str(r.status)       << "\",\n"
              << "      \"duration_ms\": " << std::fixed << std::setprecision(2)
              << r.duration_ms;
            if (!r.failure_msg.empty())
                f << ",\n      \"message\": \"" << json_escape(r.failure_msg) << "\"";
            f << "\n    }" << (i + 1 < results.size() ? "," : "") << "\n";
        }
        f << "  ]\n}\n";

        std::cout << "[CppUnit-X] JSON report written to: " << path << "\n";
    }

private:
    bool verbose_;

    static std::string format_duration(double ms) {
        if (ms < 1.0)   return std::to_string(static_cast<int>(ms * 1000)) + "μs";
        if (ms < 1000)  return std::to_string(static_cast<int>(ms)) + "ms";
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << ms / 1000.0 << "s";
        return oss.str();
    }

    static std::string xml_escape(const std::string& s) {
        std::string r;
        for (char c : s) {
            switch (c) {
                case '&':  r += "&amp;";  break;
                case '<':  r += "&lt;";   break;
                case '>':  r += "&gt;";   break;
                case '"':  r += "&quot;"; break;
                case '\'': r += "&apos;"; break;
                default:   r += c;
            }
        }
        return r;
    }

    static std::string json_escape(const std::string& s) {
        std::string r;
        for (char c : s) {
            switch (c) {
                case '"':  r += "\\\""; break;
                case '\\': r += "\\\\"; break;
                case '\n': r += "\\n";  break;
                case '\t': r += "\\t";  break;
                default:   r += c;
            }
        }
        return r;
    }
};

} // namespace cppunitx
