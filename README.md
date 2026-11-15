# C Language Lexical Analyzer (Lexer / Scanner)

A complete, high-performance, hand-written finite-state lexical analyzer (scanner) for the C programming language (C11 standard) written in ISO C11 and compiled with `gcc`.

---

## Table of Contents
- [Overview](#overview)
- [Design and Architecture](#design-and-architecture)
- [Token Types Supported](#token-types-supported)
- [Maximal Munch Rule](#maximal-munch-rule)
- [Error Handling and Recovery](#error-handling-and-recovery)
- [Memory Management](#memory-management)
- [Building and Running](#building-and-running)
- [Test Suite](#test-suite)
- [Sample Output](#sample-output)

---

## Overview

This lexical analyzer reads a C source code file, strips comments and whitespace, tracks 1-based line and column coordinates, and breaks the input stream into a sequence of strongly-typed tokens formatted as:
```text
<TOKEN_TYPE, "lexeme", line:column>
```
At the end of processing, a summary statistics table is displayed showing counts for each token type and total lexical errors encountered.

---

## Design and Architecture

Implemented as a hand-written Finite-State Automaton (FSM) without relying on Lex or Flex.

### 1. Data Structures (`lexer.h`)
- **`Token`**:
  ```c
  typedef struct {
      TokenType type;     /* Enumeration representing the token category */
      char *lexeme;       /* Dynamically allocated string containing the lexeme text */
      int line;           /* 1-based source line */
      int column;         /* 1-based source column */
      char *error_msg;    /* Detailed diagnostic message if type == TOKEN_ERROR */
  } Token;
  ```
- **`Lexer`**:
  ```c
  typedef struct {
      const char *source; /* Source buffer */
      size_t length;      /* Source buffer length in bytes */
      size_t cursor;      /* Current byte index */
      int line;           /* Current line */
      int column;         /* Current column */
      int error_count;    /* Number of lexical errors recorded */
  } Lexer;
  ```

### 2. Keyword Lookup Table
Identifies all 32 C89 keywords and modern C99/C11 keywords using an in-memory sorted table searched via `bsearch()` in $O(\log N)$ time with zero dynamic allocation overhead.

### 3. Public API
- `Lexer *initLexer(const char *source, size_t length)`: Initializes lexer state.
- `void freeLexer(Lexer *lx)`: Releases lexer state.
- `Token getNextToken(Lexer *lx)`: Advances the scanner state machine and returns the next token.
- `void freeToken(Token *tok)`: Deallocates dynamically allocated memory in the token.
- `const char *tokenTypeToString(TokenType type)`: Returns human-readable token name string.
- `bool isKeyword(const char *str)`: Fast keyword check.
- `void printToken(FILE *out, const Token *tok)`: Prints token in `<TOKEN_TYPE, "lexeme", line:column>` format with escaped control characters.

---

## Token Types Supported

| Token Type | Description & Pattern | Examples |
|---|---|---|
| `KEYWORD` | All 32 standard C89 keywords plus C99/C11 extensions (`inline`, `restrict`, `_Bool`, `_Complex`, `_Imaginary`, `_Alignas`, `_Alignof`, `_Atomic`, `_Generic`, `_Noreturn`, `_Static_assert`, `_Thread_local`) | `int`, `return`, `volatile`, `_Bool`, `_Alignas` |
| `IDENTIFIER` | Standard C identifier pattern `[a-zA-Z_][a-zA-Z0-9_]*` that is not a reserved keyword | `main`, `counter`, `ptr_next`, `_var1` |
| `INTEGER_CONSTANT` | Decimal (`42`), Octal (`0755`), Hexadecimal (`0x1F`, `0xDEADBEEF`), with suffixes (`u`, `U`, `l`, `L`, `ll`, `LL`, `ul`, `UL`, `ull`, `ULL`, etc.) | `0`, `42`, `1000u`, `0777`, `0x1F`, `0xCAFEBABEULL` |
| `FLOAT_CONSTANT` | Decimal floats with fractional parts and/or exponents (`e`/`E`), hex floats (`0x...p...`), and float suffixes (`f`, `F`, `l`, `L`) | `3.14`, `1e-5`, `2.5f`, `.75`, `1.`, `0x1.fp3`, `0x1p+4` |
| `CHAR_LITERAL` | Enclosed in single quotes with escape sequences (`\n`, `\t`, `\r`, `\0`, `\\`, `\'`, `\"`, `\x...`) | `'a'`, `'\n'`, `'\0'`, `'\\'`, `'\x1b'` |
| `STRING_LITERAL` | Enclosed in double quotes with escape sequences and line continuation support | `"Hello, World!\n"`, `"Escaped \"quote\""` |
| `OPERATOR` | Complete set of single and multi-character operators with maximal munch | `+`, `-`, `*`, `/`, `%`, `++`, `--`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `\|\|`, `!`, `&`, `\|`, `^`, `~`, `<<`, `>>`, `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `\|=`, `^=`, `<<=`, `>>=`, `->`, `.`, `?`, `:` |
| `PUNCTUATOR` | Standard punctuation characters | `;`, `,`, `(`, `)`, `{`, `}`, `[`, `]` |
| `PREPROCESSOR` | Preprocessor directives starting with `#` | `#include`, `#define`, `#ifdef`, `#ifndef`, `#endif`, `#pragma`, `#undef`, `#error` |

---

## Maximal Munch Rule

The scanner strictly implements the **maximal munch** (longest match) principle:
- `>>=` is recognized as a single 3-character operator token, never `>>` followed by `=`.
- `<<=` is recognized as a single 3-character operator token, never `<<` followed by `=`.
- `++`, `--`, `->`, `==`, `!=`, `<=`, `>=`, `&&`, `||`, `<<`, `>>`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=` are scanned as 2-character operators.

---

## Error Handling and Recovery

When a lexical error is encountered, the scanner:
1. Reports the error immediately with line and column: `Error at <line>:<column>: <description> (lexeme: "<lexeme>")`
2. Increments the `error_count`.
3. **Recovers and continues scanning** the remaining file without crashing or terminating prematurely.

### Handled Error Categories:
- **Unterminated String Literal**: String open quote `"` not closed before an unescaped newline or EOF.
- **Unterminated Character Literal**: Char literal `'` not closed before a newline or EOF.
- **Empty Character Literal**: `''` with no content inside.
- **Unterminated Multi-Line Comment**: Block comment `/*` not terminated with `*/` before EOF.
- **Invalid Characters**: Extraneous characters outside strings/comments (e.g. `@`, `` ` ``, `$`).
- **Malformed Numbers**:
  - Invalid hex literals without digits (e.g. `0xZZ`, `0x`).
  - Numbers with invalid suffix characters (e.g. `12abc`).
  - Octal constants containing non-octal digits (e.g. `089`).
  - Malformed exponents in floating constants (e.g. `1e+`).

---

## Memory Management

- **Zero Memory Leaks**: Every dynamically allocated lexeme string and error message is systematically tracked and freed with `freeToken()`.
- Source file buffers are allocated dynamically and freed upon scanner completion.
- Valgrind verification target provided in Makefile (`make valgrind`).

---

## Building and Running

### Prerequisites
- GCC compiler with C11 support (`gcc -std=c11`)
- GNU Make (`make` or `mingw32-make`)

### Make Targets
- `make all`: Compiles the executable `lexer` (or `lexer.exe` on Windows).
- `make run`: Compiles and runs the lexer on `tests/test1_basic.c`.
- `make test`: Executes the comprehensive test suite across all 6 test files.
- `make clean`: Cleans object files and built binaries.
- `make valgrind`: Runs valgrind memory leak checks on test cases (Linux).

### Running Manually
```bash
./lexer <path_to_c_file.c>
```

---

## Test Suite

The `tests/` directory contains 6 sample test files designed for rigorous verification:
1. **`tests/test1_basic.c`**: Factorial calculation with functions, loops, variables, line and multi-line comments, and `#include`.
2. **`tests/test2_keywords.c`**: Full coverage of all 32 C89 keywords and C99/C11 keywords (`_Bool`, `_Complex`, `_Imaginary`, `_Alignas`, `_Alignof`, `_Atomic`, `_Static_assert`, `_Noreturn`, `inline`, `restrict`).
3. **`tests/test3_constants.c`**: Numeric constants covering decimal, octal (`0755`), hex (`0x1F`, `0xDEADBEEFu`), float suffixes (`2.5f`, `1.0e-3L`), and hex floats (`0x1.fp3`, `0x1p+4`).
4. **`tests/test4_operators.c`**: Maximal munch verification (`>>=`, `<<=`, `->`, `++`, `--`, `==`, `!=`, `&&`, compound assignments, ternary `?:`).
5. **`tests/test5_strings_escapes.c`**: Escape sequences (`\n`, `\t`, `\r`, `\0`, `\\`, `\'`, `\"`, `\x1b`), line continuation, and preprocessor directives.
6. **`tests/test6_errors.c`**: Comprehensive error detection and recovery (`0xZZ`, `12abc`, `089`, `1e+`, `@`, `` ` ``, `$`, `''`, unterminated strings, unterminated char literals, unterminated comments).

---

## Sample Output

Running `tests/test1_basic.c`:
```text
<PREPROCESSOR, "#include", 2:1>
<OPERATOR, "<", 2:10>
<IDENTIFIER, "stdio", 2:11>
<OPERATOR, ".", 2:16>
<IDENTIFIER, "h", 2:17>
<OPERATOR, ">", 2:18>
<KEYWORD, "int", 5:1>
<IDENTIFIER, "factorial", 5:5>
<PUNCTUATOR, "(", 5:14>
<KEYWORD, "int", 5:15>
<IDENTIFIER, "n", 5:19>
<PUNCTUATOR, ")", 5:20>
<PUNCTUATOR, "{", 5:22>
<KEYWORD, "if", 6:5>
<PUNCTUATOR, "(", 6:8>
<IDENTIFIER, "n", 6:9>
<OPERATOR, "<=", 6:11>
<INTEGER_CONSTANT, "1", 6:14>
<PUNCTUATOR, ")", 6:15>
<PUNCTUATOR, "{", 6:17>
<KEYWORD, "return", 7:9>
<INTEGER_CONSTANT, "1", 7:16>
<PUNCTUATOR, ";", 7:17>
<PUNCTUATOR, "}", 8:5>
<KEYWORD, "return", 9:5>
<IDENTIFIER, "n", 9:12>
<OPERATOR, "*", 9:14>
<IDENTIFIER, "factorial", 9:16>
<PUNCTUATOR, "(", 9:25>
<IDENTIFIER, "n", 9:26>
<OPERATOR, "-", 9:28>
<INTEGER_CONSTANT, "1", 9:30>
<PUNCTUATOR, ")", 9:31>
<PUNCTUATOR, ";", 9:32>
<PUNCTUATOR, "}", 10:1>
<KEYWORD, "int", 12:1>
<IDENTIFIER, "main", 12:5>
<PUNCTUATOR, "(", 12:9>
<KEYWORD, "void", 12:10>
<PUNCTUATOR, ")", 12:14>
<PUNCTUATOR, "{", 12:16>
<KEYWORD, "int", 13:5>
<IDENTIFIER, "num", 13:9>
<OPERATOR, "=", 13:13>
<INTEGER_CONSTANT, "5", 13:15>
<PUNCTUATOR, ";", 13:16>
<KEYWORD, "int", 14:5>
<IDENTIFIER, "result", 14:9>
<OPERATOR, "=", 14:16>
<IDENTIFIER, "factorial", 14:18>
<PUNCTUATOR, "(", 14:27>
<IDENTIFIER, "num", 14:28>
<PUNCTUATOR, ")", 14:31>
<PUNCTUATOR, ";", 14:32>
<IDENTIFIER, "printf", 17:5>
<PUNCTUATOR, "(", 17:11>
<STRING_LITERAL, "\"Factorial of %d is %d\n\"", 17:12>
<PUNCTUATOR, ",", 17:37>
<IDENTIFIER, "num", 17:39>
<PUNCTUATOR, ",", 17:42>
<IDENTIFIER, "result", 17:44>
<PUNCTUATOR, ")", 17:50>
<PUNCTUATOR, ";", 17:51>
<KEYWORD, "return", 18:5>
<INTEGER_CONSTANT, "0", 18:12>
<PUNCTUATOR, ";", 18:13>
<PUNCTUATOR, "}", 19:1>

==================================================
                  TOKEN SUMMARY                   
==================================================
Token Type                             Count
--------------------------------------------------
KEYWORD                                   10
IDENTIFIER                                16
INTEGER_CONSTANT                           5
FLOAT_CONSTANT                             0
CHAR_LITERAL                               0
STRING_LITERAL                             1
OPERATOR                                   8
PUNCTUATOR                                26
PREPROCESSOR                               1
--------------------------------------------------
Total Valid Tokens                        67
Total Lexical Errors                       0
==================================================
```
