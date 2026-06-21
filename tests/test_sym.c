#include "../src/sym.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    scope_reset(); \
    tests_run++; \
    printf("  %-40s", name); \
} while(0)

#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { FAIL(msg); return; } \
} while(0)

static void test_scope_push_pop(void) {
    TEST("scope_push/pop basic");
    ASSERT(scope_depth() == 0, "initial depth should be 0");
    scope_push();
    ASSERT(scope_depth() == 1, "depth after push should be 1");
    scope_push();
    ASSERT(scope_depth() == 2, "depth after 2nd push should be 2");
    scope_pop();
    ASSERT(scope_depth() == 1, "depth after pop should be 1");
    scope_pop();
    ASSERT(scope_depth() == 0, "depth after 2nd pop should be 0");
    PASS();
}

static void test_declare_and_find(void) {
    TEST("declare and find in same scope");
    scope_push();
    Symbol *s = sym_declare("x", SYM_VAR, ty_int());
    ASSERT(s != NULL, "sym_declare returned NULL");
    ASSERT(strcmp(s->name, "x") == 0, "name mismatch");
    ASSERT(s->kind == SYM_VAR, "kind mismatch");

    Symbol *found = sym_find("x");
    ASSERT(found == s, "sym_find should return same symbol");
    ASSERT(sym_find("y") == NULL, "sym_find nonexistent should return NULL");
    scope_pop();
    PASS();
}

static void test_shadowing(void) {
    TEST("variable shadowing in nested scope");
    scope_push(); // global scope
    scope_push(); // block scope
    Symbol *outer = sym_declare("x", SYM_VAR, ty_int());

    scope_push(); // nested block
    Symbol *inner = sym_declare("x", SYM_VAR, ty_char());

    Symbol *found = sym_find("x");
    ASSERT(found == inner, "should find inner symbol");
    ASSERT(found->type->kind == TY_CHAR, "inner type should be char");

    Symbol *cur = sym_find_current("x");
    ASSERT(cur == inner, "find_current should find inner");

    scope_pop();

    found = sym_find("x");
    ASSERT(found == outer, "after pop should find outer");
    ASSERT(found->type->kind == TY_INT, "outer type should be int");

    scope_pop();
    scope_pop();
    PASS();
}

static void test_find_parent(void) {
    TEST("find from parent scope");
    scope_push();
    Symbol *outer = sym_declare("a", SYM_VAR, ty_int());

    scope_push();
    Symbol *found = sym_find("a");
    ASSERT(found == outer, "should find symbol in parent scope");

    Symbol *cur = sym_find_current("a");
    ASSERT(cur == NULL, "should not find in current scope");

    scope_pop();
    scope_pop();
    PASS();
}

static void test_multiple_symbols(void) {
    TEST("multiple symbols in same scope");
    scope_push();
    Symbol *a = sym_declare("a", SYM_VAR, ty_int());
    Symbol *b = sym_declare("b", SYM_FUNC, ty_int());
    Symbol *c = sym_declare("c", SYM_TYPEDEF, ty_char());

    ASSERT(sym_find("a") == a, "find a");
    ASSERT(sym_find("b") == b, "find b");
    ASSERT(sym_find("c") == c, "find c");
    ASSERT(a->kind == SYM_VAR, "a kind");
    ASSERT(b->kind == SYM_FUNC, "b kind");
    ASSERT(c->kind == SYM_TYPEDEF, "c kind");

    scope_pop();
    PASS();
}

static void test_enum_const(void) {
    TEST("enum constant declaration");
    scope_push();
    Symbol *e = sym_declare_enum("RED", 0);
    ASSERT(e != NULL, "sym_declare_enum returned NULL");
    ASSERT(e->kind == SYM_ENUM_CONST, "kind should be SYM_ENUM_CONST");
    ASSERT(e->enum_val == 0, "enum_val should be 0");
    ASSERT(e->type->kind == TY_INT, "enum const type should be int");

    Symbol *e2 = sym_declare_enum("GREEN", 1);
    ASSERT(e2->enum_val == 1, "GREEN val should be 1");

    ASSERT(sym_find("RED") == e, "find RED");
    ASSERT(sym_find("GREEN") == e2, "find GREEN");

    scope_pop();
    PASS();
}

static void test_global_scope(void) {
    TEST("global scope find_global");
    scope_push(); // global
    Symbol *g = sym_declare("global_var", SYM_VAR, ty_int());

    scope_push(); // local
    scope_push(); // nested local

    Symbol *found = sym_find_global("global_var");
    ASSERT(found == g, "find_global should find global symbol");

    found = sym_find("global_var");
    ASSERT(found == g, "find should also find global through parents");

    scope_pop();
    scope_pop();
    scope_pop();
    PASS();
}

static void test_symbol_kinds(void) {
    TEST("all symbol kinds");
    scope_push();
    sym_declare("v", SYM_VAR, ty_int());
    sym_declare("f", SYM_FUNC, ty_int());
    sym_declare("t", SYM_TYPEDEF, ty_int());
    sym_declare("l", SYM_LABEL, NULL);
    sym_declare("tag", SYM_TAG, NULL);

    ASSERT(sym_find("v")->kind == SYM_VAR, "var kind");
    ASSERT(sym_find("f")->kind == SYM_FUNC, "func kind");
    ASSERT(sym_find("t")->kind == SYM_TYPEDEF, "typedef kind");
    ASSERT(sym_find("l")->kind == SYM_LABEL, "label kind");
    ASSERT(sym_find("tag")->kind == SYM_TAG, "tag kind");

    scope_pop();
    PASS();
}

static void test_deep_nesting(void) {
    TEST("deep nesting (5 levels)");
    scope_push();
    Symbol *s0 = sym_declare("v", SYM_VAR, ty_int());
    scope_push();
    scope_push();
    scope_push();
    scope_push();
    Symbol *found = sym_find("v");
    ASSERT(found == s0, "should find through 5 levels");

    scope_pop();
    scope_pop();
    scope_pop();
    scope_pop();
    scope_pop();
    PASS();
}

static void test_redeclare_same_scope(void) {
    TEST("redeclare in same scope (last wins)");
    scope_push();
    sym_declare("x", SYM_VAR, ty_int());
    Symbol *s2 = sym_declare("x", SYM_VAR, ty_char());

    Symbol *found = sym_find("x");
    ASSERT(found == s2, "redeclared symbol should be found");

    scope_pop();
    PASS();
}

int main(void) {
    printf("=== Symbol Table Tests ===\n");

    test_scope_push_pop();
    test_declare_and_find();
    test_shadowing();
    test_find_parent();
    test_multiple_symbols();
    test_enum_const();
    test_global_scope();
    test_symbol_kinds();
    test_deep_nesting();
    test_redeclare_same_scope();

    printf("\nResults: %d/%d passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
