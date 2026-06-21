#include "utils.h"

void *xmalloc(size_t size) {
    void *p = malloc(size);
    if (!p) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return p;
}

void *xrealloc(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if (!p) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return p;
}

char *xstrdup(const char *s) {
    char *p = strdup(s);
    if (!p) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return p;
}

static char *current_file = NULL;
static char *current_input = NULL;

void set_current_file(const char *file) { current_file = (char *)file; }
void set_current_input(const char *input) { current_input = (char *)input; }

void error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

void error_at(const char *loc, const char *fmt, ...) {
    int line = 1;
    const char *line_start = current_input;
    for (const char *p = current_input; p && p < loc; p++) {
        if (*p == '\n') {
            line++;
            line_start = p + 1;
        }
    }

    fprintf(stderr, "%s:%d: ", current_file ? current_file : "<input>", line);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");

    const char *line_end = line_start;
    while (*line_end && *line_end != '\n') line_end++;
    fprintf(stderr, "%.*s\n", (int)(line_end - line_start), line_start);

    int pos = (int)(loc - line_start);
    fprintf(stderr, "%*s^\n", pos, "");
    exit(1);
}

String string_new(const char *s) {
    String str;
    str.data = xstrdup(s);
    str.len = strlen(s);
    return str;
}

String string_append(String a, String b) {
    String result;
    result.len = a.len + b.len;
    result.data = xmalloc(result.len + 1);
    memcpy(result.data, a.data, a.len);
    memcpy(result.data + a.len, b.data, b.len);
    result.data[result.len] = '\0';
    return result;
}
