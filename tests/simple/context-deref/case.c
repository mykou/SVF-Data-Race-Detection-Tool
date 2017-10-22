void foo(int **x, int*y){
   *x = y;
}

int main(){
  int *p,*q,a,b;
  foo(&p, &a);
  int * s = p;
  foo(&q, &b);
  int *k = q;
}
