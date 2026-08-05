CC = gcc

# -Wno-unused-function: phoneBook.c включается целиком, часть static-функций
# в тестах не вызывается, и без этого флага gcc заваливает вывод warning'ами.
# -DUNITY_INCLUDE_CONFIG_H: подключает unity_config.h, который уводит отчёт
# Unity в stderr (stdout на время тестов заглушен).
CFLAGS = -Wall -Wextra -Wno-unused-function -g -I. -I.. -DUNITY_INCLUDE_CONFIG_H

SRC    = unity/unity.c test_phoneBook.c
TARGET = run_tests.exe

.PHONY: all test clean

all: test

$(TARGET): $(SRC) ../phoneBook.c ../phoneBook.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) stdin_fixture.tmp
