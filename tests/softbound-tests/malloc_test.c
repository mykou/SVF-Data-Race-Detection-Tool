#include<stdio.h>
#include<stdlib.h>

int main(int argc, char** argv){
  
  int* ptr, *ptr2, *ptr3;
  size_t sum = 0;
  int i = 0;

  ptr = (int*)malloc(100*sizeof(int));
  ptr2= (int*) malloc(200*sizeof(int));
  ptr3 = ptr;
  //  ptr3 = (int*)realloc(ptr, 200*(sizeof(int)));
  
  printf("ptr =%p, ptr2=%p, ptr3=%p\n", ptr, ptr2, ptr3);
  
  for(i = 0; i < 100; i++){
    ptr2[i] = rand();
    ptr3[i] = ptr2[i] + 10;
  }

  for(i =0; i < 100 ; i++){
    sum += ptr3[i];
  }
  
  free(ptr2);
  free(ptr3);
  printf("sum = %zx\n", sum);
  
  return 0;
}
