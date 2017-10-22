typedef struct __attribute__((__packed__)){

 char c;
 double d;
} agg;

typedef struct{
	char c;
	char c2;
	char arrty[7];
	double d;
} large;

typedef struct {
	int a;
	int b;
} TwoInt;

typedef union {
 double d;
 double d2;
 TwoInt ti;
}nu;

typedef union {
 int h;
 char k;
}au;

typedef struct{
	char c;
	agg a;
	au s;
	int x;
} group;



void main(){
  //agg a ;
  //large l;
  //group g;
  nu uni;
  double f1 = uni.d;
  double f2 = uni.d2;
  //int * second = &uni.ti.b;
  //double *partial = &uni.d;
  //char *c = &g.s.k;
  //int *i = &g.s.h;

}
