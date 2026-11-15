/* Test 3: Integer and Floating-point constants with various notations and suffixes */

int main(void) {
    /* Decimal integers */
    int d0 = 0;
    int d1 = 42;
    int d2 = 123456789;

    /* Suffixes for integers (case variations) */
    unsigned int u1 = 100u;
    unsigned int u2 = 200U;
    long l1 = 1000L;
    long l2 = 2000l;
    unsigned long ul1 = 3000ul;
    unsigned long ul2 = 4000UL;
    unsigned long ul3 = 5000Lu;
    long long ll1 = 1234567890123LL;
    unsigned long long ull1 = 9876543210987ULL;
    unsigned long long ull2 = 1111111111111llu;

    /* Octal integers */
    int oct1 = 00;
    int oct2 = 052;
    int oct3 = 0777;
    unsigned long oct4 = 0755UL;

    /* Hexadecimal integers */
    int hex1 = 0x0;
    int hex2 = 0x1F;
    int hex3 = 0xabcd;
    int hex4 = 0xABCD;
    unsigned int hex5 = 0xDEADBEEFu;
    unsigned long long hex6 = 0xCAFEBABE1234ULL;

    /* Floating-point constants */
    double f1 = 3.14159;
    float f2 = 2.5f;
    float f3 = 0.5F;
    double f4 = 1.;
    double f5 = .75;
    double f6 = 1e-5;
    double f7 = 2.5e+3;
    double f8 = 10E6;
    long double f9 = 1.0e-3L;

    /* Hexadecimal floating-point constants (C99/C11) */
    double hf1 = 0x1.fp3;
    double hf2 = 0x1p+4;
    float hf3 = 0x0.8p-1f;

    return 0;
}
