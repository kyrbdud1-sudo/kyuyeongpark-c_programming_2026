# makefile - 빌드 자동화 설정
#
# 사용법:
#   make        - 프로그램 빌드
#   make test   - 테스트 실행
#   make clean  - 빌드 산출물 삭제

CC = gcc
CFLAGS = -Wall -Wextra -g

# 최종 실행 파일 (Code Runner가 읽음)
TARGET = sample.out

# 프로그램 소스 (main.c + sample.c)
PROG_SRCS = main.c sample.c
PROG_OBJS = $(PROG_SRCS:.c=.o)

# 테스트 설정
TEST_DIR = tests
TEST_TARGET = $(TEST_DIR)/test_sample.out

# 기본 타겟: 프로그램 빌드
all: $(TARGET)

# 프로그램 빌드
$(TARGET): $(PROG_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# 테스트 빌드 및 실행
test: $(TEST_TARGET)
	@echo "\n🧪 Unity 테스트 실행 중...\n"
	./$(TEST_TARGET)

# 테스트 실행파일 빌드
$(TEST_TARGET): $(TEST_DIR)/test_sample.c sample.c
	$(CC) $(CFLAGS) -o $@ $^

# 컴파일: 각 .c 파일을 .o로 컴파일
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 정리
clean:
	rm -f *.o $(TARGET) $(TEST_TARGET)

.PHONY: all test clean
