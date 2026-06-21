typedef struct {
    int x;
    int y;
} Point;

int main() {
    Point p = (Point){10, 20};
    return p.x + p.y;
}
