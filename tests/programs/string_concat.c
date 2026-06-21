int main() {
    const char *msg = "Hello" " " "World";
    int len = 0;
    while (msg[len]) len++;
    return len;
}
