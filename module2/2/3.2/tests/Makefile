CC = gcc
CFLAGS = -Wall -Wextra -g -I. -I..

# Весь API ipnet.c публичный, поэтому файл линкуется как обычно,
# включать .c в тест не требуется. main.c в сборку не входит.
SRC    = unity/unity.c test_ipnet.c ../ipnet.c
TARGET = run_tests.exe

.PHONY: all test clean

all: test

$(TARGET): $(SRC) ../ipnet.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
