#include "../uaf.h"

void rec(int* x) {
  free(x);
  uaf_use(x);
  int* y = (int*)malloc(1);
  rec(y);
}

void shc() {
  int* x = (int*)malloc(1);
  rec(x);
}

int main() {
  shc();
  UAF_TP(4,5);
  return 0;
}

