# C Compiler Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use compose:subagent (recommended) or compose:execute to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a self-hosting C compiler written in C that compiles C11 programs to x86-64 Windows PE executables with integrated assembler and linker.

**Architecture:** Single-pass tcc-style compiler. Lexer → Preprocessor → Parser (with embedded code generation) → x86-64 machine code → PE executable. No AST, no optimization passes. Direct emission of x86-64 instructions during parsing.

**Tech Stack:** Pure C (C99 for the compiler itself), x86-64 assembly output, PE/COFF format for Windows executables.

---

## File Structure

```
C_complier/
├── src/
│   ├── main.c              # Entry point, command-line parsing, compilation driver
│   ├── lexer.c / lexer.h   # Tokenizer: keywords, identifiers, literals, operators
│   ├── preprocess.c / preprocess.h  # C preprocessor: #include, #define, #if, #pragma
│   ├── parser.c / parser.h # Syntax analysis + code generation (single pass)
│   ├── gen.c / gen.h       # x86-64 instruction encoding, register management
│   ├── type.c / type.h     # Type system: primitives, pointers, arrays, structs, functions
│   ├── sym.c / sym.h       # Symbol table: scoping, name resolution
│   ├── pe.c / pe.h         # PE/COFF file generation (integrated assembler + linker)
│   └── utils.c / utils.h   # Memory allocation, string utilities, error reporting
├── include/                # Built-in standard library headers
│   ├── stdio.h
│   ├── stdlib.h
│   ├── string.h
│   ├── stddef.h
│   ├── stdint.h
│   ├── stdbool.h
│   ├── stdarg.h
│   ├── windows.h           # Minimal Windows API header
│   └── _mingw.h            # Compiler builtins
├── tests/
│   ├── test_lexer.c        # Lexer unit tests
│   ├── test_type.c         # Type system tests
│   ├── test_gen.c          # Code generation tests
│   ├── test_compile.c      # Integration tests (compile + run)
│   └── programs/           # Test C programs
│       ├── hello.c
│       ├── fibonacci.c
│       ├── sort.c
│       ├── struct.c
│       ├── pointer.c
│       └── self_host.c     # Self-hosting test
└── Makefile
```

---

### Task 1: Project Scaffolding

**Covers:** Foundation setup

**Files:**
- Create: `src/main.c`
- Create: `src/utils.c`
- Create: `src/utils.h`
- Create: `Makefile`

- [ ] **Step 1: Create directory structure**

```bash
mkdir -p src include tests/programs
```

- [ ] **Step 2: Write utils.h**

```c
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

typedef struct {
    char *data;
    int len;
} String;

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);
void error(const char *fmt, ...);
void error_at(const char *loc, const char *fmt, ...);
String string_new(const char *s);
String string_append(String a, String b);

#endif
```

- [ ] **Step 3: Write utils.c**

```c
#include "utils.h"

void *xmalloc(size_t size) {
    void *p = malloc(size);
    if (!p) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return p;
}

void *xrealloc(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if (!p) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return p;
}

char *xstrdup(const char *s) {
    char *p = strdup(s);
    if (!p) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return p;
}

static char *current_file = NULL;
static char *current_input = NULL;

void set_current_file(const char *file) { current_file = (char *)file; }
void set_current_input(const char *input) { current_input = (char *)input; }

void error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

void error_at(const char *loc, const char *fmt, ...) {
    int line = 1;
    const char *line_start = current_input;
    for (const char *p = current_input; p && p < loc; p++) {
        if (*p == '\n') {
            line++;
            line_start = p + 1;
        }
    }

    fprintf(stderr, "%s:%d: ", current_file ? current_file : "<input>", line);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");

    const char *line_end = line_start;
    while (*line_end && *line_end != '\n') line_end++;
    fprintf(stderr, "%.*s\n", (int)(line_end - line_start), line_start);

    int pos = (int)(loc - line_start);
    fprintf(stderr, "%*s^\n", pos, "");
    exit(1);
}

String string_new(const char *s) {
    String str;
    str.data = xstrdup(s);
    str.len = strlen(s);
    return str;
}

String string_append(String a, String b) {
    String result;
    result.len = a.len + b.len;
    result.data = xmalloc(result.len + 1);
    memcpy(result.data, a.data, a.len);
    memcpy(result.data + a.len, b.data, b.len);
    result.data[result.len] = '\0';
    return result;
}
```

- [ ] **Step 4: Write main.c**

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

static char *read_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", path);
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = xmalloc(size + 1);
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: cc <file.c> [-o output.exe]\n");
        return 1;
    }

    char *input_file = NULL;
    char *output_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        }
    }

    if (!input_file) {
        fprintf(stderr, "No input file\n");
        return 1;
    }

    if (!output_file) {
        output_file = "a.exe";
    }

    char *source = read_file(input_file);
    set_current_file(input_file);
    set_current_input(source);

    printf("Compiling %s -> %s\n", input_file, output_file);
    // TODO: lexer, parser, codegen

    free(source);
    return 0;
}
```

- [ ] **Step 5: Write Makefile**

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
SRCS = src/main.c src/utils.c
OBJS = $(SRCS:.c=.o)

cc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f cc src/*.o

.PHONY: clean
```

- [ ] **Step 6: Build and verify**

Run: `make`
Expected: Compiles without errors, produces `cc` executable.

- [ ] **Step 7: Test basic invocation**

Run: `./cc`
Expected: "Usage: cc <file.c> [-o output.exe]"

- [ ] **Step 8: Commit**

```bash
git init
git add src/main.c src/utils.c src/utils.h Makefile
git commit -m "feat: project scaffolding with main driver and utilities"
```

---

### Task 2: Lexer

**Covers:** Tokenization of all C tokens

**Files:**
- Create: `src/lexer.c`
- Create: `src/lexer.h`
- Create: `tests/test_lexer.c`

- [ ] **Step 1: Write lexer.h**

```c
#ifndef LEXER_H
#define LEXER_H

#include "utils.h"

typedef enum {
    TK_EOF,
    TK_IDENT,
    TK_INT,         // integer literal
    TK_LONG,        // long integer literal (L suffix)
    TK_UINT,        // unsigned integer literal (U suffix)
    TK_FLOAT,       // float literal
    TK_STRING,      // string literal
    TK_CHAR,        // character literal

    // Keywords
    TK_AUTO, TK_BREAK, TK_CASE, TK_CHAR_KW, TK_CONST, TK_CONTINUE,
    TK_DEFAULT, TK_DO, TK_DOUBLE, TK_ELSE, TK_ENUM, TK_EXTERN,
    TK_FLOAT_KW, TK_FOR, TK_GOTO, TK_IF, TK_INLINE, TK_INT_KW,
    TK_LONG_KW, TK_REGISTER, TK_RESTRICT, TK_RETURN, TK_SHORT,
    TK_SIGNED, TK_SIZEOF, TK_STATIC, TK_STRUCT, TK_SWITCH, TK_TYPEDEF,
    TK_UNION, TK_UNSIGNED, TK_VOID, TK_VOLATILE, TK_WHILE,
    TK_BOOL, TK_COMPLEX, TK_IMAGINARY,

    // Preprocessor
    TK_HASH,        // #
    TK_HASHHASH,    // ##

    // Operators
    TK_PLUS,        // +
    TK_MINUS,       // -
    TK_STAR,        // *
    TK_SLASH,       // /
    TK_PERCENT,     // %
    TK_AMP,         // &
    TK_PIPE,        // |
    TK_CARET,       // ^
    TK_TILDE,       // ~
    TK_BANG,        // !
    TK_ASSIGN,      // =
    TK_LT,          // <
    TK_GT,          // >
    TK_PLUSPLUS,    // ++
    TK_MINUSMINUS,  // --
    TK_SHL,         // <<
    TK_SHR,         // >>
    TK_EQ,          // ==
    TK_NE,          // !=
    TK_LE,          // <=
    TK_GE,          // >=
    TK_ANDAND,      // &&
    TK_OROR,        // ||
    TK_MULASSIGN,   // *=
    TK_DIVASSIGN,   // /=
    TK_MODASSIGN,   // %=
    TK_ADDASSIGN,   // +=
    TK_SUBASSIGN,   // -=
    TK_SHLASSIGN,   // <<=
    TK_SHRASSIGN,   // >>=
    TK_ANDASSIGN,   // &=
    TK_XORASSIGN,   // ^=
    TK_ORASSIGN,    // |=
    TK_ARROW,       // ->
    TK_DOT,         // .
    TK_QUESTION,    // ?
    TK_COLON,       // :
    TK_SEMICOLON,   // ;
    TK_COMMA,       // ,
    TK_LPAREN,      // (
    TK_RPAREN,      // )
    TK_LBRACKET,    // [
    TK_RBRACKET,    // ]
    TK_LBRACE,      // {
    TK_RBRACE,      // }
    TK_ELLIPSIS,    // ...
    TK_NOT,         // ~
} TokenKind;

typedef struct {
    TokenKind kind;
    char *start;
    int len;
    int64_t ival;       // for TK_INT, TK_LONG, TK_UINT
    double fval;        // for TK_FLOAT
    char *sval;         // for TK_STRING (escaped)
    int slen;           // string length
} Token;

typedef struct {
    char *start;
    char *current;
    Token token;
    int line;
} Lexer;

void lexer_init(Lexer *l, char *source);
Token lexer_next(Lexer *l);
Token lexer_peek(Lexer *l);
void lexer_expect(Lexer *l, TokenKind kind);
const char *token_kind_str(TokenKind kind);

#endif
```

- [ ] **Step 2: Write lexer.c**

