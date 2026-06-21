#include <stdio.h>
#include <string.h>
#include "../src/type.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    tests_run++; \
    printf("  %-40s ", #name); \
    name(); \
    tests_passed++; \
    printf("PASS\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL\n    assertion failed: %s\n    at %s:%d\n", #cond, __FILE__, __LINE__); \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    int _a = (a), _b = (b); \
    if (_a != _b) { \
        printf("FAIL\n    %s == %d, expected %d\n    at %s:%d\n", #a, _a, _b, __FILE__, __LINE__); \
        return; \
    } \
} while(0)

static void test_void(void) {
    Type *t = ty_void();
    ASSERT_EQ(t->kind, TY_VOID);
    ASSERT_EQ(t->size, 0);
    ASSERT_EQ(t->align, 1);
    ASSERT_EQ(t->is_unsigned, 0);
}

static void test_bool(void) {
    Type *t = ty_bool();
    ASSERT_EQ(t->kind, TY_BOOL);
    ASSERT_EQ(t->size, 1);
    ASSERT_EQ(t->align, 1);
}

static void test_char(void) {
    Type *t = ty_char();
    ASSERT_EQ(t->kind, TY_CHAR);
    ASSERT_EQ(t->size, 1);
    ASSERT_EQ(t->align, 1);
    ASSERT_EQ(t->is_unsigned, 0);
}

static void test_uchar(void) {
    Type *t = ty_uchar();
    ASSERT_EQ(t->kind, TY_CHAR);
    ASSERT_EQ(t->size, 1);
    ASSERT_EQ(t->is_unsigned, 1);
}

static void test_short(void) {
    Type *t = ty_short();
    ASSERT_EQ(t->kind, TY_SHORT);
    ASSERT_EQ(t->size, 2);
    ASSERT_EQ(t->align, 2);
}

static void test_ushort(void) {
    Type *t = ty_ushort();
    ASSERT_EQ(t->kind, TY_SHORT);
    ASSERT_EQ(t->size, 2);
    ASSERT_EQ(t->is_unsigned, 1);
}

static void test_int(void) {
    Type *t = ty_int();
    ASSERT_EQ(t->kind, TY_INT);
    ASSERT_EQ(t->size, 4);
    ASSERT_EQ(t->align, 4);
    ASSERT_EQ(t->is_unsigned, 0);
}

static void test_uint(void) {
    Type *t = ty_uint();
    ASSERT_EQ(t->kind, TY_INT);
    ASSERT_EQ(t->size, 4);
    ASSERT_EQ(t->is_unsigned, 1);
}

static void test_long(void) {
    Type *t = ty_long();
    ASSERT_EQ(t->kind, TY_LONG);
    ASSERT_EQ(t->size, 8);
    ASSERT_EQ(t->align, 8);
}

static void test_ulong(void) {
    Type *t = ty_ulong();
    ASSERT_EQ(t->kind, TY_LONG);
    ASSERT_EQ(t->size, 8);
    ASSERT_EQ(t->is_unsigned, 1);
}

static void test_llong(void) {
    Type *t = ty_llong();
    ASSERT_EQ(t->kind, TY_LLONG);
    ASSERT_EQ(t->size, 8);
    ASSERT_EQ(t->align, 8);
}

static void test_ullong(void) {
    Type *t = ty_ullong();
    ASSERT_EQ(t->kind, TY_LLONG);
    ASSERT_EQ(t->size, 8);
    ASSERT_EQ(t->is_unsigned, 1);
}

static void test_float(void) {
    Type *t = ty_float();
    ASSERT_EQ(t->kind, TY_FLOAT);
    ASSERT_EQ(t->size, 4);
    ASSERT_EQ(t->align, 4);
}

static void test_double(void) {
    Type *t = ty_double();
    ASSERT_EQ(t->kind, TY_DOUBLE);
    ASSERT_EQ(t->size, 8);
    ASSERT_EQ(t->align, 8);
}

static void test_ptr(void) {
    Type *base = ty_int();
    Type *t = ty_ptr(base);
    ASSERT_EQ(t->kind, TY_PTR);
    ASSERT_EQ(t->size, 8);
    ASSERT_EQ(t->align, 8);
    ASSERT(t->base == base);
    ASSERT_EQ(t->base->kind, TY_INT);
}

static void test_ptr_to_ptr(void) {
    Type *t = ty_ptr(ty_ptr(ty_int()));
    ASSERT_EQ(t->kind, TY_PTR);
    ASSERT_EQ(t->base->kind, TY_PTR);
    ASSERT_EQ(t->base->base->kind, TY_INT);
}

static void test_array(void) {
    Type *t = ty_array(ty_int(), 10);
    ASSERT_EQ(t->kind, TY_ARRAY);
    ASSERT_EQ(t->size, 40);
    ASSERT_EQ(t->align, 4);
    ASSERT_EQ(t->array_len, 10);
    ASSERT_EQ(t->base->kind, TY_INT);
}

static void test_array_of_array(void) {
    Type *t = ty_array(ty_array(ty_char(), 5), 3);
    ASSERT_EQ(t->kind, TY_ARRAY);
    ASSERT_EQ(t->size, 15);
    ASSERT_EQ(t->array_len, 3);
    ASSERT_EQ(t->base->array_len, 5);
}

static void test_struct(void) {
    Type *t = ty_struct("point");
    ASSERT_EQ(t->kind, TY_STRUCT);
    ASSERT(strcmp(t->name, "point") == 0);
}

static void test_union(void) {
    Type *t = ty_union("val");
    ASSERT_EQ(t->kind, TY_UNION);
    ASSERT(strcmp(t->name, "val") == 0);
}

static void test_enum(void) {
    Type *t = ty_enum("color");
    ASSERT_EQ(t->kind, TY_ENUM);
    ASSERT_EQ(t->size, 4);
    ASSERT_EQ(t->align, 4);
    ASSERT(strcmp(t->name, "color") == 0);
}

static void test_func(void) {
    Type *ret = ty_int();
    Type *params[2] = {ty_int(), ty_ptr(ty_char())};
    Type *t = ty_func(ret, params, 2, 0);
    ASSERT_EQ(t->kind, TY_FUNC);
    ASSERT(t->return_type == ret);
    ASSERT_EQ(t->param_count, 2);
    ASSERT_EQ(t->is_variadic, 0);
    ASSERT(t->params[0] == params[0]);
    ASSERT(t->params[1] == params[1]);
}

static void test_func_variadic(void) {
    Type *ret = ty_int();
    Type *params[1] = {ty_ptr(ty_char())};
    Type *t = ty_func(ret, params, 1, 1);
    ASSERT_EQ(t->kind, TY_FUNC);
    ASSERT_EQ(t->is_variadic, 1);
}

static void test_is_integer(void) {
    ASSERT(is_integer(ty_bool()));
    ASSERT(is_integer(ty_char()));
    ASSERT(is_integer(ty_uchar()));
    ASSERT(is_integer(ty_short()));
    ASSERT(is_integer(ty_int()));
    ASSERT(is_integer(ty_uint()));
    ASSERT(is_integer(ty_long()));
    ASSERT(is_integer(ty_llong()));
    ASSERT(is_integer(ty_enum("e")));
    ASSERT(!is_integer(ty_float()));
    ASSERT(!is_integer(ty_double()));
    ASSERT(!is_integer(ty_ptr(ty_int())));
    ASSERT(!is_integer(ty_void()));
}

static void test_is_floating(void) {
    ASSERT(is_floating(ty_float()));
    ASSERT(is_floating(ty_double()));
    ASSERT(!is_floating(ty_int()));
    ASSERT(!is_floating(ty_ptr(ty_int())));
}

static void test_is_numeric(void) {
    ASSERT(is_numeric(ty_int()));
    ASSERT(is_numeric(ty_float()));
    ASSERT(!is_numeric(ty_ptr(ty_int())));
    ASSERT(!is_numeric(ty_void()));
}

static void test_is_scalar(void) {
    ASSERT(is_scalar(ty_int()));
    ASSERT(is_scalar(ty_float()));
    ASSERT(is_scalar(ty_ptr(ty_int())));
    ASSERT(!is_scalar(ty_void()));
    ASSERT(!is_scalar(ty_array(ty_int(), 5)));
}

static void test_promote(void) {
    ASSERT_EQ(promote(ty_bool())->kind, TY_INT);
    ASSERT_EQ(promote(ty_char())->kind, TY_INT);
    ASSERT_EQ(promote(ty_uchar())->kind, TY_INT);
    ASSERT_EQ(promote(ty_short())->kind, TY_INT);
    ASSERT_EQ(promote(ty_int())->kind, TY_INT);
    ASSERT_EQ(promote(ty_long())->kind, TY_LONG);
    ASSERT_EQ(promote(ty_float())->kind, TY_FLOAT);
}

static void test_usual_arith_conv(void) {
    Type *r;

    r = usual_arith_conv(ty_int(), ty_int());
    ASSERT_EQ(r->kind, TY_INT);

    r = usual_arith_conv(ty_int(), ty_long());
    ASSERT_EQ(r->kind, TY_LONG);

    r = usual_arith_conv(ty_int(), ty_float());
    ASSERT_EQ(r->kind, TY_FLOAT);

    r = usual_arith_conv(ty_float(), ty_double());
    ASSERT_EQ(r->kind, TY_DOUBLE);

    r = usual_arith_conv(ty_char(), ty_char());
    ASSERT_EQ(r->kind, TY_INT);

    r = usual_arith_conv(ty_uint(), ty_int());
    ASSERT_EQ(r->kind, TY_INT);
    ASSERT_EQ(r->is_unsigned, 1);
}

static void test_pointer_to(void) {
    Type *t = pointer_to(ty_char());
    ASSERT_EQ(t->kind, TY_PTR);
    ASSERT_EQ(t->base->kind, TY_CHAR);
}

static void test_array_of(void) {
    Type *t = array_of(ty_int(), 5);
    ASSERT_EQ(t->kind, TY_ARRAY);
    ASSERT_EQ(t->array_len, 5);
    ASSERT_EQ(t->size, 20);
}

static void test_copy_type(void) {
    Type *orig = ty_int();
    Type *cp = copy_type(orig);
    ASSERT_EQ(cp->kind, orig->kind);
    ASSERT_EQ(cp->size, orig->size);
    ASSERT_EQ(cp->align, orig->align);
    ASSERT(cp != orig);
}

static void test_copy_type_null(void) {
    ASSERT(copy_type(NULL) == NULL);
}

int main(void) {
    printf("Running type system tests...\n");

    TEST(test_void);
    TEST(test_bool);
    TEST(test_char);
    TEST(test_uchar);
    TEST(test_short);
    TEST(test_ushort);
    TEST(test_int);
    TEST(test_uint);
    TEST(test_long);
    TEST(test_ulong);
    TEST(test_llong);
    TEST(test_ullong);
    TEST(test_float);
    TEST(test_double);
    TEST(test_ptr);
    TEST(test_ptr_to_ptr);
    TEST(test_array);
    TEST(test_array_of_array);
    TEST(test_struct);
    TEST(test_union);
    TEST(test_enum);
    TEST(test_func);
    TEST(test_func_variadic);
    TEST(test_is_integer);
    TEST(test_is_floating);
    TEST(test_is_numeric);
    TEST(test_is_scalar);
    TEST(test_promote);
    TEST(test_usual_arith_conv);
    TEST(test_pointer_to);
    TEST(test_array_of);
    TEST(test_copy_type);
    TEST(test_copy_type_null);

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
