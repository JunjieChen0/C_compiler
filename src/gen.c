#include "gen.h"

static int reg_used[REG_COUNT] = {0};

static const char *reg_names_64[] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
};

static const char *reg_names_32[] = {
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
};

static const char *reg_names_16[] = {
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
    "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
};

static const char *reg_names_8[] = {
    "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil",
    "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
};

static const char *size_prefix[] = {
    "", "byte ", "word ", "dword ", "qword ",
};

void gen_init(CodeGen *gen) {
    gen->funcs = NULL;
    gen->func_count = 0;
    gen->func_cap = 0;
    gen->output = string_new("");
    memset(reg_used, 0, sizeof(reg_used));
}

void gen_free(CodeGen *gen) {
    for (int i = 0; i < gen->func_count; i++) {
        Function *f = &gen->funcs[i];
        Instr *cur = f->head;
        while (cur) {
            Instr *next = cur->next;
            free(cur);
            cur = next;
        }
    }
    free(gen->funcs);
    free(gen->output.data);
}

void gen_func_begin(CodeGen *gen, const char *name, int num_params) {
    if (gen->func_count >= gen->func_cap) {
        gen->func_cap = gen->func_cap ? gen->func_cap * 2 : 16;
        gen->funcs = xrealloc(gen->funcs, sizeof(Function) * gen->func_cap);
    }
    Function *f = &gen->funcs[gen->func_count++];
    strncpy(f->name, name, sizeof(f->name) - 1);
    f->name[sizeof(f->name) - 1] = '\0';
    f->stack_size = 0;
    f->num_params = num_params;
    f->head = NULL;
    f->tail = NULL;
}

void gen_func_end(CodeGen *gen) {
    (void)gen;
}

void gen_emit(CodeGen *gen, Instr instr) {
    if (gen->func_count == 0)
        error("gen_emit: no active function");

    Function *f = &gen->funcs[gen->func_count - 1];
    Instr *node = xmalloc(sizeof(Instr));
    *node = instr;
    node->next = NULL;

    if (f->tail) {
        f->tail->next = node;
        f->tail = node;
    } else {
        f->head = node;
        f->tail = node;
    }
}

static String str_append_cstr(String s, const char *cstr) {
    String b = string_new(cstr);
    String result = string_append(s, b);
    free(s.data);
    free(b.data);
    return result;
}

static String str_append(String s, String b) {
    String result = string_append(s, b);
    free(s.data);
    free(b.data);
    return result;
}

static String format_operand(Operand *op) {
    char buf[256];
    switch (op->kind) {
    case OP_REG:
        snprintf(buf, sizeof(buf), "%s", reg_name(op->reg, op->size));
        break;
    case OP_IMM:
        snprintf(buf, sizeof(buf), "%lld", (long long)op->imm);
        break;
    case OP_MEM: {
        const char *prefix = (op->size != SZ_NONE) ? size_prefix[op->size] : "";
        if (op->base != REG_NONE && op->disp == 0)
            snprintf(buf, sizeof(buf), "%s[%s]", prefix, reg_name_64(op->base));
        else if (op->base != REG_NONE && op->disp > 0)
            snprintf(buf, sizeof(buf), "%s[%s+%d]", prefix, reg_name_64(op->base), op->disp);
        else if (op->base != REG_NONE && op->disp < 0)
            snprintf(buf, sizeof(buf), "%s[%s%d]", prefix, reg_name_64(op->base), op->disp);
        else
            snprintf(buf, sizeof(buf), "%s", prefix);
        break;
    }
    case OP_LABEL:
        snprintf(buf, sizeof(buf), "%s", op->label);
        break;
    default:
        buf[0] = '\0';
        break;
    }
    return string_new(buf);
}

