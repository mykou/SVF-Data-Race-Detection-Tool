#include "../uaf.h"
int k = 100;

void m1(int** p1) {  *p1 = (int*)malloc(1); }

void m2(int** p2) {  m1(p2); }

void m3(int** p3) {  m2(p3); }

void r1();
void r2(int*);
void r3(int**);

void r1() {
  if (k == 0) return;
  int* x1;
  m3(&x1);
  r2(x1);
}

void r2(int* x2) {
  if (k == 0) return;
  free(x2);
  r3(&x2);
}

void r3(int** x3) {
  if (k == 0) return;
  k--;
  uaf_use(*x3);
  r1();
}

void shc() {
  r1();
}

int main() {
  shc();
  UAF_TP(23,30);
  return 0;
}

