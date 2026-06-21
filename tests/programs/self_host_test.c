int sum_to_n(int n) {
    int sum = 0;
    int i = 1;
    while (i <= n) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}

int is_even(int n) {
    if (n % 2 == 0) return 1;
    return 0;
}

int main() {
    int result = 0;

    int a = 10;
    int b = 20;
    int c = a + b;
    result = result + c;

    int d = a * b;
    result = result + d;

    int e = b / a;
    result = result + e;

    int f = b % a;
    result = result + f;

    int g = a << 2;
    result = result + g;

    int h = b >> 1;
    result = result + h;

    int i = a & b;
    result = result + i;

    int j = a | b;
    result = result + j;

    int k = a ^ b;
    result = result + k;

    int x = 0;
    while (x < 10) {
        x = x + 1;
    }
    result = result + x;

    int y = 0;
    do {
        y = y + 1;
    } while (y < 5);
    result = result + y;

    int z;
    for (z = 0; z < 3; z++) {
        result = result + 1;
    }

    if (result > 100) {
        result = result + 10;
    } else {
        result = result + 20;
    }

    return result;
}
