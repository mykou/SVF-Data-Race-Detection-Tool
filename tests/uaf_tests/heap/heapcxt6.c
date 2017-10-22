#include "../uaf.h"
int *alloc( int size){
  return malloc(1);//o
}
void foo(int **p){
  *p = alloc(1);//[cs4]:o
}
void bar(int** q) {
  free(q);//[cs1,cs4]:o, [cs2,cs4]:o, [cs3,cs4]:o
}
void ham(int* r) {
  uaf_use(r);//[cs1,cs4]:o, [cs2,cs4]:o, [cs3,cs4]:o
}

void main(){
  int *a,*b,*c;
  foo(&a);//[cs1,cs4]:o
  foo(&b);//[cs2,cs4]:o
  foo(&c);//[cs3,cs4]:o
  ham(a);
  bar(a);
  ham(b);
  bar(b);
  ham(c);
  bar(c);

  UAF_TN(9,12);//FP if correlation-free
}
