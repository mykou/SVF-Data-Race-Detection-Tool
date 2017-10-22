/*
 * Reorder.h
 *
 *  Created on: May 14, 2016
 *      Author: Peng Di
 */

#ifndef Reorder_H_
#define Reorder_H_

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "rc"
#endif

/*!
 * Reorder pairs which are stored in DCI_pairs.inc and create the new file
 * DCI_final_pairs.inc to record new order. The pair with the higher appearing
 * frequency will has higher checking priority.
 */
struct Reorder {

    /*!
     * Run reorder
     */
    static bool run();
};

#endif /* Reorder_H_ */
