#include"../uaf.h"

int main() {
  int* p = malloc(1);
  free(p);
  uaf_use(p);
  UAF_TP(5,6);
  return 0;
}
