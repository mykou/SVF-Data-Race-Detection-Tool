//char p = 'a';
char q = 'b';

void goo(){
	//foo();

}


int foo(){
	int x = 10;
	if(x > 5) goo();

	return 0;
}


int main(){

	char p = 'a';
	char* buf = malloc(10);
	char* i = buf;
	//while(i > 10){
	if(i > 1)
		if(i > 10)
		p  = *i;
	else
	p = 'c';
	char q = p;
	printf("%d%d%d\n",p,q,i);
	p = i[11];
	foo();
	//}
	//exit(1);

}
