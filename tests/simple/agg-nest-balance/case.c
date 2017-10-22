int x;
int y;
struct Nest{
   int *c;
   int *d;
};

struct Z {
    int *s;
    struct Nest *t;
};

struct Z z = {&y,&x};
struct Z u = {&y,&x};

int main(){
	//int **q,*r; 
	int *p, a;
	struct Z *w = &z;
	struct Z *v = &z;
	//q = &r;
	//*q = &a;
	w->t->d = &a;
	p = v->t->d;
return 0;
}