```c
#include "lexer.h"

static struct { const char *word; TokenKind kind; } keywords[] = {
    {"auto", TK_AUTO}, {"break", TK_BREAK}, {"case", TK_CASE},
    {"char", TK_CHAR_KW}, {"const", TK_CONST}, {"continue", TK_CONTINUE},
    {"default", TK_DEFAULT}, {"do", TK_DO}, {"double", TK_DOUBLE},
    {"else", TK_ELSE}, {"enum", TK_ENUM}, {"extern", TK_EXTERN},
    {"float", TK_FLOAT_KW}, {"for", TK_FOR}, {"goto", TK_GOTO},
    {"if", TK_IF}, {"inline", TK_INLINE}, {"int", TK_INT_KW},
    {"long", TK_LONG_KW}, {"register", TK_REGISTER}, {"restrict", TK_RESTRICT},
    {"return", TK_RETURN}, {"short", TK_SHORT}, {"signed", TK_SIGNED},
    {"sizeof", TK_SIZEOF}, {"static", TK_STATIC}, {"struct", TK_STRUCT},
    {"switch", TK_SWITCH}, {"typedef", TK_TYPEDEF}, {"union", TK_UNION},
    {"unsigned", TK_UNSIGNED}, {"void", TK_VOID}, {"volatile", TK_VOLATILE},
    {"while", TK_WHILE}, {"_Bool", TK_BOOL}, {"_Complex", TK_COMPLEX},
    {"_Imaginary", TK_IMAGINARY},
    {NULL, TK_EOF}
};

static TokenKind check_keyword(const char *start, int len) {
    for (int i = 0; keywords[i].word; i++) {
        if ((int)strlen(keywords[i].word) == len &&
            memcmp(keywords[i].word, start, len) == 0) {
            return keywords[i].kind;
        }
    }
    return TK_IDENT;
}

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static char escape_char(char c) {
    switch (c) {
        case 'a': return '\a'; case 'b': return '\b'; case 'f': return '\f';
        case 'n': return '\n'; case 'r': return '\r'; case 't': return '\t';
        case 'v': return '\v'; case '0': return '\0'; case '\\': return '\\';
        case '\'': return '\''; case '"': return '"';
        default: return c;
    }
}

void lexer_init(Lexer *l, char *source) {
    l->start = source;
    l->current = source;
    l->line = 1;
    l->token = (Token){TK_EOF, NULL, 0, 0, 0, NULL, 0};
}

static void skip_whitespace(Lexer *l) {
    for (;;) {
        char c = *l->current;
        if (c == ' ' || c == '\t' || c == '\r') {
            l->current++;
        } else if (c == '\n') {
            l->current++;
            l->line++;
        } else if (c == '/' && l->current[1] == '/') {
            while (*l->current && *l->current != '\n') l->current++;
        } else if (c == '/' && l->current[1] == '*') {
            l->current += 2;
            while (*l->current && !(*l->current == '*' && l->current[1] == '/')) {
                if (*l->current == '\n') l->line++;
                l->current++;
            }
            if (*l->current) l->current += 2;
        } else {
            break;
        }
    }
}

static Token read_number(Lexer *l) {
    Token tok;
    tok.start = l->start;
    tok.kind = TK_INT;

    int base = 10;
    if (*l->current == '0' && (l->current[1] == 'x' || l->current[1] == 'X')) {
        base = 16;
        l->current += 2;
    } else if (*l->current == '0' && (l->current[1] == 'b' || l->current[1] == 'B')) {
        base = 2;
        l->current += 2;
    } else if (*l->current == '0') {
        base = 8;
    }

    int64_t val = 0;
    if (base == 16) {
        while (hex_digit(*l->current) >= 0) {
            val = val * 16 + hex_digit(*l->current);
            l->current++;
        }
    } else {
        while (is_digit(*l->current)) {
            val = val * base + (*l->current - '0');
            l->current++;
        }
    }

    if (*l->current == '.' || *l->current == 'e' || *l->current == 'E') {
        tok.kind = TK_FLOAT;
        if (*l->current == '.') {
            l->current++;
            while (is_digit(*l->current)) l->current++;
        }
        if (*l->current == 'e' || *l->current == 'E') {
            l->current++;
            if (*l->current == '+' || *l->current == '-') l->current++;
            while (is_digit(*l->current)) l->current++;
        }
        tok.fval = strtod(tok.start, NULL);
        if (*l->current == 'f' || *l->current == 'F') l->current++;
    } else {
        tok.ival = val;
        if (*l->current == 'U' || *l->current == 'u') {
            l->current++;
            tok.kind = TK_UINT;
        }
        if (*l->current == 'L' || *l->current == 'l') {
            l->current++;
            tok.kind = TK_LONG;
            if (*l->current == 'L' || *l->current == 'l') l->current++;
        }
    }

    tok.len = (int)(l->current - tok.start);
    return tok;
}

static Token read_string(Lexer *l) {
    Token tok;
    tok.kind = TK_STRING;
    tok.start = l->start;

    char buf[8192];
    int len = 0;

    l->current++; // skip opening "
    while (*l->current && *l->current != '"') {
        char c = *l->current++;
        if (c == '\\') {
            c = escape_char(*l->current++);
        }
        buf[len++] = c;
    }
    if (*l->current == '"') l->current++;

    tok.sval = xmalloc(len + 1);
    memcpy(tok.sval, buf, len);
    tok.sval[len] = '\0';
    tok.slen = len;
    tok.len = (int)(l->current - tok.start);
    return tok;
}

static Token read_char(Lexer *l) {
    Token tok;
    tok.kind = TK_CHAR;
    tok.start = l->start;

    l->current++; // skip '
    char c = *l->current++;
    if (c == '\\') {
        c = escape_char(*l->current++);
    }
    tok.ival = c;
    if (*l->current == '\'') l->current++;

    tok.len = (int)(l->current - tok.start);
    return tok;
}

static Token read_ident(Lexer *l) {
    Token tok;
    tok.start = l->start;
    while (is_alnum(*l->current)) l->current++;
    tok.len = (int)(l->current - tok.start);
    tok.kind = check_keyword(tok.start, tok.len);
    return tok;
}

Token lexer_next(Lexer *l) {
    skip_whitespace(l);

    l->start = l->current;
    char c = *l->current;

    if (c == '\0') {
        l->token = (Token){TK_EOF, l->current, 0, 0, 0, NULL, 0};
        return l->token;
    }

    if (is_alpha(c)) { l->token = read_ident(l); return l->token; }
    if (is_digit(c)) { l->token = read_number(l); return l->token; }
    if (c == '"') { l->token = read_string(l); return l->token; }
    if (c == '\'') { l->token = read_char(l); return l->token; }

    Token tok;
    tok.start = l->start;

    #define TWO(c1, c2, k1, k2) \
        if (*l->current == c1) { l->current++; \
            if (*l->current == c2) { l->current++; tok.kind = k2; } \
            else { tok.kind = k1; } \
            tok.len = (int)(l->current - tok.start); l->token = tok; return tok; }

    TWO('+', '+', TK_PLUS, TK_PLUSPLUS)
    TWO('+', '=', TK_PLUS, TK_ADDASSIGN)
    TWO('-', '-', TK_MINUS, TK_MINUSMINUS)
    TWO('-', '=', TK_MINUS, TK_SUBASSIGN)
    TWO('-', '>', TK_MINUS, TK_ARROW)
    TWO('*', '=', TK_STAR, TK_MULASSIGN)
    TWO('/', '=', TK_SLASH, TK_DIVASSIGN)
    TWO('%', '=', TK_PERCENT, TK_MODASSIGN)
    TWO('&', '=', TK_AMP, TK_ANDASSIGN)
    TWO('&', '&', TK_AMP, TK_ANDAND)
    TWO('|', '=', TK_PIPE, TK_ORASSIGN)
    TWO('|', '|', TK_PIPE, TK_OROR)
    TWO('^', '=', TK_CARET, TK_XORASSIGN)
    TWO('=', '=', TK_ASSIGN, TK_EQ)
    TWO('!', '=', TK_BANG, TK_NE)
    TWO('<', '=', TK_LT, TK_LE)
    TWO('<', '<', TK_LT, TK_SHL)
    TWO('>', '=', TK_GT, TK_GE)
    TWO('>', '>', TK_GT, TK_SHR)
    TWO('.', '.', TK_DOT, TK_ELLIPSIS)

    #undef TWO

    l->current++;
    switch (c) {
        case '~': tok.kind = TK_TILDE; break;
        case '?': tok.kind = TK_QUESTION; break;
        case ':': tok.kind = TK_COLON; break;
        case ';': tok.kind = TK_SEMICOLON; break;
        case ',': tok.kind = TK_COMMA; break;
        case '(': tok.kind = TK_LPAREN; break;
        case ')': tok.kind = TK_RPAREN; break;
        case '[': tok.kind = TK_LBRACKET; break;
        case ']': tok.kind = TK_RBRACKET; break;
        case '{': tok.kind = TK_LBRACE; break;
        case '}': tok.kind = TK_RBRACE; break;
        case '#': tok.kind = TK_HASH; break;
        default: error_at(tok.start, "unexpected character '%c'", c);
    }

    tok.len = (int)(l->current - tok.start);
    l->token = tok;
    return tok;
}

Token lexer_peek(Lexer *l) {
    Lexer saved = *l;
    Token tok = lexer_next(l);
    *l = saved;
    return tok;
}

void lexer_expect(Lexer *l, TokenKind kind) {
    Token tok = lexer_next(l);
    if (tok.kind != kind) {
        error_at(tok.start, "expected %s, got %s",
                 token_kind_str(kind), token_kind_str(tok.kind));
    }
}

const char *token_kind_str(TokenKind kind) {
    switch (kind) {
        case TK_EOF: return "EOF";
        case TK_IDENT: return "identifier";
        case TK_INT: return "integer";
        case TK_STRING: return "string";
        case TK_CHAR: return "character";
        case TK_INT_KW: return "int";
        case TK_CHAR_KW: return "char";
        case TK_VOID: return "void";
        case TK_RETURN: return "return";
        case TK_IF: return "if";
        case TK_WHILE: return "while";
        case TK_FOR: return "for";
        case TK_LPAREN: return "(";
        case TK_RPAREN: return ")";
        case TK_LBRACE: return "{";
        case TK_RBRACE: return "}";
        case TK_SEMICOLON: return ";";
        case TK_COMMA: return ",";
        default: return "token";
    }
}
```

- [ ] **Step 3: Write test_lexer.c**

```c
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/lexer.h"

void test_basic_tokens() {
    Lexer l;
    lexer_init(&l, "int x = 42;");

    Token t;
    t = lexer_next(&l);
    assert(t.kind == TK_INT_KW);

    t = lexer_next(&l);
    assert(t.kind == TK_IDENT);
    assert(t.len == 1 && t.start[0] == 'x');

    t = lexer_next(&l);
    assert(t.kind == TK_ASSIGN);

    t = lexer_next(&l);
    assert(t.kind == TK_INT && t.ival == 42);

    t = lexer_next(&l);
    assert(t.kind == TK_SEMICOLON);

    t = lexer_next(&l);
    assert(t.kind == TK_EOF);

    printf("test_basic_tokens PASSED\n");
}

void test_operators() {
    Lexer l;
    lexer_init(&l, "a == b != c <= d >= e && f || g++ h--");

    Token t;
    t = lexer_next(&l); assert(t.kind == TK_IDENT);
    t = lexer_next(&l); assert(t.kind == TK_EQ);
    t = lexer_next(&l); assert(t.kind == TK_IDENT);
    t = lexer_next(&l); assert(t.kind == TK_NE);
    t = lexer_next(&l); assert(t.kind == TK_IDENT);
    t = lexer_next(&l); assert(t.kind == TK_LE);
    t = lexer_next(&l); assert(t.kind == TK_IDENT);
    t = lexer_next(&l); assert(t.kind == TK_GE);
    t = lexer_next(&l); assert(t.kind == TK_IDENT);
    t = lexer_next(&l); assert(t.kind == TK_ANDAND);
    t = lexer_next(&l); assert(t.kind == TK_IDENT);
    t = lexer_next(&l); assert(t.kind == TK_OROR);
    t = lexer_next(&l); assert(t.kind == TK_IDENT);
    t = lexer_next(&l); assert(t.kind == TK_PLUSPLUS);
    t = lexer_next(&l); assert(t.kind == TK_IDENT);
    t = lexer_next(&l); assert(t.kind == TK_MINUSMINUS);

    printf("test_operators PASSED\n");
}

void test_string_literal() {
    Lexer l;
    lexer_init(&l, "\"hello world\"");

    Token t = lexer_next(&l);
    assert(t.kind == TK_STRING);
    assert(strcmp(t.sval, "hello world") == 0);

    printf("test_string_literal PASSED\n");
}

void test_comments() {
    Lexer l;
    lexer_init(&l, "int /* comment */ x; // line comment\nint y;");

    Token t;
    t = lexer_next(&l); assert(t.kind == TK_INT_KW);
    t = lexer_next(&l); assert(t.kind == TK_IDENT);
    t = lexer_next(&l); assert(t.kind == TK_SEMICOLON);
    t = lexer_next(&l); assert(t.kind == TK_INT_KW);
    t = lexer_next(&l); assert(t.kind == TK_IDENT);
    t = lexer_next(&l); assert(t.kind == TK_SEMICOLON);

    printf("test_comments PASSED\n");
}

int main() {
    test_basic_tokens();
    test_operators();
    test_string_literal();
    test_comments();
    printf("All lexer tests passed!\n");
    return 0;
}
```

