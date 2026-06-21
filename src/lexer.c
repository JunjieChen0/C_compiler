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

    #define THREE(c1, c2, c3, k) \
        if (*l->current == c1 && l->current[1] == c2 && l->current[2] == c3) { \
            l->current += 3; tok.kind = k; \
            tok.len = (int)(l->current - tok.start); l->token = tok; return tok; }

    #define TWO(c1, c2, k) \
        if (*l->current == c1 && l->current[1] == c2) { l->current += 2; \
            tok.kind = k; \
            tok.len = (int)(l->current - tok.start); l->token = tok; return tok; }

    THREE('<', '<', '=', TK_SHLASSIGN)
    THREE('>', '>', '=', TK_SHRASSIGN)

    TWO('+', '+', TK_PLUSPLUS)
    TWO('+', '=', TK_ADDASSIGN)
    TWO('-', '-', TK_MINUSMINUS)
    TWO('-', '=', TK_SUBASSIGN)
    TWO('-', '>', TK_ARROW)
    TWO('*', '=', TK_MULASSIGN)
    TWO('/', '=', TK_DIVASSIGN)
    TWO('%', '=', TK_MODASSIGN)
    TWO('&', '=', TK_ANDASSIGN)
    TWO('&', '&', TK_ANDAND)
    TWO('|', '=', TK_ORASSIGN)
    TWO('|', '|', TK_OROR)
    TWO('^', '=', TK_XORASSIGN)
    TWO('=', '=', TK_EQ)
    TWO('!', '=', TK_NE)
    TWO('<', '=', TK_LE)
    TWO('<', '<', TK_SHL)
    TWO('>', '=', TK_GE)
    TWO('>', '>', TK_SHR)
    TWO('#', '#', TK_HASHHASH)

    #undef TWO
    #undef THREE

    l->current++;
    switch (c) {
        case '+': tok.kind = TK_PLUS; break;
        case '-': tok.kind = TK_MINUS; break;
        case '*': tok.kind = TK_STAR; break;
        case '/': tok.kind = TK_SLASH; break;
        case '%': tok.kind = TK_PERCENT; break;
        case '&': tok.kind = TK_AMP; break;
        case '|': tok.kind = TK_PIPE; break;
        case '^': tok.kind = TK_CARET; break;
        case '=': tok.kind = TK_ASSIGN; break;
        case '!': tok.kind = TK_BANG; break;
        case '<': tok.kind = TK_LT; break;
        case '>': tok.kind = TK_GT; break;
        case '#': tok.kind = TK_HASH; break;
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
        case '.':
            if (*l->current == '.' && l->current[1] == '.') {
                l->current += 2;
                tok.kind = TK_ELLIPSIS;
            } else {
                tok.kind = TK_DOT;
            }
            break;
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
        case TK_LONG: return "long";
        case TK_UINT: return "unsigned integer";
        case TK_FLOAT: return "float";
        case TK_STRING: return "string";
        case TK_CHAR: return "character";
        case TK_INT_KW: return "int";
        case TK_CHAR_KW: return "char";
        case TK_VOID: return "void";
        case TK_RETURN: return "return";
        case TK_IF: return "if";
        case TK_ELSE: return "else";
        case TK_WHILE: return "while";
        case TK_FOR: return "for";
        case TK_DO: return "do";
        case TK_BREAK: return "break";
        case TK_CONTINUE: return "continue";
        case TK_SWITCH: return "switch";
        case TK_CASE: return "case";
        case TK_DEFAULT: return "default";
        case TK_GOTO: return "goto";
        case TK_SIZEOF: return "sizeof";
        case TK_STRUCT: return "struct";
        case TK_UNION: return "union";
        case TK_ENUM: return "enum";
        case TK_TYPEDEF: return "typedef";
        case TK_EXTERN: return "extern";
        case TK_STATIC: return "static";
        case TK_CONST: return "const";
        case TK_VOLATILE: return "volatile";
        case TK_REGISTER: return "register";
        case TK_INLINE: return "inline";
        case TK_RESTRICT: return "restrict";
        case TK_SIGNED: return "signed";
        case TK_UNSIGNED: return "unsigned";
        case TK_SHORT: return "short";
        case TK_LONG_KW: return "long";
        case TK_DOUBLE: return "double";
        case TK_FLOAT_KW: return "float";
        case TK_BOOL: return "_Bool";
        case TK_COMPLEX: return "_Complex";
        case TK_IMAGINARY: return "_Imaginary";
        case TK_PLUS: return "+";
        case TK_MINUS: return "-";
        case TK_STAR: return "*";
        case TK_SLASH: return "/";
        case TK_PERCENT: return "%";
        case TK_AMP: return "&";
        case TK_PIPE: return "|";
        case TK_CARET: return "^";
        case TK_TILDE: return "~";
        case TK_BANG: return "!";
        case TK_ASSIGN: return "=";
        case TK_LT: return "<";
        case TK_GT: return ">";
        case TK_PLUSPLUS: return "++";
        case TK_MINUSMINUS: return "--";
        case TK_SHL: return "<<";
        case TK_SHR: return ">>";
        case TK_EQ: return "==";
        case TK_NE: return "!=";
        case TK_LE: return "<=";
        case TK_GE: return ">=";
        case TK_ANDAND: return "&&";
        case TK_OROR: return "||";
        case TK_MULASSIGN: return "*=";
        case TK_DIVASSIGN: return "/=";
        case TK_MODASSIGN: return "%=";
        case TK_ADDASSIGN: return "+=";
        case TK_SUBASSIGN: return "-=";
        case TK_SHLASSIGN: return "<<=";
        case TK_SHRASSIGN: return ">>=";
        case TK_ANDASSIGN: return "&=";
        case TK_XORASSIGN: return "^=";
        case TK_ORASSIGN: return "|=";
        case TK_ARROW: return "->";
        case TK_DOT: return ".";
        case TK_QUESTION: return "?";
        case TK_COLON: return ":";
        case TK_SEMICOLON: return ";";
        case TK_COMMA: return ",";
        case TK_LPAREN: return "(";
        case TK_RPAREN: return ")";
        case TK_LBRACKET: return "[";
        case TK_RBRACKET: return "]";
        case TK_LBRACE: return "{";
        case TK_RBRACE: return "}";
        case TK_ELLIPSIS: return "...";
        case TK_HASH: return "#";
        case TK_HASHHASH: return "##";
        default: return "token";
    }
}
