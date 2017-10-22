//#include <stdlib.h>
void swap(char **a, char **b) {
   char * c;
   c = *a;
}

void * mymalloc(unsigned i) {
   return malloc(i);
}

void  my_malloc(char **ret) {
   *ret = mymalloc(10);
}


int main (){
    char * p1, *p2;

    my_malloc(&p1);
    swap(&p1, &p2);
}

