#include "../uaf.h"
int k, n = 5, m = 3;
struct LL {
  int val;
  struct LL* nxt;
};

int main() {
  struct LL* head = (struct LL*)malloc(sizeof(struct LL));
  struct LL* pre = head;
  for (int i = 0; i < n; i++) {
    struct LL* node = (struct LL*)malloc(sizeof(struct LL));
    pre->nxt = node;
    node->nxt = 0;
    pre = node;
  }
  struct LL* succ = head->nxt;
  free(head);
  k = head->val;
  head = succ;
  for (int i = 0; i < m; i++) {
    struct LL* succ = head->nxt;
    free(head);
    k = head->val;
    head = succ;
    k = head->val;
  }
  UAF_TP(18,19);
  UAF_FN(23,24);
  UAF_TN(23,26);
  return 0;
}
