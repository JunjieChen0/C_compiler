#ifndef _STDLIB_H
#define _STDLIB_H

#include "stddef.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767
#define MB_CUR_MAX 1

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

void abort(void);
void exit(int status);
// int atexit(void (*func)(void));  // Function pointer not supported yet

int system(const char *command);
char *getenv(const char *name);

int abs(int n);
long labs(long n);
long long llabs(long long n);

int atoi(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);
long strtol(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);
float strtof(const char *nptr, char **endptr);

// void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
//               int (*compar)(const void *, const void *));
// void qsort(void *base, size_t nmemb, size_t size,
//            int (*compar)(const void *, const void *));

int rand(void);
void srand(unsigned int seed);

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

#endif
