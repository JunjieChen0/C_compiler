int main() {
    int sum = 0;

    int i = 0;
    while (i < 10) {
        sum = sum + i;
        i = i + 1;
    }

    int j = 0;
    do {
        sum = sum + j;
        j = j + 1;
    } while (j < 5);

    int k;
    for (k = 0; k < 10; k++) {
        sum = sum + k;
    }

    if (sum > 100) {
        sum = sum - 50;
    } else {
        sum = sum + 50;
    }

    int val = 42;
    if (val == 42) {
        val = 1;
    } else if (val == 43) {
        val = 2;
    } else {
        val = 3;
    }

    sum = sum + val;

    int counter = 0;
    while (counter < 20) {
        counter = counter + 1;
        if (counter % 2 == 0) continue;
        sum = sum + counter;
    }

    int x = 0;
    while (x < 100) {
        x = x + 1;
        if (x > 50) break;
        sum = sum + x;
    }

    return sum;
}
