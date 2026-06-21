#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "type.h"
#include "sym.h"
#include "gen.h"

#define MAX_STRINGS 256

typedef struct {
    char *label;
    char *data;
    int len;
} StringLit;

typedef struct {
    Lexer lexer;
    CodeGen *gen;
    int label_count;
    int local_offset;
    Type *cur_func_ret_type;
    int return_label;
    int break_label;
    int continue_label;
    Type *expr_type;
    int has_lvalue;
    Register lvalue_base;
    int lvalue_offset;
    Type *lvalue_type;
    int lvalue_is_stack;
    StringLit strings[MAX_STRINGS];
    int string_count;
    char last_ident[256];
} Parser;

void parser_init(Parser *p, char *source, CodeGen *gen);
void parse_translation_unit(Parser *p);
char *parser_flush(Parser *p);

#endif
