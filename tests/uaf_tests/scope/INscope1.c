#include "../uaf.h"
void m1(int** x1)   {  *x1 = (int*)malloc(1); }

void m2(int*** x2)  {  m1(*x2); }

void m3(int**** x3) {  m2(*x3); }

void f1(int** y1)   {  free(*y1); }

void f2(int*** y2)  {  f1(*y2); }

void f3(int**** y3) {  f2(*y3); }

void u1(int** z1)   {  uaf_use(*z1); }

void u2(int*** z2)  {  u1(*z2); }

void u3(int**** z3) {  u2(*z3); }

void sch1() {
  int* p;
  int** p1 = &p;
  int*** p2 = &p1;
  int**** p3 = &p2;
  m3(p3);
  f3(p3);
  u3(p3);
}

void sch2() {  sch1(); }

void sch3() {  sch2(); }

int main() {
  sch3();
  UAF_TP(8,14);
  return 1;
}