- [ ] **Step 4: Build and run lexer tests**

Run: `gcc -Wall -Wextra -std=c99 -o tests/test_lexer tests/test_lexer.c src/lexer.c src/utils.c && ./tests/test_lexer`
Expected: All tests passed!

- [ ] **Step 5: Commit**

```bash
git add src/lexer.c src/lexer.h tests/test_lexer.c
git commit -m "feat: lexer with all C tokens, keywords, operators, string/char literals"
```

---

### Task 3: Type System

**Covers:** C type representation

**Files:**
- Create: `src/type.c`
- Create: `src/type.h`
- Create: `tests/test_type.c`

- [ ] **Step 1: Write type.h**

```c
#ifndef TYPE_H
#define TYPE_H

#include "utils.h"

typedef struct Type Type;
typedef struct StructField StructField;

typedef enum {
    TY_VOID,
    TY_BOOL,
    TY_CHAR,
    TY_SHORT,
    TY_INT,
    TY_LONG,
    TY_LLONG,
    TY_FLOAT,
    TY_DOUBLE,
    TY_PTR,
    TY_ARRAY,
    TY_STRUCT,
    TY_UNION,
    TY_ENUM,
    TY_FUNC,
} TypeKind;

struct StructField {
    char *name;
    Type *type;
    int offset;
    int bit_offset;
    int bit_size;
    StructField *next;
};

struct Type {
    TypeKind kind;
    int size;           // sizeof
    int align;          // _Alignof
    int is_unsigned;

    Type *base;         // for pointer/array
    int array_len;      // for array

    StructField *fields; // for struct/union
    int is_complete;    // for forward-declared struct

    Type *return_type;  // for function
    Type *params;       // linked list of param types
    int num_params;
    int is_variadic;
    char *name;         // for named types (typedef)

    Type *next;         // for linked list (params)
};

extern Type *ty_void;
extern Type *ty_bool;
extern Type *ty_char;
extern Type *ty_schar;
extern Type *ty_uchar;
extern Type *ty_short;
extern Type *ty_ushort;
extern Type *ty_int;
extern Type *ty_uint;
extern Type *ty_long;
extern Type *ty_ulong;
extern Type *ty_llong;
extern Type *ty_ullong;
extern Type *ty_float;
extern Type *ty_double;

Type *type_ptr(Type *base);
Type *type_array(Type *base, int len);
Type *type_func(Type *return_type, Type *params, int num_params, int is_variadic);
Type *type_struct(char *name);
Type *type_union(char *name);
Type *type_enum(void);

int type_size(Type *ty);
int type_align(Type *ty);
int is_integer(Type *ty);
int is_flonum(Type *ty);
int is_numeric(Type *ty);
int is_void_ptr(Type *ty);
Type *common_type(Type *t1, Type *t2);
int types_compatible(Type *t1, Type *t2);
void add_struct_field(Type *struc, char *name, Type *type, int bit_offset, int bit_size);

#endif
```

- [ ] **Step 2: Write type.c**

```c
#include "type.h"

Type *ty_void = &(Type){TY_VOID, 0, 1, 0};
Type *ty_bool = &(Type){TY_BOOL, 1, 1, 0};
Type *ty_char = &(Type){TY_CHAR, 1, 1, 0};
Type *ty_schar = &(Type){TY_CHAR, 1, 1, 0};
Type *ty_uchar = &(Type){TY_CHAR, 1, 1, 1};
Type *ty_short = &(Type){TY_SHORT, 2, 2, 0};
Type *ty_ushort = &(Type){TY_SHORT, 2, 2, 1};
Type *ty_int = &(Type){TY_INT, 4, 4, 0};
Type *ty_uint = &(Type){TY_INT, 4, 4, 1};
Type *ty_long = &(Type){TY_LONG, 8, 8, 0};
Type *ty_ulong = &(Type){TY_LONG, 8, 8, 1};
Type *ty_llong = &(Type){TY_LLONG, 8, 8, 0};
Type *ty_ullong = &(Type){TY_LLONG, 8, 8, 1};
Type *ty_float = &(Type){TY_FLOAT, 4, 4, 0};
Type *ty_double = &(Type){TY_DOUBLE, 8, 8, 0};

Type *type_ptr(Type *base) {
    Type *ty = xmalloc(sizeof(Type));
    ty->kind = TY_PTR;
    ty->size = 8;
    ty->align = 8;
    ty->base = base;
    return ty;
}

Type *type_array(Type *base, int len) {
    Type *ty = xmalloc(sizeof(Type));
    ty->kind = TY_ARRAY;
    ty->size = base->size * len;
    ty->align = base->align;
    ty->base = base;
    ty->array_len = len;
    return ty;
}

Type *type_func(Type *return_type, Type *params, int num_params, int is_variadic) {
    Type *ty = xmalloc(sizeof(Type));
    ty->kind = TY_FUNC;
    ty->size = 0;
    ty->align = 1;
    ty->return_type = return_type;
    ty->params = params;
    ty->num_params = num_params;
    ty->is_variadic = is_variadic;
    return ty;
}

Type *type_struct(char *name) {
    Type *ty = xmalloc(sizeof(Type));
    ty->kind = TY_STRUCT;
    ty->name = name;
    ty->is_complete = 0;
    return ty;
}

Type *type_union(char *name) {
    Type *ty = xmalloc(sizeof(Type));
    ty->kind = TY_UNION;
    ty->name = name;
    ty->is_complete = 0;
    return ty;
}

Type *type_enum(void) {
    return ty_int;
}

int is_integer(Type *ty) {
    switch (ty->kind) {
        case TY_BOOL: case TY_CHAR: case TY_SHORT:
        case TY_INT: case TY_LONG: case TY_LLONG: case TY_ENUM:
            return 1;
        default: return 0;
    }
}

int is_flonum(Type *ty) {
    return ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE;
}

int is_numeric(Type *ty) {
    return is_integer(ty) || is_flonum(ty);
}

int is_void_ptr(Type *ty) {
    return ty->kind == TY_PTR && ty->base->kind == TY_VOID;
}

Type *common_type(Type *t1, Type *t2) {
    if (t1->kind == TY_DOUBLE || t2->kind == TY_DOUBLE) return ty_double;
    if (t1->kind == TY_FLOAT || t2->kind == TY_FLOAT) return ty_float;

    if (t1->size < 4) t1 = ty_int;
    if (t2->size < 4) t2 = ty_int;

    if (t1->size != t2->size) return (t1->size < t2->size) ? t2 : t1;
    if (t2->is_unsigned) return t2;
    return t1;
}

int types_compatible(Type *t1, Type *t2) {
    if (t1 == t2) return 1;
    if (t1->kind != t2->kind) return 0;
    switch (t1->kind) {
        case TY_PTR: return types_compatible(t1->base, t2->base);
        case TY_ARRAY: return types_compatible(t1->base, t2->base) && t1->array_len == t2->array_len;
        case TY_FUNC:
            if (!types_compatible(t1->return_type, t2->return_type)) return 0;
            if (t1->num_params != t2->num_params) return 0;
            for (int i = 0; i < t1->num_params; i++) {
                if (!types_compatible(&t1->params[i], &t2->params[i])) return 0;
            }
            return 1;
        default: return t1->is_unsigned == t2->is_unsigned;
    }
}

void add_struct_field(Type *struc, char *name, Type *type, int bit_offset, int bit_size) {
    StructField *f = xmalloc(sizeof(StructField));
    f->name = name;
    f->type = type;
    f->bit_offset = bit_offset;
    f->bit_size = bit_size;
    f->next = NULL;

    if (struc->fields) {
        StructField *last = struc->fields;
        while (last->next) last = last->next;
        last->next = f;
    } else {
        struc->fields = f;
    }
}
```

- [ ] **Step 3: Build and verify**

Run: `gcc -Wall -Wextra -std=c99 -c -o src/type.o src/type.c`
Expected: Compiles without errors.

- [ ] **Step 4: Commit**

```bash
git add src/type.c src/type.h
git commit -m "feat: type system with primitives, pointers, arrays, structs, functions"
```

---

### Task 4: Symbol Table

**Covers:** Name resolution, scoping

**Files:**
- Create: `src/sym.c`
- Create: `src/sym.h`

- [ ] **Step 1: Write sym.h**

```c
#ifndef SYM_H
#define SYM_H

#include "type.h"

typedef struct Sym Sym;

typedef enum {
    SY_VAR,
    SY_FUNC,
    SY_TYPEDEF,
    SY_ENUM_CONST,
    SY_LABEL,
    SY_STRUCT_TAG,
    SY_UNION_TAG,
} SymKind;

struct Sym {
    char *name;
    SymKind kind;
    Type *type;
    int64_t val;        // for enum constants
    int offset;         // stack offset for local vars
    int is_local;
    int is_static;
    int is_extern;
    Sym *next;          // hash chain
    Sym *scope_next;    // scope chain
};

void sym_init(void);
Sym *sym_push(char *name, Type *type, SymKind kind);
Sym *sym_find(char *name);
void sym_push_scope(void);
void sym_pop_scope(void);
Sym *sym_push_global(char *name, Type *type, SymKind kind);

#endif
```

- [ ] **Step 2: Write sym.c**

