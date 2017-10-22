#include <stdlib.h>
#include <stdio.h>
#include "aliascheck.h"
int a;
void f(int* x) { MUSTALIAS(x,&a); printf("f\n"); }
void g() { printf("g\n"); }

struct S {
  void (*p)(int*);
};

struct S *s;

void fun(int* y) {
  s->p(y);
}

int main(int argc, char **argv)
{

  // alloca
  struct S s1;
  s1.p = &f;
  s = &s1;
  fun(&a);
/*
  struct S s2;
  s2.p = &g;
  s = &s2;
  fun();
*/
  ////// heap
  //s = (struct S*)malloc(sizeof(struct S));
  //s->p = &f;
  //fun();

  //s = (struct S*)malloc(sizeof(struct S));
  //s->p = &g;
  //fun();

  return 0;
}
