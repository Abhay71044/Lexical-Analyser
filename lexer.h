#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

/* Token types recognized by the lexical analyzer */
typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER_CONSTANT,
    TOKEN_FLOAT_CONSTANT,
    TOKEN_CHAR_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_OPERATOR,
    TOKEN_PUNCTUATOR,
    TOKEN_PREPROCESSOR,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

/* Token representation */
typedef struct {
    TokenType type;
    char *lexeme;       /* Dynamically allocated null-terminated string */
    int line;           /* 1-based line number */
    int column;         /* 1-based column number */
    char *error_msg;    /* Detailed error description if type == TOKEN_ERROR, else NULL */
} Token;

/* Lexer state */
typedef struct {
    const char *source; /* Source code buffer */
    size_t length;      /* Total character length of buffer */
    size_t cursor;      /* Current read index */
    int line;           /* Current line (1-based) */
    int column;         /* Current column (1-based) */
    int error_count;    /* Total count of lexical errors */
} Lexer;

/* Function prototypes */
Lexer *initLexer(const char *source, size_t length);
void freeLexer(Lexer *lx);

Token getNextToken(Lexer *lx);
void freeToken(Token *tok);

const char *tokenTypeToString(TokenType type);
bool isKeyword(const char *str);
void printToken(FILE *out, const Token *tok);

#endif /* LEXER_H */
