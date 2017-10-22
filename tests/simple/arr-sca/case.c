int main(){

char A[10], B[10];
char *a,*b,**t1,**t2;

t1 = &a;
t2 = &b;
a = &A;
b = &B;
printf("%d%d%d%d%d%d",t1,t2,a,b,A,B);
return 0;

}
