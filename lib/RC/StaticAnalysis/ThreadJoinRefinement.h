/*
 * ThreadJoinRefinement.h
 *
 *  Created on: 03/02/2016
 *      Author: dye
 */

#ifndef THREADJOINREFINEMENT_H_
#define THREADJOINREFINEMENT_H_

#include "llvm/IR/Instruction.h"

class MhpAnalysis;

/*!
 * \brief The refinement analysis is to handle thread join more precisely.
 *
 * The coarse-grain MhpAnalysis performs thread join analysis in a
 * conservative manner, where the branch join (i.e., threads get joined at
 * a thread other than their spawner thread.) is not handled.
 * This refinement analysis can identify the branch joins, and refine
 * the Instructions that cannot actually happen in parallel in case of
 * branch join.
 *
 * For example, T1 spawns two different threads T2 and T3. After a while,
 * T2 joins T3 and continues executing its following code C.
 * MhpAnalysis conservatively reports T2's code and C may happen in parallel,
 * while ThreadJoinRefinement can identify they cannot happen in parallel.
 */
class ThreadJoinRefinement {
public:
    /// Initialization
    void init(MhpAnalysis *mhp);

    /// Check if two Instructions cannot happen in parallel due to branch join.
    bool branchJoinRefined(const llvm::Instruction *I1,
            const llvm::Instruction *I2) const;

private:
    MhpAnalysis *mhp;
};


#endif /* THREADJOINREFINEMENT_H_ */
