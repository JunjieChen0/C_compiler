#ifndef SYM_H
#define SYM_H

#include "type.h"

typedef enum {
    SYM_VAR,
    SYM_FUNC,
    SYM_TYPEDEF,
    SYM_ENUM_CONST,
    SYM_LABEL,
    SYM_TAG,
} SymbolKind;

typedef struct Symbol Symbol;
struct Symbol {
    SymbolKind kind;
    char *name;
    Type *type;
    int enum_val;
    int offset;     // stack offset for local variables (bytes from rbp)
    Symbol *next;
};

#define SCOPE_BUCKET_COUNT 256

typedef struct Scope Scope;
struct Scope {
    Symbol *table[SCOPE_BUCKET_COUNT];
    Scope *parent;
    int depth;
};

void scope_push(void);
void scope_pop(void);
int scope_depth(void);
void scope_reset(void);

Symbol *sym_declare(char *name, SymbolKind kind, Type *type);
Symbol *sym_declare_enum(char *name, int val);
Symbol *sym_find(char *name);
Symbol *sym_find_current(char *name);
Symbol *sym_find_global(char *name);

#endif
