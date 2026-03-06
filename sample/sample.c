/**
 * sample.c - 별(*)로 산 모양 출력하기
 *
 * 간단한 for loop 예제
 */
#include <stdio.h>

int main() {
  int height = 5; // 산의 높이

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

  return 0;
}
