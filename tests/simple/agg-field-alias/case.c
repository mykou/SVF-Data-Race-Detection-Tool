int x;
int y;
struct Z {
    int *s;
    int *t;
};

//struct Z z = {&y,&x};
//struct Z u = {&y,&x};

int main(){
	//int **q,*r; 
	struct Z z;
	int *p, a;
	struct Z *w = &z;
	//struct Z *v = &z;
	//q = &r;
	//*q = &a;
	w->s = &a;
	p = w->s;
return 0;
}
