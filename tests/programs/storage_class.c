static int global_var = 42;

extern int external_func(int x);

static int helper(int x) {
    return x * 2;
}

int main() {
    static int local_static = 10;
    int result = helper(local_static) + global_var;
    return result;
}
