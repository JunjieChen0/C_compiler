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
