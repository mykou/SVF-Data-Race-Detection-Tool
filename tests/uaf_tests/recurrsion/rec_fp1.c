#include "../uaf.h"

void rec(int* x) {
  uaf_use(x);
  free(x);
  int* y = (int*)malloc(1);
  rec(y);
}

int main() {
  int* x = (int*)malloc(1);
  rec(x);
  UAF_TN(5,4);
  return 0;
}
