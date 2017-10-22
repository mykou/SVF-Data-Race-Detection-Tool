#include "../uaf.h"
/*
 * on call graph, free and use are first converged at fu1, and then fu1 and malloc are converged at mfu1
 */
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
  f1(q2);		//c5			///[c2,c1]:o
}
void u1(int* r1){	//1st tier use wrapper
  uaf_use(r1);		//use site		///[c2,c1]:o
}
void u2(int* r2){	//2nd tier use wrapper
  u1(r2);		//c6			///[c2,c1]:o
}
void fu1(int* x1) {	//1st tier free-use wrapper
  f2(x1);		//c7			///[c2,c1]:o
  u2(x1);
}
void fu2(int* x2) {	//2nd tier free-use wrapper
  fu1(x2);		//c3			///[c2,c1]:o
}
void mfu1() {		//1st tier malloc-free-use wrapper
  int* y = m2();	//c4			///[c2,c1]:o
  fu2(y);		//c8			///[c2,c1]:o
}
void mfu2() {		//2nd tire malloc-free-use wrapper
  mfu1();		//c9
}
int main() {
  mfu2();		//c10
  UAF_TP(14,20);
  return 0;
}
