#include"../uaf.h"

int* h(){
  int* r = (int*)malloc(1);
  return r;
}

void f() {
  int *p = h();
  free(p);
  uaf_use(p);
}

void g(){
  int* q = h();
  uaf_use(q);
  free(q);
}

void k1() {
  f();
  g();
}

void k2(){
  k1();
}

int main(){
  k2();
  UAF_TP(10,11);
  return 0;
}
