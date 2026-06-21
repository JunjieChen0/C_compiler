#include "preprocess.h"
#include "lexer.h"

#define MAX_OUTPUT (4 * 1024 * 1024)
#define MAX_PATH 512
#define MAX_NAME 256
#define MAX_MACRO_BODY 4096
#define MAX_INCLUDE_DEPTH 64

typedef struct Macro Macro;

struct Macro {
    char *name;
    char *body;
    int is_builtin;
    Macro *next;
};

static Macro *macros = NULL;
static const char *current_filename = NULL;

static void define_macro(const char *name, const char *body, int is_builtin) {
    for (Macro *m = macros; m; m = m->next) {
        if (strcmp(m->name, name) == 0) {
            if (!m->is_builtin) {
                free(m->body);
                m->body = xstrdup(body);
            }
            return;
        }
    }
    Macro *m = xmalloc(sizeof(Macro));
    m->name = xstrdup(name);
    m->body = xstrdup(body);
    m->is_builtin = is_builtin;
    m->next = macros;
    macros = m;
}

static const char *find_macro(const char *name) {
    for (Macro *m = macros; m; m = m->next) {
        if (strcmp(m->name, name) == 0) return m->body;
    }
    return NULL;
}

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static char *read_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = xmalloc(size + 1);
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);
    return buf;
}

static char *find_include(const char *name, int is_angle) {
    if (!is_angle) {
        char *src = read_file(name);
        if (src) return src;

        if (current_filename) {
            char dir[MAX_PATH];
            const char *slash = strrchr(current_filename, '/');
            const char *bslash = strrchr(current_filename, '\\');
            const char *last = slash > bslash ? slash : bslash;
            if (last) {
                int dirlen = (int)(last - current_filename + 1);
                if (dirlen + (int)strlen(name) < MAX_PATH) {
                    memcpy(dir, current_filename, dirlen);
                    strcpy(dir + dirlen, name);
                    src = read_file(dir);
                    if (src) return src;
                }
            }
        }
    }

    const char *include_paths[] = {
        "include/",
        NULL
    };
    for (int i = 0; include_paths[i]; i++) {
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s%s", include_paths[i], name);
        char *src = read_file(path);
        if (src) return src;
    }

    return NULL;
}

static void skip_whitespace(const char **p) {
    while (**p == ' ' || **p == '\t') (*p)++;
}

static void skip_to_eol(const char **p) {
    while (**p && **p != '\n') (*p)++;
}

static int read_directive_name(const char **p, char *name) {
    skip_whitespace(p);
    int i = 0;
    while (is_alnum(**p) && i < MAX_NAME - 1) {
        name[i++] = *(*p)++;
    }
    name[i] = '\0';
    return i > 0;
}

static void read_ident(const char **p, char *name) {
    int i = 0;
    while ((is_alnum(**p) || **p == '_') && i < MAX_NAME - 1) {
        name[i++] = *(*p)++;
    }
    name[i] = '\0';
}

static int eval_const_expr(const char *expr) {
    while (*expr == ' ' || *expr == '\t') expr++;
    if (*expr == '\0') return 0;

    const char *body = find_macro(expr);
    if (body) return atoi(body);

    return atoi(expr);
}

static char *preprocess_internal(char *source, int depth);

char *preprocess(char *source, const char *filename) {
    current_filename = filename;
    macros = NULL;

    define_macro("__STDC__", "1", 1);
    define_macro("__STDC_VERSION__", "201112L", 1);
    define_macro("__x86_64__", "1", 1);
    define_macro("_WIN64", "1", 1);
    define_macro("_MSC_VER", "1900", 1);

    return preprocess_internal(source, 0);
}

