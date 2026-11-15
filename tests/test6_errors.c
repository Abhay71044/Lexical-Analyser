/* Test 6: Lexical errors demonstrating error detection and recovery */

int main(void) {
    /* 1. Malformed numbers */
    int bad_hex = 0xZZ;
    int bad_dec = 12abc;
    int bad_octal = 089;
    double bad_exp = 1e+;

    /* 2. Invalid characters */
    int @invalid_var = 10;
    int `another_invalid` = 20;
    int $cash = 30;

    /* 3. Empty character literal */
    char empty_ch = '';

    /* 4. Unterminated character literal */
    char unterm_ch = 'x;
    int next_statement_after_unterm_ch = 100;

    /* 5. Unterminated string literal */
    const char *unterm_str = "unterminated string literal across line
    int next_statement_after_unterm_str = 200;

    /* 6. Continued valid tokens after errors */
    int recovery_valid = 999;
    recovery_valid += 1;

    /* 7. Unterminated multi-line comment placed at end */
    /* This comment is never closed until the end of the file