static String format_instr(Instr *instr) {
    String line = string_new("");
    char buf[16];

    const char *mnemonic = NULL;
    switch (instr->kind) {
    case I_MOV:    mnemonic = "mov"; break;
    case I_LEA:    mnemonic = "lea"; break;
    case I_PUSH:   mnemonic = "push"; break;
    case I_POP:    mnemonic = "pop"; break;
    case I_ADD:    mnemonic = "add"; break;
    case I_SUB:    mnemonic = "sub"; break;
    case I_IMUL:   mnemonic = "imul"; break;
    case I_IDIV:   mnemonic = "idiv"; break;
    case I_NEG:    mnemonic = "neg"; break;
    case I_INC:    mnemonic = "inc"; break;
    case I_DEC:    mnemonic = "dec"; break;
    case I_AND:    mnemonic = "and"; break;
    case I_OR:     mnemonic = "or"; break;
    case I_XOR:    mnemonic = "xor"; break;
    case I_NOT:    mnemonic = "not"; break;
    case I_SHL:    mnemonic = "shl"; break;
    case I_SHR:    mnemonic = "shr"; break;
    case I_SAR:    mnemonic = "sar"; break;
    case I_CMP:    mnemonic = "cmp"; break;
    case I_TEST:   mnemonic = "test"; break;
    case I_JMP:    mnemonic = "jmp"; break;
    case I_JE:     mnemonic = "je"; break;
    case I_JNE:    mnemonic = "jne"; break;
    case I_JL:     mnemonic = "jl"; break;
    case I_JLE:    mnemonic = "jle"; break;
    case I_JG:     mnemonic = "jg"; break;
    case I_JGE:    mnemonic = "jge"; break;
    case I_JB:     mnemonic = "jb"; break;
    case I_JBE:    mnemonic = "jbe"; break;
    case I_JA:     mnemonic = "ja"; break;
    case I_JAE:    mnemonic = "jae"; break;
    case I_CALL:   mnemonic = "call"; break;
    case I_RET:    mnemonic = "ret"; break;
    case I_NOP:    mnemonic = "nop"; break;
    case I_CDQ:    mnemonic = "cdq"; break;
    case I_CQO:    mnemonic = "cqo"; break;
    case I_SETZ:   mnemonic = "setz"; break;
    case I_SETNZ:  mnemonic = "setnz"; break;
    case I_SETL:   mnemonic = "setl"; break;
    case I_SETLE:  mnemonic = "setle"; break;
    case I_SETG:   mnemonic = "setg"; break;
    case I_SETGE:  mnemonic = "setge"; break;
    case I_SETB:   mnemonic = "setb"; break;
    case I_SETBE:  mnemonic = "setbe"; break;
    case I_SETA:   mnemonic = "seta"; break;
    case I_SETAE:  mnemonic = "setae"; break;
    case I_MOVZX:  mnemonic = "movzx"; break;
    case I_MOVSX:  mnemonic = "movsx"; break;
    case I_LABEL:  break;
    }

    if (instr->kind == I_MOV || instr->kind == I_LEA ||
        instr->kind == I_ADD || instr->kind == I_SUB ||
        instr->kind == I_IMUL || instr->kind == I_AND ||
        instr->kind == I_OR || instr->kind == I_XOR ||
        instr->kind == I_CMP || instr->kind == I_TEST ||
        instr->kind == I_SHL || instr->kind == I_SHR ||
        instr->kind == I_SAR || instr->kind == I_MOVZX ||
        instr->kind == I_MOVSX) {
        snprintf(buf, sizeof(buf), "  %-8s", mnemonic);
        line = str_append_cstr(line, buf);
        String d = format_operand(&instr->dst);
        line = str_append(line, d);
        line = str_append_cstr(line, ", ");
        String s = format_operand(&instr->src);
        line = str_append(line, s);
    } else if (instr->kind == I_PUSH || instr->kind == I_POP ||
               instr->kind == I_NEG || instr->kind == I_INC ||
               instr->kind == I_DEC || instr->kind == I_NOT ||
               instr->kind == I_IDIV ||
               instr->kind == I_SETZ || instr->kind == I_SETNZ ||
               instr->kind == I_SETL || instr->kind == I_SETLE ||
               instr->kind == I_SETG || instr->kind == I_SETGE ||
               instr->kind == I_SETB || instr->kind == I_SETBE ||
               instr->kind == I_SETA || instr->kind == I_SETAE) {
        snprintf(buf, sizeof(buf), "  %-8s", mnemonic);
        line = str_append_cstr(line, buf);
        String d = format_operand(&instr->dst);
        line = str_append(line, d);
    } else if (instr->kind == I_JMP || instr->kind == I_JE ||
               instr->kind == I_JNE || instr->kind == I_JL ||
               instr->kind == I_JLE || instr->kind == I_JG ||
               instr->kind == I_JGE || instr->kind == I_JB ||
               instr->kind == I_JBE || instr->kind == I_JA ||
               instr->kind == I_JAE || instr->kind == I_CALL) {
        snprintf(buf, sizeof(buf), "  %-8s", mnemonic);
        line = str_append_cstr(line, buf);
        String d = format_operand(&instr->dst);
        line = str_append(line, d);
    } else {
        if (instr->kind == I_LABEL) {
            String d = format_operand(&instr->dst);
            line = str_append(line, d);
            line = str_append_cstr(line, ":");
        } else {
            snprintf(buf, sizeof(buf), "  %s", mnemonic);
            line = str_append_cstr(line, buf);
        }
    }

    return line;
}

