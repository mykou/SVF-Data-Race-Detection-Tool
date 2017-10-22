#include<stdio.h>
#include<sys/mman.h>
#include<string.h>

int main(int argc, char** argv){
  
  int *p =  NULL;
  int *q = NULL;

  int arr[100] = {1,2,3,4,5,6,7};
  int test;
  int j;

  size_t length_trie = 128 * 1024 * 1024;

  //  p = mmap((void*)0x7fca52fffff0, length_trie, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_PRIVATE, -1, 0);
  p = mmap(0, length_trie, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);

  *p = test;
  char* temp1 = (char*) p + 4;

  printf("address of temp1=%p\n", temp1);
  *((int**) temp1) = arr;

  q = mmap(0, length_trie, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);


  printf("p=%zx, q=%zx\n", p, q);

  memcpy(q, p, length_trie);

  char* temp = (char*) q + 4;

  printf("address of temp=%p\n", temp);
  int *r = *((int**)temp);

  printf("value of r is %p, arr=%zx\n", r, arr);
  for(j = 0; j < 10; j++){
    printf("value is %d\n",r[j]);
  }

  return 0;
}
