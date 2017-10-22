#include<stdio.h>
/* If the structure has just two fields then the frontend coerces the
   structure into two arguments with i64 and i64* preserving types. */

struct temp{
  size_t dummy;
  size_t dummy2;
  size_t ptr;
};

struct byval_temp{
  size_t inta;
  int arr[10];
  size_t intb;
  struct temp test;
};

struct byval_temp t1;

__attribute__((__noinline__)) size_t  foo (struct byval_temp test, int c , int d){
  
  size_t test_val = test.inta + test.arr[0] + (test.test.ptr);
  return test_val;
}

int main(int argc, char** argv){

  size_t count = 10;
  size_t result;

  printf("Inputting the values\n");

  if (argc < 3)
    exit(1);

  size_t temp1 = atoi(argv[1]);
  size_t temp2 = atoi(argv[2]);

  t1.inta = temp1;
  t1.arr[0] = temp2;
  t1.test.ptr = count;
  
  result = foo(t1, argc, argc);
  
  printf("the result is %zd\n", result);
  return 0;

}
