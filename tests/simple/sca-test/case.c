void foo(){

}

int main(){

  int **a,*b,c;
  a = &b;
  b = &c;
  int *x = *a;
  int y = *x;
  foo();
}
