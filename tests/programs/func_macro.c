#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SQUARE(x) ((x) * (x))

int main() {
    int x = 5;
    int y = 10;
    int max_val = MAX(x, y);
    int sq = SQUARE(x);
    return max_val + sq;
}
