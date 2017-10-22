#include "../uaf.h"
int *alloc( int size){
  return malloc(1);//o
}
void foo(int **p){
  *p = alloc(1);//[cs4]:o
}
void bar(int* q) {
  free(q);//[cs1,cs4]:o, [cs2,cs4]:o, [cs3,cs4]:o
}

void main(){
  int *a,*b,*c;
  foo(&a);//[cs1,cs4]:o
  foo(&b);//[cs2,cs4]:o
  foo(&c);//[cs3,cs4]:o
  uaf_use(a);
  bar(a);
  uaf_use(b);
  bar(b);
  uaf_use(c);
  bar(c);
  uaf_use(a);

  UAF_TN(9,19);//FP if correlation-free
  UAF_TN(9,21);//FP if correlation-free
  UAF_TP(9,23);
}
