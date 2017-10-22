#include "../uaf.h"

int main() {
  for (int i = 0; i < 100; i++) {
     int* p = (int*)malloc(1);
     uaf_use(p);
     free(p);
  }
  UAF_TN(7,6);
  return 0;
}
