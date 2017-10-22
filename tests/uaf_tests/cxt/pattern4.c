#include "../uaf.h"
/*
 * on call graph, malloc and free are first converged at mf1, and then mf1 and use are converged at mfu1
 */
int* gl;

int* m1() {		//1st tier malloc wrapper
  int* p1 = (int*)malloc(1);//malloc site	///o
  return p1;
}
int* m2() {		//2nd tier malloc wrapper
  int* p2 = m1();	//c1			///[c1]:o
  return p2;
}
void f1(int* q1) {	//1st tier free wrapper
  free(q1);		//free site		///[c2,c1]:o
}
void f2(int* q2) {	//2nd tier free wrapper
  f1(q2);		//c6			///[c2,c1]:o
}
void u1(int** r1){	//1st tier use wrapper
  uaf_use(*r1);		//use site		///[c5,c4,c3,c2,c1]:o
}
void u2(int** r2){	//2nd tier use wrapper
  u1(r2);		//c7			///[c5,c4,c3,c2,c1]:o
}
int* mf1() {		//1st tier malloc-free wrapper
  int* x1 = m2();	//c2			///[c2,c1]:o
  f2(x1);		//c8			///[c2,c1]:o
  return x1;
}
int* mf2() {		//2nd tier malloc-free wrapper
  int* x2 = mf1();	//c3			///[c3,c2,c1]:o
  return x2;
}
int* mf3() {
  int* x3 = mf2();	//c4			///[c4,c3,c2,c1]:o
  return x3;
}
void mfu1() {		//1st tier malloc-free-use wrapper
  int* gl = mf3();	//c5			///[c5,c4,c3,c2,c1]:o
  int** y = &gl;
  u2(y);		//c9			///[c5,c4,c3,c2,c1]:o
}
void mfu2() {		//2nd tire malloc-free-use wrapper
  mfu1();		//c10
}
int main() {
  mfu2();		//c11
  UAF_TP(16,22);
  return 0;
}
