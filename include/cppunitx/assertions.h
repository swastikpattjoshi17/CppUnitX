#pragma once

#include <string>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <cmath>

namespace cppunitx {

// ─────────────────────────────────────────────
// AssertionFailure: thrown on any ASSERT_* fail
// ─────────────────────────────────────────────
class AssertionFailure : public std::exception {
public:
    AssertionFailure(const std::string& msg,
                     const std::string& file,
                     int line)
        : msg_(msg), file_(file), line_(line)
    {
        std::ostringstream oss;
        oss << file_ << ":" << line_ << "\n  " << msg_;
        full_msg_ = oss.str();
    }

    const char* what()       const noexcept override { return full_msg_.c_str(); }
    const std::string& msg() const noexcept          { return msg_; }
    const std::string& file()const noexcept          { return file_; }
    int                line()const noexcept          { return line_; }

private:
    std::string msg_, file_, full_msg_;
    int line_;
};

// ─────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────
namespace detail {

template<typename T>
std::string to_str(const T& v) {
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

template<>
inline std::string to_str<bool>(const bool& v) {
    return v ? "true" : "false";
}

inline void fail(const std::string& msg, const char* file, int line) {
    throw AssertionFailure(msg, file, line);
}

} // namespace detail

} // namespace cppunitx

// ─────────────────────────────────────────────
// ASSERT macros — use these in your tests
// ─────────────────────────────────────────────

/// ASSERT_TRUE(expr) — fails if expr is false
#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_TRUE failed: (" #expr ") evaluated to false"; \
            cppunitx::detail::fail(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_FALSE(expr) — fails if expr is true
#define ASSERT_FALSE(expr) \
    do { \
        if ((expr)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_FALSE failed: (" #expr ") evaluated to true"; \
            cppunitx::detail::fail(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_EQ(a, b) — fails if a != b
#define ASSERT_EQ(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a == _b)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_EQ failed:\n" \
                 << "    left  (" #a "): " << cppunitx::detail::to_str(_a) << "\n" \
                 << "    right (" #b "): " << cppunitx::detail::to_str(_b); \
            cppunitx::detail::fail(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_NE(a, b) — fails if a == b
#define ASSERT_NE(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (_a == _b) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_NE failed: both sides equal " \
                 << cppunitx::detail::to_str(_a); \
            cppunitx::detail::fail(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_LT(a, b) — fails if a >= b
#define ASSERT_LT(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a < _b)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_LT failed: " \
                 << cppunitx::detail::to_str(_a) << " >= " \
                 << cppunitx::detail::to_str(_b); \
            cppunitx::detail::fail(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_LE(a, b) — fails if a > b
#define ASSERT_LE(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a <= _b)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_LE failed: " \
                 << cppunitx::detail::to_str(_a) << " > " \
                 << cppunitx::detail::to_str(_b); \
            cppunitx::detail::fail(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_GT(a, b) — fails if a <= b
#define ASSERT_GT(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a > _b)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_GT failed: " \
                 << cppunitx::detail::to_str(_a) << " <= " \
                 << cppunitx::detail::to_str(_b); \
            cppunitx::detail::fail(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_GE(a, b) — fails if a < b
#define ASSERT_GE(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a >= _b)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_GE failed: " \
                 << cppunitx::detail::to_str(_a) << " < " \
                 << cppunitx::detail::to_str(_b); \
            cppunitx::detail::fail(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_NEAR(a, b, eps) — for floating-point comparisons
#define ASSERT_NEAR(a, b, eps) \
    do { \
        double _a = (a), _b = (b), _e = (eps); \
        if (std::abs(_a - _b) > _e) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_NEAR failed: |" << _a << " - " << _b \
                 << "| = " << std::abs(_a-_b) << " > eps=" << _e; \
            cppunitx::detail::fail(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_NULL(ptr) — fails if ptr != nullptr
#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != nullptr) { \
            cppunitx::detail::fail( \
                "ASSERT_NULL failed: " #ptr " is not null", __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_NOT_NULL(ptr) — fails if ptr == nullptr
#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            cppunitx::detail::fail( \
                "ASSERT_NOT_NULL failed: " #ptr " is null", __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_THROWS(expr, ExceptionType) — fails if expr does NOT throw ExceptionType
#define ASSERT_THROWS(expr, ExceptionType) \
    do { \
        bool _threw = false; \
        try { (expr); } \
        catch (const ExceptionType&) { _threw = true; } \
        catch (...) { \
            cppunitx::detail::fail( \
                "ASSERT_THROWS: wrong exception type thrown for (" #expr ")", \
                __FILE__, __LINE__); \
        } \
        if (!_threw) { \
            cppunitx::detail::fail( \
                "ASSERT_THROWS: no exception thrown for (" #expr ")", \
                __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_NO_THROW(expr) — fails if expr throws anything
#define ASSERT_NO_THROW(expr) \
    do { \
        try { (expr); } \
        catch (const std::exception& _e) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_NO_THROW: unexpected exception: " << _e.what(); \
            cppunitx::detail::fail(_oss.str(), __FILE__, __LINE__); \
        } \
        catch (...) { \
            cppunitx::detail::fail( \
                "ASSERT_NO_THROW: unexpected unknown exception", __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_STR_EQ(a, b) — string equality with clear diff output
#define ASSERT_STR_EQ(a, b) \
    do { \
        std::string _a = (a), _b = (b); \
        if (_a != _b) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_STR_EQ failed:\n" \
                 << "    expected: \"" << _b << "\"\n" \
                 << "    actual  : \"" << _a << "\""; \
            cppunitx::detail::fail(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

/// ASSERT_CONTAINS(haystack, needle) — string substring check
#define ASSERT_CONTAINS(haystack, needle) \
    do { \
        std::string _h = (haystack), _n = (needle); \
        if (_h.find(_n) == std::string::npos) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_CONTAINS failed:\n" \
                 << "    \"" << _h << "\" does not contain \"" << _n << "\""; \
            cppunitx::detail::fail(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

/// SKIP(msg) — skip this test with a message (no failure)
#define SKIP(msg) \
    throw cppunitx::SkipException(msg, __FILE__, __LINE__)

namespace cppunitx {
struct SkipException : public std::exception {
    std::string msg;
    SkipException(const std::string& m, const char*, int) : msg(m) {}
    const char* what() const noexcept override { return msg.c_str(); }
};
} // namespace cppunitx
