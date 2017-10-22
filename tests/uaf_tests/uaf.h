#include<stdio.h>
#include<stdlib.h>

int glob;

void UAF_TP(int lineFree, int lineUse) {
  printf(" ");
}

void UAF_FP(int lineFree, int lineUse) {
  printf(" ");
}

void UAF_TN(int lineFree, int lineUse) {
  printf(" ");
}

void UAF_FN(int lineFree, int lineUse) {
  printf(" ");
}

void uaf_use(void* x) {
  printf(" ");
}

int cond() {
  return glob > 0;
}
