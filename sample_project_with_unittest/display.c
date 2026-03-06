/**
 * display.c - 출력 함수 구현
 */
#include "display.h"
#include <stdio.h>

void print_title(void) {
  printf("================================\n");
  printf("     간단한 계산기 예제\n");
  printf("================================\n\n");
}

void print_result(const char *operation, int a, int b, int result) {
  printf("%d %s %d = %d\n", a, operation, b, result);
}
