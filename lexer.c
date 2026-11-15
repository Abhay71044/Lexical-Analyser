#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================================
 * Helper Character & Lookahead Functions
 * ============================================================================ */

static inline char peekChar(const Lexer *lx) {
    if (lx->cursor < lx->length) {
        return lx->source[lx->cursor];
    }
    return '\0';
}

static inline char peekNextChar(const Lexer *lx) {
    if (lx->cursor + 1 < lx->length) {
        return lx->source[lx->cursor + 1];
    }
    return '\0';
}

static inline char peekNChar(const Lexer *lx, size_t offset) {
    if (lx->cursor + offset < lx->length) {
        return lx->source[lx->cursor + offset];
    }
    return '\0';
}

static inline bool isAtEnd(const Lexer *lx) {
    return lx->cursor >= lx->length;
}

static char advanceChar(Lexer *lx) {
    if (isAtEnd(lx)) {
        return '\0';
    }
    char c = lx->source[lx->cursor++];
    if (c == '\r') {
        if (lx->cursor < lx->length && lx->source[lx->cursor] == '\n') {
            lx->cursor++; /* Consume '\n' in CRLF */
        }
        lx->line++;
        lx->column = 1;
        return '\n'; /* Normalize newline to '\n' */
    } else if (c == '\n') {
        lx->line++;
        lx->column = 1;
    } else {
        lx->column++;
    }
    return c;
}

/* Create a dynamically allocated duplicate of a string */
static char *duplicateString(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *copy = (char *)malloc(len + 1);
    if (!copy) {
        fprintf(stderr, "Fatal error: out of memory duplicating string.\n");
        exit(EXIT_FAILURE);
    }
    memcpy(copy, s, len + 1);
    return copy;
}

/* Create a dynamically allocated substring [start_idx, end_idx) */
static char *copySubstring(const char *source, size_t start_idx, size_t end_idx) {
    if (end_idx < start_idx) {
        end_idx = start_idx;
    }
    size_t len = end_idx - start_idx;
    char *sub = (char *)malloc(len + 1);
    if (!sub) {
        fprintf(stderr, "Fatal error: out of memory allocating lexeme.\n");
        exit(EXIT_FAILURE);
    }
    memcpy(sub, source + start_idx, len);
    sub[len] = '\0';
    return sub;
}

/* ============================================================================
 * Keyword Table & Lookup
 * ============================================================================ */

static int cmpKeywords(const void *key, const void *elem) {
    const char *k = (const char *)key;
    const char *const *entry = (const char *const *)elem;
    return strcmp(k, *entry);
}

bool isKeyword(const char *str) {
    /* Alphabetical order by ASCII code for bsearch.
     * Note: '_' (ASCII 95) precedes lowercase letters (ASCII 97-122).
     */
    static const char *const keywords[] = {
        "_Alignas",
        "_Alignof",
        "_Atomic",
        "_Bool",
        "_Complex",
        "_Generic",
        "_Imaginary",
        "_Noreturn",
        "_Static_assert",
        "_Thread_local",
        "auto",
        "break",
        "case",
        "char",
        "const",
        "continue",
        "default",
        "do",
        "double",
        "else",
        "enum",
        "extern",
        "float",
        "for",
        "goto",
        "if",
        "inline",
        "int",
        "long",
        "register",
        "restrict",
        "return",
        "short",
        "signed",
        "sizeof",
        "static",
        "struct",
        "switch",
        "typedef",
        "union",
        "unsigned",
        "void",
        "volatile",
        "while"
    };
    static const size_t num_keywords = sizeof(keywords) / sizeof(keywords[0]);
    return bsearch(str, keywords, num_keywords, sizeof(char *), cmpKeywords) != NULL;
}

/* Validate integer suffixes: u, l, ul, lu, ll, ull, llu (case-insensitive) */
static bool isValidIntegerSuffix(const char *s) {
    if (!s || *s == '\0') return true;

    /* Normalize to lowercase */
    char buf[16];
    size_t i = 0;
    while (s[i] && i < sizeof(buf) - 1) {
        buf[i] = (char)tolower((unsigned char)s[i]);
        i++;
    }
    buf[i] = '\0';

    if (strcmp(buf, "u") == 0) return true;
    if (strcmp(buf, "l") == 0) return true;
    if (strcmp(buf, "ul") == 0 || strcmp(buf, "lu") == 0) return true;
    if (strcmp(buf, "ll") == 0) return true;
    if (strcmp(buf, "ull") == 0 || strcmp(buf, "llu") == 0) return true;

    return false;
}

