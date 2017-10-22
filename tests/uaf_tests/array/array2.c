#include"../uaf.h"


void allocWrapper(int* a[], int index) {
	a[index] = (int*)malloc(sizeof(int));
}

void freeWrapper(int* a[], int index) {
	free(a[index]);
}

void useWrapper1(int* a[], int index) {
	uaf_use(a[index]);
}

void useWrapper2(int* a[], int index) {
	uaf_use(a[index]);
}

int main() {
	int* a[2];
	allocWrapper(a,0);
	allocWrapper(a,1);
	useWrapper1(a,0);
	freeWrapper(a,0);
	useWrapper1(a,0);
	useWrapper2(a,1);
	freeWrapper(a,1);

	UAF_TP(9,13);
	UAF_FP(9,17);
        return 0;
}
