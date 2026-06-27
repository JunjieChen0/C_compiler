CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g

# AddressSanitizer support: make SANITIZE=1
ifeq ($(SANITIZE),1)
CFLAGS += -fsanitize=address -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address
endif

SRCS = src/main.c src/utils.c src/lexer.c src/preprocess.c src/type.c src/sym.c src/gen.c src/parser.c src/pe.c
OBJS = $(SRCS:.c=.o)

cc: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

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

test_gen: tests/test_gen.c src/gen.c src/utils.c src/common.c
	$(CC) $(CFLAGS) -o tests/test_gen.exe tests/test_gen.c src/gen.c src/utils.c src/common.c
	tests\test_gen.exe

test_preprocess: tests/test_preprocess.c src/preprocess.c src/lexer.c src/utils.c
	$(CC) $(CFLAGS) -o tests/test_preprocess.exe tests/test_preprocess.c src/preprocess.c src/lexer.c src/utils.c
	tests\test_preprocess.exe

test_pe: tests/test_pe.c src/pe.c src/utils.c
	$(CC) $(CFLAGS) -o tests/test_pe.exe tests/test_pe.c src/pe.c src/utils.c
	tests\test_pe.exe

test_ir: tests/test_ir.c src/ir.c
	$(CC) $(CFLAGS) -o tests/test_ir.exe tests/test_ir.c src/ir.c
	tests\test_ir.exe

test_errors: tests/test_errors.c cc
	$(CC) $(CFLAGS) -o tests/test_errors.exe tests/test_errors.c
	tests\test_errors.exe

TEST_PROGRAMS = simple vars factorial loop preprocess_test self_host_test control_flow pointer_array bitwise string_ops
TEST_ASM = $(addprefix tests/programs/,$(addsuffix .s,$(TEST_PROGRAMS)))

test_integration: cc $(TEST_ASM)
	@echo Integration tests passed: all test programs compiled successfully

tests/programs/%.s: tests/programs/%.c cc
	./cc.exe $< -o $@

test_all: test_lexer test_type test_sym test_gen test_preprocess test_pe test_ir test_errors test_integration
	@echo All tests passed

clean:
	del /Q src\*.o tests\test_lexer.exe tests\test_type.exe tests\test_sym.exe tests\test_gen.exe tests\test_pe.exe tests\test_ir.exe tests\test_errors.exe 2>nul || true
	del /Q tests\programs\*.s 2>nul || true

.PHONY: clean test_lexer test_type test_sym test_gen test_preprocess test_pe test_ir test_errors test_integration test_all
