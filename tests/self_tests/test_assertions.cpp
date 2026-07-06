#include <cppunitx/cppunitx.h>
#include <stdexcept>
#include <string>

// ─────────────────────────────────────────────
// Tests for ASSERT_EQ / ASSERT_NE
// ─────────────────────────────────────────────

TEST(AssertEQ, IntegersEqual) {
    ASSERT_EQ(1 + 1, 2);
}

TEST(AssertEQ, StringsEqual) {
    std::string s = "hello";
    ASSERT_EQ(s, std::string("hello"));
}

TEST(AssertEQ, FloatsTruncated) {
    int a = 42, b = 42;
    ASSERT_EQ(a, b);
}

TEST(AssertNE, IntegersDiffer) {
    ASSERT_NE(1, 2);
}

// ─────────────────────────────────────────────
// Tests for ASSERT_LT / GT / LE / GE
// ─────────────────────────────────────────────

TEST(Ordering, LessThan) {
    ASSERT_LT(3, 5);
}

TEST(Ordering, GreaterThan) {
    ASSERT_GT(10, 7);
}

TEST(Ordering, LessOrEqual) {
    ASSERT_LE(5, 5);
    ASSERT_LE(4, 5);
}

TEST(Ordering, GreaterOrEqual) {
    ASSERT_GE(5, 5);
    ASSERT_GE(6, 5);
}

// ─────────────────────────────────────────────
// Tests for ASSERT_NEAR
// ─────────────────────────────────────────────

TEST(FloatAssert, NearEqual) {
    ASSERT_NEAR(3.14159, 3.14158, 0.0001);
}

TEST(FloatAssert, ExactEqual) {
    ASSERT_NEAR(1.0, 1.0, 0.0);
}

// ─────────────────────────────────────────────
// Tests for ASSERT_TRUE / FALSE
// ─────────────────────────────────────────────

TEST(BoolAssert, TrueExpression) {
    ASSERT_TRUE(2 > 1);
    ASSERT_TRUE(!false);
}

TEST(BoolAssert, FalseExpression) {
    ASSERT_FALSE(1 > 2);
    ASSERT_FALSE(false);
}

// ─────────────────────────────────────────────
// Tests for ASSERT_THROWS / NO_THROW
// ─────────────────────────────────────────────

TEST(ExceptionAssert, ThrowsExpected) {
    ASSERT_THROWS(throw std::runtime_error("oops"), std::runtime_error);
}

TEST(ExceptionAssert, ThrowsBaseClass) {
    // std::runtime_error is-a std::exception
    ASSERT_THROWS(throw std::runtime_error("x"), std::exception);
}

TEST(ExceptionAssert, NoThrowCleanCode) {
    ASSERT_NO_THROW(int x = 1 + 1; (void)x;);
}

// ─────────────────────────────────────────────
// Tests for NULL / NOT_NULL
// ─────────────────────────────────────────────

TEST(NullAssert, NullPointer) {
    int* p = nullptr;
    ASSERT_NULL(p);
}

TEST(NullAssert, ValidPointer) {
    int x = 5;
    ASSERT_NOT_NULL(&x);
}

// ─────────────────────────────────────────────
// Tests for string assertions
// ─────────────────────────────────────────────

TEST(StringAssert, StrEqualSame) {
    ASSERT_STR_EQ(std::string("abc"), std::string("abc"));
}

TEST(StringAssert, Contains) {
    ASSERT_CONTAINS(std::string("hello world"), "world");
}

// ─────────────────────────────────────────────
// Tests for SKIP
// ─────────────────────────────────────────────

TEST(SkipTest, PlatformSpecific) {
    SKIP("Not supported on this platform");
    ASSERT_TRUE(false);  // should never reach here
}

// ─────────────────────────────────────────────
// Tests for PARAMETERIZED_TEST
// ─────────────────────────────────────────────

static bool is_positive(int n) { return n > 0; }

PARAMETERIZED_TEST(ParamTest, AllPositive, int, 1, 2, 3, 100, 999) {
    ASSERT_TRUE(is_positive(param));
}

static bool is_even(int n) { return n % 2 == 0; }

PARAMETERIZED_TEST(ParamTest, AllEven, int, 0, 2, 4, 100, 1024) {
    ASSERT_TRUE(is_even(param));
}
