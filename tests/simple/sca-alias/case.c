void foo(){
	int a,b, *w,*v,**s,*t;
	w = &a;
	v = &b;
	s = &w;
	*s = v;
	t = w;

}

int* bar(int **x, int **y){
	
	int *b,*c,d;
	b = &d;
	*x = b;
	c = *y;
	return c;	
}


int main(){

	int *r,*k,**p,**q;
	
	p = &r;
	r = *p;
	p = &k;
	q = p;
	k = bar(p,q);
	printf("%d%d%d",p,r,k);
	

}
