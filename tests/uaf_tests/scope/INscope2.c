#include "../uaf.h"
void irrelavent() { printf("1");}

void m1(int** x1)   {  *x1 = (int*)malloc(1); irrelavent(); }

void m2(int*** x2)  {  m1(*x2); irrelavent(); }

void m3(int**** x3) {  m2(*x3); irrelavent(); }

void f1(int** y1)   {  free(*y1); irrelavent(); }

void f2(int*** y2)  {  f1(*y2); irrelavent(); }

void f3(int**** y3) {  f2(*y3); irrelavent(); }

void u1(int** z1)   {  uaf_use(*z1); irrelavent(); }

void u2(int*** z2)  {  u1(*z2); irrelavent(); }

void u3(int**** z3) {  u2(*z3); irrelavent(); }

void sch1() {
  int* p;
  int** p1 = &p;
  int*** p2 = &p1;
  int**** p3 = &p2;
  m3(p3);
  f3(p3);
  u3(p3);  irrelavent();
}

void sch2() {  sch1(); irrelavent(); }

void sch3() {  sch2(); irrelavent(); }

int main() {
  sch3(); irrelavent();
  UAF_TP(10,16);
  return 1;
}