void gen_flush(CodeGen *gen) {
    gen->output = str_append_cstr(gen->output, "section .text\n");

    for (int i = 0; i < gen->func_count; i++) {
        Function *f = &gen->funcs[i];
        char buf[256];

        snprintf(buf, sizeof(buf), "\nglobal %s\n", f->name);
        gen->output = str_append_cstr(gen->output, buf);

        snprintf(buf, sizeof(buf), "%s:\n", f->name);
        gen->output = str_append_cstr(gen->output, buf);

        Instr *cur = f->head;
        while (cur) {
            String line = format_instr(cur);
            gen->output = str_append(gen->output, line);
            gen->output = str_append_cstr(gen->output, "\n");
            cur = cur->next;
        }
    }
}

char *gen_output(CodeGen *gen) {
    return gen->output.data;
}

Register reg_alloc(void) {
    static const Register alloc_order[] = {
        REG_RAX, REG_RCX, REG_RDX, REG_R8, REG_R9,
        REG_R10, REG_R11, REG_RBX, REG_RSI, REG_RDI,
        REG_R12, REG_R13, REG_R14, REG_R15,
    };
    for (int i = 0; i < (int)(sizeof(alloc_order) / sizeof(alloc_order[0])); i++) {
        Register r = alloc_order[i];
        if (!reg_used[r]) {
            reg_used[r] = 1;
            return r;
        }
    }
    error("reg_alloc: no free registers");
    return REG_NONE;
}

void reg_free(Register reg) {
    if (reg >= 0 && reg < REG_COUNT)
        reg_used[reg] = 0;
}

int reg_in_use(Register reg) {
    if (reg >= 0 && reg < REG_COUNT)
        return reg_used[reg];
    return 0;
}

const char *reg_name(Register reg, OpSize size) {
    if (reg < 0 || reg >= REG_COUNT)
        return "???";
    switch (size) {
    case SZ_BYTE:  return reg_names_8[reg];
    case SZ_WORD:  return reg_names_16[reg];
    case SZ_DWORD: return reg_names_32[reg];
    case SZ_QWORD: return reg_names_64[reg];
    default:       return reg_names_64[reg];
    }
}

const char *reg_name_64(Register reg) { return reg_name(reg, SZ_QWORD); }
const char *reg_name_32(Register reg) { return reg_name(reg, SZ_DWORD); }
const char *reg_name_16(Register reg) { return reg_name(reg, SZ_WORD); }
const char *reg_name_8(Register reg)  { return reg_name(reg, SZ_BYTE); }

Operand op_reg(Register reg, OpSize size) {
    Operand op;
    memset(&op, 0, sizeof(op));
    op.kind = OP_REG;
    op.size = size;
    op.reg = reg;
    op.base = REG_NONE;
    return op;
}

Operand op_imm(int64_t val) {
    Operand op;
    memset(&op, 0, sizeof(op));
    op.kind = OP_IMM;
    op.size = SZ_NONE;
    op.reg = REG_NONE;
    op.imm = val;
    op.base = REG_NONE;
    return op;
}

Operand op_mem(Register base, int disp, OpSize size) {
    Operand op;
    memset(&op, 0, sizeof(op));
    op.kind = OP_MEM;
    op.size = size;
    op.reg = REG_NONE;
    op.base = base;
    op.disp = disp;
    return op;
}

Operand op_label(const char *name) {
    Operand op;
    memset(&op, 0, sizeof(op));
    op.kind = OP_LABEL;
    op.size = SZ_NONE;
    op.reg = REG_NONE;
    op.base = REG_NONE;
    strncpy(op.label, name, sizeof(op.label) - 1);
    op.label[sizeof(op.label) - 1] = '\0';
    return op;
}

static void emit_2op(CodeGen *gen, InstrKind kind, OpSize size, Operand dst, Operand src) {
    Instr instr;
    memset(&instr, 0, sizeof(instr));
    instr.kind = kind;
    instr.size = size;
    instr.dst = dst;
    instr.src = src;
    gen_emit(gen, instr);
}

static void emit_1op(CodeGen *gen, InstrKind kind, OpSize size, Operand op) {
    Instr instr;
    memset(&instr, 0, sizeof(instr));
    instr.kind = kind;
    instr.size = size;
    instr.dst = op;
    gen_emit(gen, instr);
}

