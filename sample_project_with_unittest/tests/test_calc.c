/**
 * test_calc.c - calc.c 함수들의 Unity 테스트
 */
#include "../calc.h"
#include "unity.h"

/* 테스트 전후 실행 (필수 정의) */
void setUp(void) { /* 각 테스트 전에 실행할 코드 */ }

void tearDown(void) { /* 각 테스트 후에 실행할 코드 */ }

/* add 함수 테스트 */
void test_add_positive_numbers(void) {
  TEST_ASSERT_EQUAL(5, add(2, 3));
}

void test_add_with_zero(void) {
  TEST_ASSERT_EQUAL(0, add(0, 0));
}

void test_add_negative_numbers(void) {
  TEST_ASSERT_EQUAL(-1, add(-5, 4));
}

void test_add_large_numbers(void) {
  TEST_ASSERT_EQUAL(100, add(50, 50));
}

/* subtract 함수 테스트 */
void test_subtract_positive_numbers(void) {
  TEST_ASSERT_EQUAL(1, subtract(3, 2));
}

void test_subtract_same_numbers(void) {
  TEST_ASSERT_EQUAL(0, subtract(5, 5));
}

void test_subtract_negative_result(void) {
  TEST_ASSERT_EQUAL(-9, subtract(-5, 4));
}

/* multiply 함수 테스트 */
void test_multiply_positive_numbers(void) {
  TEST_ASSERT_EQUAL(6, multiply(2, 3));
}

void test_multiply_with_zero(void) {
  TEST_ASSERT_EQUAL(0, multiply(0, 100));
}

void test_multiply_negative_numbers(void) {
  TEST_ASSERT_EQUAL(-12, multiply(-3, 4));
}

void test_multiply_same_numbers(void) {
  TEST_ASSERT_EQUAL(25, multiply(5, 5));
}

/* 메인: 테스트 실행 */
int main(void) {
  UNITY_BEGIN();

  /* add 테스트 */
  RUN_TEST(test_add_positive_numbers);
  RUN_TEST(test_add_with_zero);
  RUN_TEST(test_add_negative_numbers);
  RUN_TEST(test_add_large_numbers);

  /* subtract 테스트 */
  RUN_TEST(test_subtract_positive_numbers);
  RUN_TEST(test_subtract_same_numbers);
  RUN_TEST(test_subtract_negative_result);

  /* multiply 테스트 */
  RUN_TEST(test_multiply_positive_numbers);
  RUN_TEST(test_multiply_with_zero);
  RUN_TEST(test_multiply_negative_numbers);
  RUN_TEST(test_multiply_same_numbers);

  return UNITY_END();
}
