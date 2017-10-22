#include "../uaf.h"
int k = 100;
int *p;
void foo() {
  uaf_use(p);
  free(p);
}
int main() {
  p = (int*)malloc(1);
  for (int i = 0; i < k; ++i) {
    foo();
  }
  UAF_TP(6,5);
  return 0;
}
