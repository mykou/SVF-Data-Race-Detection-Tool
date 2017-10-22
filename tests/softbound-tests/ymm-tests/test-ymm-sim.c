#include<stdio.h>
#include<immintrin.h>

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

int count = 1;
struct metadata test[100];

void ymm_store_metadata(__v4di);
__v4di ymm_load_metadata();

__v4di construct_v4di(void* base, void* bound, size_t key, void* lock){
  
  __v4di temp = {(size_t)base, (size_t)bound, key, (size_t)lock};
  return temp;
}

void ymm_store_metadata(__v4di meta){
  
  void* temp = (void*)&test[count];
  __m256* addr = (__m256*) temp;

  //  printf("addr is %zx, test[count]_addr is %zx\n", addr, &test[count]);

  __builtin_ia32_storedqu256((char*)addr, meta);  

  //*((__m256*) addr) = meta;
  
}

__v4di ymm_load_metadata(){

  void* temp = (void*)&test[count];
  
  __m256* addr = (__m256*) temp;
  //  return *((__m256*)addr);
  return (__v4di)__builtin_ia32_loaddqu256((char*)addr);
}

void ymm_check(__v4di meta, void* ptr){
  
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

  __v4di ptr_meta = construct_v4di(arr_base, arr_bound, arr_key, arr_lock);
  
  int *arr_ptr;
  
  int** ptr = &arr_ptr;

  *(ptr) = (int*)arr;

  printf("before storing metadata\n");
  ymm_store_metadata(ptr_meta);
  
  if(argc < 2){
    printf("insufficient arguments");
    exit(1);
  }

  int loop_c = atoi(argv[1]);

  printf("after processing args\n");
  int i = 0;
  
  for(i = 0; i< loop_c; i++){
    
    int* temp_ptr = *ptr;
    __v4di temp_meta = ymm_load_metadata();

    ymm_check(temp_meta, &temp_ptr[i]);
    temp_ptr[i] = i;
  }

  printf("initialized array is:\n");
  for(i=0; i< loop_c;i++){
    printf("%d ", arr[i]);
  }
  
  printf("\n");

  return 0;  
}
