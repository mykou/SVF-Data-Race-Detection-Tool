/*
 * PathCorrelationAnalysis.cpp
 *
 *  Created on: 29/02/2016
 *      Author: dye
 */


#include "PathCorrelationAnalysis.h"
#include "LocksetAnalysis.h"

using namespace rcUtil;
using namespace llvm;
using namespace std;


/*
 * Compute all valid conditions from a given Instruction
 * along with its holding status.
 * Return if the result is a valid SimpleCmpGuard.
 */
bool PathCorrelationAnaysis::SimpleCmpGuard::recalculate(const Instruction *I,
        bool hold) {

    this->I = I;
    if (!I)     return false;

    // Handle the CmpInst case
    if (const CmpInst *CI = dyn_cast<CmpInst>(I)) {
        const Value *opndLhs = I->getOperand(0);
        const Value *opndRhs = I->getOperand(1);

        // Strip all casts
        opndLhs = stripAllCasts(opndLhs);
        opndRhs = stripAllCasts(opndRhs);
        if (!opndLhs || !opndRhs)   return false;

        // Set order
        if (opndLhs < opndRhs) {
            pred = CI->getPredicate();
        } else {
            const Value *tmp = opndLhs;
            opndLhs = opndRhs;
            opndRhs = tmp;
            pred = getMirrorPredicate(CI->getPredicate());
        }

        // Determine the pred according to the condition's hold status
        if (!hold)  pred = getInversePredicate(pred);

        return true;
    }

    return false;
}


/*
 * Dump information of this  SimpleCmpGuard
 */
void PathCorrelationAnaysis::SimpleCmpGuard::print() const {
    outs() << " --- SimpleCmpGuard ---\n";
    outs() << rcUtil::getSourceLoc(I) << "\n";
    outs() << "\tExpr: " << lhs->getName()
            << " " << rcUtil::getPredicateString(pred) << " "
            << rhs->getName() << "\n";
}


/*
 * Initialization
 */
void PathCorrelationAnaysis::init(LocksetAnalysis *lsa) {
    this->lsa = lsa;

    // Initialize guardExtractor
    ThreadCallGraph *tcg = lsa->getThreadCallGraph();
    guardExtractor.init(tcg, false);
}


/*
 * Determine if the guards of bb1 subsume the guards of bb2.
 * i.e., in first-order logic, Guards(bb1) -> Guards(bb2).
 */
bool PathCorrelationAnaysis::subsumeOnGuards(const BasicBlock *bb1,
        const BasicBlock *bb2) {
    // If there are two identical BasicBlocks, then bb1 must subsume bb2.
    if (bb1 == bb2)     return true;

    // Extract the guards
    const Guards &guards1 = getGuards(bb1);
    const Guards &guards2 = getGuards(bb2);

    // FIXME: Check if there is any invalid (not recognized) guard in guards2.
    // If so, then we conservatively return false.

    // Check if guards1 contains every guard in guards2.
    for (int i2 = 0, e2 = guards2.size(); i2 != e2; ++i2) {
        const Guard &g2 = guards2[i2];
        bool matched = false;
        for (int i1 = 0, e1 = guards1.size(); i1 != e1; ++i1) {

            if (g2 == guards1[i1]) {
                matched = true;
                break;
            }
        }
        if (!matched)   return false;
    }

    return true;
}