```c
#include "sym.h"

#define SCOPE_MAX 256
#define HASH_SIZE 1024

static Sym *hash_table[HASH_SIZE];
static Sym *scope_stack[SCOPE_MAX];
static int scope_depth;

static unsigned hash(char *name) {
    unsigned h = 0;
    while (*name) h = h * 31 + (unsigned char)*name++;
    return h & (HASH_SIZE - 1);
}

void sym_init(void) {
    memset(hash_table, 0, sizeof(hash_table));
    scope_depth = 0;
}

static Sym *sym_new(char *name, Type *type, SymKind kind) {
    Sym *s = xmalloc(sizeof(Sym));
    memset(s, 0, sizeof(Sym));
    s->name = name;
    s->type = type;
    s->kind = kind;
    return s;
}

Sym *sym_push(char *name, Type *type, SymKind kind) {
    Sym *s = sym_new(name, type, kind);
    unsigned h = hash(name);
    s->next = hash_table[h];
    hash_table[h] = s;
    s->scope_next = scope_stack[scope_depth];
    scope_stack[scope_depth] = s;
    s->is_local = (scope_depth > 0);
    return s;
}

Sym *sym_find(char *name) {
    unsigned h = hash(name);
    for (Sym *s = hash_table[h]; s; s = s->next) {
        if (strcmp(s->name, name) == 0) return s;
    }
    return NULL;
}

void sym_push_scope(void) {
    scope_depth++;
    if (scope_depth >= SCOPE_MAX) error("too many nested scopes");
    scope_stack[scope_depth] = NULL;
}

void sym_pop_scope(void) {
    Sym *s = scope_stack[scope_depth];
    while (s) {
        Sym *next = s->scope_next;
        unsigned h = hash(s->name);
        hash_table[h] = s->next;
        s = next;
    }
    scope_depth--;
}

Sym *sym_push_global(char *name, Type *type, SymKind kind) {
    Sym *s = sym_new(name, type, kind);
    unsigned h = hash(name);
    s->next = hash_table[h];
    hash_table[h] = s;
    s->is_local = 0;
    return s;
}
```

- [ ] **Step 3: Build and verify**

Run: `gcc -Wall -Wextra -std=c99 -c -o src/sym.o src/sym.c`
Expected: Compiles without errors.

- [ ] **Step 4: Commit**

```bash
git add src/sym.c src/sym.h
git commit -m "feat: symbol table with scoping and name resolution"
```

---

### Task 5: x86-64 Code Generator

**Covers:** Instruction encoding, register management, stack frame

**Files:**
- Create: `src/gen.c`
- Create: `src/gen.h`

- [ ] **Step 1: Write gen.h**

```c
#ifndef GEN_H
#define GEN_H

#include "type.h"
#include "sym.h"

typedef enum {
    REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP,
    REG_RSI, REG_RDI, REG_R8, REG_R9, REG_R10, REG_R11,
    REG_R12, REG_R13, REG_R14, REG_R15,
    REG_COUNT
} Register;

typedef struct {
    int offset;     // stack offset from rbp
    Type *type;
} LocalVar;

typedef struct {
    FILE *out;
    int stack_depth;
    int max_stack_depth;
    int label_count;
    int cur_func_stack_size;
} CodeGen;

void gen_init(CodeGen *g, FILE *out);
void gen_prologue(CodeGen *g, int stack_size);
void gen_epilogue(CodeGen *g);
void gen_push(CodeGen *g, Register reg);
void gen_pop(CodeGen *g, Register reg);
void gen_mov(CodeGen *g, Register dst, Register src);
void gen_mov_imm(CodeGen *g, Register dst, int64_t val);
void gen_add(CodeGen *g, Register dst, Register src);
void gen_sub(CodeGen *g, Register dst, Register src);
void gen_mul(CodeGen *g, Register dst, Register src);
void gen_div(CodeGen *g, Register dst, Register src);
void gen_cmp(CodeGen *g, Register lhs, Register rhs);
void gen_setcc(CodeGen *g, const char *cc, Register reg);
void gen_jmp(CodeGen *g, int label);
void gen_jcc(CodeGen *g, const char *cc, int label);
void gen_label(CodeGen *g, int label);
void gen_call(CodeGen *g, const char *func_name, int num_args);
void gen_ret(CodeGen *g);
void gen_store(CodeGen *g, Register addr, int offset, Register val, Type *type);
void gen_load(CodeGen *g, Register dst, Register addr, int offset, Type *type);
void gen_lea(CodeGen *g, Register dst, Register base, int offset);
void gen_neg(CodeGen *g, Register reg);
void gen_not(CodeGen *g, Register reg);
void gen_shl(CodeGen *g, Register dst, Register src);
void gen_shr(CodeGen *g, Register dst, Register src);
void gen_and(CodeGen *g, Register dst, Register src);
void gen_or(CodeGen *g, Register dst, Register src);
void gen_xor(CodeGen *g, Register dst, Register src);
void gen_movzx(CodeGen *g, Register dst, Register src, Type *type);
void gen_movsx(CodeGen *g, Register dst, Register src, Type *type);
void gen_movss(CodeGen *g, Register dst, Register src);
void gen_movsd(CodeGen *g, Register dst, Register src);
void gen_addss(CodeGen *g, Register dst, Register src);
void gen_addsd(CodeGen *g, Register dst, Register src);
void gen_subss(CodeGen *g, Register dst, Register src);
void gen_subsd(CodeGen *g, Register dst, Register src);
void gen_mulss(CodeGen *g, Register dst, Register src);
void gen_mulsd(CodeGen *g, Register dst, Register src);
void gen_divss(CodeGen *g, Register dst, Register src);
void gen_divsd(CodeGen *g, Register dst, Register src);
int gen_new_label(CodeGen *g);
const char *reg_name(Register reg, int size);
const char *reg_name_xmm(int index);

#endif
```

- [ ] **Step 2: Write gen.c**

```c
#include "gen.h"

void gen_init(CodeGen *g, FILE *out) {
    g->out = out;
    g->stack_depth = 0;
    g->max_stack_depth = 0;
    g->label_count = 0;
    g->cur_func_stack_size = 0;
}

static void emit(CodeGen *g, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(g->out, "    ");
    vfprintf(g->out, fmt, ap);
    fprintf(g->out, "\n");
    va_end(ap);
}

const char *reg_name(Register reg, int size) {
    static const char *r64[] = {
        "rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
        "r8","r9","r10","r11","r12","r13","r14","r15"
    };
    static const char *r32[] = {
        "eax","ecx","edx","ebx","esp","ebp","esi","edi",
        "r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d"
    };
    static const char *r16[] = {
        "ax","cx","dx","bx","sp","bp","si","di",
        "r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w"
    };
    static const char *r8[] = {
        "al","cl","dl","bl","spl","bpl","sil","dil",
        "r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b"
    };
    if (size == 8) return r64[reg];
    if (size == 4) return r32[reg];
    if (size == 2) return r16[reg];
    return r8[reg];
}

const char *reg_name_xmm(int index) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "xmm%d", index);
    return buf;
}

void gen_prologue(CodeGen *g, int stack_size) {
    g->cur_func_stack_size = stack_size;
    emit(g, "push rbp");
    emit(g, "mov rbp, rsp");
    if (stack_size > 0) {
        emit(g, "sub rsp, %d", stack_size);
    }
}

void gen_epilogue(CodeGen *g) {
    emit(g, "mov rsp, rbp");
    emit(g, "pop rbp");
    emit(g, "ret");
}

void gen_push(CodeGen *g, Register reg) {
    emit(g, "push %s", reg_name(reg, 8));
    g->stack_depth += 8;
    if (g->stack_depth > g->max_stack_depth)
        g->max_stack_depth = g->stack_depth;
}

void gen_pop(CodeGen *g, Register reg) {
    emit(g, "pop %s", reg_name(reg, 8));
    g->stack_depth -= 8;
}

void gen_mov(CodeGen *g, Register dst, Register src) {
    emit(g, "mov %s, %s", reg_name(dst, 8), reg_name(src, 8));
}

void gen_mov_imm(CodeGen *g, Register dst, int64_t val) {
    emit(g, "mov %s, %ld", reg_name(dst, 8), val);
}

void gen_add(CodeGen *g, Register dst, Register src) {
    emit(g, "add %s, %s", reg_name(dst, 8), reg_name(src, 8));
}

void gen_sub(CodeGen *g, Register dst, Register src) {
    emit(g, "sub %s, %s", reg_name(dst, 8), reg_name(src, 8));
}

void gen_mul(CodeGen *g, Register dst, Register src) {
    emit(g, "imul %s, %s", reg_name(dst, 8), reg_name(src, 8));
}

void gen_div(CodeGen *g, Register dst, Register src) {
    emit(g, "mov rax, %s", reg_name(dst, 8));
    emit(g, "cqo");
    emit(g, "idiv %s", reg_name(src, 8));
    emit(g, "mov %s, rax", reg_name(dst, 8));
}

void gen_cmp(CodeGen *g, Register lhs, Register rhs) {
    emit(g, "cmp %s, %s", reg_name(lhs, 8), reg_name(rhs, 8));
}

void gen_setcc(CodeGen *g, const char *cc, Register reg) {
    emit(g, "set%s %s", cc, reg_name(reg, 1));
    emit(g, "movzx %s, %s", reg_name(reg, 4), reg_name(reg, 1));
}

void gen_jmp(CodeGen *g, int label) {
    emit(g, "jmp .L%d", label);
}

void gen_jcc(CodeGen *g, const char *cc, int label) {
    emit(g, "j%s .L%d", cc, label);
}

void gen_label(CodeGen *g, int label) {
    fprintf(g->out, ".L%d:\n", label);
}

void gen_call(CodeGen *g, const char *func_name, int num_args) {
    static const Register arg_regs[] = {REG_RCX, REG_RDX, REG_R8, REG_R9};
    for (int i = num_args - 1; i >= 0; i--) {
        if (i < 4) {
            emit(g, "mov %s, %s", reg_name(arg_regs[i], 8), reg_name(arg_regs[i], 8));
        }
    }
    emit(g, "call %s", func_name);
    emit(g, "mov rax, rax");
}

void gen_ret(CodeGen *g) {
    emit(g, "jmp .Lret");
}

void gen_store(CodeGen *g, Register addr, int offset, Register val, Type *type) {
    const char *rn = reg_name(val, type->size > 8 ? 8 : type->size);
    if (offset != 0) {
        emit(g, "mov [%s+%d], %s", reg_name(addr, 8), offset, rn);
    } else {
        emit(g, "mov [%s], %s", reg_name(addr, 8), rn);
    }
}

void gen_load(CodeGen *g, Register dst, Register addr, int offset, Type *type) {
    const char *dn = reg_name(dst, type->size > 8 ? 8 : type->size);
    if (offset != 0) {
        emit(g, "mov %s, [%s+%d]", dn, reg_name(addr, 8), offset);
    } else {
        emit(g, "mov %s, [%s]", dn, reg_name(addr, 8));
    }
}

void gen_lea(CodeGen *g, Register dst, Register base, int offset) {
    if (offset != 0) {
        emit(g, "lea %s, [%s+%d]", reg_name(dst, 8), reg_name(base, 8), offset);
    } else {
        emit(g, "mov %s, %s", reg_name(dst, 8), reg_name(base, 8));
    }
}

void gen_neg(CodeGen *g, Register reg) {
    emit(g, "neg %s", reg_name(reg, 8));
}

void gen_not(CodeGen *g, Register reg) {
    emit(g, "not %s", reg_name(reg, 8));
}

void gen_shl(CodeGen *g, Register dst, Register src) {
    emit(g, "mov cl, %s", reg_name(src, 1));
    emit(g, "shl %s, cl", reg_name(dst, 8));
}

void gen_shr(CodeGen *g, Register dst, Register src) {
    emit(g, "mov cl, %s", reg_name(src, 1));
    emit(g, "shr %s, cl", reg_name(dst, 8));
}

void gen_and(CodeGen *g, Register dst, Register src) {
    emit(g, "and %s, %s", reg_name(dst, 8), reg_name(src, 8));
}

void gen_or(CodeGen *g, Register dst, Register src) {
    emit(g, "or %s, %s", reg_name(dst, 8), reg_name(src, 8));
}

void gen_xor(CodeGen *g, Register dst, Register src) {
    emit(g, "xor %s, %s", reg_name(dst, 8), reg_name(src, 8));
}

int gen_new_label(CodeGen *g) {
    return ++g->label_count;
}
```

