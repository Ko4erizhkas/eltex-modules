CC = gcc
CFLAGS = -Wall -Wextra
AR = ar
ARFLAGS = rcs

LIB = libphonebook.a
LIBOBJ = phoneBook.o
APPOBJ = main.o menu.o

all: pb.exe

# Статическая библиотека - просто архив объектных файлов
$(LIB): $(LIBOBJ)
	$(AR) $(ARFLAGS) $(LIB) $(LIBOBJ)

phoneBook.o: phoneBook.c phoneBook.h
	$(CC) $(CFLAGS) -c phoneBook.c -o phoneBook.o

menu.o: menu.c menu.h phoneBook.h
	$(CC) $(CFLAGS) -c menu.c -o menu.o

main.o: main.c menu.h
	$(CC) $(CFLAGS) -c main.c -o main.o

pb.exe: $(APPOBJ) $(LIB)
	$(CC) $(APPOBJ) -L. -lphonebook -o pb.exe

clean:
	rm -f $(LIBOBJ) $(APPOBJ) $(LIB) pb.exe
