#include "../uaf.h"
int k = 100;
void foo(int n) {
  int* p = (int*)malloc(1);
  for (int i = 0; i < n; i++) {
     uaf_use(p);
     free(p);
  }
}
int main() {
  k = 90;
  foo(k);
  UAF_TP(7,6);
  return 0;
}
