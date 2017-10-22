#include "../uaf.h"

void rec(int* x) {
  uaf_use(x);
  free(x);
  int* y = (int*)malloc(1);
  rec(y);
}

void shc() {
  int* x = (int*)malloc(1);
  rec(x);
}

int main() {
  shc();
  UAF_TN(5,4);
  return 0;
}