/* Validate float suffixes: f, l, F, L */
static bool isValidFloatSuffix(const char *s) {
    if (!s || *s == '\0') return true;
    if (strlen(s) == 1 && (tolower((unsigned char)s[0]) == 'f' || tolower((unsigned char)s[0]) == 'l')) {
        return true;
    }
    return false;
}

/* ============================================================================
 * Token Construction and Destruction
 * ============================================================================ */

static Token makeToken(TokenType type, char *lexeme, int line, int column, const char *err_msg) {
    Token tok;
    tok.type = type;
    tok.lexeme = lexeme;
    tok.line = line;
    tok.column = column;
    tok.error_msg = err_msg ? duplicateString(err_msg) : NULL;
    return tok;
}

void freeToken(Token *tok) {
    if (tok->lexeme) {
        free(tok->lexeme);
        tok->lexeme = NULL;
    }
    if (tok->error_msg) {
        free(tok->error_msg);
        tok->error_msg = NULL;
    }
}

const char *tokenTypeToString(TokenType type) {
    switch (type) {
        case TOKEN_KEYWORD:          return "KEYWORD";
        case TOKEN_IDENTIFIER:       return "IDENTIFIER";
        case TOKEN_INTEGER_CONSTANT: return "INTEGER_CONSTANT";
        case TOKEN_FLOAT_CONSTANT:   return "FLOAT_CONSTANT";
        case TOKEN_CHAR_LITERAL:     return "CHAR_LITERAL";
        case TOKEN_STRING_LITERAL:   return "STRING_LITERAL";
        case TOKEN_OPERATOR:         return "OPERATOR";
        case TOKEN_PUNCTUATOR:       return "PUNCTUATOR";
        case TOKEN_PREPROCESSOR:     return "PREPROCESSOR";
        case TOKEN_EOF:              return "EOF";
        case TOKEN_ERROR:            return "ERROR";
        default:                     return "UNKNOWN";
    }
}

void printToken(FILE *out, const Token *tok) {
    fprintf(out, "<%s, \"", tokenTypeToString(tok->type));
    for (const char *p = tok->lexeme; *p; p++) {
        if (*p == '"') {
            fprintf(out, "\\\"");
        } else if (*p == '\\') {
            fprintf(out, "\\\\");
        } else if (*p == '\n') {
            fprintf(out, "\\n");
        } else if (*p == '\r') {
            fprintf(out, "\\r");
        } else if (*p == '\t') {
            fprintf(out, "\\t");
        } else {
            fputc(*p, out);
        }
    }
    fprintf(out, "\", %d:%d>\n", tok->line, tok->column);
}

/* ============================================================================
 * Lexer Initialization and Cleanup
 * ============================================================================ */

Lexer *initLexer(const char *source, size_t length) {
    Lexer *lx = (Lexer *)malloc(sizeof(Lexer));
    if (!lx) {
        fprintf(stderr, "Fatal error: out of memory allocating Lexer.\n");
        exit(EXIT_FAILURE);
    }
    lx->source = source;
    lx->length = length;
    lx->cursor = 0;
    lx->line = 1;
    lx->column = 1;
    lx->error_count = 0;
    return lx;
}

void freeLexer(Lexer *lx) {
    if (lx) {
        free(lx);
    }
}

/* ============================================================================
 * Whitespace and Comment Skipping
 * ============================================================================ */

