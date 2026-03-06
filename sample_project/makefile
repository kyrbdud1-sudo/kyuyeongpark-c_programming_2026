# makefile - 빌드 자동화 설정
#
# 사용법:
#   make        - 프로그램 빌드
#   make clean  - 빌드 산출물 삭제

CC = gcc
CFLAGS = -Wall -Wextra -g

# 소스 파일 목록
SRCS = main.c calc.c display.c

# .c를 .o로 치환 (main.o calc.o display.o)
OBJS = $(SRCS:.c=.o)

# 최종 실행 파일 (.out 확장자 사용)
TARGET = calculator.out

# 기본 타겟
all: $(TARGET)

# 링크: 모든 .o 파일을 합쳐서 실행파일 생성
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# 컴파일: 각 .c 파일을 .o로 컴파일
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 정리
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

