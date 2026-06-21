#ifndef TYPE_H
#define TYPE_H

#include "utils.h"

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

typedef struct Type Type;
typedef struct Member Member;

struct Type {
    TypeKind kind;
    int size;
    int align;
    int is_unsigned;

    Type *base;
    int array_len;

    char *name;
    Member *members;
    int member_count;

    Type *return_type;
    Type **params;
    int param_count;
    int is_variadic;
};

struct Member {
    Type *type;
    char *name;
    int offset;
};

Type *ty_void(void);
Type *ty_bool(void);
Type *ty_char(void);
Type *ty_uchar(void);
Type *ty_short(void);
Type *ty_ushort(void);
Type *ty_int(void);
Type *ty_uint(void);
Type *ty_long(void);
Type *ty_ulong(void);
Type *ty_llong(void);
Type *ty_ullong(void);
Type *ty_float(void);
Type *ty_double(void);

Type *ty_ptr(Type *base);
Type *ty_array(Type *base, int len);
Type *ty_struct(char *name);
Type *ty_union(char *name);
Type *ty_enum(char *name);
Type *ty_func(Type *return_type, Type **params, int param_count, int is_variadic);

int type_size(Type *ty);
int type_align(Type *ty);
int is_integer(Type *ty);
int is_floating(Type *ty);
int is_numeric(Type *ty);
int is_scalar(Type *ty);
Type *promote(Type *ty);
Type *usual_arith_conv(Type *a, Type *b);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int len);
Type *copy_type(Type *ty);

void add_field(Type *struc, char *name, Type *type, int offset);
Member *find_field(Type *struc, char *name);

#endif