/* Returns true if an error token was produced (unterminated multi-line comment) */
static bool skipWhitespaceAndComments(Lexer *lx, Token *errTok) {
    while (!isAtEnd(lx)) {
        char c = peekChar(lx);

        /* Skip standard whitespace */
        if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r' || c == '\n') {
            advanceChar(lx);
            continue;
        }

        /* Check for comments */
        if (c == '/') {
            char next = peekNextChar(lx);
            if (next == '/') {
                /* Single-line comment: skip until end of line or EOF */
                advanceChar(lx); /* '/' */
                advanceChar(lx); /* '/' */
                while (!isAtEnd(lx)) {
                    char ch = peekChar(lx);
                    if (ch == '\n' || ch == '\r') {
                        advanceChar(lx);
                        break;
                    }
                    advanceChar(lx);
                }
                continue;
            } else if (next == '*') {
                /* Multi-line comment */
                int start_line = lx->line;
                int start_col = lx->column;
                size_t start_idx = lx->cursor;

                advanceChar(lx); /* '/' */
                advanceChar(lx); /* '*' */

                bool closed = false;
                while (!isAtEnd(lx)) {
                    if (peekChar(lx) == '*' && peekNextChar(lx) == '/') {
                        advanceChar(lx); /* '*' */
                        advanceChar(lx); /* '/' */
                        closed = true;
                        break;
                    }
                    advanceChar(lx);
                }

                if (!closed) {
                    /* Unterminated multi-line comment */
                    lx->error_count++;
                    char *lex = copySubstring(lx->source, start_idx, lx->cursor);
                    *errTok = makeToken(TOKEN_ERROR, lex, start_line, start_col,
                                        "unterminated multi-line comment");
                    return true;
                }
                continue;
            }
        }

        /* Neither whitespace nor comment */
        break;
    }
    return false;
}

/* ============================================================================
 * Number Scanning (Integer, Float, and Malformed Numbers)
 * ============================================================================ */

