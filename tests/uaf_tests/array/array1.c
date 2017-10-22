#include"../uaf.h"


int main() {
	int* a[2];
	a[0] = (int*)malloc(sizeof(int));
	a[1] = (int*)malloc(sizeof(int));
	uaf_use(a[0]);
	free(a[0]);
	uaf_use(a[0]);
	uaf_use(a[1]);
	free(a[1]);

	UAF_TP(9,10);
	UAF_FP(9,11);
        return 0;
}
