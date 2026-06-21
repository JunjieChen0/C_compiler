CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
SRCS = src/main.c src/utils.c src/lexer.c src/preprocess.c src/type.c src/sym.c src/gen.c src/parser.c src/pe.c
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

test_gen: tests/test_gen.c src/gen.c src/utils.c
	$(CC) $(CFLAGS) -o tests/test_gen.exe tests/test_gen.c src/gen.c src/utils.c
	tests\test_gen.exe

test_preprocess: tests/test_preprocess.c src/preprocess.c src/lexer.c src/utils.c
	$(CC) $(CFLAGS) -o tests/test_preprocess.exe tests/test_preprocess.c src/preprocess.c src/lexer.c src/utils.c
	tests\test_preprocess.exe

test_pe: tests/test_pe.c src/pe.c src/utils.c
	$(CC) $(CFLAGS) -o tests/test_pe.exe tests/test_pe.c src/pe.c src/utils.c
	tests\test_pe.exe

clean:
	del /Q src\*.o tests\test_lexer.exe tests\test_type.exe tests\test_sym.exe tests\test_gen.exe tests\test_pe.exe 2>nul || true

.PHONY: clean test_lexer test_type test_sym test_gen test_preprocess test_pe
