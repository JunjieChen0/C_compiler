#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/preprocess.h"
#include "../src/utils.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    do { \
        printf("Running %s... ", #name); \
        name(); \
        printf("PASSED\n"); \
        tests_passed++; \
    } while(0)

#define ASSERT_STR_EQ(actual, expected) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            fprintf(stderr, "FAIL: expected \"%s\", got \"%s\"\n", (expected), (actual)); \
            tests_failed++; \
            return; \
        } \
    } while(0)

static void test_object_macro() {
    char input[] = "#define FOO 42\nint x = FOO;\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "int x = 42;\n");
    free(result);
}

static void test_multiple_macros() {
    char input[] = "#define A 1\n#define B 2\nint x = A + B;\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "int x = 1 + 2;\n");
    free(result);
}

static void test_ifdef_defined() {
    char input[] = "#define FOO\n#ifdef FOO\nint x = 1;\n#endif\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "int x = 1;\n");
    free(result);
}

static void test_ifdef_undefined() {
    char input[] = "#ifdef FOO\nint x = 1;\n#endif\nint y = 2;\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "int y = 2;\n");
    free(result);
}

static void test_ifndef() {
    char input[] = "#ifndef FOO\nint x = 1;\n#endif\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "int x = 1;\n");
    free(result);
}

static void test_builtin_macros() {
    char input[] = "int x = __STDC__;\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "int x = 1;\n");
    free(result);
}

static void test_pragma_skip() {
    char input[] = "#pragma once\nint x = 1;\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "int x = 1;\n");
    free(result);
}

static void test_comments_removed() {
    char input[] = "int x = 1; /* comment */\nint y = 2; // line comment\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "int x = 1; \nint y = 2; \n");
    free(result);
}

static void test_string_preserved() {
    char input[] = "char *s = \"hello world\";\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "char *s = \"hello world\";\n");
    free(result);
}

static void test_macro_in_string_not_expanded() {
    char input[] = "#define FOO 42\nchar *s = \"FOO\";\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "char *s = \"FOO\";\n");
    free(result);
}

static void test_undef() {
    char input[] = "#define FOO 42\n#undef FOO\nint x = FOO;\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "int x = FOO;\n");
    free(result);
}

static void test_nested_ifdef() {
    char input[] =
        "#define OUTER\n#ifdef OUTER\n"
        "#define INNER 10\n"
        "int x = INNER;\n"
        "#endif\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "int x = 10;\n");
    free(result);
}

static void test_nested_object_macro() {
    char input[] = "#define A B\n#define B 42\nint x = A;\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "int x = 42;\n");
    free(result);
}

static void test_chained_object_macro() {
    char input[] = "#define A B\n#define B C\n#define C 99\nint x = A;\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "int x = 99;\n");
    free(result);
}

static void test_elif_basic() {
    char input[] = "#if 0\nA\n#elif 1\nB\n#endif\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "B\n");
    free(result);
}

static void test_elif_false() {
    char input[] = "#if 0\nA\n#elif 0\nB\n#endif\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "");
    free(result);
}

static void test_elif_chain() {
    char input[] = "#if 0\nA\n#elif 0\nB\n#elif 1\nC\n#elif 1\nD\n#endif\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "C\n");
    free(result);
}

static void test_elif_then_else() {
    char input[] = "#if 0\nA\n#elif 0\nB\n#else\nC\n#endif\n";
    char *result = preprocess(input, "test.c");
    ASSERT_STR_EQ(result, "C\n");
    free(result);
}

int main() {
    set_current_file("test.c");
    set_current_input("");

    TEST(test_object_macro);
    TEST(test_multiple_macros);
    TEST(test_ifdef_defined);
    TEST(test_ifdef_undefined);
    TEST(test_ifndef);
    TEST(test_builtin_macros);
    TEST(test_pragma_skip);
    TEST(test_comments_removed);
    TEST(test_string_preserved);
    TEST(test_macro_in_string_not_expanded);
    TEST(test_undef);
    TEST(test_nested_ifdef);
    TEST(test_nested_object_macro);
    TEST(test_chained_object_macro);
    TEST(test_elif_basic);
    TEST(test_elif_false);
    TEST(test_elif_chain);
    TEST(test_elif_then_else);

    printf("\n%d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