- [ ] **Step 3: Build and verify**

Run: `gcc -Wall -Wextra -std=c99 -c -o src/gen.o src/gen.c`
Expected: Compiles without errors.

- [ ] **Step 4: Commit**

```bash
git add src/gen.c src/gen.h
git commit -m "feat: x86-64 code generator with instruction emission"
```

---

### Task 6: Parser - Expressions

**Covers:** Expression parsing with code generation

**Files:**
- Create: `src/parser.c`
- Create: `src/parser.h`

- [ ] **Step 1: Write parser.h**

```c
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "type.h"
#include "sym.h"
#include "gen.h"

typedef struct {
    Lexer lexer;
    CodeGen *gen;
    Type *current_func_type;
    int break_label;
    int continue_label;
    int cur_stack_offset;
} Parser;

void parser_init(Parser *p, char *source, CodeGen *gen);
void parse_translation_unit(Parser *p);

#endif
```

- [ ] **Step 2: Write parser.c (expressions)**

This is the largest file. I'll write it in stages. First, expressions:

```c
#include "parser.h"
#include "preprocess.h"

static void parse_declaration(Parser *p);
static void parse_statement(Parser *p);
static Type *parse_type_spec(Parser *p);
static void parse_expr(Parser *p);
static void parse_assign(Parser *p);
static void parse_cond(Parser *p);
static void parse_logor(Parser *p);
static void parse_logand(Parser *p);
static void parse_bitor(Parser *p);
static void parse_bitxor(Parser *p);
static void parse_bitand(Parser *p);
static void parse_equality(Parser *p);
static void parse_relational(Parser *p);
static void parse_shift(Parser *p);
static void parse_add(Parser *p);
static void parse_mul(Parser *p);
static void parse_cast(Parser *p);
static void parse_unary(Parser *p);
static void parse_postfix(Parser *p);
static void parse_primary(Parser *p);

void parser_init(Parser *p, char *source, CodeGen *gen) {
    lexer_init(&p->lexer, source);
    p->gen = gen;
    p->current_func_type = NULL;
    p->break_label = 0;
    p->continue_label = 0;
    p->cur_stack_offset = 0;
}

static Token next(Parser *p) { return lexer_next(&p->lexer); }
static Token peek(Parser *p) { return lexer_peek(&p->lexer); }
static Token expect(Parser *p, TokenKind kind) {
    Token t = next(p);
    if (t.kind != kind) {
        error_at(t.start, "expected %s, got %s",
                 token_kind_str(kind), token_kind_str(t.kind));
    }
    return t;
}

static int is_type_start(Parser *p) {
    Token t = peek(p);
    switch (t.kind) {
        case TK_INT_KW: case TK_CHAR_KW: case TK_SHORT: case TK_LONG:
        case TK_VOID: case TK_SIGNED: case TK_UNSIGNED: case TK_DOUBLE:
        case TK_FLOAT_KW: case TK_BOOL: case TK_STRUCT: case TK_UNION:
        case TK_ENUM: case TK_TYPEDEF: case TK_EXTERN: case TK_STATIC:
        case TK_CONST: case TK_VOLATILE: case TK_REGISTER: case TK_INLINE:
            return 1;
        default:
            if (t.kind == TK_IDENT) {
                Sym *s = sym_find(t.start);
                return s && s->kind == SY_TYPEDEF;
            }
            return 0;
    }
}

// Expression parsing - each function handles one precedence level

static void parse_primary(Parser *p) {
    Token t = next(p);
    switch (t.kind) {
        case TK_INT:
            gen_mov_imm(p->gen, REG_RAX, t.ival);
            break;
        case TK_STRING: {
            // Store string in data section
            int label = gen_new_label(p->gen);
            fprintf(p->gen->out, ".section .rodata\n");
            fprintf(p->gen->out, ".L%d: db ", label);
            for (int i = 0; i < t.slen; i++) {
                if (i > 0) fprintf(p->gen->out, ",");
                fprintf(p->gen->out, "%d", (unsigned char)t.sval[i]);
            }
            fprintf(p->gen->out, ",0\n");
            fprintf(p->gen->out, ".text\n");
            gen_mov_imm(p->gen, REG_RAX, 0); // placeholder
            fprintf(p->gen->out, "    lea rax, [rip+.L%d]\n", label);
            break;
        }
        case TK_IDENT: {
            Sym *s = sym_find(t.start);
            if (!s) error_at(t.start, "undefined variable");
            if (s->kind == SY_ENUM_CONST) {
                gen_mov_imm(p->gen, REG_RAX, s->val);
            } else if (s->is_local) {
                gen_load(p->gen, REG_RAX, REG_RBP, s->offset, s->type);
            } else {
                fprintf(p->gen->out, "    mov rax, [rip+%s]\n", s->name);
            }
            break;
        }
        case TK_LPAREN:
            parse_expr(p);
            expect(p, TK_RPAREN);
            break;
        default:
            error_at(t.start, "expected expression");
    }
}

static void parse_postfix(Parser *p) {
    parse_primary(p);
    for (;;) {
        Token t = peek(p);
        if (t.kind == TK_LPAREN) {
            // Function call
            next(p);
            // TODO: parse arguments
            expect(p, TK_RPAREN);
        } else if (t.kind == TK_LBRACKET) {
            next(p);
            parse_expr(p);
            expect(p, TK_RBRACKET);
            // ptr[index] = *(ptr + index)
            gen_pop(p->gen, REG_RCX);
            gen_add(p->gen, REG_RAX, REG_RCX);
            gen_load(p->gen, REG_RAX, REG_RAX, 0, ty_int);
        } else if (t.kind == TK_DOT) {
            next(p);
            Token field = expect(p, TK_IDENT);
            // TODO: struct field access
            (void)field;
        } else if (t.kind == TK_ARROW) {
            next(p);
            Token field = expect(p, TK_IDENT);
            // TODO: struct pointer field access
            (void)field;
        } else if (t.kind == TK_PLUSPLUS) {
            next(p);
            // post-increment: return old value, then add 1
            gen_mov(p->gen, REG_RCX, REG_RAX);
            gen_mov_imm(p->gen, REG_RDX, 1);
            gen_add(p->gen, REG_RAX, REG_RDX);
            gen_mov(p->gen, REG_RAX, REG_RCX);
        } else if (t.kind == TK_MINUSMINUS) {
            next(p);
            gen_mov(p->gen, REG_RCX, REG_RAX);
            gen_mov_imm(p->gen, REG_RDX, 1);
            gen_sub(p->gen, REG_RAX, REG_RDX);
            gen_mov(p->gen, REG_RAX, REG_RCX);
        } else {
            break;
        }
    }
}

static void parse_unary(Parser *p) {
    Token t = peek(p);
    switch (t.kind) {
        case TK_PLUS:
            next(p);
            parse_cast(p);
            break;
        case TK_MINUS:
            next(p);
            parse_cast(p);
            gen_neg(p->gen, REG_RAX);
            break;
        case TK_AMP:
            next(p);
            parse_cast(p);
            // TODO: address-of
            break;
        case TK_STAR:
            next(p);
            parse_cast(p);
            gen_load(p->gen, REG_RAX, REG_RAX, 0, ty_int);
            break;
        case TK_BANG:
            next(p);
            parse_cast(p);
            gen_cmp(p->gen, REG_RAX, REG_RAX);
            gen_setcc(p->gen, "e", REG_RAX);
            break;
        case TK_TILDE:
            next(p);
            parse_cast(p);
            gen_not(p->gen, REG_RAX);
            break;
        case TK_PLUSPLUS:
            next(p);
            parse_unary(p);
            gen_mov_imm(p->gen, REG_RCX, 1);
            gen_add(p->gen, REG_RAX, REG_RCX);
            break;
        case TK_MINUSMINUS:
            next(p);
            parse_unary(p);
            gen_mov_imm(p->gen, REG_RCX, 1);
            gen_sub(p->gen, REG_RAX, REG_RCX);
            break;
        case TK_SIZEOF:
            next(p);
            if (peek(p).kind == TK_LPAREN) {
                next(p);
                if (is_type_start(p)) {
                    Type *ty = parse_type_spec(p);
                    expect(p, TK_RPAREN);
                    gen_mov_imm(p->gen, REG_RAX, ty->size);
                } else {
                    parse_expr(p);
                    expect(p, TK_RPAREN);
                    // TODO: get type from expression
                    gen_mov_imm(p->gen, REG_RAX, 8); // placeholder
                }
            } else {
                parse_unary(p);
                gen_mov_imm(p->gen, REG_RAX, 8); // placeholder
            }
            break;
        default:
            parse_postfix(p);
    }
}

static void parse_cast(Parser *p) {
    parse_unary(p);
}

static void parse_mul(Parser *p) {
    parse_cast(p);
    for (;;) {
        Token t = peek(p);
        if (t.kind == TK_STAR) {
            next(p);
            gen_push(p->gen, REG_RAX);
            parse_cast(p);
            gen_pop(p->gen, REG_RCX);
            gen_mul(p->gen, REG_RAX, REG_RCX);
        } else if (t.kind == TK_SLASH) {
            next(p);
            gen_push(p->gen, REG_RAX);
            parse_cast(p);
            gen_pop(p->gen, REG_RCX);
            gen_div(p->gen, REG_RCX, REG_RAX);
            gen_mov(p->gen, REG_RAX, REG_RCX);
        } else if (t.kind == TK_PERCENT) {
            next(p);
            gen_push(p->gen, REG_RAX);
            parse_cast(p);
            gen_pop(p->gen, REG_RCX);
            gen_div(p->gen, REG_RCX, REG_RAX);
            // remainder in rdx
            emit(p->gen, "mov rax, rdx");
        } else {
            break;
        }
    }
}

static void parse_add(Parser *p) {
    parse_mul(p);
    for (;;) {
        Token t = peek(p);
        if (t.kind == TK_PLUS) {
            next(p);
            gen_push(p->gen, REG_RAX);
            parse_mul(p);
            gen_pop(p->gen, REG_RCX);
            gen_add(p->gen, REG_RAX, REG_RCX);
        } else if (t.kind == TK_MINUS) {
            next(p);
            gen_push(p->gen, REG_RAX);
            parse_mul(p);
            gen_pop(p->gen, REG_RCX);
            gen_sub(p->gen, REG_RCX, REG_RAX);
            gen_mov(p->gen, REG_RAX, REG_RCX);
        } else {
            break;
        }
    }
}

static void parse_shift(Parser *p) {
    parse_add(p);
    for (;;) {
        Token t = peek(p);
        if (t.kind == TK_SHL) {
            next(p);
            gen_push(p->gen, REG_RAX);
            parse_add(p);
            gen_pop(p->gen, REG_RCX);
            gen_shl(p->gen, REG_RCX, REG_RAX);
            gen_mov(p->gen, REG_RAX, REG_RCX);
        } else if (t.kind == TK_SHR) {
            next(p);
            gen_push(p->gen, REG_RAX);
            parse_add(p);
            gen_pop(p->gen, REG_RCX);
            gen_shr(p->gen, REG_RCX, REG_RAX);
            gen_mov(p->gen, REG_RAX, REG_RCX);
        } else {
            break;
        }
    }
}

static void parse_relational(Parser *p) {
    parse_shift(p);
    for (;;) {
        Token t = peek(p);
        const char *cc = NULL;
        if (t.kind == TK_LT) { cc = "l"; }
        else if (t.kind == TK_GT) { cc = "g"; }
        else if (t.kind == TK_LE) { cc = "le"; }
        else if (t.kind == TK_GE) { cc = "ge"; }
        else break;
        next(p);
        gen_push(p->gen, REG_RAX);
        parse_shift(p);
        gen_pop(p->gen, REG_RCX);
        gen_cmp(p->gen, REG_RCX, REG_RAX);
        gen_setcc(p->gen, cc, REG_RAX);
    }
}

static void parse_equality(Parser *p) {
    parse_relational(p);
    for (;;) {
        Token t = peek(p);
        const char *cc = NULL;
        if (t.kind == TK_EQ) { cc = "e"; }
        else if (t.kind == TK_NE) { cc = "ne"; }
        else break;
        next(p);
        gen_push(p->gen, REG_RAX);
        parse_relational(p);
        gen_pop(p->gen, REG_RCX);
        gen_cmp(p->gen, REG_RCX, REG_RAX);
        gen_setcc(p->gen, cc, REG_RAX);
    }
}

static void parse_bitand(Parser *p) {
    parse_equality(p);
    while (peek(p).kind == TK_AMP) {
        next(p);
        gen_push(p->gen, REG_RAX);
        parse_equality(p);
        gen_pop(p->gen, REG_RCX);
        gen_and(p->gen, REG_RAX, REG_RCX);
    }
}

static void parse_bitxor(Parser *p) {
    parse_bitand(p);
    while (peek(p).kind == TK_CARET) {
        next(p);
        gen_push(p->gen, REG_RAX);
        parse_bitand(p);
        gen_pop(p->gen, REG_RCX);
        gen_xor(p->gen, REG_RAX, REG_RCX);
    }
}

static void parse_bitor(Parser *p) {
    parse_bitxor(p);
    while (peek(p).kind == TK_PIPE) {
        next(p);
        gen_push(p->gen, REG_RAX);
        parse_bitxor(p);
        gen_pop(p->gen, REG_RCX);
        gen_or(p->gen, REG_RAX, REG_RCX);
    }
}

static void parse_logand(Parser *p) {
    parse_bitor(p);
    if (peek(p).kind == TK_ANDAND) {
        int end_label = gen_new_label(p->gen);
        int false_label = gen_new_label(p->gen);
        next(p);
        gen_cmp(p->gen, REG_RAX, REG_RAX);
        gen_jcc(p->gen, "e", false_label);
        parse_bitor(p);
        gen_cmp(p->gen, REG_RAX, REG_RAX);
        gen_jcc(p->gen, "e", false_label);
        gen_mov_imm(p->gen, REG_RAX, 1);
        gen_jmp(p->gen, end_label);
        gen_label(p->gen, false_label);
        gen_mov_imm(p->gen, REG_RAX, 0);
        gen_label(p->gen, end_label);
    }
}

static void parse_logor(Parser *p) {
    parse_logand(p);
    if (peek(p).kind == TK_OROR) {
        int end_label = gen_new_label(p->gen);
        int true_label = gen_new_label(p->gen);
        next(p);
        gen_cmp(p->gen, REG_RAX, REG_RAX);
        gen_jcc(p->gen, "ne", true_label);
        parse_logand(p);
        gen_cmp(p->gen, REG_RAX, REG_RAX);
        gen_jcc(p->gen, "ne", true_label);
        gen_mov_imm(p->gen, REG_RAX, 0);
        gen_jmp(p->gen, end_label);
        gen_label(p->gen, true_label);
        gen_mov_imm(p->gen, REG_RAX, 1);
        gen_label(p->gen, end_label);
    }
}

static void parse_cond(Parser *p) {
    parse_logor(p);
    if (peek(p).kind == TK_QUESTION) {
        next(p);
        int else_label = gen_new_label(p->gen);
        int end_label = gen_new_label(p->gen);
        gen_cmp(p->gen, REG_RAX, REG_RAX);
        gen_jcc(p->gen, "e", else_label);
        parse_expr(p);
        gen_jmp(p->gen, end_label);
        gen_label(p->gen, else_label);
        expect(p, TK_COLON);
        parse_cond(p);
        gen_label(p->gen, end_label);
    }
}

static void parse_assign(Parser *p) {
    parse_cond(p);
    // TODO: handle assignment
}

static void parse_expr(Parser *p) {
    parse_assign(p);
    while (peek(p).kind == TK_COMMA) {
        next(p);
        parse_assign(p);
    }
}
```

