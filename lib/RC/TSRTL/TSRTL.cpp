/*
 * RFRTL.cpp
 *
 *  Created on: 22 Jun 2016
 *      Author: pengd
 */

#include "TSRTL.h"
#include "TSInterceptors.h"

#include <iostream>


BlockingMutex print_mtx;
namespace __ts {

/// Stall breaker for thread scheduling
StallBreaker *stallbreaker;

/*
 * Initialize function
 */
void Initialize() {
    // The following code force the initialization of std::io_base
    std::ios_base::Init dummyInitializer;

    // Thread safe because done before all threads exist.
    static bool is_initialized = false;
    if (is_initialized)
        return;
    is_initialized = true;

    stallbreaker = new StallBreaker();

    atomic_store(&stallbreaker->total_thread, 0, memory_order_relaxed);
    stallbreaker->postponed_thread.clear();
    stallbreaker->enabled_thread.clear();
    stallbreaker->alive_thread.clear();
    stallbreaker->thrToWaitTime.clear();

    FILE *pfile;
    if ((pfile = fopen("TSRTL_config.inc", "r")) == NULL) {
        if ((pfile = fopen("../TSRTL_config.inc", "r")) == NULL) {
            std::cout << "Can't open TSRTL_config.inc, using default configuration\n";
        }
    } else {
        if (EOF == fscanf(pfile, "%d", &stallbreaker->waitTime) ||
                EOF == fscanf(pfile, "%d", &stallbreaker->maxWaitNumber)) {
            std::cout << "Error reading TSRTL_config.inc, using default configuration\n";
        }
    }

    InitializeInterceptors();

}

/*
 * Finalize function. This function is used for printing debug.
 */
void Finalize() {
}

}

