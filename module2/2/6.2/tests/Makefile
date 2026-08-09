CC = gcc
CFLAGS = -Wall -Wextra -g -I. -I..

ifeq ($(OS),Windows_NT)
LIB     = ../libphonebook.dll
LDFLAGS = -L.. -lphonebook
# Windows ищет DLL рядом с исполняемым файлом - кладём копию в каталог тестов
PREPARE = cp ../libphonebook.dll .
LOCALLIB = libphonebook.dll
else
LIB     = ../libphonebook.so
LDFLAGS = -L.. -lphonebook -Wl,-rpath,..
PREPARE = true
LOCALLIB =
endif

SRC    = unity/unity.c test_phonebook.c
TARGET = run_tests.exe

.PHONY: all test clean

all: test

# Библиотеку собирает Makefile задания - тест только линкуется с ней
$(LIB):
	$(MAKE) -C .. $(notdir $(LIB))

$(TARGET): $(SRC) $(LIB) ../phoneBook.h
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

test: $(TARGET)
	$(PREPARE)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(LOCALLIB)
