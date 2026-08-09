CC = gcc
CFLAGS = -Wall -Wextra
PLUGIN_DIR = plugins

ifeq ($(OS),Windows_NT)
LIB_EXT = .dll
DL_LIB =
else
LIB_EXT = .so
CFLAGS += -fPIC
DL_LIB = -ldl
endif

APPOBJ = main.o calc.o loader.o
PLUGIN_SRC = $(wildcard $(PLUGIN_DIR)/*.c)
PLUGIN_LIB = $(PLUGIN_SRC:.c=$(LIB_EXT))

all: calc.exe $(PLUGIN_LIB)

calc.exe: $(APPOBJ)
	$(CC) $(APPOBJ) $(DL_LIB) -o calc.exe

main.o: main.c calc.h loader.h plugin.h
	$(CC) $(CFLAGS) -c main.c -o main.o

calc.o: calc.c calc.h loader.h plugin.h
	$(CC) $(CFLAGS) -c calc.c -o calc.o

loader.o: loader.c loader.h plugin.h
	$(CC) $(CFLAGS) -c loader.c -o loader.o

# Одна операция - одна разделяемая библиотека
$(PLUGIN_DIR)/%$(LIB_EXT): $(PLUGIN_DIR)/%.c plugin.h
	$(CC) $(CFLAGS) -I. -shared $< -o $@

clean:
	rm -f $(APPOBJ) calc.exe $(PLUGIN_DIR)/*$(LIB_EXT)
