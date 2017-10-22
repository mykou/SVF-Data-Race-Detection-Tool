#include "../uaf.h"
int k = 100;
void foo() {
  int* p = (int*)malloc(1);
  uaf_use(p);
  free(p);
}
int main() {
  for (int i = 0; i < k; ++i) {
    foo();
  }
  UAF_TN(6,5);
  return 0;
}
