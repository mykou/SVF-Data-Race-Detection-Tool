struct x{
	int * b[10];
	int (*c)[10];
};

struct y{
	int a[10];
};

struct cin{
   int a;
};
struct din{
   struct cin a;
};
struct ein{
   struct din a;
};

struct munger_struct {
  int f1;
  int f2;
};

void munge(struct munger_struct *P) {
  //P[0].f1 = P[1].f1 + P[2].f2;
}

int main(){
   struct x x1;
   struct y y1;
   struct ein c;
   int** p = (int*)malloc(10);
   int* q  = p[9];
   p[2]  = q;
   //c.a.a.a = 10;
   //y1.a[2] = 10;
   //x1.b[1][1] = 10;
   //*(x1.b[1]) = 10;
}
