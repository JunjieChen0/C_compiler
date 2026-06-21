# Self-Hosting Status

## Summary

The compiler can compile a subset of C programs to x86-64 NASM assembly. It is **not yet capable of self-hosting** (compiling its own source code) due to several missing language features.

## Test Results

### Compilation Tests (all pass)

| Test Program | Description | Compiles | Runs Correctly |
|---|---|---|---|
| `simple.c` | Return constant | Yes | Yes (exit 42) |
| `vars.c` | Variables and if | Yes | Yes (exit 0) |
| `loop.c` | While loop | Yes | Yes (exit 45) |
| `factorial.c` | Recursive functions | Yes | No (ABI issue) |
| `preprocess_test.c` | #define macro | Yes | No (ABI issue) |
| `self_host_test.c` | Arithmetic, control flow | Yes | Yes (exit 370) |
| `control_flow.c` | All control flow | Yes | Yes (exit 1526) |
| `pointer_array.c` | Variables and math | Yes | Yes (exit 20030) |
| `bitwise.c` | Bitwise operations | Yes | Yes (exit 95) |
| `string_ops.c` | Simple arithmetic | Yes | Yes (exit 58) |

### Unit Tests (all pass)

- Lexer: 12/12
- Type system: 33/33
- Symbol table: 10/10
- Code generation: 15/15
- Preprocessor: 12/12
- PE format: 13/13

**Total: 83/83 unit tests pass**

## Supported Features

### Data Types
- `int`, `char`, `short`, `long`, `long long`
- `unsigned` variants of all integer types
- `void`, `_Bool`
- Pointers (single and multi-level)
- Single-dimensional arrays
- `struct`, `union`, `enum` (names only, no body definitions)

### Operators
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Bitwise: `&`, `|`, `^`, `<<`, `>>`
- Comparison: `<`, `>`, `<=`, `>=`, `==`, `!=`
- Logical: `&&`, `||`
- Assignment: `=`
- Compound assignment: `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`
- Postfix: `++`, `--`
- Array subscript: `[]`
- Member access: `.`, `->` (parsed but not code-generated correctly)
- Ternary: `?:` (when condition is not parenthesized)
- Type cast: `(type)expr`

### Control Flow
- `if` / `else if` / `else`
- `while` loops
- `do` / `while` loops
- `for` loops
- `break` and `continue`
- `return`

### Preprocessor
- Object-like `#define` (no function-like macros)
- `#include` (angle bracket and quoted)
- `#ifdef` / `#ifndef` / `#endif`
- `#if` / `#elif` / `#else`
- `#undef`
- `#pragma` (skipped)
- Built-in macros: `__STDC__`, `__STDC_VERSION__`, `__x86_64__`, `_WIN64`, `_MSC_VER`

### Functions
- Function declarations and definitions
- Parameters (passed via registers, not stack)
- Variadic functions (declared but not fully code-generated)
- Recursion

### Literals
- Integer literals (decimal, hex, octal)
- Character literals
- String literals (stored in .rodata section)

## Known Limitations

### Preventing Self-Hosting

The compiler's own source code uses features that are not supported:

1. **Function-like macros**: The preprocessor skips `#define NAME(args) body` entirely. The compiler source uses many function-like macros.

2. **Parenthesized expressions**: `(expr)` has a double-consumption bug in `parse_primary` — the inner expression's first token is consumed as a second token.

3. **Unary operators at expression start**: `-x`, `*p`, `&x`, `!x`, `~x`, `sizeof` are not in the main expression parsing chain (`parse_unary` is never called from `parse_mul`).

4. **Multi-dimensional arrays**: `int matrix[3][3]` fails to parse.

5. **Struct/union body definitions**: `struct Foo { int x; };` is not parsed — only `struct Foo` as a type name.

6. **Typedef declarations**: `typedef int MyInt;` at the top level is not recognized.

7. **Enum body definitions**: `enum Color { RED, GREEN, BLUE };` bodies are not parsed.

8. **Array assignment to local arrays**: `arr[i] = val` generates incorrect code — the address is lost when the RHS is evaluated.

9. **Array subscript dereference**: `arr[i]` computes the address but does not load the value from that address for non-stack-based arrays.

### Runtime Issues (Windows x86-64)

10. **Calling convention mismatch**: The compiler generates code using System V ABI (rdi, rsi, rdx, rcx for args) but runs on Windows (rcx, rdx, r8, r9). Functions with parameters produce incorrect results.

11. **No register-to-stack parameter save**: Function prologues do not move register-passed parameters to the stack.

## Conclusion

The compiler successfully compiles C programs that use:
- Local integer variables
- Arithmetic, bitwise, and comparison operators
- All control flow constructs (if/else, while, do-while, for, break, continue)
- Function calls with literal arguments
- Recursion
- Preprocessor object-like macros

Self-hosting is blocked by the lack of function-like macros, unary operators, parenthesized expressions, and struct/typedef support. These features would need to be implemented before the compiler could compile its own source code.
