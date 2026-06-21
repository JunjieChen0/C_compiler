#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

static char *read_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", path);
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = xmalloc(size + 1);
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: cc <file.c> [-o output.exe]\n");
        return 1;
    }

    char *input_file = NULL;
    char *output_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        }
    }

    if (!input_file) {
        fprintf(stderr, "No input file\n");
        return 1;
    }

    if (!output_file) {
        output_file = "a.exe";
    }

    char *source = read_file(input_file);
    set_current_file(input_file);
    set_current_input(source);

    printf("Compiling %s -> %s\n", input_file, output_file);
    // TODO: lexer, parser, codegen

    free(source);
    return 0;
}
