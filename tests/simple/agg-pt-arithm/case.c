int x;
int y;
struct Z {
    int *s;
    int *t;
    int *k;
};

int main(){
	//int **q,*r; 
	struct Z z;
	int *p, a;
	struct Z *w = &z;
	int **x1 = (int**)w+2;
	//int **x2 = (int**)x1+1;
	*x1 = &a;
	p = w->k;
return 0;
}
