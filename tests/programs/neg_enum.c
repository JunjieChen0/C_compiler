typedef enum {
    REG_NONE = -1,
    REG_RAX = 0,
    REG_RCX = 1,
    REG_RDX = 2,
} Register;

int main() {
    Register r = REG_NONE;
    return (int)r;
}
