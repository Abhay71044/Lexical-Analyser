/* Test 4: Operators and Punctuators with Maximal Munch rule */

struct Node {
    int value;
    struct Node *next;
};

int main(void) {
    int a = 10;
    int b = 20;

    /* Maximal munch: >>= vs >> vs > */
    a >>= 2;
    int c = a >> 1;
    int d = a > b;

    /* Maximal munch: <<= vs << vs < */
    a <<= 1;
    int e = a << 2;
    int f = a < b;

    /* Comparisons */
    int eq = (a == b);
    int neq = (a != b);
    int le = (a <= b);
    int ge = (a >= b);

    /* Increment / Decrement */
    a++;
    ++b;
    a--;
    --b;

    /* Arithmetic and compound assignment */
    a += 5;
    b -= 3;
    a *= 2;
    b /= 4;
    a %= 3;

    /* Logical and Bitwise */
    int log_and = (a && b);
    int log_or = (a || b);
    int log_not = !a;
    int bit_and = a & b;
    int bit_or = a | b;
    int bit_xor = a ^ b;
    int bit_not = ~a;
    a &= 0xFF;
    b |= 0x10;
    a ^= 0x01;

    /* Pointer member and struct member */
    struct Node n;
    n.value = 42;
    struct Node *p = &n;
    int val = p->value;

    /* Conditional / Ternary operator */
    int max = (a > b) ? a : b;

    return max;
}
