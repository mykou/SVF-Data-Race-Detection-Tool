/*
 * Simple alias check
 * Author: dye
 * Date: 12/11/2015
 */
#include "aliascheck.h"
#include "pthread.h"

/*
 * Example from goblint_knot.
 */

int attr_init_done = 0;

int g_spawn_on_demand = 0;


void *foo(void *args) {

    if (g_spawn_on_demand) {

        int flag = !attr_init_done;
        RC_ACCESS(1, RC_ALIAS);

        if (flag) {
            // do sth here

            attr_init_done = 1;
            RC_ACCESS(1, RC_ALIAS);
        }
    }

}



int main(int argc, char *argv[]) {

    g_spawn_on_demand = (argv[1][0] == 'x');

    if (g_spawn_on_demand) {
        foo(NULL);

    } else {
        pthread_t thread[10];
        for (int i = 0; i < 10; ++i) {
            pthread_create(thread + i, NULL, foo, (void *) NULL);
        }
    }

    return 0;
}
