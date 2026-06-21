#include "type.h"

static Type *new_type(TypeKind kind, int size, int align) {
    Type *ty = xmalloc(sizeof(Type));
    ty->kind = kind;
    ty->size = size;
    ty->align = align;
    ty->is_unsigned = 0;
    ty->base = NULL;
    ty->array_len = 0;
    ty->name = NULL;
    ty->members = NULL;
    ty->member_count = 0;
    ty->return_type = NULL;
    ty->params = NULL;
    ty->param_count = 0;
    ty->is_variadic = 0;
    return ty;
}

Type *ty_void(void)   { return new_type(TY_VOID, 0, 1); }
Type *ty_bool(void)   { return new_type(TY_BOOL, 1, 1); }
Type *ty_char(void)   { return new_type(TY_CHAR, 1, 1); }
Type *ty_uchar(void)  { Type *t = new_type(TY_CHAR, 1, 1); t->is_unsigned = 1; return t; }
Type *ty_short(void)  { return new_type(TY_SHORT, 2, 2); }
Type *ty_ushort(void) { Type *t = new_type(TY_SHORT, 2, 2); t->is_unsigned = 1; return t; }
Type *ty_int(void)    { return new_type(TY_INT, 4, 4); }
Type *ty_uint(void)   { Type *t = new_type(TY_INT, 4, 4); t->is_unsigned = 1; return t; }
Type *ty_long(void)   { return new_type(TY_LONG, 8, 8); }
Type *ty_ulong(void)  { Type *t = new_type(TY_LONG, 8, 8); t->is_unsigned = 1; return t; }
Type *ty_llong(void)  { return new_type(TY_LLONG, 8, 8); }
Type *ty_ullong(void) { Type *t = new_type(TY_LLONG, 8, 8); t->is_unsigned = 1; return t; }
Type *ty_float(void)  { return new_type(TY_FLOAT, 4, 4); }
Type *ty_double(void) { return new_type(TY_DOUBLE, 8, 8); }

Type *ty_ptr(Type *base) {
    Type *ty = new_type(TY_PTR, 8, 8);
    ty->base = base;
    return ty;
}

Type *ty_array(Type *base, int len) {
    Type *ty = new_type(TY_ARRAY, base->size * len, base->align);
    ty->base = base;
    ty->array_len = len;
    return ty;
}

Type *ty_struct(char *name) {
    Type *ty = new_type(TY_STRUCT, 0, 1);
    ty->name = name;
    return ty;
}

Type *ty_union(char *name) {
    Type *ty = new_type(TY_UNION, 0, 1);
    ty->name = name;
    return ty;
}

Type *ty_enum(char *name) {
    Type *ty = new_type(TY_ENUM, 4, 4);
    ty->name = name;
    return ty;
}

Type *ty_func(Type *return_type, Type **params, int param_count, int is_variadic) {
    Type *ty = new_type(TY_FUNC, 0, 1);
    ty->return_type = return_type;
    ty->params = params;
    ty->param_count = param_count;
    ty->is_variadic = is_variadic;
    return ty;
}

int type_size(Type *ty) { return ty->size; }
int type_align(Type *ty) { return ty->align; }

int is_integer(Type *ty) {
    switch (ty->kind) {
    case TY_BOOL:
    case TY_CHAR:
    case TY_SHORT:
    case TY_INT:
    case TY_LONG:
    case TY_LLONG:
    case TY_ENUM:
        return 1;
    default:
        return 0;
    }
}

int is_floating(Type *ty) {
    return ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE;
}

int is_numeric(Type *ty) {
    return is_integer(ty) || is_floating(ty);
}

int is_scalar(Type *ty) {
    return is_numeric(ty) || ty->kind == TY_PTR;
}

Type *promote(Type *ty) {
    if (ty->kind == TY_BOOL || ty->kind == TY_CHAR || ty->kind == TY_SHORT)
        return ty_int();
    return ty;
}

Type *usual_arith_conv(Type *a, Type *b) {
    a = promote(a);
    b = promote(b);

    if (a->kind == TY_DOUBLE || b->kind == TY_DOUBLE)
        return ty_double();
    if (a->kind == TY_FLOAT || b->kind == TY_FLOAT)
        return ty_float();

    if (a->size != b->size)
        return a->size > b->size ? a : b;

    if (b->is_unsigned)
        return b;
    return a;
}

Type *pointer_to(Type *base) {
    return ty_ptr(base);
}

Type *array_of(Type *base, int len) {
    return ty_array(base, len);
}

Type *copy_type(Type *ty) {
    if (!ty) return NULL;
    Type *copy = xmalloc(sizeof(Type));
    *copy = *ty;
    return copy;
}
