/**
 * sample.c - 별(*)로 산 모양 출력하기
 *
 * 함수 구현 파일 (main 없음, 테스트 가능)
 */
#include "sample.h"
#include <stdio.h>

void print_mountain(int height) {
  printf("=== 별로 산 만들기 ===\n\n");

  for (int i = 1; i <= height; i++) {
    // 공백 출력 (오른쪽 정렬용)
    for (int j = 0; j < height - i; j++) {
      printf(" ");
    }
    // 별 출력 (홀수 개씩: 1, 3, 5, 7, 9...)
    for (int k = 0; k < 2 * i - 1; k++) {
      printf("*");
    }
    printf("\n");
  }
}

int get_star_count(int row, int height) {
  if (row < 1 || row > height) {
    return 0;
  }
  return 2 * row - 1; // 홀수 개: 1, 3, 5, 7, 9...
}
