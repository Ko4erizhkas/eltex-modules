CC = gcc

# -DUNITY_INCLUDE_CONFIG_H: подключает unity_config.h, который уводит отчёт
# Unity в stderr (stdout на время тестов перехватывается).
CFLAGS = -Wall -Wextra -g -I. -I.. -DUNITY_INCLUDE_CONFIG_H

# Весь API perm.c публичный, поэтому файл линкуется как обычно,
# включать .c в тест не требуется. main.c в сборку не входит.
SRC    = unity/unity.c test_perm.c ../perm.c
TARGET = run_tests.exe

.PHONY: all test clean

all: test

$(TARGET): $(SRC) ../perm.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) stdin_fixture.tmp stdout_capture.tmp perm_fixture.tmp
