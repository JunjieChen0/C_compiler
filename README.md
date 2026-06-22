# C_compiler

A hand-written C compiler targeting Windows x86-64 (PE format, Microsoft x64 calling convention).

## Features

- Single-pass, syntax-directed compilation (no AST)
- C99 core language support
- C preprocessor with macro expansion
- x86-64 assembly output (NASM syntax)
- PE executable generation
- Self-hosting capable (in progress)

## Build

```bash
make cc
```

## Usage

```bash
./cc.exe input.c -o output.s
```

## Run Tests

```bash
make test_all
```

## Architecture

```
Source → Preprocessor → Lexer → Parser + CodeGen → .s Assembly
                                                    → PE Executable
```

## Modules

| Module | File | Description |
|--------|------|-------------|
| Lexer | `src/lexer.c` | Tokenizer with 76 token types |
| Preprocessor | `src/preprocess.c` | #define, #include, #ifdef, macros |
| Type System | `src/type.c` | C types: void, int, ptr, array, struct, union, enum, func |
| Symbol Table | `src/sym.c` | Scoped hash table with scope chain |
| Code Generator | `src/gen.c` | x86-64 instruction abstraction and NASM output |
| Parser | `src/parser.c` | Recursive descent parser with direct code emission |
| PE Writer | `src/pe.c` | Windows PE32+ executable generator |
