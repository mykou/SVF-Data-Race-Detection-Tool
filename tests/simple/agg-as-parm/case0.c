//char p = 'a';
char q = 'b';
char g = 'b';
char *gptr = "ass";
typedef struct Agg{
	int x;
	int y;
}Agg;

void foo(int *s){

}

void bar(int k){

}

void goo(int k){

}

int main(){
	int array[100];
	int* ptr = &array;
	int k = ptr[101];
	foo(ptr);
	foo(&q);
	foo(&g);
	//foo(gptr);
	bar(10);
	goo(10);
	Agg agg;  
	int* ptragg = &agg.x;
	int m = *(ptragg+2);
	/*
	char p = 'a';
	char* buf = malloc(10);
	char* i = buf;
	//while(i > 10){
	if(i > 1)
	p  = *i;
	else
	p = 'c';
	char q = p;
	printf("%d%d%d\n",p,q,i);
	p = i[11];
	//}
	//exit(1);
	*/
}
