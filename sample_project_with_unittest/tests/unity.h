/**
 * unity.h - Unity Test Framework (Lightweight Version)
 *
 * Unity 스타일의 경량 테스트 프레임워크
 * 실제 Unity: https://github.com/ThrowTheSwitch/Unity
 */
#ifndef UNITY_H
#define UNITY_H

#include <stdio.h>
#include <stdlib.h>

/* Unity 내부 상태 */
static int unity_tests_run __attribute__((unused)) = 0;
static int unity_tests_passed __attribute__((unused)) = 0;
static int unity_tests_failed __attribute__((unused)) = 0;
static const char *unity_current_test __attribute__((unused)) = "";

/* 색상 출력 */
#define UNITY_COLOR_RED "\033[31m"
#define UNITY_COLOR_GREEN "\033[32m"
#define UNITY_COLOR_RESET "\033[0m"

/* setUp/tearDown 선언 (사용자가 정의) */
void setUp(void);
void tearDown(void);

/* Unity 시작/종료 */
#define UNITY_BEGIN()       \
  do {                      \
    unity_tests_run = 0;    \
    unity_tests_passed = 0; \
    unity_tests_failed = 0; \
    printf("\n");           \
  } while (0)

#define UNITY_END()                                                        \
  ({                                                                       \
    printf("\n-----------------------\n");                                 \
    printf("%d Tests %d Failures\n", unity_tests_run, unity_tests_failed); \
    if (unity_tests_failed == 0) {                                         \
      printf(UNITY_COLOR_GREEN "OK" UNITY_COLOR_RESET "\n");               \
    } else {                                                               \
      printf(UNITY_COLOR_RED "FAIL" UNITY_COLOR_RESET "\n");               \
    }                                                                      \
    unity_tests_failed;                                                    \
  })

/* 테스트 실행 매크로 */
#define RUN_TEST(func)          \
  do {                          \
    unity_current_test = #func; \
    setUp();                    \
    func();                     \
    tearDown();                 \
    unity_tests_run++;          \
  } while (0)

/* Assert 매크로들 */
#define TEST_ASSERT_EQUAL_INT(expected, actual)                           \
  do {                                                                    \
    int _e = (expected);                                                  \
    int _a = (actual);                                                    \
    if (_e != _a) {                                                       \
      printf("%s:%d:%s:" UNITY_COLOR_RED "FAIL" UNITY_COLOR_RESET         \
             ": Expected %d Was %d\n",                                    \
             __FILE__, __LINE__, unity_current_test, _e, _a);             \
      unity_tests_failed++;                                               \
      return;                                                             \
    } else {                                                              \
      unity_tests_passed++;                                               \
      printf("%s:%d:%s:" UNITY_COLOR_GREEN "PASS" UNITY_COLOR_RESET "\n", \
             __FILE__, __LINE__, unity_current_test);                     \
    }                                                                     \
  } while (0)

#define TEST_ASSERT_EQUAL(expected, actual) \
  TEST_ASSERT_EQUAL_INT(expected, actual)

#define TEST_ASSERT_TRUE(condition)                                       \
  do {                                                                    \
    if (!(condition)) {                                                   \
      printf("%s:%d:%s:" UNITY_COLOR_RED "FAIL" UNITY_COLOR_RESET         \
             ": Expression was FALSE\n",                                  \
             __FILE__, __LINE__, unity_current_test);                     \
      unity_tests_failed++;                                               \
      return;                                                             \
    } else {                                                              \
      unity_tests_passed++;                                               \
      printf("%s:%d:%s:" UNITY_COLOR_GREEN "PASS" UNITY_COLOR_RESET "\n", \
             __FILE__, __LINE__, unity_current_test);                     \
    }                                                                     \
  } while (0)

#define TEST_ASSERT_FALSE(condition) TEST_ASSERT_TRUE(!(condition))

#define TEST_ASSERT_NULL(pointer)                                         \
  do {                                                                    \
    if ((pointer) != NULL) {                                              \
      printf("%s:%d:%s:" UNITY_COLOR_RED "FAIL" UNITY_COLOR_RESET         \
             ": Expected NULL\n",                                         \
             __FILE__, __LINE__, unity_current_test);                     \
      unity_tests_failed++;                                               \
      return;                                                             \
    } else {                                                              \
      unity_tests_passed++;                                               \
      printf("%s:%d:%s:" UNITY_COLOR_GREEN "PASS" UNITY_COLOR_RESET "\n", \
             __FILE__, __LINE__, unity_current_test);                     \
    }                                                                     \
  } while (0)

#define TEST_ASSERT_NOT_NULL(pointer)                                     \
  do {                                                                    \
    if ((pointer) == NULL) {                                              \
      printf("%s:%d:%s:" UNITY_COLOR_RED "FAIL" UNITY_COLOR_RESET         \
             ": Expected Not NULL\n",                                     \
             __FILE__, __LINE__, unity_current_test);                     \
      unity_tests_failed++;                                               \
      return;                                                             \
    } else {                                                              \
      unity_tests_passed++;                                               \
      printf("%s:%d:%s:" UNITY_COLOR_GREEN "PASS" UNITY_COLOR_RESET "\n", \
             __FILE__, __LINE__, unity_current_test);                     \
    }                                                                     \
  } while (0)

#endif /* UNITY_H */
