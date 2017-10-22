#include "../uaf.h"

struct LL {
  int val;
  struct LL* nxt;
};

int main() {
  struct LL* head = (struct LL*)malloc(sizeof(struct LL));
  struct LL* pre = head;
  for (int i = 0; i < 5; i++) {
    struct LL* node = (struct LL*)malloc(sizeof(struct LL));
    pre->nxt = node;
    node->nxt = 0;
  }
  for (int i = 0; i < 3; i++) {
    struct LL* succ = head->nxt;
    free(head);//free an element in linked list
    head = succ;
  }
  uaf_use(head->val);//use an element in linked list
  UAF_FP(18,21);
  return 0;
}