static void emit_0op(CodeGen *gen, InstrKind kind) {
    Instr instr;
    memset(&instr, 0, sizeof(instr));
    instr.kind = kind;
    gen_emit(gen, instr);
}

static void emit_jump(CodeGen *gen, InstrKind kind, const char *label) {
    Instr instr;
    memset(&instr, 0, sizeof(instr));
    instr.kind = kind;
    instr.dst = op_label(label);
    gen_emit(gen, instr);
}

void emit_mov(CodeGen *gen, Operand dst, Operand src)  { emit_2op(gen, I_MOV, dst.size, dst, src); }
void emit_lea(CodeGen *gen, Operand dst, Operand src)  { emit_2op(gen, I_LEA, dst.size, dst, src); }
void emit_push(CodeGen *gen, Operand op)               { emit_1op(gen, I_PUSH, SZ_QWORD, op); }
void emit_pop(CodeGen *gen, Operand op)                { emit_1op(gen, I_POP, SZ_QWORD, op); }

void emit_add(CodeGen *gen, Operand dst, Operand src)  { emit_2op(gen, I_ADD, dst.size, dst, src); }
void emit_sub(CodeGen *gen, Operand dst, Operand src)  { emit_2op(gen, I_SUB, dst.size, dst, src); }
void emit_imul(CodeGen *gen, Operand dst, Operand src) { emit_2op(gen, I_IMUL, dst.size, dst, src); }
void emit_idiv(CodeGen *gen, Operand src)              { emit_1op(gen, I_IDIV, src.size, src); }
void emit_neg(CodeGen *gen, Operand op)                { emit_1op(gen, I_NEG, op.size, op); }
void emit_inc(CodeGen *gen, Operand op)                { emit_1op(gen, I_INC, op.size, op); }
void emit_dec(CodeGen *gen, Operand op)                { emit_1op(gen, I_DEC, op.size, op); }

void emit_and(CodeGen *gen, Operand dst, Operand src)  { emit_2op(gen, I_AND, dst.size, dst, src); }
void emit_or(CodeGen *gen, Operand dst, Operand src)   { emit_2op(gen, I_OR, dst.size, dst, src); }
void emit_xor(CodeGen *gen, Operand dst, Operand src)  { emit_2op(gen, I_XOR, dst.size, dst, src); }
void emit_not(CodeGen *gen, Operand op)                { emit_1op(gen, I_NOT, op.size, op); }
void emit_shl(CodeGen *gen, Operand dst, Operand src)  { emit_2op(gen, I_SHL, dst.size, dst, src); }
void emit_shr(CodeGen *gen, Operand dst, Operand src)  { emit_2op(gen, I_SHR, dst.size, dst, src); }
void emit_sar(CodeGen *gen, Operand dst, Operand src)  { emit_2op(gen, I_SAR, dst.size, dst, src); }

void emit_cmp(CodeGen *gen, Operand a, Operand b)     { emit_2op(gen, I_CMP, a.size, a, b); }
void emit_test(CodeGen *gen, Operand a, Operand b)    { emit_2op(gen, I_TEST, a.size, a, b); }

void emit_jmp(CodeGen *gen, const char *label)   { emit_jump(gen, I_JMP, label); }
void emit_je(CodeGen *gen, const char *label)    { emit_jump(gen, I_JE, label); }
void emit_jne(CodeGen *gen, const char *label)   { emit_jump(gen, I_JNE, label); }
void emit_jl(CodeGen *gen, const char *label)    { emit_jump(gen, I_JL, label); }
void emit_jle(CodeGen *gen, const char *label)   { emit_jump(gen, I_JLE, label); }
void emit_jg(CodeGen *gen, const char *label)    { emit_jump(gen, I_JG, label); }
void emit_jge(CodeGen *gen, const char *label)   { emit_jump(gen, I_JGE, label); }
void emit_jb(CodeGen *gen, const char *label)    { emit_jump(gen, I_JB, label); }
void emit_jbe(CodeGen *gen, const char *label)   { emit_jump(gen, I_JBE, label); }
void emit_ja(CodeGen *gen, const char *label)    { emit_jump(gen, I_JA, label); }
void emit_jae(CodeGen *gen, const char *label)   { emit_jump(gen, I_JAE, label); }

void emit_call(CodeGen *gen, const char *func_name) {
    emit_jump(gen, I_CALL, func_name);
}

