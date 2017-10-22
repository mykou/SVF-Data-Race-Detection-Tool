#include "../uaf.h"
int k = 100;
void foo(int n) {
  for (int i = 0; i < n; i++) {
     int* p = (int*)malloc(1);
     uaf_use(p);
     free(p);
  }
}
int main() {
  k = 90;
  foo(k);
  UAF_TN(7,6);
  return 0;
}
