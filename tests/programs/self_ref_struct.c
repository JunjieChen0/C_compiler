struct Node {
    int data;
    struct Node *next;
};

int main() {
    struct Node n1;
    struct Node n2;
    n1.data = 10;
    n1.next = &n2;
    n2.data = 20;
    n2.next = 0;
    return n1.next->data;
}