void emit_ret(CodeGen *gen)  { emit_0op(gen, I_RET); }
void emit_nop(CodeGen *gen)  { emit_0op(gen, I_NOP); }

void emit_setz(CodeGen *gen, Operand dst)   { emit_1op(gen, I_SETZ, SZ_BYTE, dst); }
void emit_setnz(CodeGen *gen, Operand dst)  { emit_1op(gen, I_SETNZ, SZ_BYTE, dst); }
void emit_setl(CodeGen *gen, Operand dst)   { emit_1op(gen, I_SETL, SZ_BYTE, dst); }
void emit_setle(CodeGen *gen, Operand dst)  { emit_1op(gen, I_SETLE, SZ_BYTE, dst); }
void emit_setg(CodeGen *gen, Operand dst)   { emit_1op(gen, I_SETG, SZ_BYTE, dst); }
void emit_setge(CodeGen *gen, Operand dst)  { emit_1op(gen, I_SETGE, SZ_BYTE, dst); }
void emit_setb(CodeGen *gen, Operand dst)   { emit_1op(gen, I_SETB, SZ_BYTE, dst); }
void emit_setbe(CodeGen *gen, Operand dst)  { emit_1op(gen, I_SETBE, SZ_BYTE, dst); }
void emit_seta(CodeGen *gen, Operand dst)   { emit_1op(gen, I_SETA, SZ_BYTE, dst); }
void emit_setae(CodeGen *gen, Operand dst)  { emit_1op(gen, I_SETAE, SZ_BYTE, dst); }

void emit_movzx(CodeGen *gen, Operand dst, Operand src) { emit_2op(gen, I_MOVZX, dst.size, dst, src); }
void emit_movsx(CodeGen *gen, Operand dst, Operand src) { emit_2op(gen, I_MOVSX, dst.size, dst, src); }

void emit_label(CodeGen *gen, const char *name) {
    Instr instr;
    memset(&instr, 0, sizeof(instr));
    instr.kind = I_LABEL;
    instr.dst = op_label(name);
    gen_emit(gen, instr);
}

void emit_directive(CodeGen *gen, const char *dir) {
    (void)gen;
    (void)dir;
}

void emit_comment(CodeGen *gen, const char *comment) {
    (void)gen;
    (void)comment;
}

void emit_raw(CodeGen *gen, const char *line) {
    (void)gen;
    (void)line;
}

void emit_prologue(CodeGen *gen, int stack_size) {
    emit_push(gen, op_reg(REG_RBP, SZ_QWORD));
    emit_mov(gen, op_reg(REG_RBP, SZ_QWORD), op_reg(REG_RSP, SZ_QWORD));
    if (stack_size > 0)
        emit_sub(gen, op_reg(REG_RSP, SZ_QWORD), op_imm(stack_size));
}

void emit_epilogue(CodeGen *gen) {
    emit_mov(gen, op_reg(REG_RSP, SZ_QWORD), op_reg(REG_RBP, SZ_QWORD));
    emit_pop(gen, op_reg(REG_RBP, SZ_QWORD));
    emit_ret(gen);
}

static const Register arg_regs[] = {
    REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9,
};
#define ARG_REG_COUNT 6

void emit_load_arg(CodeGen *gen, int index, Register dst) {
    if (index < 0)
        error("emit_load_arg: negative index");
    if (index < ARG_REG_COUNT) {
        if (dst != arg_regs[index])
            emit_mov(gen, op_reg(dst, SZ_QWORD), op_reg(arg_regs[index], SZ_QWORD));
    } else {
        int disp = 16 + (index - ARG_REG_COUNT) * 8;
        emit_mov(gen, op_reg(dst, SZ_QWORD), op_mem(REG_RBP, disp, SZ_QWORD));
    }
}

void emit_store_arg(CodeGen *gen, int index, int stack_offset) {
    if (index < 0)
        error("emit_store_arg: negative index");
    if (index < ARG_REG_COUNT) {
        emit_mov(gen, op_mem(REG_RBP, -stack_offset, SZ_QWORD), op_reg(arg_regs[index], SZ_QWORD));
    } else {
        int disp = 16 + (index - ARG_REG_COUNT) * 8;
        emit_mov(gen, op_reg(REG_RAX, SZ_QWORD), op_mem(REG_RBP, disp, SZ_QWORD));
        emit_mov(gen, op_mem(REG_RBP, -stack_offset, SZ_QWORD), op_reg(REG_RAX, SZ_QWORD));
    }
}
