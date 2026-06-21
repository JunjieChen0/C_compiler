#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "preprocess.h"
#include "parser.h"
#include "sym.h"

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
        fprintf(stderr, "Usage: cc <file.c> [-o output.s]\n");
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

    char *source = read_file(input_file);
    set_current_file(input_file);

    char *preprocessed = preprocess(source, input_file);
    set_current_input(preprocessed);

    scope_reset();

    CodeGen gen;
    gen_init(&gen);

    Parser parser;
    parser_init(&parser, preprocessed, &gen);
    parse_translation_unit(&parser);

    char *asm_output = parser_flush(&parser);

    if (!output_file) {
        char *dot = strrchr(input_file, '.');
        int base_len = dot ? (int)(dot - input_file) : strlen(input_file);
        output_file = xmalloc(base_len + 3);
        memcpy(output_file, input_file, base_len);
        strcpy(output_file + base_len, ".s");
    }

    FILE *f = fopen(output_file, "w");
    if (!f) {
        fprintf(stderr, "Cannot open output file %s\n", output_file);
        return 1;
    }
    fprintf(f, "%s", asm_output);
    fclose(f);

    fprintf(stderr, "Compiled %s -> %s\n", input_file, output_file);

    gen_free(&gen);
    free(preprocessed);
    free(source);
    return 0;
}
