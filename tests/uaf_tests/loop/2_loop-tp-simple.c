#include "../uaf.h"

int main() {
  int* p = (int*)malloc(1);
  for (int i = 0; i < 100; i++) {
     uaf_use(p);
     free(p);
  }
  UAF_TP(7,6);
  return 0;
}
