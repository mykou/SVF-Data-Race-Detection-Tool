struct B {
  int c;
};

struct A {
  struct B b;
  int d;
};

void bar(int x, int y){

}
int foo() {

  struct A a;
//  a.b.c = 10;
  a.d = 5;
  bar(10, a.d);
  a.d = 15;
  bar(a.d, 11);
/*
 int a,b,c;
 a = 10;
 b = a;
 a = 15;
 c = a;
*/
}
