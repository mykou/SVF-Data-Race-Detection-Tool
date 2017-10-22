/*
 * DCIRTL.cpp
 *
 *  Created on: 4 Oct 2016
 *      Author: pengd
 */

#include "DCIRTL.h"
#include "DCIInterceptors.h"


namespace __dci {

/// DCI information
DCIInfo *dci;

/*
 * Initialize function
 */
void Initialize(unsigned instr_num) {
    // The following code force the initialization of std::io_base
    std::ios_base::Init dummyInitializer;

    // Thread safe because done before all threads exist.
    static bool is_initialized = false;
    if (is_initialized)
        return;
    is_initialized = true;

    InitializePairInfo();
    InitializeInterceptors();

}

/*
 * Finalize function
 */
void Finalize() {
    CollectPairs();
}

/*
 * Initialize pair information. Store all RC pairs into instToGroup
 */
void InitializePairInfo() {

    dci = new DCIInfo();
    dci->pairs.clear();
    dci->instToGroup.clear();

    FILE *pfile;
    if((pfile = fopen("RC.pairs", "r")) == NULL) {
        if((pfile = fopen("../RC.pairs", "r")) == NULL) {
            printf("Can't open RC.pairs\n");
        }
    }
    if (pfile!=NULL) {
        unsigned a, b;
        while(!feof(pfile)) {
            if (EOF == fscanf(pfile, "%u %u\n", &a, &b)) {
                printf("Error reading RC.pairs\n");
            }
            dci->instToGroup[a].set(b);
            dci->instToGroup[b].set(a);
        }
        fclose(pfile);
    }
}

/*
 * Collect refined pairs and store result into DCI.pairs
 */
void CollectPairs() {
    FILE *pfile;
    if((pfile = fopen("DCI.pairs", "wb")) == NULL) {
        printf("Can't open DCI.pairs\n");
    }
    if (pfile!=NULL) {
        unsigned a,b;
        for (Pairs::iterator it = dci->pairs.begin(), ei = dci->pairs.end(); it != ei; it++) {

            Pair pair = *it;
            dci->getPair(a,b,pair);
            fprintf(pfile, "%u %u\n", a, b);
        }
        fclose(pfile);
    }
}

}

