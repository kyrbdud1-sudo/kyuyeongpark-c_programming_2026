/**
 * test_sample.c - sample.c 함수들의 Unity 테스트
 */
#include "../sample.h"
#include "unity.h"

/* 테스트 전후 실행 (필수 정의) */
void setUp(void) { /* 각 테스트 전에 실행할 코드 */ }

void tearDown(void) { /* 각 테스트 후에 실행할 코드 */ }

/* 테스트 함수들 */
void test_star_count_row_1(void) {
  TEST_ASSERT_EQUAL(1, get_star_count(1, 5));
}

void test_star_count_row_2(void) {
  TEST_ASSERT_EQUAL(3, get_star_count(2, 5));
}

void test_star_count_row_3(void) {
  TEST_ASSERT_EQUAL(5, get_star_count(3, 5));
}

void test_star_count_row_4(void) {
  TEST_ASSERT_EQUAL(7, get_star_count(4, 5));
}

void test_star_count_row_5(void) {
  TEST_ASSERT_EQUAL(9, get_star_count(5, 5));
}

void test_star_count_out_of_range_low(void) {
  TEST_ASSERT_EQUAL(0, get_star_count(0, 5));
}

void test_star_count_out_of_range_high(void) {
  TEST_ASSERT_EQUAL(0, get_star_count(6, 5));
}

void test_star_count_different_height(void) {
  TEST_ASSERT_EQUAL(1, get_star_count(1, 3));
  TEST_ASSERT_EQUAL(3, get_star_count(2, 3));
  TEST_ASSERT_EQUAL(5, get_star_count(3, 3));
}

/* 메인: 테스트 실행 */
int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_star_count_row_1);
  RUN_TEST(test_star_count_row_2);
  RUN_TEST(test_star_count_row_3);
  RUN_TEST(test_star_count_row_4);
  RUN_TEST(test_star_count_row_5);
  RUN_TEST(test_star_count_out_of_range_low);
  RUN_TEST(test_star_count_out_of_range_high);
  RUN_TEST(test_star_count_different_height);

  return UNITY_END();
}