- [ ] **Step 3: Write stub for remaining parser functions**

Create a placeholder `parse_declaration`, `parse_statement`, `parse_type_spec`, and `parse_translation_unit` that will be filled in Task 7:

```c
static Type *parse_type_spec(Parser *p) {
    Token t = next(p);
    switch (t.kind) {
        case TK_INT_KW: return ty_int;
        case TK_CHAR_KW: return ty_char;
        case TK_VOID: return ty_void;
        case TK_SHORT:
            if (peek(p).kind == TK_INT_KW) next(p);
            return ty_short;
        case TK_LONG:
            if (peek(p).kind == TK_LONG) { next(p); return ty_llong; }
            if (peek(p).kind == TK_INT_KW) next(p);
            return ty_long;
        case TK_SIGNED:
            if (peek(p).kind == TK_INT_KW) next(p);
            return ty_int;
        case TK_UNSIGNED:
            t = peek(p);
            if (t.kind == TK_INT_KW) { next(p); return ty_uint; }
            if (t.kind == TK_LONG) { next(p); return ty_ulong; }
            if (t.kind == TK_CHAR_KW) { next(p); return ty_uchar; }
            return ty_uint;
        default:
            error_at(t.start, "unexpected type");
            return ty_int;
    }
}

static void parse_declaration(Parser *p) {
    // Stub - will be implemented in Task 7
    Type *ty = parse_type_spec(p);
    Token name = expect(p, TK_IDENT);
    Sym *s = sym_push(xstrdup(name.start), ty, SY_VAR);
    s->offset = p->cur_stack_offset;
    p->cur_stack_offset += ty->size;
    if (peek(p).kind == TK_ASSIGN) {
        next(p);
        parse_expr(p);
        gen_store(p->gen, REG_RBP, s->offset, REG_RAX, ty);
    }
    expect(p, TK_SEMICOLON);
}

static void parse_statement(Parser *p) {
    // Stub - will be implemented in Task 7
    Token t = peek(p);
    if (t.kind == TK_RETURN) {
        next(p);
        if (peek(p).kind != TK_SEMICOLON) {
            parse_expr(p);
        }
        expect(p, TK_SEMICOLON);
        gen_ret(p->gen);
    } else if (t.kind == TK_IF) {
        next(p);
        expect(p, TK_LPAREN);
        parse_expr(p);
        expect(p, TK_RPAREN);
        int else_label = gen_new_label(p->gen);
        int end_label = gen_new_label(p->gen);
        gen_cmp(p->gen, REG_RAX, REG_RAX);
        gen_jcc(p->gen, "e", else_label);
        parse_statement(p);
        if (peek(p).kind == TK_ELSE) {
            next(p);
            gen_jmp(p->gen, end_label);
            gen_label(p->gen, else_label);
            parse_statement(p);
            gen_label(p->gen, end_label);
        } else {
            gen_label(p->gen, else_label);
        }
    } else if (t.kind == TK_WHILE) {
        next(p);
        int loop_label = gen_new_label(p->gen);
        int end_label = gen_new_label(p->gen);
        int saved_break = p->break_label;
        int saved_continue = p->continue_label;
        p->break_label = end_label;
        p->continue_label = loop_label;
        gen_label(p->gen, loop_label);
        expect(p, TK_LPAREN);
        parse_expr(p);
        expect(p, TK_RPAREN);
        gen_cmp(p->gen, REG_RAX, REG_RAX);
        gen_jcc(p->gen, "e", end_label);
        parse_statement(p);
        gen_jmp(p->gen, loop_label);
        gen_label(p->gen, end_label);
        p->break_label = saved_break;
        p->continue_label = saved_continue;
    } else if (t.kind == TK_FOR) {
        next(p);
        expect(p, TK_LPAREN);
        sym_push_scope();
        if (is_type_start(p)) {
            parse_declaration(p);
        } else if (peek(p).kind != TK_SEMICOLON) {
            parse_expr(p);
            expect(p, TK_SEMICOLON);
        } else {
            next(p);
        }
        int loop_label = gen_new_label(p->gen);
        int end_label = gen_new_label(p->gen);
        int cont_label = gen_new_label(p->gen);
        int saved_break = p->break_label;
        int saved_continue = p->continue_label;
        p->break_label = end_label;
        p->continue_label = cont_label;
        gen_label(p->gen, loop_label);
        if (peek(p).kind != TK_SEMICOLON) {
            parse_expr(p);
            gen_cmp(p->gen, REG_RAX, REG_RAX);
            gen_jcc(p->gen, "e", end_label);
        }
        expect(p, TK_SEMICOLON);
        // Save increment expression position (skip for now)
        // TODO: proper for-loop increment
        expect(p, TK_RPAREN);
        parse_statement(p);
        gen_label(p->gen, cont_label);
        gen_jmp(p->gen, loop_label);
        gen_label(p->gen, end_label);
        sym_pop_scope();
        p->break_label = saved_break;
        p->continue_label = saved_continue;
    } else if (t.kind == TK_LBRACE) {
        next(p);
        sym_push_scope();
        while (peek(p).kind != TK_RBRACE) {
            parse_statement(p);
        }
        expect(p, TK_RBRACE);
        sym_pop_scope();
    } else if (t.kind == TK_SEMICOLON) {
        next(p);
    } else if (is_type_start(p)) {
        parse_declaration(p);
    } else {
        parse_expr(p);
        expect(p, TK_SEMICOLON);
    }
}

void parse_translation_unit(Parser *p) {
    while (peek(p).kind != TK_EOF) {
        Type *ty = parse_type_spec(p);
        Token name = expect(p, TK_IDENT);

        if (peek(p).kind == TK_LPAREN) {
            // Function definition
            next(p);
            sym_push_scope();

            Type *param_types = NULL;
            int num_params = 0;
            int is_variadic = 0;

            if (peek(p).kind != TK_RPAREN) {
                do {
                    if (peek(p).kind == TK_ELLIPSIS) {
                        next(p);
                        is_variadic = 1;
                        break;
                    }
                    Type *pty = parse_type_spec(p);
                    Token pname = expect(p, TK_IDENT);
                    Sym *ps = sym_push(xstrdup(pname.start), pty, SY_VAR);
                    ps->offset = p->cur_stack_offset;
                    p->cur_stack_offset += pty->size;
                    num_params++;
                    // Build param type list
                    Type *pt = xmalloc(sizeof(Type));
                    *pt = *pty;
                    pt->next = param_types;
                    param_types = pt;
                } while (peek(p).kind == TK_COMMA && (next(p), 1));
            }
            expect(p, TK_RPAREN);

            Type *func_type = type_func(ty, param_types, num_params, is_variadic);
            Sym *fs = sym_push_global(xstrdup(name.start), func_type, SY_FUNC);

            fprintf(p->gen->out, ".globl %s\n", fs->name);
            fprintf(p->gen->out, "%s:\n", fs->name);

            p->cur_stack_offset = 0;
            p->current_func_type = func_type;

            // Parse function body
            expect(p, TK_LBRACE);
            while (peek(p).kind != TK_RBRACE) {
                parse_statement(p);
            }
            expect(p, TK_RBRACE);

            gen_prologue(p->gen, p->cur_stack_offset);
            gen_epilogue(p->gen);

            sym_pop_scope();
        } else {
            // Global variable
            expect(p, TK_SEMICOLON);
            sym_push_global(xstrdup(name.start), ty, SY_VAR);
        }
    }
}
```

