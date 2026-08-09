CC = gcc
CFLAGS = -Wall -Wextra

# Имя и способ сборки разделяемой библиотеки различаются по системам
ifeq ($(OS),Windows_NT)
LIB = libphonebook.dll
SHAREDFLAGS = -shared -Wl,--out-implib,libphonebook.dll.a
LDFLAGS = -L. -lphonebook
IMPLIB = libphonebook.dll.a
else
LIB = libphonebook.so
SHAREDFLAGS = -shared
LDFLAGS = -L. -lphonebook -Wl,-rpath,.
CFLAGS += -fPIC
IMPLIB =
endif

LIBOBJ = phoneBook.o
APPOBJ = main.o menu.o

all: pb.exe

# Разделяемая библиотека: код остаётся в отдельном файле и грузится при запуске
$(LIB): $(LIBOBJ)
	$(CC) $(SHAREDFLAGS) $(LIBOBJ) -o $(LIB)

phoneBook.o: phoneBook.c phoneBook.h
	$(CC) $(CFLAGS) -c phoneBook.c -o phoneBook.o

menu.o: menu.c menu.h phoneBook.h
	$(CC) $(CFLAGS) -c menu.c -o menu.o

main.o: main.c menu.h
	$(CC) $(CFLAGS) -c main.c -o main.o

pb.exe: $(APPOBJ) $(LIB)
	$(CC) $(APPOBJ) $(LDFLAGS) -o pb.exe

clean:
	rm -f $(LIBOBJ) $(APPOBJ) $(LIB) $(IMPLIB) pb.exe
