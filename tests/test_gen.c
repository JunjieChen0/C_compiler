#include <stdio.h>
#include <string.h>
#include "../src/gen.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    tests_run++; \
    printf("  %-40s ", name); \
} while(0)

#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

static void test_reg_names(void) {
    TEST("reg_name_64");
    if (strcmp(reg_name_64(REG_RAX), "rax") == 0 &&
        strcmp(reg_name_64(REG_R15), "r15") == 0)
        PASS();
    else
        FAIL("wrong 64-bit names");

    TEST("reg_name_32");
    if (strcmp(reg_name_32(REG_RAX), "eax") == 0 &&
        strcmp(reg_name_32(REG_R8), "r8d") == 0)
        PASS();
    else
        FAIL("wrong 32-bit names");

    TEST("reg_name_16");
    if (strcmp(reg_name_16(REG_RAX), "ax") == 0 &&
        strcmp(reg_name_16(REG_R10), "r10w") == 0)
        PASS();
    else
        FAIL("wrong 16-bit names");

    TEST("reg_name_8");
    if (strcmp(reg_name_8(REG_RAX), "al") == 0 &&
        strcmp(reg_name_8(REG_RCX), "cl") == 0)
        PASS();
    else
        FAIL("wrong 8-bit names");
}

static void test_operands(void) {
    TEST("op_reg");
    Operand r = op_reg(REG_RAX, SZ_QWORD);
    if (r.kind == OP_REG && r.reg == REG_RAX && r.size == SZ_QWORD)
        PASS();
    else
        FAIL("reg operand");

    TEST("op_imm");
    Operand i = op_imm(42);
    if (i.kind == OP_IMM && i.imm == 42)
        PASS();
    else
        FAIL("imm operand");

    TEST("op_mem");
    Operand m = op_mem(REG_RBP, -8, SZ_DWORD);
    if (m.kind == OP_MEM && m.base == REG_RBP && m.disp == -8 && m.size == SZ_DWORD)
        PASS();
    else
        FAIL("mem operand");

    TEST("op_label");
    Operand l = op_label("main");
    if (l.kind == OP_LABEL && strcmp(l.label, "main") == 0)
        PASS();
    else
        FAIL("label operand");
}

static void test_reg_alloc(void) {
    TEST("reg_alloc/free");
    Register a = reg_alloc();
    Register b = reg_alloc();
    if (a != b && reg_in_use(a) && reg_in_use(b))
        PASS();
    else
        FAIL("alloc conflict");
    reg_free(a);
    reg_free(b);
}

static void test_simple_func(void) {
    CodeGen gen;
    gen_init(&gen);

    TEST("emit simple function");
    gen_func_begin(&gen, "add", 2, 0);
    emit_prologue(&gen, 16);
    emit_mov(&gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_RDI, SZ_QWORD));
    emit_add(&gen, op_reg(REG_RAX, SZ_QWORD), op_reg(REG_RSI, SZ_QWORD));
    emit_epilogue(&gen);
    gen_func_end(&gen);
    gen_flush(&gen);

    char *out = gen_output(&gen);
    if (strstr(out, "global add") &&
        strstr(out, "add:") &&
        strstr(out, "push") &&
        strstr(out, "mov") &&
        strstr(out, "add") &&
        strstr(out, "ret"))
        PASS();
    else
        FAIL("missing expected output");

    gen_free(&gen);
}

static void test_branches(void) {
    CodeGen gen;
    gen_init(&gen);

    TEST("emit branches");
    gen_func_begin(&gen, "test_branch", 1, 0);
    emit_prologue(&gen, 0);
    emit_cmp(&gen, op_reg(REG_RDI, SZ_QWORD), op_imm(0));
    emit_je(&gen, ".L0");
    emit_mov(&gen, op_reg(REG_RAX, SZ_QWORD), op_imm(1));
    emit_jmp(&gen, ".L1");
    emit_label(&gen, ".L0");
    emit_mov(&gen, op_reg(REG_RAX, SZ_QWORD), op_imm(0));
    emit_label(&gen, ".L1");
    emit_epilogue(&gen);
    gen_func_end(&gen);
    gen_flush(&gen);

    char *out = gen_output(&gen);
    if (strstr(out, "je") &&
        strstr(out, "jmp") &&
        strstr(out, ".L0:") &&
        strstr(out, ".L1:"))
        PASS();
    else
        FAIL("missing branch output");

    gen_free(&gen);
}

