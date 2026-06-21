CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
SRCS = src/main.c src/utils.c src/lexer.c src/type.c src/sym.c
OBJS = $(SRCS:.c=.o)

cc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test_lexer: tests/test_lexer.c src/lexer.c src/utils.c
	$(CC) $(CFLAGS) -o tests/test_lexer.exe tests/test_lexer.c src/lexer.c src/utils.c
	tests\test_lexer.exe

test_type: tests/test_type.c src/type.c src/utils.c
	$(CC) $(CFLAGS) -o tests/test_type.exe tests/test_type.c src/type.c src/utils.c
	tests\test_type.exe

test_sym: tests/test_sym.c src/sym.c src/type.c src/utils.c
	$(CC) $(CFLAGS) -o tests/test_sym.exe tests/test_sym.c src/sym.c src/type.c src/utils.c
	tests\test_sym.exe

clean:
	del /Q src\*.o tests\test_lexer.exe tests\test_type.exe tests\test_sym.exe 2>nul || true

.PHONY: clean test_lexer test_type test_sym
