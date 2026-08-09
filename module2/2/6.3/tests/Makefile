CC = gcc

# -DUNITY_INCLUDE_DOUBLE: сравнение double в Unity по умолчанию выключено,
# без этого флага TEST_ASSERT_EQUAL_DOUBLE всегда падает с
# "Unity Double Precision Disabled".
CFLAGS = -Wall -Wextra -g -I. -I.. -DUNITY_INCLUDE_DOUBLE
LDLIBS = -lm

ifeq ($(OS),Windows_NT)
LIB_EXT = .dll
DL_LIB =
else
LIB_EXT = .so
CFLAGS += -fPIC
DL_LIB = -ldl
endif

# Весь API loader.c публичный, поэтому файл линкуется как обычно.
SRC    = unity/unity.c test_loader.c ../loader.c
TARGET = run_tests.exe

FIXTURE_DIR = fixtures
FIXTURE_SRC = $(wildcard $(FIXTURE_DIR)/*.c)
FIXTURES    = $(FIXTURE_SRC:.c=$(LIB_EXT))

.PHONY: all test clean

all: test

$(TARGET): $(SRC) ../loader.h ../plugin.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(DL_LIB) $(LDLIBS)

# Библиотеки-фикстуры: две корректные и одна без нужных символов
$(FIXTURE_DIR)/%$(LIB_EXT): $(FIXTURE_DIR)/%.c ../plugin.h
	$(CC) $(CFLAGS) -shared $< -o $@

test: $(TARGET) $(FIXTURES)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(FIXTURE_DIR)/*$(LIB_EXT)
