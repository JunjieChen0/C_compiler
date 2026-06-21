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

void test_char_literal() {
    Lexer l;
    lexer_init(&l, "'a' '\\n' '\\0'");

    Token t;
    t = lexer_next(&l);
    assert(t.kind == TK_CHAR && t.ival == 'a');

    t = lexer_next(&l);
    assert(t.kind == TK_CHAR && t.ival == '\n');

    t = lexer_next(&l);
    assert(t.kind == TK_CHAR && t.ival == '\0');

    printf("test_char_literal PASSED\n");
}

void test_numbers() {
    Lexer l;
    lexer_init(&l, "42 0xff 077 3.14 1e10 2.5f 100U 100L 100UL");

    Token t;
    t = lexer_next(&l); assert(t.kind == TK_INT && t.ival == 42);
    t = lexer_next(&l); assert(t.kind == TK_INT && t.ival == 0xff);
    t = lexer_next(&l); assert(t.kind == TK_INT && t.ival == 077);
    t = lexer_next(&l); assert(t.kind == TK_FLOAT && t.fval > 3.13 && t.fval < 3.15);
    t = lexer_next(&l); assert(t.kind == TK_FLOAT && t.fval > 9.9e9);
    t = lexer_next(&l); assert(t.kind == TK_FLOAT && t.fval > 2.4 && t.fval < 2.6);
    t = lexer_next(&l); assert(t.kind == TK_UINT && t.ival == 100);
    t = lexer_next(&l); assert(t.kind == TK_LONG && t.ival == 100);
    t = lexer_next(&l); assert(t.kind == TK_LONG && t.ival == 100);

    printf("test_numbers PASSED\n");
}

void test_keywords() {
    Lexer l;
    lexer_init(&l, "if else while for return struct typedef void int char");

    Token t;
    t = lexer_next(&l); assert(t.kind == TK_IF);
    t = lexer_next(&l); assert(t.kind == TK_ELSE);
    t = lexer_next(&l); assert(t.kind == TK_WHILE);
    t = lexer_next(&l); assert(t.kind == TK_FOR);
    t = lexer_next(&l); assert(t.kind == TK_RETURN);
    t = lexer_next(&l); assert(t.kind == TK_STRUCT);
    t = lexer_next(&l); assert(t.kind == TK_TYPEDEF);
    t = lexer_next(&l); assert(t.kind == TK_VOID);
    t = lexer_next(&l); assert(t.kind == TK_INT_KW);
    t = lexer_next(&l); assert(t.kind == TK_CHAR_KW);

    printf("test_keywords PASSED\n");
}

void test_compound_assign() {
    Lexer l;
    lexer_init(&l, "+= -= *= /= %= <<= >>= &= ^= |= -> ...");

    Token t;
    t = lexer_next(&l); assert(t.kind == TK_ADDASSIGN);
    t = lexer_next(&l); assert(t.kind == TK_SUBASSIGN);
    t = lexer_next(&l); assert(t.kind == TK_MULASSIGN);
    t = lexer_next(&l); assert(t.kind == TK_DIVASSIGN);
    t = lexer_next(&l); assert(t.kind == TK_MODASSIGN);
    t = lexer_next(&l); assert(t.kind == TK_SHLASSIGN);
    t = lexer_next(&l); assert(t.kind == TK_SHRASSIGN);
    t = lexer_next(&l); assert(t.kind == TK_ANDASSIGN);
    t = lexer_next(&l); assert(t.kind == TK_XORASSIGN);
    t = lexer_next(&l); assert(t.kind == TK_ORASSIGN);
    t = lexer_next(&l); assert(t.kind == TK_ARROW);
    t = lexer_next(&l); assert(t.kind == TK_ELLIPSIS);

    printf("test_compound_assign PASSED\n");
}

void test_preprocessor_tokens() {
    Lexer l;
    lexer_init(&l, "# ##");

    Token t;
    t = lexer_next(&l); assert(t.kind == TK_HASH);
    t = lexer_next(&l); assert(t.kind == TK_HASHHASH);

    printf("test_preprocessor_tokens PASSED\n");
}

void test_string_escape() {
    Lexer l;
    lexer_init(&l, "\"hello\\nworld\\t\\\"\"");

    Token t = lexer_next(&l);
    assert(t.kind == TK_STRING);
    assert(strcmp(t.sval, "hello\nworld\t\"") == 0);

    printf("test_string_escape PASSED\n");
}

void test_peek() {
    Lexer l;
    lexer_init(&l, "int x");

    Token t = lexer_peek(&l);
    assert(t.kind == TK_INT_KW);

    t = lexer_next(&l);
    assert(t.kind == TK_INT_KW);

    t = lexer_peek(&l);
    assert(t.kind == TK_IDENT);

    t = lexer_next(&l);
    assert(t.kind == TK_IDENT);

    t = lexer_next(&l);
    assert(t.kind == TK_EOF);

    printf("test_peek PASSED\n");
}

void test_punctuation() {
    Lexer l;
    lexer_init(&l, "( ) [ ] { } ; , . ? : ~");

    Token t;
    t = lexer_next(&l); assert(t.kind == TK_LPAREN);
    t = lexer_next(&l); assert(t.kind == TK_RPAREN);
    t = lexer_next(&l); assert(t.kind == TK_LBRACKET);
    t = lexer_next(&l); assert(t.kind == TK_RBRACKET);
    t = lexer_next(&l); assert(t.kind == TK_LBRACE);
    t = lexer_next(&l); assert(t.kind == TK_RBRACE);
    t = lexer_next(&l); assert(t.kind == TK_SEMICOLON);
    t = lexer_next(&l); assert(t.kind == TK_COMMA);
    t = lexer_next(&l); assert(t.kind == TK_DOT);
    t = lexer_next(&l); assert(t.kind == TK_QUESTION);
    t = lexer_next(&l); assert(t.kind == TK_COLON);
    t = lexer_next(&l); assert(t.kind == TK_TILDE);

    printf("test_punctuation PASSED\n");
}

int main() {
    test_basic_tokens();
    test_operators();
    test_string_literal();
    test_comments();
    test_char_literal();
    test_numbers();
    test_keywords();
    test_compound_assign();
    test_preprocessor_tokens();
    test_string_escape();
    test_peek();
    test_punctuation();
    printf("All lexer tests passed!\n");
    return 0;
}
