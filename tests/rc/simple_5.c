/*
 * Simple race check
 * Author: dye
 * Date: 10/11/2016
 */
#include "aliascheck.h"
#include "pthread.h"

int g = 0;

void *foo() {
    g = 10;
    RC_ACCESS(1, RC_ALIAS);

    return 0;
}

void dead_func_1() {
    pthread_t thread;
    int ret;
    long t;

    ret = pthread_create(&thread, NULL, &foo, (void *) t);
    if (ret) {
        exit(-1);
    }
}

void dead_func_2() {
    dead_func_1();
}

int main(int argc, char *argv[]) {

    int x = g;
    RC_ACCESS(1, RC_ALIAS);

    printf("%d\n", x);
    return 0;
}
