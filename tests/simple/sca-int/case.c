void foo(int q){
  int i = 10;
  int k = i;

}
void main(){

int *s,*r,***x,**y,t,z,k;
	s = &t;
	r = &z;
	y = &r;
	s = r;;
	x = *y;
	foo(k);

}
