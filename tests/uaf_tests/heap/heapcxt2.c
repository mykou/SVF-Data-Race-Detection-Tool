#include "../uaf.h"
int* foo() {
  int* x = (int*)malloc(sizeof(int));//o
  return x;
}

int* bar() {
  int* y = foo();//[c3]:o
  return y;
}

int main() {
  int* p = bar();//[c1,c3]:o
  int* q = bar();//[c2,c3]:o
  free(p);
  uaf_use(q);
  UAF_TN(15,16);
  return 0;
}
