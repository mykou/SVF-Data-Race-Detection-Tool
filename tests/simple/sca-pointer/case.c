typedef struct {
    int s;
    int t;
} Z;



void check(void *p, void *q){

}

int main(){
   int *a,*b,x,y;
   Z agg;
   Z *z = &agg;
   a = &x;
   b = &y;
   check(a,b);
}
