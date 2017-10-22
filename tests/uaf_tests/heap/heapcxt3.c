#include "../uaf.h"
int *alloc( int size){
  return malloc(1);//o
}
void foo(int **p){
  *p = alloc(1);//[cs4]:o
}

void main(){
  int *a,*b,*c;
  foo(&a);//[cs1,cs4]:o
  foo(&b);//[cs2,cs4]:o
  foo(&c);//[cs3,cs4]:o
  uaf_use(a);
  free(a);
  uaf_use(b);
  free(b);
  uaf_use(c);
  free(c);
  uaf_use(a);

  UAF_TN(15,16);
  UAF_TN(15,18);

  UAF_TN(17,18);
  UAF_TN(17,20);
  
  UAF_TN(16,20);
  UAF_TN(19,20);
  UAF_TP(15,20);
}
