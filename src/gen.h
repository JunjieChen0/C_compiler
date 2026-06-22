#ifndef GEN_H
#define GEN_H

#include "utils.h"

typedef enum {
    REG_NONE = -1,

    REG_RAX, REG_RCX, REG_RDX, REG_RBX,
    REG_RSP, REG_RBP, REG_RSI, REG_RDI,
    REG_R8,  REG_R9,  REG_R10, REG_R11,
    REG_R12, REG_R13, REG_R14, REG_R15,

    REG_COUNT,
} Register;

typedef enum {
    SZ_NONE,
    SZ_BYTE,
    SZ_WORD,
    SZ_DWORD,
    SZ_QWORD,
} OpSize;

typedef enum {
    OP_REG,
    OP_IMM,
    OP_MEM,
    OP_LABEL,
} OperandKind;

typedef struct {
    OperandKind kind;
    OpSize size;
    Register reg;
    int64_t imm;
    Register base;
    int disp;
    char label[128];
} Operand;

typedef enum {
    I_MOV,
    I_LEA,
    I_PUSH,
    I_POP,

    I_ADD,
    I_SUB,
    I_IMUL,
    I_IDIV,
    I_NEG,
    I_INC,
    I_DEC,

    I_AND,
    I_OR,
    I_XOR,
    I_NOT,
    I_SHL,
    I_SHR,
    I_SAR,

    I_CMP,
    I_TEST,

    I_JMP,
    I_JE, I_JNE, I_JL, I_JLE, I_JG, I_JGE,
    I_JB, I_JBE, I_JA, I_JAE,

    I_CALL,
    I_RET,

    I_NOP,
    I_CDQ,
    I_CQO,

    I_SETZ, I_SETNZ, I_SETL, I_SETLE, I_SETG, I_SETGE,
    I_SETB, I_SETBE, I_SETA, I_SETAE,

    I_MOVZX,
    I_MOVSX,
    I_LABEL,
} InstrKind;

typedef struct Instr Instr;
struct Instr {
    InstrKind kind;
    OpSize size;
    Operand dst;
    Operand src;
    Instr *next;
};

typedef struct {
    char name[128];
    int stack_size;
    int num_params;
    int is_static;
    Instr *head;
    Instr *tail;
} Function;

typedef struct {
    Function *funcs;
    int func_count;
    int func_cap;

    String output;

    char **externs;
    int extern_count;
    int extern_cap;
} CodeGen;

void gen_init(CodeGen *gen);
void gen_free(CodeGen *gen);

void gen_func_begin(CodeGen *gen, const char *name, int num_params, int is_static);
void gen_func_end(CodeGen *gen);

void gen_emit(CodeGen *gen, Instr instr);
void gen_flush(CodeGen *gen);
char *gen_output(CodeGen *gen);

Register reg_alloc(void);
void reg_free(Register reg);
int reg_in_use(Register reg);

const char *reg_name(Register reg, OpSize size);
const char *reg_name_64(Register reg);
const char *reg_name_32(Register reg);
const char *reg_name_16(Register reg);
const char *reg_name_8(Register reg);

Operand op_reg(Register reg, OpSize size);
Operand op_imm(int64_t val);
Operand op_mem(Register base, int disp, OpSize size);
Operand op_label(const char *name);

void emit_mov(CodeGen *gen, Operand dst, Operand src);
void emit_lea(CodeGen *gen, Operand dst, Operand src);
void emit_push(CodeGen *gen, Operand op);
void emit_pop(CodeGen *gen, Operand op);

void emit_add(CodeGen *gen, Operand dst, Operand src);
void emit_sub(CodeGen *gen, Operand dst, Operand src);
void emit_imul(CodeGen *gen, Operand dst, Operand src);
void emit_idiv(CodeGen *gen, Operand src);
void emit_neg(CodeGen *gen, Operand op);
void emit_inc(CodeGen *gen, Operand op);
void emit_dec(CodeGen *gen, Operand op);

void emit_and(CodeGen *gen, Operand dst, Operand src);
void emit_or(CodeGen *gen, Operand dst, Operand src);
void emit_xor(CodeGen *gen, Operand dst, Operand src);
void emit_not(CodeGen *gen, Operand op);
void emit_shl(CodeGen *gen, Operand dst, Operand src);
void emit_shr(CodeGen *gen, Operand dst, Operand src);
void emit_sar(CodeGen *gen, Operand dst, Operand src);

void emit_cmp(CodeGen *gen, Operand a, Operand b);
void emit_test(CodeGen *gen, Operand a, Operand b);

void emit_jmp(CodeGen *gen, const char *label);
void emit_je(CodeGen *gen, const char *label);
void emit_jne(CodeGen *gen, const char *label);
void emit_jl(CodeGen *gen, const char *label);
void emit_jle(CodeGen *gen, const char *label);
void emit_jg(CodeGen *gen, const char *label);
void emit_jge(CodeGen *gen, const char *label);
void emit_jb(CodeGen *gen, const char *label);
void emit_jbe(CodeGen *gen, const char *label);
void emit_ja(CodeGen *gen, const char *label);
void emit_jae(CodeGen *gen, const char *label);

void emit_call(CodeGen *gen, const char *func_name);
void emit_ret(CodeGen *gen);
void emit_nop(CodeGen *gen);

void emit_setz(CodeGen *gen, Operand dst);
void emit_setnz(CodeGen *gen, Operand dst);
void emit_setl(CodeGen *gen, Operand dst);
void emit_setle(CodeGen *gen, Operand dst);
void emit_setg(CodeGen *gen, Operand dst);
void emit_setge(CodeGen *gen, Operand dst);
void emit_setb(CodeGen *gen, Operand dst);
void emit_setbe(CodeGen *gen, Operand dst);
void emit_seta(CodeGen *gen, Operand dst);
void emit_setae(CodeGen *gen, Operand dst);

void emit_movzx(CodeGen *gen, Operand dst, Operand src);
void emit_movsx(CodeGen *gen, Operand dst, Operand src);

void emit_label(CodeGen *gen, const char *name);
void emit_directive(CodeGen *gen, const char *dir);
void emit_comment(CodeGen *gen, const char *comment);
void emit_raw(CodeGen *gen, const char *line);

void emit_prologue(CodeGen *gen, int stack_size);
void emit_epilogue(CodeGen *gen);

void emit_load_arg(CodeGen *gen, int index, Register dst);
void emit_store_arg(CodeGen *gen, int index, int stack_offset);

#endif
