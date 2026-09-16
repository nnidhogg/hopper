int main() {
    const int n = 10;
    int* p = &n;
    for (int i = 0; i < n; i = i + 1) { if (i % 2 == 0) { continue; } else { total += i; } }
    while (running) { do { step(); } while (again()); break; }
    return static_cast<int>(*p);
}
