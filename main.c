#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read entire file into a heap-allocated buffer */
static char *readFile(const char *path, size_t *out_size) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Error: could not open file '%s'\n", path);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "Error: could not determine size of file '%s'\n", path);
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fprintf(stderr, "Error: could not determine size of file '%s'\n", path);
        fclose(file);
        return NULL;
    }
    rewind(file);

    char *buffer = (char *)malloc((size_t)file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: out of memory reading file '%s'\n", path);
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, (size_t)file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    if (out_size) {
        *out_size = bytes_read;
    }
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file.c>\n", argv[0]);
        return EXIT_FAILURE;
    }

    size_t file_len = 0;
    char *source = readFile(argv[1], &file_len);
    if (!source) {
        return EXIT_FAILURE;
    }

    Lexer *lx = initLexer(source, file_len);

    /* Token statistics */
    unsigned long counts[TOKEN_ERROR + 1];
    memset(counts, 0, sizeof(counts));
    unsigned long total_tokens = 0;
    unsigned long total_errors = 0;

    Token tok;
    do {
        tok = getNextToken(lx);
        if (tok.type == TOKEN_ERROR) {
            total_errors++;
            printf("Error at %d:%d: %s (lexeme: \"%s\")\n",
                   tok.line, tok.column,
                   tok.error_msg ? tok.error_msg : "lexical error",
                   tok.lexeme ? tok.lexeme : "");
        } else if (tok.type != TOKEN_EOF) {
            printToken(stdout, &tok);
            counts[tok.type]++;
            total_tokens++;
        }
        freeToken(&tok);
    } while (tok.type != TOKEN_EOF);

    /* Print Summary Table */
    printf("\n");
    printf("==================================================\n");
    printf("                  TOKEN SUMMARY                   \n");
    printf("==================================================\n");
    printf("%-28s %15s\n", "Token Type", "Count");
    printf("--------------------------------------------------\n");
    printf("%-28s %15lu\n", "KEYWORD",            counts[TOKEN_KEYWORD]);
    printf("%-28s %15lu\n", "IDENTIFIER",         counts[TOKEN_IDENTIFIER]);
    printf("%-28s %15lu\n", "INTEGER_CONSTANT",   counts[TOKEN_INTEGER_CONSTANT]);
    printf("%-28s %15lu\n", "FLOAT_CONSTANT",     counts[TOKEN_FLOAT_CONSTANT]);
    printf("%-28s %15lu\n", "CHAR_LITERAL",       counts[TOKEN_CHAR_LITERAL]);
    printf("%-28s %15lu\n", "STRING_LITERAL",     counts[TOKEN_STRING_LITERAL]);
    printf("%-28s %15lu\n", "OPERATOR",           counts[TOKEN_OPERATOR]);
    printf("%-28s %15lu\n", "PUNCTUATOR",         counts[TOKEN_PUNCTUATOR]);
    printf("%-28s %15lu\n", "PREPROCESSOR",       counts[TOKEN_PREPROCESSOR]);
    printf("--------------------------------------------------\n");
    printf("%-28s %15lu\n", "Total Valid Tokens", total_tokens);
    printf("%-28s %15lu\n", "Total Lexical Errors", total_errors);
    printf("==================================================\n");

    freeLexer(lx);
    free(source);

    return (total_errors > 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}