static Token scanNumber(Lexer *lx) {
    int start_line = lx->line;
    int start_col = lx->column;
    size_t start_idx = lx->cursor;

    /* Check if starts with 0x / 0X (Hexadecimal) */
    if (peekChar(lx) == '0' && (peekNextChar(lx) == 'x' || peekNextChar(lx) == 'X')) {
        advanceChar(lx); /* '0' */
        advanceChar(lx); /* 'x' or 'X' */

        /* Check if followed by at least one hex digit */
        if (!isxdigit((unsigned char)peekChar(lx))) {
            /* Malformed hex constant like 0xZZ, 0x, 0x_ */
            while (isalnum((unsigned char)peekChar(lx)) || peekChar(lx) == '_') {
                advanceChar(lx);
            }
            lx->error_count++;
            char *lex = copySubstring(lx->source, start_idx, lx->cursor);
            return makeToken(TOKEN_ERROR, lex, start_line, start_col, "malformed hexadecimal constant");
        }

        /* Consume valid hex digits */
        while (isxdigit((unsigned char)peekChar(lx))) {
            advanceChar(lx);
        }

        /* Check for hex floating point */
        bool is_float = false;
        if (peekChar(lx) == '.') {
            is_float = true;
            advanceChar(lx); /* '.' */
            while (isxdigit((unsigned char)peekChar(lx))) {
                advanceChar(lx);
            }
        }

        /* Hex floats require binary exponent 'p' or 'P' */
        if (peekChar(lx) == 'p' || peekChar(lx) == 'P') {
            is_float = true;
            advanceChar(lx); /* 'p' or 'P' */
            if (peekChar(lx) == '+' || peekChar(lx) == '-') {
                advanceChar(lx);
            }
            if (!isdigit((unsigned char)peekChar(lx))) {
                while (isalnum((unsigned char)peekChar(lx)) || peekChar(lx) == '_') {
                    advanceChar(lx);
                }
                lx->error_count++;
                char *lex = copySubstring(lx->source, start_idx, lx->cursor);
                return makeToken(TOKEN_ERROR, lex, start_line, start_col,
                                 "malformed exponent in hexadecimal floating constant");
            }
            while (isdigit((unsigned char)peekChar(lx))) {
                advanceChar(lx);
            }
        } else if (is_float) {
            /* Hex float with decimal point but no exponent is invalid in C */
            while (isalnum((unsigned char)peekChar(lx)) || peekChar(lx) == '_') {
                advanceChar(lx);
            }
            lx->error_count++;
            char *lex = copySubstring(lx->source, start_idx, lx->cursor);
            return makeToken(TOKEN_ERROR, lex, start_line, start_col,
                             "hexadecimal floating constant requires an exponent");
        }

        if (is_float) {
            /* Check float suffix */
            size_t suf_start = lx->cursor;
            while (isalnum((unsigned char)peekChar(lx)) || peekChar(lx) == '_') {
                advanceChar(lx);
            }
            char *suffix = copySubstring(lx->source, suf_start, lx->cursor);
            bool ok = isValidFloatSuffix(suffix);
            free(suffix);
            char *lex = copySubstring(lx->source, start_idx, lx->cursor);
            if (!ok) {
                lx->error_count++;
                return makeToken(TOKEN_ERROR, lex, start_line, start_col,
                                 "malformed floating constant suffix");
            }
            return makeToken(TOKEN_FLOAT_CONSTANT, lex, start_line, start_col, NULL);
        } else {
            /* Hex integer suffix */
            size_t suf_start = lx->cursor;
            while (isalnum((unsigned char)peekChar(lx)) || peekChar(lx) == '_') {
                advanceChar(lx);
            }
            char *suffix = copySubstring(lx->source, suf_start, lx->cursor);
            bool ok = isValidIntegerSuffix(suffix);
            free(suffix);
            char *lex = copySubstring(lx->source, start_idx, lx->cursor);
            if (!ok) {
                lx->error_count++;
                return makeToken(TOKEN_ERROR, lex, start_line, start_col,
                                 "malformed hexadecimal integer suffix");
            }
            return makeToken(TOKEN_INTEGER_CONSTANT, lex, start_line, start_col, NULL);
        }
    }

    /* Check if starts with '0' (Octal or Decimal 0) */
    if (peekChar(lx) == '0') {
        char next = peekNextChar(lx);
        if (next != '.' && next != 'e' && next != 'E' && isdigit((unsigned char)next)) {
            /* Potential octal constant */
            advanceChar(lx); /* '0' */
            bool has_invalid_octal_digit = false;
            while (isdigit((unsigned char)peekChar(lx))) {
                char ch = peekChar(lx);
                if (ch == '8' || ch == '9') {
                    has_invalid_octal_digit = true;
                }
                advanceChar(lx);
            }

            /* If followed by '.' or 'e'/'E', it was actually a malformed/nonstandard float */
            if (peekChar(lx) == '.' || peekChar(lx) == 'e' || peekChar(lx) == 'E') {
                /* Treat as float below */
                goto parse_float_rest;
            }

            size_t suf_start = lx->cursor;
            while (isalnum((unsigned char)peekChar(lx)) || peekChar(lx) == '_') {
                advanceChar(lx);
            }
            char *suffix = copySubstring(lx->source, suf_start, lx->cursor);
            bool valid_suf = isValidIntegerSuffix(suffix);
            free(suffix);
            char *lex = copySubstring(lx->source, start_idx, lx->cursor);

            if (has_invalid_octal_digit) {
                lx->error_count++;
                return makeToken(TOKEN_ERROR, lex, start_line, start_col,
                                 "invalid digit in octal constant");
            }
            if (!valid_suf) {
                lx->error_count++;
                return makeToken(TOKEN_ERROR, lex, start_line, start_col,
                                 "malformed octal constant");
            }
            return makeToken(TOKEN_INTEGER_CONSTANT, lex, start_line, start_col, NULL);
        }
    }

    /* Decimal integer or float starting with '.' or digits */
    bool is_float = false;

    if (peekChar(lx) == '.') {
        is_float = true;
        advanceChar(lx); /* '.' */
        while (isdigit((unsigned char)peekChar(lx))) {
            advanceChar(lx);
        }
        goto parse_exponent_and_suffix;
    }

    /* Consume initial digits */
    while (isdigit((unsigned char)peekChar(lx))) {
        advanceChar(lx);
    }

    if (peekChar(lx) == '.') {
        is_float = true;
        advanceChar(lx); /* '.' */
        while (isdigit((unsigned char)peekChar(lx))) {
            advanceChar(lx);
        }
    }

parse_float_rest:
    if (peekChar(lx) == 'e' || peekChar(lx) == 'E') {
        is_float = true;
        advanceChar(lx); /* 'e' or 'E' */
        if (peekChar(lx) == '+' || peekChar(lx) == '-') {
            advanceChar(lx);
        }
        if (!isdigit((unsigned char)peekChar(lx))) {
            while (isalnum((unsigned char)peekChar(lx)) || peekChar(lx) == '_') {
                advanceChar(lx);
            }
            lx->error_count++;
            char *lex = copySubstring(lx->source, start_idx, lx->cursor);
            return makeToken(TOKEN_ERROR, lex, start_line, start_col,
                             "malformed exponent in floating constant");
        }
        while (isdigit((unsigned char)peekChar(lx))) {
            advanceChar(lx);
        }
    }

parse_exponent_and_suffix:
    if (is_float) {
        size_t suf_start = lx->cursor;
        while (isalnum((unsigned char)peekChar(lx)) || peekChar(lx) == '_') {
            advanceChar(lx);
        }
        char *suffix = copySubstring(lx->source, suf_start, lx->cursor);
        bool ok = isValidFloatSuffix(suffix);
        free(suffix);
        char *lex = copySubstring(lx->source, start_idx, lx->cursor);
        if (!ok) {
            lx->error_count++;
            return makeToken(TOKEN_ERROR, lex, start_line, start_col,
                             "malformed floating constant");
        }
        return makeToken(TOKEN_FLOAT_CONSTANT, lex, start_line, start_col, NULL);
    } else {
        size_t suf_start = lx->cursor;
        while (isalnum((unsigned char)peekChar(lx)) || peekChar(lx) == '_') {
            advanceChar(lx);
        }
        char *suffix = copySubstring(lx->source, suf_start, lx->cursor);
        bool ok = isValidIntegerSuffix(suffix);
        free(suffix);
        char *lex = copySubstring(lx->source, start_idx, lx->cursor);
        if (!ok) {
            lx->error_count++;
            return makeToken(TOKEN_ERROR, lex, start_line, start_col,
                             "malformed integer constant");
        }
        return makeToken(TOKEN_INTEGER_CONSTANT, lex, start_line, start_col, NULL);
    }
}

