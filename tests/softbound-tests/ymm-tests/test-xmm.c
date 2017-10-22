#include<stdio.h>
#include<smmintrin.h>

struct metadata{
  void* base;
  void* bound;
  size_t key;
  void* lock;  
};

int a;
int b;
int c;
int d;

#define NUM_ITER 1000

size_t global_key=1;
void* global_lock = &global_key;

void * global_base = &a;
void*  global_bound = &b;

void* global_base2= &c;
void* global_bound2=&d;

int count = 0;
struct metadata test[100];

void xmm_store_metadata(__v2di, __v2di);
void xmm_load_metadata(__v2di*, __v2di*);

__v2di construct_v2di_ptr_ptr(void* base, void* bound){
  
  __v2di temp = {(size_t) base, (size_t) bound};
  return temp; 
}

__v2di construct_v2di_sizet_ptr(size_t key, void* lock){
  
  __v2di temp = {key, (size_t)lock};
  return temp;
}

void xmm_store_metadata(__v2di meta1, __v2di meta2){

  char* addr = (char*) &test[count];

  char* addr2 = addr + 16;
  
  *((__m128*) addr) = meta1;
  *((__m128*) addr2) = meta2;
  //  __builtin_ia32_storedqu128(addr, meta1);  
  //  __builtin_ia32_storedqu128(addr2, meta2);
  
}

void xmm_load_metadata(__v2di * base_bound_addr, __v2di * key_lock_addr){
  
  char* addr = (char*) &test[count];
  char* addr2 = addr + 16;
  *((__v2di *)base_bound_addr) = *((__m128*) addr);
  //__builtin_ia32_loaddqu(addr);
  *((__v2di *)key_lock_addr) =  *((__m128*) addr2);
    //__builtin_ia32_loaddqu(addr2);
  
}

void xmm_check(__v2di meta, void* ptr){
  
  void* base = (void*) meta[0];
  void* bound = (void*)meta[1];

  if (ptr < base || ptr >= bound){
    printf("bounds violation\n");
    abort();    
  }
  
}


int main(int argc, char** argv){


  int arr[100];

  void* arr_base = &arr[0];
  void* arr_bound = &arr[100];
  size_t arr_key = global_key;
  void* arr_lock = global_lock;

  __v2di base_bound = construct_v2di_ptr_ptr(arr_base, arr_bound);
  __v2di key_lock = construct_v2di_sizet_ptr(arr_key, arr_lock);

  
  int *arr_ptr;
  
  int** ptr = &arr_ptr;

  *(ptr) = (int*)arr;

  xmm_store_metadata(base_bound, key_lock);
  
  if(argc < 2){
    printf("insufficient arguments");
    exit(1);
  }

  int loop_c = atoi(argv[1]);
  int i = 0;
  
  for(i = 0; i< loop_c; i++){
    
    int* temp_ptr = *ptr;
    __v2di temp_base_bound;
    __v2di temp_key_lock;
    xmm_load_metadata(&temp_base_bound, &temp_key_lock);

    xmm_check(temp_base_bound, &temp_ptr[i]);
    temp_ptr[i] = i;
  }

  printf("initialized array is:\n");
  for(i=0; i< loop_c;i++){
    printf("%d ", arr[i]);
  }
  
  printf("\n");

  return 0;  
}
