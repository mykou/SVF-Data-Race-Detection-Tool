#include "../uaf.h"

int main() {
  int* p = (int*)malloc(1);
  if (cond()) {
    free(p);
    if (!cond()) {
      uaf_use(p);
    }
  }
  UAF_TP(6,8);
  return 0;
}