/* ============================================================================
 * String & Character Literal Scanning
 * ============================================================================ */

static Token scanStringLiteral(Lexer *lx) {
    int start_line = lx->line;
    int start_col = lx->column;
    size_t start_idx = lx->cursor;

    advanceChar(lx); /* Consume opening '"' */

    bool closed = false;
    while (!isAtEnd(lx)) {
        char c = peekChar(lx);
        if (c == '"') {
            advanceChar(lx);
            closed = true;
            break;
        } else if (c == '\n' || c == '\r') {
            /* Unescaped newline in string literal -> unterminated */
            break;
        } else if (c == '\\') {
            advanceChar(lx); /* Consume '\\' */
            if (!isAtEnd(lx)) {
                /* Handles line continuation and escaped chars */
                advanceChar(lx);
            }
        } else {
            advanceChar(lx);
        }
    }

    char *lex = copySubstring(lx->source, start_idx, lx->cursor);
    if (!closed) {
        lx->error_count++;
        return makeToken(TOKEN_ERROR, lex, start_line, start_col,
                         "unterminated string literal");
    }
    return makeToken(TOKEN_STRING_LITERAL, lex, start_line, start_col, NULL);
}

static Token scanCharLiteral(Lexer *lx) {
    int start_line = lx->line;
    int start_col = lx->column;
    size_t start_idx = lx->cursor;

    advanceChar(lx); /* Consume opening '\'' */

    if (peekChar(lx) == '\'') {
        advanceChar(lx);
        lx->error_count++;
        char *lex = copySubstring(lx->source, start_idx, lx->cursor);
        return makeToken(TOKEN_ERROR, lex, start_line, start_col,
                         "empty character literal");
    }

    bool closed = false;
    while (!isAtEnd(lx)) {
        char c = peekChar(lx);
        if (c == '\'') {
            advanceChar(lx);
            closed = true;
            break;
        } else if (c == '\n' || c == '\r') {
            /* Unescaped newline in character literal */
            break;
        } else if (c == '\\') {
            advanceChar(lx); /* Consume '\\' */
            if (!isAtEnd(lx)) {
                advanceChar(lx);
            }
        } else {
            advanceChar(lx);
        }
    }

    char *lex = copySubstring(lx->source, start_idx, lx->cursor);
    if (!closed) {
        lx->error_count++;
        return makeToken(TOKEN_ERROR, lex, start_line, start_col,
                         "unterminated character literal");
    }
    return makeToken(TOKEN_CHAR_LITERAL, lex, start_line, start_col, NULL);
}

/* ============================================================================
 * Preprocessor Directive Scanning
 * ============================================================================ */

