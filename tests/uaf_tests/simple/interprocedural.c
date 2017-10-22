#include "../uaf.h"

void foo(int* x) {
  free(x);
}

void bar(int* y) {
  uaf_use(y);
}

int main() {
  int* p = (int*)malloc(1);
  foo(p);
  bar(p);
  UAF_TP(4,8);
}
