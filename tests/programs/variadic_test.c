#include <stdarg.h>

int sum(int count, ...) {
    va_list ap;
    va_start(ap, count);
    
    int total = 0;
    for (int i = 0; i < count; i++) {
        total += va_arg(ap, int);
    }
    
    va_end(ap);
    return total;
}

int main() {
    int s1 = sum(3, 10, 20, 30);
    int s2 = sum(2, 100, 200);
    return s1 + s2;
}
