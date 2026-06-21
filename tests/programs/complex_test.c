#include <stdarg.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    char *data;
    int len;
} String;

typedef struct {
    int kind;
    char *name;
    int value;
} Symbol;

#define MAX_SYMBOLS 100

static Symbol symbols[MAX_SYMBOLS];
static int symbol_count = 0;

Symbol *find_symbol(const char *name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbols[i].name, name) == 0) {
            return &symbols[i];
        }
    }
    return NULL;
}

Symbol *add_symbol(const char *name, int kind, int value) {
    if (symbol_count >= MAX_SYMBOLS) return NULL;
    Symbol *s = &symbols[symbol_count++];
    s->name = (char *)name;
    s->kind = kind;
    s->value = value;
    return s;
}

int string_length(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

char *string_copy(char *dest, const char *src) {
    char *d = dest;
    while (*src) *d++ = *src++;
    *d = '\0';
    return dest;
}

int main() {
    add_symbol("x", 1, 42);
    add_symbol("y", 2, 100);
    
    Symbol *s = find_symbol("x");
    if (s) {
        return s->value;
    }
    return 0;
}
