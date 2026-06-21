int main() {
    int a = 48;
    int b = 18;

    int shifted = 1 << 4;
    int masked = shifted & 31;
    int xored = masked ^ 31;
    int ored = shifted | masked;
    int anded = a & b;

    int result = shifted + masked + xored + ored + anded;

    int x = 100;
    int y = 7;
    int quot = x / y;
    int rem = x % y;
    result = result + quot + rem;

    return result;
}
