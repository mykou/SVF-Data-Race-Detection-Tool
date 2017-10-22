int * foo(int *r){
 return r;
}

int main(){
  int *p,*q,a,b;
  p = foo(&a);
  q = foo(&b);
}
