int main(){
  
  int* a,c;//*b;//,c;
  printf("a %zx\n",&a);
  //printf("b %zx\n",&b);
  a = malloc(sizeof(int)*10);
  //b = a;
  c = a[10];
  //int a[10];
  //int b = a[10];
}
