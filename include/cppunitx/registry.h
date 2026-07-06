#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

namespace cppunitx {

// ─────────────────────────────────────────────
// TestResult: outcome of a single test run
// ─────────────────────────────────────────────
enum class Status { PASS, FAIL, SKIP, TIMEOUT, CRASH };

struct TestResult {
    std::string suite_name;
    std::string test_name;
    Status      status       = Status::PASS;
    std::string failure_msg  = "";
    std::string file         = "";
    int         line         = 0;
    double      duration_ms  = 0.0;
};

// ─────────────────────────────────────────────
// TestCase: metadata + runnable for one test
// ─────────────────────────────────────────────
struct TestCase {
    std::string              suite_name;
    std::string              test_name;
    std::vector<std::string> tags;
    int                      timeout_ms = 5000;   // default 5s timeout
    std::function<void()>    fn;

    std::string full_name() const {
        return suite_name + "." + test_name;
    }
};

// ─────────────────────────────────────────────
// Registry: singleton that owns all TestCases
// ─────────────────────────────────────────────
class Registry {
public:
    static Registry& instance() {
        static Registry reg;
        return reg;
    }

    // Register a test — called by TEST() macro at static init time
    void add(TestCase tc) {
        tests_.push_back(std::move(tc));
    }

    // Returns all registered tests, optionally filtered
    std::vector<const TestCase*> get_tests(
        const std::string& filter = "",
        const std::string& tag    = "") const
    {
        std::vector<const TestCase*> result;
        for (const auto& tc : tests_) {
            if (!filter.empty() && !matches_filter(tc.full_name(), filter))
                continue;
            if (!tag.empty() && !has_tag(tc, tag))
                continue;
            result.push_back(&tc);
        }
        return result;
    }

    size_t total() const { return tests_.size(); }

private:
    Registry() = default;
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    std::vector<TestCase> tests_;

    static bool matches_filter(const std::string& full_name,
                                const std::string& pattern) {
        // Support wildcard '*' at end: "Math.*" matches "Math.Add", "Math.Sub"
        if (pattern.back() == '*') {
            std::string prefix = pattern.substr(0, pattern.size() - 1);
            return full_name.rfind(prefix, 0) == 0;
        }
        return full_name == pattern;
    }

    static bool has_tag(const TestCase& tc, const std::string& tag) {
        for (const auto& t : tc.tags)
            if (t == tag) return true;
        return false;
    }
};

// ─────────────────────────────────────────────
// Registrar: RAII helper used by TEST() macro
// ─────────────────────────────────────────────
struct Registrar {
    explicit Registrar(TestCase tc) {
        Registry::instance().add(std::move(tc));
    }
};

} // namespace cppunitx
