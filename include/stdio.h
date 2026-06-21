#ifndef _STDIO_H
#define _STDIO_H

#include "stddef.h"
#include "stdarg.h"

typedef struct _FILE FILE;

#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define stdin  ((FILE *)0)
#define stdout ((FILE *)1)
#define stderr ((FILE *)2)

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *fp);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp);
int fseek(FILE *fp, long offset, int whence);
long ftell(FILE *fp);
int feof(FILE *fp);
int fflush(FILE *fp);
char *fgets(char *s, int size, FILE *fp);
int fputs(const char *s, FILE *fp);
int fputc(int c, FILE *fp);
int fgetc(FILE *fp);

int printf(const char *fmt, ...);
int fprintf(FILE *fp, const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int vfprintf(FILE *fp, const char *fmt, va_list ap);
int vsprintf(char *buf, const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

int scanf(const char *fmt, ...);
int fscanf(FILE *fp, const char *fmt, ...);
int sscanf(const char *str, const char *fmt, ...);

int puts(const char *s);
int putchar(int c);
int getchar(void);
int ungetc(int c, FILE *fp);

int rename(const char *oldpath, const char *newpath);
int remove(const char *path);
FILE *tmpfile(void);
char *tmpnam(char *s);

int sscanf(const char *str, const char *fmt, ...);

void perror(const char *s);

#endif
