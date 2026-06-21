#include "sym.h"

static Scope *current_scope = NULL;
static int current_depth = 0;

static unsigned hash(char *name) {
    unsigned h = 5381;
    for (; *name; name++)
        h = h * 33 + (unsigned char)*name;
    return h;
}

static Symbol *new_symbol(SymbolKind kind, char *name, Type *type) {
    Symbol *sym = xmalloc(sizeof(Symbol));
    sym->kind = kind;
    sym->name = name;
    sym->type = type;
    sym->enum_val = 0;
    sym->offset = 0;
    sym->next = NULL;
    return sym;
}

static Scope *new_scope(Scope *parent) {
    Scope *sc = xmalloc(sizeof(Scope));
    memset(sc->table, 0, sizeof(sc->table));
    sc->parent = parent;
    sc->depth = parent ? parent->depth + 1 : 1;
    return sc;
}

static void scope_insert(Scope *sc, Symbol *sym) {
    unsigned idx = hash(sym->name) % SCOPE_BUCKET_COUNT;
    sym->next = sc->table[idx];
    sc->table[idx] = sym;
}

static Symbol *scope_lookup(Scope *sc, char *name) {
    unsigned idx = hash(name) % SCOPE_BUCKET_COUNT;
    for (Symbol *s = sc->table[idx]; s; s = s->next) {
        if (strcmp(s->name, name) == 0)
            return s;
    }
    return NULL;
}

void scope_push(void) {
    current_scope = new_scope(current_scope);
    current_depth = current_scope->depth;
}

void scope_pop(void) {
    if (!current_scope)
        error("scope_pop: no scope to pop");
    current_scope = current_scope->parent;
    current_depth = current_scope ? current_scope->depth : 0;
}

int scope_depth(void) {
    return current_depth;
}

void scope_reset(void) {
    current_scope = NULL;
    current_depth = 0;
}

Symbol *sym_declare(char *name, SymbolKind kind, Type *type) {
    if (!current_scope)
        error("sym_declare: no active scope");
    Symbol *sym = new_symbol(kind, name, type);
    scope_insert(current_scope, sym);
    return sym;
}

Symbol *sym_declare_enum(char *name, int val) {
    if (!current_scope)
        error("sym_declare_enum: no active scope");
    Symbol *sym = new_symbol(SYM_ENUM_CONST, name, ty_int());
    sym->enum_val = val;
    scope_insert(current_scope, sym);
    return sym;
}

Symbol *sym_declare_typedef(char *name, Type *type) {
    if (!current_scope)
        error("sym_declare_typedef: no active scope");
    Symbol *sym = new_symbol(SYM_TYPEDEF, name, type);
    scope_insert(current_scope, sym);
    return sym;
}

Symbol *sym_find(char *name) {
    for (Scope *sc = current_scope; sc; sc = sc->parent) {
        Symbol *s = scope_lookup(sc, name);
        if (s) return s;
    }
    return NULL;
}

Symbol *sym_find_current(char *name) {
    if (!current_scope) return NULL;
    return scope_lookup(current_scope, name);
}

Symbol *sym_find_global(char *name) {
    Scope *sc = current_scope;
    if (!sc) return NULL;
    while (sc->parent) sc = sc->parent;
    return scope_lookup(sc, name);
}
