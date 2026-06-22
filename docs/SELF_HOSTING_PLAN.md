# C编译器自举计划

## 目标

让编译器能够编译自身源码，实现自举（self-hosting）。

## 当前状态

### 已通过编译的源文件
- ✅ `src/utils.c` - 编译成功
- ✅ `src/lexer.c` - 编译成功
- ✅ `src/type.c` - 编译成功
- ✅ `src/sym.c` - 编译成功

### 编译失败的源文件及原因

| 源文件 | 失败原因 | 错误位置 |
|--------|----------|----------|
| `src/preprocess.c` | 混合声明(C99) | `char name[MAX_NAME]` 在语句之后 |
| `src/gen.c` | 类型转换+混合声明 | `(long long)` 转换、`static` 数组初始化 |
| `src/parser.c` | 静态数组初始化 | `static const Register arr[] = {...}` |
| `src/main.c` | 混合声明 | `long size = ftell(fp)` 在语句之后 |

## 已完成的修复

### 1. 预处理器修复 (`src/preprocess.c`)
- ✅ `\r\n` 规范化：添加 `normalize_source()` 函数
- ✅ 字符字面量转义 bug：修复 `'\''` 处理
- ✅ 行首 `#` 检测：支持缩进的预处理指令
- ✅ 宏体续行处理：支持 `\\\n` 续行

### 2. 自引用结构体指针 (`src/parser.c`)
- ✅ `parse_type_spec` 支持 `SYM_TAG` 类型查找
- ✅ `typedef struct Foo Foo;` 后跟 `struct Foo { ... }` 的符号表覆盖修复

### 3. 三元运算符修复 (`src/parser.c`)
- ✅ `parse_cond` 使用 `parse_assign` 替代 `parse_primary`

### 4. 数组参数支持 (`src/parser.c`)
- ✅ 函数参数中的 `int arr[]` 改为 `int *arr`

## 下一步计划

### 阶段1：修复混合声明（优先级：高）

编译器不支持 C99 风格的混合声明（声明在语句之后）。

**实现方案**：
- 方案A：修改 parser，在 `parse_statement` 中支持声明
- 方案B：修改源码，将所有声明移到函数顶部

**修改文件**：
- `src/parser.c` - `parse_statement()` 和 `parse_compound_stmt()`

**预计工作量**：2-3小时

### 阶段2：修复类型转换（优先级：高）

编译器不支持 `(long long)` 类型转换。

**实现方案**：
- 在 `parse_cast` 中支持 `long long` 类型

**修改文件**：
- `src/parser.c` - `parse_cast()`

**预计工作量**：1小时

### 阶段3：修复静态数组初始化（优先级：中）

编译器不支持 `static const Register arr[] = {...}` 语法。

**实现方案**：
- 在变量声明中支持 `[]` 空数组大小
- 在初始化中支持数组字面量 `{...}`

**修改文件**：
- `src/parser.c` - `parse_var_decl()`

**预计工作量**：2-3小时

## 预计总工作量

- 阶段1：2-3小时
- 阶段2：1小时
- 阶段3：2-3小时

**总计**：5-7小时

## 成功标准

编译器自举成功的标准：
1. 能够编译自身所有源文件
2. 生成的可执行文件能够正常运行
3. 新编译器能够编译测试程序并产生正确结果
4. 所有现有测试继续通过