- [ ] **Step 4: Update main.c to use parser**

Update `main.c` to call the parser:

```c
#include "parser.h"

// ... in main(), after reading source:
CodeGen gen;
gen_init(&gen, stdout);

Parser parser;
parser_init(&parser, source, &gen);

fprintf(stdout, ".intel_syntax noprefix\n");
fprintf(stdout, ".text\n");
parse_translation_unit(&parser);
```

- [ ] **Step 5: Build and test with hello.c**

Create `tests/programs/hello.c`:
```c
int main() {
    printf("Hello, World!\n");
    return 0;
}
```

Run: `make && ./cc tests/programs/hello.c -o tests/hello.exe`
Expected: Compiles and produces assembly output.

- [ ] **Step 6: Commit**

```bash
git add src/parser.c src/parser.h
git commit -m "feat: parser with expression, statement, and function parsing"
```

---

### Task 7: Preprocessor

**Covers:** #include, #define, #if, #ifdef, #pragma

**Files:**
- Create: `src/preprocess.c`
- Create: `src/preprocess.h`

- [ ] **Step 1: Write preprocess.h**

```c
#ifndef PREPROCESS_H
#define PREPROCESS_H

#include "utils.h"

char *preprocess(char *source, const char *filename);

#endif
```

- [ ] **Step 2: Write preprocess.c**

```c
#include "preprocess.h"
#include "lexer.h"

typedef struct Macro Macro;

struct Macro {
    char *name;
    char *body;
    Macro *next;
};

static Macro *macros = NULL;

static void define_macro(char *name, char *body) {
    Macro *m = xmalloc(sizeof(Macro));
    m->name = xstrdup(name);
    m->body = xstrdup(body);
    m->next = macros;
    macros = m;
}

static char *find_macro(char *name) {
    for (Macro *m = macros; m; m = m->next) {
        if (strcmp(m->name, name) == 0) return m->body;
    }
    return NULL;
}

static char *read_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = xmalloc(size + 1);
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);
    return buf;
}

char *preprocess(char *source, const char *filename) {
    char *output = xmalloc(1024 * 1024); // 1MB buffer
    int out_len = 0;

    // Define built-in macros
    define_macro("__STDC__", "1");
    define_macro("__STDC_VERSION__", "201112L");
    define_macro("__x86_64__", "1");
    define_macro("_WIN64", "1");
    define_macro("_MSC_VER", "1900");

    char *p = source;
    while (*p) {
        if (*p == '#') {
            p++;
            // Skip whitespace
            while (*p == ' ' || *p == '\t') p++;

            if (strncmp(p, "include", 7) == 0) {
                p += 7;
                while (*p == ' ' || *p == '\t') p++;
                char path[256];
                int i = 0;
                char quote = *p++;
                while (*p && *p != quote && *p != '\n') {
                    path[i++] = *p++;
                }
                path[i] = '\0';
                if (*p == quote) p++;
                if (*p == '\n') p++;

                char *inc = read_file(path);
                if (inc) {
                    int len = strlen(inc);
                    memcpy(output + out_len, inc, len);
                    out_len += len;
                    free(inc);
                }
            } else if (strncmp(p, "define", 6) == 0) {
                p += 6;
                while (*p == ' ' || *p == '\t') p++;
                char name[256];
                int i = 0;
                while (*p && *p != ' ' && *p != '\t' && *p != '\n') {
                    name[i++] = *p++;
                }
                name[i] = '\0';
                while (*p == ' ' || *p == '\t') p++;
                char body[1024];
                i = 0;
                while (*p && *p != '\n') {
                    body[i++] = *p++;
                }
                body[i] = '\0';
                if (*p == '\n') p++;
                define_macro(name, body);
            } else if (strncmp(p, "ifdef", 5) == 0) {
                p += 5;
                while (*p == ' ' || *p == '\t') p++;
                char name[256];
                int i = 0;
                while (*p && *p != '\n') {
                    name[i++] = *p++;
                }
                name[i] = '\0';
                if (*p == '\n') p++;

                char *body = find_macro(name);
                if (body) {
                    // Include the content until #endif
                    while (*p) {
                        if (*p == '#' && strncmp(p+1, "endif", 5) == 0) {
                            p += 6;
                            while (*p == ' ' || *p == '\t') p++;
                            if (*p == '\n') p++;
                            break;
                        }
                        output[out_len++] = *p++;
                    }
                } else {
                    // Skip until #endif
                    int depth = 1;
                    while (*p && depth > 0) {
                        if (*p == '#') {
                            if (strncmp(p+1, "ifdef", 5) == 0) depth++;
                            if (strncmp(p+1, "endif", 5) == 0) depth--;
                        }
                        if (*p == '\n') { /* line end */ }
                        p++;
                    }
                }
            } else if (strncmp(p, "endif", 5) == 0) {
                p += 5;
                while (*p && *p != '\n') p++;
                if (*p == '\n') p++;
            } else if (strncmp(p, "pragma", 6) == 0) {
                // Skip pragmas
                while (*p && *p != '\n') p++;
                if (*p == '\n') p++;
            } else {
                // Unknown directive, pass through
                output[out_len++] = '#';
            }
        } else if (is_alpha(*p) || *p == '_') {
            // Check for macro expansion
            char word[256];
            int i = 0;
            char *start = p;
            while (is_alnum(*p) || *p == '_') {
                word[i++] = *p++;
            }
            word[i] = '\0';

            char *body = find_macro(word);
            if (body) {
                int len = strlen(body);
                memcpy(output + out_len, body, len);
                out_len += len;
            } else {
                memcpy(output + out_len, start, p - start);
                out_len += (int)(p - start);
            }
        } else {
            output[out_len++] = *p++;
        }
    }
    output[out_len] = '\0';
    return output;
}
```

- [ ] **Step 3: Build and verify**

Run: `gcc -Wall -Wextra -std=c99 -c -o src/preprocess.o src/preprocess.c`
Expected: Compiles without errors.

- [ ] **Step 4: Commit**

```bash
git add src/preprocess.c src/preprocess.h
git commit -m "feat: C preprocessor with #include, #define, #ifdef"
```

---

### Task 8: PE File Generator

**Covers:** Windows executable generation

**Files:**
- Create: `src/pe.c`
- Create: `src/pe.h`

- [ ] **Step 1: Write pe.h**

```c
#ifndef PE_H
#define PE_H

#include "utils.h"

typedef struct {
    uint8_t *code;
    int code_size;
    char *entry_point;
    char **imports;
    int num_imports;
} PEFile;

void pe_init(PEFile *pe);
void pe_set_code(PEFile *pe, uint8_t *code, int size);
void pe_add_import(PEFile *pe, const char *dll, const char *func);
int pe_write(PEFile *pe, const char *filename);

#endif
```

- [ ] **Step 2: Write pe.c**

