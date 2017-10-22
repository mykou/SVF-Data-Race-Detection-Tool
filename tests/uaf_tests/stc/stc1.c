#include "../uaf.h"

void m(int** x) { *x = (int*)malloc(1); }

void f(int** y) { free(*y); }

void u(int** z) { uaf_use(*z); }

void sch() {
  int* p;
  m(&p);
  u(&p);
  f(&p);
  
  int* q;
  m(&q);
  u(&q);
  f(&q);
}

int main() {
  sch();
  UAF_TN(5,7);
  return 1;
}
