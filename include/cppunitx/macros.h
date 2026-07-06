#pragma once

#include "registry.h"
#include <string>
#include <vector>

// ─────────────────────────────────────────────
// Internal: unique variable name per line
// ─────────────────────────────────────────────
#define CPPUNITX_CONCAT_IMPL(a, b) a##b
#define CPPUNITX_CONCAT(a, b)      CPPUNITX_CONCAT_IMPL(a, b)
#define CPPUNITX_UNIQUE(base)      CPPUNITX_CONCAT(base, __LINE__)

// ─────────────────────────────────────────────
// TEST(Suite, Name) — define and register a test
//
// Usage:
//   TEST(MathTests, Addition) {
//       ASSERT_EQ(1 + 1, 2);
//   }
// ─────────────────────────────────────────────
#define TEST(Suite, Name)                                                        \
    static void CPPUNITX_CONCAT(_cppunitx_fn_, CPPUNITX_CONCAT(Suite, Name))(); \
                                                                                 \
    static cppunitx::Registrar CPPUNITX_UNIQUE(_cppunitx_reg_)(                 \
        cppunitx::TestCase{                                                      \
            #Suite, #Name, {}, 5000,                                             \
            &CPPUNITX_CONCAT(_cppunitx_fn_, CPPUNITX_CONCAT(Suite, Name))       \
        });                                                                      \
                                                                                 \
    static void CPPUNITX_CONCAT(_cppunitx_fn_, CPPUNITX_CONCAT(Suite, Name))()

// ─────────────────────────────────────────────
// TEST_TAGGED(Suite, Name, ...) — test with tags
//
// Usage:
//   TEST_TAGGED(NetTests, Connect, "slow", "integration") {
//       ...
//   }
// ─────────────────────────────────────────────
#define TEST_TAGGED(Suite, Name, ...)                                            \
    static void CPPUNITX_CONCAT(_cppunitx_fn_, CPPUNITX_CONCAT(Suite, Name))(); \
                                                                                 \
    static cppunitx::Registrar CPPUNITX_UNIQUE(_cppunitx_reg_)(                 \
        cppunitx::TestCase{                                                      \
            #Suite, #Name,                                                       \
            std::vector<std::string>{__VA_ARGS__},                              \
            5000,                                                                \
            &CPPUNITX_CONCAT(_cppunitx_fn_, CPPUNITX_CONCAT(Suite, Name))       \
        });                                                                      \
                                                                                 \
    static void CPPUNITX_CONCAT(_cppunitx_fn_, CPPUNITX_CONCAT(Suite, Name))()

// ─────────────────────────────────────────────
// TEST_TIMEOUT(Suite, Name, ms) — custom timeout
//
// Usage:
//   TEST_TIMEOUT(PerfTests, SortLargeArray, 200) { ... }
// ─────────────────────────────────────────────
#define TEST_TIMEOUT(Suite, Name, timeout_ms)                                    \
    static void CPPUNITX_CONCAT(_cppunitx_fn_, CPPUNITX_CONCAT(Suite, Name))(); \
                                                                                 \
    static cppunitx::Registrar CPPUNITX_UNIQUE(_cppunitx_reg_)(                 \
        cppunitx::TestCase{                                                      \
            #Suite, #Name, {}, (timeout_ms),                                    \
            &CPPUNITX_CONCAT(_cppunitx_fn_, CPPUNITX_CONCAT(Suite, Name))       \
        });                                                                      \
                                                                                 \
    static void CPPUNITX_CONCAT(_cppunitx_fn_, CPPUNITX_CONCAT(Suite, Name))()

// ─────────────────────────────────────────────
// PARAMETERIZED_TEST(Suite, Name, Type, values...)
//
// Generates one test per value. Each test receives
// the value as a local variable named `param`.
//
// Usage:
//   PARAMETERIZED_TEST(PrimeTests, IsPrime, int, 2, 3, 5, 7, 11) {
//       ASSERT_TRUE(is_prime(param));
//   }
// ─────────────────────────────────────────────
namespace cppunitx::detail {

template<typename T, typename Fn>
void register_parameterized(const std::string& suite,
                             const std::string& base_name,
                             const std::vector<T>& values,
                             Fn fn_factory)
{
    for (size_t i = 0; i < values.size(); ++i) {
        T val = values[i];
        std::string test_name = base_name + "/" + std::to_string(i);
        cppunitx::Registry::instance().add(cppunitx::TestCase{
            suite, test_name, {}, 5000,
            [fn_factory, val]() { fn_factory(val); }
        });
    }
}

} // namespace cppunitx::detail

#define PARAMETERIZED_TEST(Suite, Name, Type, ...)                              \
    static void CPPUNITX_CONCAT(_cppunitx_param_fn_,                           \
                                CPPUNITX_CONCAT(Suite, Name))(Type param);      \
                                                                                \
    struct CPPUNITX_UNIQUE(_cppunitx_param_reg_) {                             \
        CPPUNITX_UNIQUE(_cppunitx_param_reg_)() {                              \
            cppunitx::detail::register_parameterized<Type>(                    \
                #Suite, #Name,                                                  \
                std::vector<Type>{__VA_ARGS__},                                \
                &CPPUNITX_CONCAT(_cppunitx_param_fn_,                          \
                                 CPPUNITX_CONCAT(Suite, Name)));               \
        }                                                                       \
    };                                                                          \
    static CPPUNITX_UNIQUE(_cppunitx_param_reg_)                               \
        CPPUNITX_UNIQUE(_cppunitx_param_reg_inst_);                            \
                                                                               \
    static void CPPUNITX_CONCAT(_cppunitx_param_fn_,                          \
                                CPPUNITX_CONCAT(Suite, Name))(Type param)