static char *preprocess_internal(char *source, int depth) {
    if (depth > MAX_INCLUDE_DEPTH) {
        error("maximum include depth exceeded");
    }

    char *output = xmalloc(MAX_OUTPUT);
    int out_len = 0;

    const char *p = source;

    while (*p) {
        if (*p == '#') {
            p++;

            if (*p == '#') {
                p++;
                skip_whitespace(&p);
                continue;
            }

            skip_whitespace(&p);

            char directive[MAX_NAME];
            if (!read_directive_name(&p, directive)) {
                if (out_len < MAX_OUTPUT - 1)
                    output[out_len++] = '#';
                continue;
            }

            if (strcmp(directive, "include") == 0) {
                skip_whitespace(&p);
                int is_angle = 0;
                char path[MAX_PATH];
                int i = 0;

                if (*p == '"') {
                    p++;
                    while (*p && *p != '"' && *p != '\n' && i < MAX_PATH - 1)
                        path[i++] = *p++;
                    if (*p == '"') p++;
                    is_angle = 0;
                } else if (*p == '<') {
                    p++;
                    while (*p && *p != '>' && *p != '\n' && i < MAX_PATH - 1)
                        path[i++] = *p++;
                    if (*p == '>') p++;
                    is_angle = 1;
                }
                path[i] = '\0';

                skip_to_eol(&p);
                if (*p == '\n') p++;

                char *inc = find_include(path, is_angle);
                if (inc) {
                    char *preprocessed = preprocess_internal(inc, depth + 1);
                    int len = strlen(preprocessed);
                    if (out_len + len < MAX_OUTPUT) {
                        memcpy(output + out_len, preprocessed, len);
                        out_len += len;
                    }
                    free(preprocessed);
                    free(inc);
                }
            } else if (strcmp(directive, "define") == 0) {
                skip_whitespace(&p);
                char name[MAX_NAME];
                read_ident(&p, name);

                if (*p == '(') {
                    skip_to_eol(&p);
                    if (*p == '\n') p++;
                    continue;
                }

                skip_whitespace(&p);
                char body[MAX_MACRO_BODY];
                int bi = 0;
                while (*p && *p != '\n' && bi < MAX_MACRO_BODY - 1) {
                    body[bi++] = *p++;
                }
                body[bi] = '\0';

                while (bi > 0 && (body[bi-1] == ' ' || body[bi-1] == '\t'))
                    body[--bi] = '\0';

                if (*p == '\n') p++;

                define_macro(name, body, 0);
            } else if (strcmp(directive, "undef") == 0) {
                skip_whitespace(&p);
                char name[MAX_NAME];
                read_ident(&p, name);
                skip_to_eol(&p);
                if (*p == '\n') p++;

                Macro **mp = &macros;
                while (*mp) {
                    if (strcmp((*mp)->name, name) == 0) {
                        Macro *del = *mp;
                        *mp = del->next;
                        free(del->name);
                        free(del->body);
                        free(del);
                        break;
                    }
                    mp = &(*mp)->next;
                }
            } else if (strcmp(directive, "ifdef") == 0) {
                skip_whitespace(&p);
                char name[MAX_NAME];
                read_ident(&p, name);
                skip_to_eol(&p);
                if (*p == '\n') p++;

                int cond = find_macro(name) != NULL;
                int depth2 = 1;
                const char *start = p;

                while (*p && depth2 > 0) {
                    if (*p == '#') {
                        const char *q = p + 1;
                        while (*q == ' ' || *q == '\t') q++;
                        char d[MAX_NAME];
                        int qi = 0;
                        while (is_alnum(*q) && qi < MAX_NAME - 1)
                            d[qi++] = *q++;
                        d[qi] = '\0';

                        if (strcmp(d, "ifdef") == 0 || strcmp(d, "ifndef") == 0)
                            depth2++;
                        else if (strcmp(d, "endif") == 0) {
                            depth2--;
                            if (depth2 == 0) {
                                if (cond) {
                                    int block_len = (int)(p - start);
                                    char *block = xmalloc(block_len + 1);
                                    memcpy(block, start, block_len);
                                    block[block_len] = '\0';

                                    char *expanded = preprocess_internal(block, depth + 1);
                                    int elen = strlen(expanded);
                                    if (out_len + elen < MAX_OUTPUT) {
                                        memcpy(output + out_len, expanded, elen);
                                        out_len += elen;
                                    }
                                    free(expanded);
                                    free(block);
                                }
                                p = q;
                                skip_to_eol(&p);
                                if (*p == '\n') p++;
                                break;
                            }
                        }
                    }
                    p++;
                }
            } else if (strcmp(directive, "ifndef") == 0) {
                skip_whitespace(&p);
                char name[MAX_NAME];
                read_ident(&p, name);
                skip_to_eol(&p);
                if (*p == '\n') p++;

                int cond = find_macro(name) == NULL;
                int depth2 = 1;
                const char *start = p;

                while (*p && depth2 > 0) {
                    if (*p == '#') {
                        const char *q = p + 1;
                        while (*q == ' ' || *q == '\t') q++;
                        char d[MAX_NAME];
                        int qi = 0;
                        while (is_alnum(*q) && qi < MAX_NAME - 1)
                            d[qi++] = *q++;
                        d[qi] = '\0';

                        if (strcmp(d, "ifdef") == 0 || strcmp(d, "ifndef") == 0)
                            depth2++;
                        else if (strcmp(d, "endif") == 0) {
                            depth2--;
                            if (depth2 == 0) {
                                if (cond) {
                                    int block_len = (int)(p - start);
                                    char *block = xmalloc(block_len + 1);
                                    memcpy(block, start, block_len);
                                    block[block_len] = '\0';

                                    char *expanded = preprocess_internal(block, depth + 1);
                                    int elen = strlen(expanded);
                                    if (out_len + elen < MAX_OUTPUT) {
                                        memcpy(output + out_len, expanded, elen);
                                        out_len += elen;
                                    }
                                    free(expanded);
                                    free(block);
                                }
                                p = q;
                                skip_to_eol(&p);
                                if (*p == '\n') p++;
                                break;
                            }
                        }
                    }
                    p++;
                }
            } else if (strcmp(directive, "if") == 0) {
                skip_whitespace(&p);
                char expr[256];
                int ei = 0;
                while (*p && *p != '\n' && ei < 255) {
                    expr[ei++] = *p++;
                }
                expr[ei] = '\0';
                if (*p == '\n') p++;

                int cond = eval_const_expr(expr);
                int depth2 = 1;
                const char *start = p;

                while (*p && depth2 > 0) {
                    if (*p == '#') {
                        const char *q = p + 1;
                        while (*q == ' ' || *q == '\t') q++;
                        char d[MAX_NAME];
                        int qi = 0;
                        while (is_alnum(*q) && qi < MAX_NAME - 1)
                            d[qi++] = *q++;
                        d[qi] = '\0';

                        if (strcmp(d, "if") == 0 || strcmp(d, "ifdef") == 0 || strcmp(d, "ifndef") == 0)
                            depth2++;
                        else if (strcmp(d, "endif") == 0) {
                            depth2--;
                            if (depth2 == 0) {
                                if (cond) {
                                    int block_len = (int)(p - start);
                                    char *block = xmalloc(block_len + 1);
                                    memcpy(block, start, block_len);
                                    block[block_len] = '\0';

                                    char *expanded = preprocess_internal(block, depth + 1);
                                    int elen = strlen(expanded);
                                    if (out_len + elen < MAX_OUTPUT) {
                                        memcpy(output + out_len, expanded, elen);
                                        out_len += elen;
                                    }
                                    free(expanded);
                                    free(block);
                                }
                                p = q;
                                skip_to_eol(&p);
                                if (*p == '\n') p++;
                                break;
                            }
                        }
                    }
                    p++;
                }
            } else if (strcmp(directive, "elif") == 0 || strcmp(directive, "else") == 0) {
                skip_to_eol(&p);
                if (*p == '\n') p++;
            } else if (strcmp(directive, "endif") == 0) {
                skip_to_eol(&p);
                if (*p == '\n') p++;
            } else if (strcmp(directive, "pragma") == 0) {
                skip_to_eol(&p);
                if (*p == '\n') p++;
            } else if (strcmp(directive, "error") == 0) {
                char msg[256];
                int mi = 0;
                skip_whitespace(&p);
                while (*p && *p != '\n' && mi < 255) {
                    msg[mi++] = *p++;
                }
                msg[mi] = '\0';
                error("%s: #error %s", current_filename ? current_filename : "<input>", msg);
            } else if (strcmp(directive, "warning") == 0) {
                skip_to_eol(&p);
                if (*p == '\n') p++;
            } else if (strcmp(directive, "line") == 0) {
                skip_to_eol(&p);
                if (*p == '\n') p++;
            } else {
                if (out_len < MAX_OUTPUT - 1)
                    output[out_len++] = '#';
                int dlen = strlen(directive);
                if (out_len + dlen < MAX_OUTPUT) {
                    memcpy(output + out_len, directive, dlen);
                    out_len += dlen;
                }
            }
        } else if (is_alpha(*p) || *p == '_') {
            const char *start = p;
            char word[MAX_NAME];
            int i = 0;
            while ((is_alnum(*p) || *p == '_') && i < MAX_NAME - 1) {
                word[i++] = *p++;
            }
            word[i] = '\0';

            const char *body = find_macro(word);
            if (body) {
                int len = strlen(body);
                if (out_len + len < MAX_OUTPUT) {
                    memcpy(output + out_len, body, len);
                    out_len += len;
                }
            } else {
                int wlen = (int)(p - start);
                if (out_len + wlen < MAX_OUTPUT) {
                    memcpy(output + out_len, start, wlen);
                    out_len += wlen;
                }
            }
        } else if (*p == '/' && p[1] == '/') {
            while (*p && *p != '\n') p++;
        } else if (*p == '/' && p[1] == '*') {
            p += 2;
            while (*p && !(*p == '*' && p[1] == '/')) p++;
            if (*p) p += 2;
        } else if (*p == '\'' || *p == '"') {
            char quote = *p;
            if (out_len < MAX_OUTPUT - 1)
                output[out_len++] = *p++;
            while (*p && *p != quote) {
                if (*p == '\\') {
                    if (out_len < MAX_OUTPUT - 1)
                        output[out_len++] = *p++;
                }
                if (*p && *p != quote) {
                    if (out_len < MAX_OUTPUT - 1)
                        output[out_len++] = *p++;
                }
            }
            if (*p == quote) {
                if (out_len < MAX_OUTPUT - 1)
                    output[out_len++] = *p++;
            }
        } else {
            if (out_len < MAX_OUTPUT - 1)
                output[out_len++] = *p++;
        }
    }

    output[out_len] = '\0';
    return output;
}
