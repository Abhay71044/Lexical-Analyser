/* Test 5: String literals, character literals, escape sequences, and preprocessors */

#include <stdio.h>
#define BUFFER_SIZE 1024
#define MESSAGE "Lexical Analysis in C\n"

#ifdef DEBUG
#define LOG(msg) printf("[DEBUG]: %s\n", msg)
#else
#define LOG(msg)
#endif

#pragma once

int main(void) {
    /* Character literals with escape sequences */
    char c1 = 'a';
    char c2 = '\n';
    char c3 = '\t';
    char c4 = '\0';
    char c5 = '\\';
    char c6 = '\'';
    char c7 = '\"';
    char c8 = '\a';
    char c9 = '\b';
    char c10 = '\r';
    char c11 = '\x1b';

    /* String literals with escapes and line continuation */
    const char *str1 = "Simple string literal";
    const char *str2 = "String with escapes: \t tab, \n newline, \"quotes\", and \\ backslash";
    const char *str3 = "Multi-line string \
using backslash line continuation";

    printf("%s %c\n", str1, c1);
    return 0;
}
