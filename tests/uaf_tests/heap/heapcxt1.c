#include "../uaf.h"
int* foo() {
  int* x = (int*)malloc(sizeof(int));//o
  return x;
}

int main() {
  int* p = foo();//c1:o
  int* q = foo();//c2:o
  free(p);
  uaf_use(q);
  UAF_TN(10,11);
  return 0;
}
