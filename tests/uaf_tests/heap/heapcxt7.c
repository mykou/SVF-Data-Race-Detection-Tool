#include "../uaf.h"
int* foo() {
  int* x = (int*)malloc(sizeof(int));//o
  return x;
}

void bar() {
  int* p = foo();//c1:o
  int* q = foo();//c2:o
  free(p);
  uaf_use(q);
}

int main() {
  bar();
  UAF_TN(10,11);
  return 0;
}
