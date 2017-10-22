int x = 10;
int *y = &x;
struct Z {
    int s;
    int *t;
};

struct Z z = {10,&x};
struct Z *m = &z;
struct Z n = {10,&z.s};

void foo(int* t){
}
int main(){

        int *a;
        int *b;
        a = b;
        a = n.t;
        b = m->t;
        foo(a = (*m).t);
	//struct Z k = n;
return 0;

}
