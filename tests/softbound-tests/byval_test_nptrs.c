#include<stdio.h>
/* If the structure has just two fields then the frontend coerces the
   structure into two arguments with i64 and i64* preserving types. */

struct byval_temp{
  size_t inta;
  size_t intb;
  size_t dummy;
  size_t dummy2;
};

__attribute__((__noinline__)) size_t  foo (struct byval_temp test, int c , int d){
  
  size_t test_val = test.inta + test.intb;
  return test_val;
}

int main(int argc, char** argv){

  struct byval_temp t1;
  size_t count = 10;
  size_t result;

  printf("Inputting the values\n");

  if (argc < 3)
    exit(1);

  size_t temp1 = atoi(argv[1]);
  size_t temp2 = atoi(argv[2]);

  t1.inta = temp1;
  t1.intb = temp2;
  
  result = foo(t1, argc, argc);
  
  printf("the result is %zd\n", result);
  return 0;

}