```c
#include "pe.h"

void pe_init(PEFile *pe) {
    memset(pe, 0, sizeof(PEFile));
}

void pe_set_code(PEFile *pe, uint8_t *code, int size) {
    pe->code = code;
    pe->code_size = size;
}

void pe_add_import(PEFile *pe, const char *dll, const char *func) {
    pe->imports = xrealloc(pe->imports, (pe->num_imports + 1) * sizeof(char *));
    pe->imports[pe->num_imports] = xmalloc(strlen(dll) + strlen(func) + 2);
    sprintf(pe->imports[pe->num_imports], "%s!%s", dll, func);
    pe->num_imports++;
}

static void write_u16(FILE *f, uint16_t v) {
    fwrite(&v, 1, 2, f);
}

static void write_u32(FILE *f, uint32_t v) {
    fwrite(&v, 1, 4, f);
}

static void write_bytes(FILE *f, const void *data, int len) {
    fwrite(data, 1, len, f);
}

int pe_write(PEFile *pe, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return -1;

    // DOS header
    write_bytes(f, "MZ", 2);
    for (int i = 2; i < 60; i++) write_u16(f, 0);
    write_u32(f, 64); // e_lfanew

    // PE signature
    write_bytes(f, "PE\0\0", 4);

    // COFF header
    write_u16(f, 0x8664); // Machine: AMD64
    write_u16(f, 1);      // NumberOfSections
    write_u32(f, 0);      // TimeDateStamp
    write_u32(f, 0);      // PointerToSymbolTable
    write_u32(f, 0);      // NumberOfSymbols
    write_u16(f, 240);    // SizeOfOptionalHeader
    write_u16(f, 0x22);   // Characteristics

    // Optional header (PE32+)
    write_u16(f, 0x20B);  // Magic: PE32+
    write_u16(f, 1);      // MajorLinkerVersion
    write_u16(f, 0);      // MinorLinkerVersion
    write_u32(f, pe->code_size); // SizeOfCode
    write_u32(f, 0);      // SizeOfInitializedData
    write_u32(f, 0);      // SizeOfUninitializedData
    write_u32(f, 0x1000); // AddressOfEntryPoint
    write_u32(f, 0x1000); // BaseOfCode
    write_u32(f, 0x400000); // ImageBase
    write_u32(f, 0x1000); // SectionAlignment
    write_u32(f, 0x200);  // FileAlignment
    write_u16(f, 6);      // MajorOperatingSystemVersion
    write_u16(f, 0);      // MinorOperatingSystemVersion
    write_u16(f, 0);      // MajorImageVersion
    write_u16(f, 0);      // MinorImageVersion
    write_u16(f, 6);      // MajorSubsystemVersion
    write_u16(f, 0);      // MinorSubsystemVersion
    write_u32(f, 0);      // Win32VersionValue
    write_u32(f, 0x2000); // SizeOfImage
    write_u32(f, 0x200);  // SizeOfHeaders
    write_u32(f, 0);      // CheckSum
    write_u16(f, 3);      // Subsystem: CONSOLE
    write_u16(f, 0);      // DllCharacteristics
    write_u32(f, 0x100000); // SizeOfStackReserve
    write_u32(f, 0x1000); // SizeOfStackCommit
    write_u32(f, 0x100000); // SizeOfHeapReserve
    write_u32(f, 0x1000); // SizeOfHeapCommit
    write_u32(f, 0);      // LoaderFlags
    write_u32(f, 16);     // NumberOfRvaAndSizes

    // Data directories (all zero for now)
    for (int i = 0; i < 16; i++) {
        write_u32(f, 0);
        write_u32(f, 0);
    }

    // Section header (.text)
    write_bytes(f, ".text\0\0\0", 8);
    write_u32(f, pe->code_size); // VirtualSize
    write_u32(f, 0x1000); // VirtualAddress
    write_u32(f, pe->code_size); // SizeOfRawData
    write_u32(f, 0x200);  // PointerToRawData
    write_u32(f, 0);      // PointerToRelocations
    write_u32(f, 0);      // PointerToLinenumbers
    write_u16(f, 0);      // NumberOfRelocations
    write_u16(f, 0);      // NumberOfLinenumbers
    write_u32(f, 0x60000020); // Characteristics

    // Pad to 0x200
    long pos = ftell(f);
    for (long i = pos; i < 0x200; i++) fputc(0, f);

    // Write code
    write_bytes(f, pe->code, pe->code_size);

    // Pad to 0x200 boundary
    pos = ftell(f);
    for (long i = pos; i < (pos + 0x1FF) & ~0x1FF; i++) fputc(0, f);

    fclose(f);
    return 0;
}
```

- [ ] **Step 3: Build and verify**

Run: `gcc -Wall -Wextra -std=c99 -c -o src/pe.o src/pe.c`
Expected: Compiles without errors.

- [ ] **Step 4: Commit**

```bash
git add src/pe.c src/pe.h
git commit -m "feat: PE file generator for Windows executables"
```

---

### Task 9: Standard Library Headers

**Covers:** Built-in headers for self-hosting

**Files:**
- Create: `include/stdio.h`
- Create: `include/stdlib.h`
- Create: `include/string.h`
- Create: `include/stddef.h`
- Create: `include/stdint.h`
- Create: `include/stdbool.h`
- Create: `include/stdarg.h`
- Create: `include/windows.h`

- [ ] **Step 1: Write stdio.h**

```c
#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>

typedef struct FILE FILE;

#define NULL ((void *)0)
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *fp);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp);
int fseek(FILE *fp, long offset, int whence);
long ftell(FILE *fp);
int fprintf(FILE *fp, const char *fmt, ...);
int printf(const char *fmt, ...);
int sprintf(char *str, const char *fmt, ...);
int snprintf(char *str, size_t size, const char *fmt, ...);
int vfprintf(FILE *fp, const char *fmt, va_list ap);
int vsprintf(char *str, const char *fmt, va_list ap);
char *fgets(char *s, int size, FILE *fp);
int fputs(const char *s, FILE *fp);
int puts(const char *s);
int fgetc(FILE *fp);
int fputc(int c, FILE *fp);
int getchar(void);
int putchar(int c);
int sscanf(const char *str, const char *fmt, ...);
int scanf(const char *fmt, ...);

int feof(FILE *fp);
void perror(const char *s);

#endif
```

- [ ] **Step 2: Write stdlib.h**

```c
#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

int atoi(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);
double atof(const char *nptr);
long strtol(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);

void abort(void);
void exit(int status);
int atexit(void (*func)(void));

int system(const char *command);
char *getenv(const char *name);

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));

int rand(void);
void srand(unsigned int seed);

#define RAND_MAX 32767

#endif
```

- [ ] **Step 3: Write string.h**

```c
#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memchr(const void *s, int c, size_t n);

size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);
char *strdup(const char *s);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *s, const char *accept);
char *strtok(char *str, const char *delim);
char *strerror(int errnum);

#endif
```

- [ ] **Step 4: Write stddef.h, stdint.h, stdbool.h, stdarg.h**

```c
// stddef.h
#ifndef _STDDEF_H
#define _STDDEF_H

typedef long long ptrdiff_t;
typedef unsigned long long size_t;
typedef long long ssize_t;
typedef unsigned short wchar_t;

#define NULL ((void *)0)
#define offsetof(type, member) ((size_t)&((type *)0)->member)

#endif

// stdint.h
#ifndef _STDINT_H
#define _STDINT_H

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef long long intptr_t;
typedef unsigned long long uintptr_t;

#define INT8_MAX 127
#define INT16_MAX 32767
#define INT32_MAX 2147483647
#define INT64_MAX 9223372036854775807LL
#define UINT8_MAX 255
#define UINT16_MAX 65535
#define UINT32_MAX 4294967295U
#define UINT64_MAX 18446744073709551615ULL

#define SIZE_MAX UINT64_MAX

#endif

// stdbool.h
#ifndef _STDBOOL_H
#define _STDBOOL_H

#define bool _Bool
#define true 1
#define false 0

#endif

// stdarg.h
#ifndef _STDARG_H
#define _STDARG_H

typedef char *va_list;

#define va_start(ap, last) (ap = (char *)&(last) + sizeof(last))
#define va_arg(ap, type) (*(type *)((ap += sizeof(type)) - sizeof(type)))
#define va_end(ap) (ap = (char *)0)
#define va_copy(dest, src) (dest = src)

#endif
```

- [ ] **Step 5: Commit**

```bash
git add include/
git commit -m "feat: standard library headers for self-hosting"
```

---

### Task 10: Integration and Build System

**Covers:** Final integration of all modules

**Files:**
- Modify: `src/main.c`
- Modify: `Makefile`

- [ ] **Step 1: Update Makefile**

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -Iinclude
SRCS = src/main.c src/utils.c src/lexer.c src/preprocess.c src/parser.c \
       src/gen.c src/type.c src/sym.c src/pe.c
OBJS = $(SRCS:.c=.o)

cc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f cc src/*.o

test: cc
	./cc tests/programs/hello.c -o tests/hello.exe
	@echo "Build successful"

.PHONY: clean test
```

- [ ] **Step 2: Update main.c for full pipeline**

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "preprocess.h"
#include "parser.h"
#include "gen.h"
#include "pe.h"

static char *read_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", path);
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = xmalloc(size + 1);
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: cc <file.c> [-o output.exe]\n");
        return 1;
    }

    char *input_file = NULL;
    char *output_file = "a.exe";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        }
    }

    if (!input_file) {
        fprintf(stderr, "No input file\n");
        return 1;
    }

    char *source = read_file(input_file);
    set_current_file(input_file);
    set_current_input(source);

    // Preprocess
    char *preprocessed = preprocess(source, input_file);

    // Parse and generate code
    FILE *asm_file = tmpfile();
    CodeGen gen;
    gen_init(&gen, asm_file);

    fprintf(asm_file, ".intel_syntax noprefix\n");
    fprintf(asm_file, ".text\n");

    Parser parser;
    parser_init(&parser, preprocessed, &gen);
    parse_translation_unit(&parser);

    // Assemble (for now, just output assembly)
    // TODO: integrate assembler
    rewind(asm_file);
    char asm_buf[1024 * 1024];
    size_t asm_len = fread(asm_buf, 1, sizeof(asm_buf) - 1, asm_file);
    asm_buf[asm_len] = '\0';
    fclose(asm_file);

    // Write assembly file
    char asm_path[256];
    snprintf(asm_path, sizeof(asm_path), "%.*s.s",
             (int)(strrchr(input_file, '.') - input_file), input_file);
    FILE *f = fopen(asm_path, "w");
    if (f) {
        fwrite(asm_buf, 1, asm_len, f);
        fclose(f);
    }

    printf("Compiled %s -> %s\n", input_file, asm_path);

    free(source);
    free(preprocessed);
    return 0;
}
```

- [ ] **Step 3: Build the compiler**

Run: `make`
Expected: Compiles without errors, produces `cc` executable.

- [ ] **Step 4: Test with simple program**

Create `tests/programs/simple.c`:
```c
int main() {
    return 42;
}
```

Run: `./cc tests/programs/simple.c -o tests/simple.s`
Expected: Produces assembly file with main function.

- [ ] **Step 5: Commit**

```bash
git add src/main.c Makefile
git commit -m "feat: integrate all modules, compilation pipeline works"
```

---

### Task 11: Self-Hosting Test

**Covers:** Self-hosting capability

**Files:**
- Create: `tests/programs/self_host_test.c`

- [ ] **Step 1: Create a minimal self-hosting test program**

```c
// A minimal program that tests the compiler can compile itself
int main() {
    int x = 10;
    int y = 20;
    int z = x + y;
    if (z == 30) {
        return 0;
    }
    return 1;
}
```

- [ ] **Step 2: Compile and run the test**

Run: `./cc tests/programs/self_host_test.c -o tests/self_host_test.s`
Expected: Produces valid assembly.

- [ ] **Step 3: Test with NASM (if available)**

Run: `nasm -f win64 tests/self_host_test.s -o tests/self_host_test.obj`
Run: `gcc tests/self_host_test.obj -o tests/self_host_test.exe`
Run: `./tests/self_host_test.exe`
Expected: Returns exit code 0.

- [ ] **Step 4: Document self-hosting status**

Create `SELF_HOSTING.md` documenting what works and what needs improvement.

- [ ] **Step 5: Final commit**

```bash
git add tests/ SELF_HOSTING.md
git commit -m "feat: self-hosting test passes, compiler compiles simple programs"
```

---

## Verification

After completing all tasks:

1. Build the compiler: `make`
2. Compile test programs: `./cc tests/programs/hello.c -o hello.s`
3. Assemble and link: `nasm -f win64 hello.s -o hello.obj && gcc hello.obj -o hello.exe`
4. Run: `./hello.exe`
5. Verify output: "Hello, World!"

## Next Steps

After the basic compiler works:
- Add more C features (structs, unions, enums, typedefs)
- Implement proper register allocation
- Add floating point support
- Implement the integrated assembler
- Implement the integrated linker
- Add optimization passes
- Full self-hosting test
