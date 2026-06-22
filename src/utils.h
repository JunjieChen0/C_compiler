#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#define NULL ((void *)0)

typedef struct {
    char *data;
    int len;
} String;

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);
void error(const char *fmt, ...);
void error_at(const char *loc, const char *fmt, ...);
void error_at_line(int line, const char *fmt, ...);
void set_current_file(const char *file);
void set_current_input(const char *input);
String string_new(const char *s);
String string_append(String a, String b);

#endif
