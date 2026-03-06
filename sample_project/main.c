/**
 * main.c - 메인 함수
 *
 * 프로그램의 진입점(entry point)
 * 다른 모듈의 함수들을 사용하여 프로그램 실행
 */
#include "calc.h"    // 계산 함수들
#include "display.h" // 출력 함수들
#include <stdio.h>

int main() {
  int a = 10;
  int b = 3;

  // 타이틀 출력
  print_title();

  // 계산 및 결과 출력
  print_result("+", a, b, add(a, b));
  print_result("-", a, b, subtract(a, b));
  print_result("*", a, b, multiply(a, b));

  printf("\n");
  return 0;
}
