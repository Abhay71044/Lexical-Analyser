/* Test 2: Comprehensive test for C89, C99, and C11 keywords */

typedef unsigned long ulong_t;

struct Point {
    int x;
    int y;
};

union Data {
    int i;
    float f;
    char str[20];
};

enum Color {
    RED,
    GREEN,
    BLUE
};

extern int global_counter;
static const volatile double PI = 3.141592653589793;

inline static int add(int a, int b) {
    return a + b;
}

_Noreturn void fatal_exit(void) {
    auto int code = 1;
    register int fast_val = code;
    (void)fast_val;
    while (1) {
        break;
    }
}

int main(void) {
    _Bool flag = 1;
    _Complex double cd = 1.0;
    _Imaginary double id = 2.0;
    _Static_assert(sizeof(int) >= 2, "int size check");
    _Alignas(16) int aligned_var = 0;
    size_t sz = _Alignof(int);

    int val = 10;
    switch (val) {
        case 1:
            goto label_end;
        default:
            break;
    }

    for (int i = 0; i < 5; i++) {
        if (i == 2) continue;
        do {
            i++;
        } while (0);
    }

label_end:
    return sizeof(ulong_t);
}