static Token scanPreprocessor(Lexer *lx) {
    int start_line = lx->line;
    int start_col = lx->column;
    size_t start_idx = lx->cursor;

    advanceChar(lx); /* Consume '#' */

    /* Skip optional spaces/tabs between '#' and directive name */
    while (!isAtEnd(lx) && (peekChar(lx) == ' ' || peekChar(lx) == '\t')) {
        advanceChar(lx);
    }

    /* Consume directive name if present */
    if (isalpha((unsigned char)peekChar(lx)) || peekChar(lx) == '_') {
        while (isalnum((unsigned char)peekChar(lx)) || peekChar(lx) == '_') {
            advanceChar(lx);
        }
    }

    char *lex = copySubstring(lx->source, start_idx, lx->cursor);
    return makeToken(TOKEN_PREPROCESSOR, lex, start_line, start_col, NULL);
}

/* ============================================================================
 * Primary getNextToken Implementation (Maximal Munch FSM)
 * ============================================================================ */

Token getNextToken(Lexer *lx) {
    Token errTok;
    if (skipWhitespaceAndComments(lx, &errTok)) {
        return errTok;
    }

    if (isAtEnd(lx)) {
        return makeToken(TOKEN_EOF, duplicateString(""), lx->line, lx->column, NULL);
    }

    int start_line = lx->line;
    int start_col = lx->column;
    char c = peekChar(lx);

    /* 1. Preprocessor directives */
    if (c == '#') {
        return scanPreprocessor(lx);
    }

    /* 2. String literal */
    if (c == '"') {
        return scanStringLiteral(lx);
    }

    /* 3. Character literal */
    if (c == '\'') {
        return scanCharLiteral(lx);
    }

    /* 4. Float starting with '.' followed by a digit */
    if (c == '.' && isdigit((unsigned char)peekNextChar(lx))) {
        return scanNumber(lx);
    }

    /* 5. Numbers starting with digit */
    if (isdigit((unsigned char)c)) {
        return scanNumber(lx);
    }

    /* 6. Identifiers and Keywords */
    if (isalpha((unsigned char)c) || c == '_') {
        size_t start_idx = lx->cursor;
        while (isalnum((unsigned char)peekChar(lx)) || peekChar(lx) == '_') {
            advanceChar(lx);
        }
        char *lex = copySubstring(lx->source, start_idx, lx->cursor);
        if (isKeyword(lex)) {
            return makeToken(TOKEN_KEYWORD, lex, start_line, start_col, NULL);
        } else {
            return makeToken(TOKEN_IDENTIFIER, lex, start_line, start_col, NULL);
        }
    }

    /* 7. Punctuators: ; , ( ) { } [ ] */
    if (c == ';' || c == ',' || c == '(' || c == ')' ||
        c == '{' || c == '}' || c == '[' || c == ']') {
        advanceChar(lx);
        char buf[2] = { c, '\0' };
        return makeToken(TOKEN_PUNCTUATOR, duplicateString(buf), start_line, start_col, NULL);
    }

    /* 8. Operators with Maximal Munch:
     * Length 3: >>=, <<=
     * Length 2: ++, --, ==, !=, <=, >=, &&, ||, <<, >>, +=, -=, *=, /=, %=, &=, |=, ^=, ->
     * Length 1: +, -, *, /, %, <, >, =, !, &, |, ^, ~, ., ?, :
     */
    switch (c) {
        case '>': {
            advanceChar(lx);
            if (peekChar(lx) == '>') {
                advanceChar(lx);
                if (peekChar(lx) == '=') {
                    advanceChar(lx);
                    return makeToken(TOKEN_OPERATOR, duplicateString(">>="), start_line, start_col, NULL);
                }
                return makeToken(TOKEN_OPERATOR, duplicateString(">>"), start_line, start_col, NULL);
            } else if (peekChar(lx) == '=') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString(">="), start_line, start_col, NULL);
            }
            return makeToken(TOKEN_OPERATOR, duplicateString(">"), start_line, start_col, NULL);
        }

        case '<': {
            advanceChar(lx);
            if (peekChar(lx) == '<') {
                advanceChar(lx);
                if (peekChar(lx) == '=') {
                    advanceChar(lx);
                    return makeToken(TOKEN_OPERATOR, duplicateString("<<="), start_line, start_col, NULL);
                }
                return makeToken(TOKEN_OPERATOR, duplicateString("<<"), start_line, start_col, NULL);
            } else if (peekChar(lx) == '=') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("<="), start_line, start_col, NULL);
            }
            return makeToken(TOKEN_OPERATOR, duplicateString("<"), start_line, start_col, NULL);
        }

        case '+': {
            advanceChar(lx);
            if (peekChar(lx) == '+') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("++"), start_line, start_col, NULL);
            } else if (peekChar(lx) == '=') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("+="), start_line, start_col, NULL);
            }
            return makeToken(TOKEN_OPERATOR, duplicateString("+"), start_line, start_col, NULL);
        }

        case '-': {
            advanceChar(lx);
            if (peekChar(lx) == '-') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("--"), start_line, start_col, NULL);
            } else if (peekChar(lx) == '=') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("-="), start_line, start_col, NULL);
            } else if (peekChar(lx) == '>') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("->"), start_line, start_col, NULL);
            }
            return makeToken(TOKEN_OPERATOR, duplicateString("-"), start_line, start_col, NULL);
        }

        case '*': {
            advanceChar(lx);
            if (peekChar(lx) == '=') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("*="), start_line, start_col, NULL);
            }
            return makeToken(TOKEN_OPERATOR, duplicateString("*"), start_line, start_col, NULL);
        }

        case '/': {
            advanceChar(lx);
            if (peekChar(lx) == '=') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("/="), start_line, start_col, NULL);
            }
            return makeToken(TOKEN_OPERATOR, duplicateString("/"), start_line, start_col, NULL);
        }

        case '%': {
            advanceChar(lx);
            if (peekChar(lx) == '=') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("%="), start_line, start_col, NULL);
            }
            return makeToken(TOKEN_OPERATOR, duplicateString("%"), start_line, start_col, NULL);
        }

        case '&': {
            advanceChar(lx);
            if (peekChar(lx) == '&') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("&&"), start_line, start_col, NULL);
            } else if (peekChar(lx) == '=') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("&="), start_line, start_col, NULL);
            }
            return makeToken(TOKEN_OPERATOR, duplicateString("&"), start_line, start_col, NULL);
        }

        case '|': {
            advanceChar(lx);
            if (peekChar(lx) == '|') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("||"), start_line, start_col, NULL);
            } else if (peekChar(lx) == '=') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("|="), start_line, start_col, NULL);
            }
            return makeToken(TOKEN_OPERATOR, duplicateString("|"), start_line, start_col, NULL);
        }

        case '^': {
            advanceChar(lx);
            if (peekChar(lx) == '=') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("^="), start_line, start_col, NULL);
            }
            return makeToken(TOKEN_OPERATOR, duplicateString("^"), start_line, start_col, NULL);
        }

        case '=': {
            advanceChar(lx);
            if (peekChar(lx) == '=') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("=="), start_line, start_col, NULL);
            }
            return makeToken(TOKEN_OPERATOR, duplicateString("="), start_line, start_col, NULL);
        }

        case '!': {
            advanceChar(lx);
            if (peekChar(lx) == '=') {
                advanceChar(lx);
                return makeToken(TOKEN_OPERATOR, duplicateString("!="), start_line, start_col, NULL);
            }
            return makeToken(TOKEN_OPERATOR, duplicateString("!"), start_line, start_col, NULL);
        }

        case '~': {
            advanceChar(lx);
            return makeToken(TOKEN_OPERATOR, duplicateString("~"), start_line, start_col, NULL);
        }

        case '.': {
            advanceChar(lx);
            return makeToken(TOKEN_OPERATOR, duplicateString("."), start_line, start_col, NULL);
        }

        case '?': {
            advanceChar(lx);
            return makeToken(TOKEN_OPERATOR, duplicateString("?"), start_line, start_col, NULL);
        }

        case ':': {
            advanceChar(lx);
            return makeToken(TOKEN_OPERATOR, duplicateString(":"), start_line, start_col, NULL);
        }

        default:
            break;
    }

    /* 9. Invalid characters (e.g. @, `, $, etc.) */
    {
        advanceChar(lx);
        lx->error_count++;
        char buf[2] = { c, '\0' };
        return makeToken(TOKEN_ERROR, duplicateString(buf), start_line, start_col, "invalid character");
    }
}
