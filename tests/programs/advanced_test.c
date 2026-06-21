#include <stdio.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct {
    int x;
    int y;
} Point;

int add(int a, int b) {
    return a + b;
}

int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    Point p;
    p.x = 10;
    p.y = 20;
    
    int sum = add(p.x, p.y);
    int max_val = MAX(p.x, p.y);
    int min_val = MIN(p.x, p.y);
    int fact = factorial(5);
    
    return sum + max_val + min_val + fact;
}
