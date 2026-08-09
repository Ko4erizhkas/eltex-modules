CC = gcc
CFLAGS = -Wall -Wextra -g -I. -I..

SRC    = unity/unity.c test_phonebook.c
TARGET = run_tests.exe
LIB    = ../libphonebook.a

.PHONY: all test clean

all: test

# Библиотеку собирает Makefile задания - тест только линкуется с ней
$(LIB):
	$(MAKE) -C .. libphonebook.a

$(TARGET): $(SRC) $(LIB) ../phoneBook.h
	$(CC) $(CFLAGS) $(SRC) -L.. -lphonebook -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
