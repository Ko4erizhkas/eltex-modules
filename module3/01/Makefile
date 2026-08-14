CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = q01.exe

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

clean:
	rm -f $(TARGET)
	rm -rf testdir

.PHONY: all test clean
