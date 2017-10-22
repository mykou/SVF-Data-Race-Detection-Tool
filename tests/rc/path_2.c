/*
 * Simple race check
 * Author: dye
 * Date: 05/01/2016
 */
#include "aliascheck.h"
#include "pthread.h"


int attr_init_done = 0;

int g_spawn_on_demand = 0;


void *foo(void *args) {

    if (g_spawn_on_demand) {

        int flag = !attr_init_done;
        RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);

        if (flag) {
            // do sth here

            attr_init_done = 1;
            RC_ACCESS(1, RC_ALIAS | RC_MHP | RC_RACE);
        }
    }

}


void trick() {
    g_spawn_on_demand = 1;
}


int main(int argc, char *argv[]) {

    g_spawn_on_demand = (argv[1][0] == 'x');

    if (g_spawn_on_demand) {
        foo(NULL);

    } else {
        // Let's play a trick before spawning threads
        trick();

        pthread_t thread[10];
        for (int i = 0; i < 10; ++i) {
            pthread_create(thread + i, NULL, foo, (void *) NULL);
        }
    }

    return 0;
}
