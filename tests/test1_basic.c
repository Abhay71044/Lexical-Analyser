/* Test 1: Basic C program with functions, variables, arithmetic, and comments */
#include <stdio.h>

// Function to compute factorial
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main(void) {
    int num = 5;
    int result = factorial(num);
    /* Print the factorial result
       spanning multiple lines */
    printf("Factorial of %d is %d\n", num, result);
    return 0;
}
