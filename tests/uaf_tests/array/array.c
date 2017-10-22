#include"../uaf.h"

struct MyStruct {
        int * f1;
        int * f2;
};

int main() {
        struct MyStruct s[2];

        s[0].f1 = (int*)malloc(1);
        s[1].f1 = (int*)malloc(1);

        // Different fields of different elements in a 
        // certain array are treated as different objects.
	free(s[0].f1);
	uaf_use(s[1].f1);
        //s[0].f1 and s[1].f1 are aliases.
	UAF_FP(16,17);
        return 0;
}
