#include <stdarg.h>

int my_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    
    int count = 0;
    const char *p = fmt;
    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == 'd') {
                int val = va_arg(ap, int);
                // Simple integer to string conversion
                if (val < 0) {
                    count++;
                    val = -val;
                }
                int digits = 0;
                int tmp = val;
                do {
                    digits++;
                    tmp /= 10;
                } while (tmp > 0);
                count += digits;
            } else if (*p == 's') {
                const char *s = va_arg(ap, const char *);
                while (*s) {
                    count++;
                    s++;
                }
            } else if (*p == '%') {
                count++;
            }
        } else {
            count++;
        }
        p++;
    }
    
    va_end(ap);
    return count;
}

int main() {
    int len = my_printf("Hello %s, you have %d messages", "World", 42);
    return len;
}
