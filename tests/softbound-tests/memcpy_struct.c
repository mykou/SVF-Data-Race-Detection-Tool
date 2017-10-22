#include<stdio.h>
#include<sys/mman.h>
#include<string.h>

struct tstruct{
  char test_var;
  int *ptr;
};

int main(int argc, char** argv){
  
  int *p =  NULL;
  int *q = NULL;

  struct tstruct temp_st;

  int arr[100] = {1,2,3,4,5,6,7};
  int test;
  int j;

  temp_st.test_var = 1;
  temp_st.ptr = arr;


  size_t length_trie = 128 * 1024 * 1024;

  p = mmap(0, length_trie, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);

  printf("doing the memcopy of the structure in to the mmap region\n");

  memcpy(p, &temp_st, sizeof(struct tstruct));

  q = mmap(0, length_trie, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);


  printf("p=%p, q=%p\n", p, q);

  memcpy(q, p, length_trie);

  struct tstruct* temp_ptr = (struct tstruct*) q;
  int* r = temp_ptr->ptr;


  printf("address of temp_ptr->ptr=%p\n", &temp_ptr->ptr);

  printf("value of r is %p, arr=%zx\n", r, arr);
  for(j = 0; j < 10; j++){
    printf("value is %d\n",r[j]);
  }

  return 0;
}
