CC = gcc

# -Wno-unused-function: calc.c включается целиком, часть static-функций
# в отдельных тестах не вызывается, и без этого флага gcc заваливает вывод
# warning'ами.
# -DUNITY_INCLUDE_CONFIG_H: подключает unity_config.h, который уводит отчёт
# Unity в stderr (stdout на время тестов перехватывается).
CFLAGS = -Wall -Wextra -Wno-unused-function -g -I. -I.. -DUNITY_INCLUDE_CONFIG_H
LDLIBS = -lm

SRC    = unity/unity.c test_calc.c
TARGET = run_tests.exe

.PHONY: all test clean

all: test

$(TARGET): $(SRC) ../calc.c ../calc.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) stdin_fixture.tmp stdout_capture.tmp