static void test_memory_ops(void) {
    CodeGen gen;
    gen_init(&gen);

    TEST("emit memory operations");
    gen_func_begin(&gen, "mem_test", 0, 0);
    emit_prologue(&gen, 16);
    emit_mov(&gen, op_mem(REG_RBP, -4, SZ_DWORD), op_imm(42));
    emit_mov(&gen, op_reg(REG_RAX, SZ_DWORD), op_mem(REG_RBP, -4, SZ_DWORD));
    emit_epilogue(&gen);
    gen_func_end(&gen);
    gen_flush(&gen);

    char *out = gen_output(&gen);
    if (strstr(out, "dword [rbp-4]") &&
        strstr(out, "eax"))
        PASS();
    else
        FAIL("missing memory output");

    gen_free(&gen);
}

static void test_call_convention(void) {
    CodeGen gen;
    gen_init(&gen);

    TEST("emit function call");
    gen_func_begin(&gen, "caller", 2, 0);
    emit_prologue(&gen, 0);
    emit_mov(&gen, op_reg(REG_RDI, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
    emit_call(&gen, "callee");
    emit_epilogue(&gen);
    gen_func_end(&gen);
    gen_flush(&gen);

    char *out = gen_output(&gen);
    if (strstr(out, "call") && strstr(out, "callee"))
        PASS();
    else
        FAIL("missing call");

    gen_free(&gen);
}

static void test_bitwise(void) {
    CodeGen gen;
    gen_init(&gen);

    TEST("emit bitwise ops");
    gen_func_begin(&gen, "bits", 2, 0);
    emit_prologue(&gen, 0);
    emit_and(&gen, op_reg(REG_RDI, SZ_QWORD), op_reg(REG_RSI, SZ_QWORD));
    emit_or(&gen, op_reg(REG_RDI, SZ_QWORD), op_reg(REG_RSI, SZ_QWORD));
    emit_xor(&gen, op_reg(REG_RDI, SZ_QWORD), op_reg(REG_RSI, SZ_QWORD));
    emit_not(&gen, op_reg(REG_RDI, SZ_QWORD));
    emit_shl(&gen, op_reg(REG_RDI, SZ_QWORD), op_imm(2));
    emit_shr(&gen, op_reg(REG_RSI, SZ_QWORD), op_imm(3));
    emit_epilogue(&gen);
    gen_func_end(&gen);
    gen_flush(&gen);

    char *out = gen_output(&gen);
    if (strstr(out, "and") && strstr(out, "or") &&
        strstr(out, "xor") && strstr(out, "not") &&
        strstr(out, "shl") && strstr(out, "shr"))
        PASS();
    else
        FAIL("missing bitwise output");

    gen_free(&gen);
}

static void test_setcc(void) {
    CodeGen gen;
    gen_init(&gen);

    TEST("emit setcc");
    gen_func_begin(&gen, "cmp_test", 2, 0);
    emit_prologue(&gen, 0);
    emit_cmp(&gen, op_reg(REG_RDI, SZ_QWORD), op_reg(REG_RSI, SZ_QWORD));
    emit_setl(&gen, op_reg(REG_RAX, SZ_BYTE));
    emit_movzx(&gen, op_reg(REG_RAX, SZ_DWORD), op_reg(REG_RAX, SZ_BYTE));
    emit_epilogue(&gen);
    gen_func_end(&gen);
    gen_flush(&gen);

    char *out = gen_output(&gen);
    if (strstr(out, "setl") && strstr(out, "al") && strstr(out, "movzx"))
        PASS();
    else
        FAIL("missing setcc output");

    gen_free(&gen);
}

int main(void) {
    printf("Running gen tests:\n");
    test_reg_names();
    test_operands();
    test_reg_alloc();
    test_simple_func();
    test_branches();
    test_memory_ops();
    test_call_convention();
    test_bitwise();
    test_setcc();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
