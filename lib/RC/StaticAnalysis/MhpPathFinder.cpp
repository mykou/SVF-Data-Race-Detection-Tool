/*
 * MhpPathFinder.cpp
 *
 *  Created on: 14/04/2016
 *      Author: dye
 */

#include "MhpPathFinder.h"
#include "Util/ThreadCallGraph.h"

using namespace llvm;
using namespace std;
using namespace analysisUtil;


/*
 * Initialization
 */
void MhpPathFinder::init(MhpAnalysis *mhp) {
    this->mhp = mhp;
}


/*
 * Get the MhpPaths for two given Instructions.
 * @param I1 the input Instruction
 * @param rp1 the input ReachablePoint
 * @param I2 the other input Instruction
 * @param rp2 the other input ReachablePoint
 * @return the output MhpPaths of the two input Instructions
 */
const MhpPathFinder::MhpPaths *MhpPathFinder::getMhpPaths(
        const Instruction *I1,
        const MhpAnalysis::ReachablePoint &rp1,
        const Instruction *I2,
        const MhpAnalysis::ReachablePoint &rp2) {

    // Set status as Init
    setInitStatus();

    // Get the CtxPathss of the two Instructions
    CtxPaths ctxPaths1(rp1);
    CtxPaths ctxPaths2(rp2);

    if (!getCtxPaths(I1, ctxPaths1)) {
        setOutOfBudget();
        return NULL;
    }
    if (!getCtxPaths(I2, ctxPaths2)) {
        setOutOfBudget();
        return NULL;
    }

    // Compute the MhpPaths
    mhpPaths.reset(I1, I2);
    if (!computeMhpPaths(ctxPaths1, ctxPaths2)) {
        setOutOfBudget();
        return NULL;
    }

    return &mhpPaths;
}


/*
 * Get all feasible context paths from a context-root to
 * a given Instruction through a given ReachablePoint.
 * @param I the input Instruction
 * @param ctxPaths the output CtxPaths
 * @return true if solved, or false otherwise
 */
bool MhpPathFinder::getCtxPaths(const Instruction *I, CtxPaths &ctxPaths) const {
    const Instruction *spawnSite = ctxPaths.getReachablePoint().getSpawnSite();
    if (isThreadForkCall(spawnSite)) {
        return getCtxPathsForForkJoin(I, ctxPaths);
    } else {
        assert(isHareParForCall(spawnSite));
        return getCtxPathsForParFor(I, ctxPaths);
    }
}


/*
 * Get all feasible context paths from the "main" Function to
 * a given Instruction through a given ReachablePoint.
 * @param I the input Instruction
 * @param ctxPaths the output CtxPaths
 * @return true if solved, or false otherwise
 */
bool MhpPathFinder::getCtxPathsForForkJoin(const Instruction *I,
        CtxPaths &ctxPaths) const {

    /*
     * This traversal algorithm traverses the call graph in a
     * backward direction (i.e., from callees to callers).
     * Every traversal path consists of two stages, with the ReachablePoint
     * as the stage transition point:
     *
     *   --- Stage 0: when the thread spawn site must happen before
     *                the Instruction being traversed.
     *
     *   --- Stage 1: when the thread spawn site may not happen before
     *                the Instruction being traversed.
     */

    // Get the ReachablePoint information
    const MhpAnalysis::ReachablePoint &rp = ctxPaths.getReachablePoint();
    const llvm::Instruction *spawnSite = rp.getSpawnSite();
    MhpAnalysis::ReachableType t = rp.getReachableType();
    Ctx ctx;

    // Prepare to traverse on the ThreadCallGraph
    ThreadCallGraph *tcg = mhp->getThreadCallGraph();
    OperationCollector *oc = OperationCollector::getInstance();
    set<OperationCollector::CsID> csIds;

    // Get the root Function
    const Module *M = tcg->getModule();
    const Function *root = M->getFunction("main");
    assert(root && "The program must have a main() function!");

    // Setup the initial stage
    const Instruction *splittingSiteForInitStage = NULL;
    if (MhpAnalysis::REACHABLE_TRUNK == t) {
        splittingSiteForInitStage = isIntraprocedurallyTrunkReachable(I, spawnSite);
    }
    else {
        splittingSiteForInitStage = (I == spawnSite) ? I : NULL;
    }

    // The class to represent the status of the analysis.
    struct Status {
        Status(size_t wlIndex, const Instruction *splittingSite) :
                wlIndex(wlIndex), splittingSite(splittingSite) {
        }
        size_t wlIndex;
        const Instruction *splittingSite;
    };

    // The class to represent the work in the worklist.
    struct Work {
        Work(const Instruction *I, const Instruction *splittingSite) :
                I(I), splittingSite(splittingSite) {
        }
        const Instruction *I;
        const Instruction *splittingSite;
    };

    // Prepare for the traversal
    stack<Work> worklist;
    stack<Status> statusStack;
    worklist.push(Work(I, splittingSiteForInitStage));

    // Perform the traversal
    while (!worklist.empty()) {
        Work &w = worklist.top();
        const Instruction *csInst = w.I;
        const Instruction *splittingSite = w.splittingSite;
        OperationCollector::CsID csId = -1;
        if (analysisUtil::isCallSite(csInst)) {
            const Function *callee = getCallee(csInst);
            if (callee && callee->isIntrinsic()) {
                assert(I == csInst);
            } else {
                csId = oc->getCsID(csInst);
            }
        }
        worklist.pop();

        const Function *F = csInst->getParent()->getParent();
        do {
            // Skip the dead Functions.
            if (mhp->isDeadFunction(F))     break;

            // Skip the cyclic case.
            if (ctx.hasCycle(csId))       break;

            // Push the csInst into ctx.
            ctx.push(csId);
            statusStack.push(Status(worklist.size(), splittingSite));

            // Reaching the root Function
            if (F == root) {
                pushCtxIntoCtxPaths(ctx, splittingSite, ctxPaths);
                if (maxCtxPathSize <= ctxPaths.size()) {
                    return false;
                }
                break;
            }

            // Analyze all the call sites that call F.
            // Check the reachability from each call site to spawnSite.
            csIds.clear();
            rcUtil::getAllValidCsIdsInvokingCallee(tcg, F, csIds);
            for (auto it = csIds.begin(), ie = csIds.end();
                    it != ie; ++it) {
                OperationCollector::CsID csId = *it;
                const Instruction *csInst = oc->getCsInst(csId);

                // Handle the trunk reachable case
                if (MhpAnalysis::REACHABLE_TRUNK == t) {
                    // Handle Stage 1 (already passed the splitting site)
                    if (splittingSite) {
                        worklist.push(Work(csInst, splittingSite));
                        continue;
                    }

                    // Check for branch splitting site
                    const Instruction *findSplittingSite =
                            isIntraprocedurallyTrunkReachable(csInst, spawnSite);

                    // Handle Stage 0 -> 1
                    if (findSplittingSite) {
                        worklist.push(Work(csInst, findSplittingSite));
                        continue;
                    }

                    // Handle Stage 0, reachable
                    bool isReachable = mhp->isTrunkReachable(spawnSite, csInst);
                    if (isReachable) {
                        worklist.push(Work(csInst, NULL));
                        continue;
                    }
                }
                // Handle the branch reachable case
                else {
                    // Handle Stage 1
                    if (splittingSite) {
                        worklist.push(Work(csInst, splittingSite));
                        continue;
                    }

                    // Handle Stage 0 -> 1
                    if (csInst == spawnSite) {
                        worklist.push(Work(csInst, csInst));
                        continue;
                    }

                    // Handle Stage 0, reachable
                    bool isReachable = mhp->isBranchReachable(spawnSite, csInst);
                    if (isReachable) {
                        worklist.push(Work(csInst, NULL));
                        continue;
                    }

                }
            }

        } while (false);

        // Pop the ctx and statusStack when appropriate
        while (!statusStack.empty()) {
            if (statusStack.top().wlIndex != worklist.size())   break;
            ctx.pop();
            statusStack.pop();
        }
    }

    // It is suspicious when the ctxPaths is empty.
    if (0 == ctxPaths.size()) {
        outs() << "Something is suspicious: \t";
        outs() << "empty fork-join ctxPaths for:\n";
        outs() << rcUtil::getSourceLoc(I) << "\n";
        outs() << rp.getReachableType() << "  " <<
                rcUtil::getSourceLoc(rp.getSpawnSite()) << "\n\n";
    }

    return true;
}


/*
 * Get all feasible context paths from the relevant parallel-for call site to
 * a given Instruction through a given ReachablePoint.
 * @param I the input Instruction
 * @param ctxPaths the output CtxPaths
 * @return true if solved, or false otherwise
 */
bool MhpPathFinder::getCtxPathsForParFor(const Instruction *I,
        CtxPaths &ctxPaths) const {

    // Get the ReachablePoint information
    const MhpAnalysis::ReachablePoint &rp = ctxPaths.getReachablePoint();
    const llvm::Instruction *parForSite = rp.getSpawnSite();
    Ctx ctx;

    // Prepare to traverse on the ThreadCallGraph
    ThreadCallGraph *tcg = mhp->getThreadCallGraph();
    set<OperationCollector::CsID> csIds;
    OperationCollector *oc = OperationCollector::getInstance();


    // Prepare for the traversal
    stack<const Instruction*> worklist;
    stack<size_t> statusIndex;
    worklist.push(I);

    // Perform the traversal
    while (!worklist.empty()) {
        const Instruction *csInst = worklist.top();
        OperationCollector::CsID csId = -1;
        if (analysisUtil::isCallSite(csInst)) {
            const Function *callee = getCallee(csInst);
            if (callee && callee->isIntrinsic()) {
                assert(I == csInst);
            } else {
                csId = oc->getCsID(csInst);
            }
        }
        worklist.pop();

        const Function *F = csInst->getParent()->getParent();
        do {
            // Skip the dead Functions.
            if (mhp->isDeadFunction(F))     break;

            // Skip the cyclic case.
            if (ctx.hasCycle(csId))       break;

            // Push the csInst into ctx.
            ctx.push(csId);
            statusIndex.push(worklist.size());

            // Reaching parForSite
            if (csInst == parForSite) {
                pushCtxIntoCtxPaths(ctx, NULL, ctxPaths);
                if (maxCtxPathSize <= ctxPaths.size()) {
                    return false;
                }
                break;
            }

            // Analyze all the call sites that call F.
            csIds.clear();
            rcUtil::getAllValidCsIdsInvokingCallee(tcg, F, csIds);
            for (auto it = csIds.begin(), ie = csIds.end();
                    it != ie; ++it) {
                // Check the reachability from csInst to spawnSite.
                OperationCollector::CsID csId = *it;
                const Instruction *csInst = oc->getCsInst(csId);
                worklist.push(csInst);
            }

        } while (false);

        // Pop the ctx and statusStack when appropriate
        while (!statusIndex.empty()) {
            if (statusIndex.top() != worklist.size())   break;
            ctx.pop();
            statusIndex.pop();
        }
    }

    // It is suspicious when the ctxPaths is empty.
    if (0 == ctxPaths.size()) {
        outs() << "Something is suspicious: \t";
        outs() << "empty parallel-for ctxPaths for:\n";
        outs() << rcUtil::getSourceLoc(I) << "\n";
        outs() << rp.getReachableType() << "  " <<
                rcUtil::getSourceLoc(rp.getSpawnSite()) << "\n\n";
    }

    return true;
}


/*
 * Compute the MhpPaths for two given CtxPaths. The result is recorded
 * into this->mhpPaths.
 * @param ctxPaths1 the input CtxPath
 * @param ctxPaths2 the other input CtxPath
 * @return true if solved within budget, or false otherwise
 */
bool MhpPathFinder::computeMhpPaths(CtxPaths &ctxPaths1, CtxPaths &ctxPaths2) {
    // Check if the number of paths exceeds the budget
    if (maxPathPairCount <= ctxPaths1.size() * ctxPaths2.size()) {
        setOutOfBudget();
        return false;
    }

    // Get the ReachableType from CtxPaths
    auto t = ctxPaths1.getReachablePoint().getReachableType();

    // Reset the MhpPaths and form the pairs
    for (auto it1 = ctxPaths1.begin(), ie1 = ctxPaths1.end();
            it1 != ie1; ++it1) {
        for (auto it2 = ctxPaths2.begin(), ie2 = ctxPaths2.end();
                it2 != ie2; ++it2) {
            // Check the possible infeasibility for the fork-join case
            bool isInfeasible = false;
            if (it1->second) {
                assert(it2->second);
                // ctxPaths1 is branch-reachable
                if (MhpAnalysis::REACHABLE_BRANCH == t) {
                    isInfeasible = !isFeasibleBranchReachableCtxPath(it1->first,
                            it2->second);
                }
                // ctxPaths2 is branch-reachable
                else {
                    isInfeasible = !isFeasibleBranchReachableCtxPath(it2->first,
                            it1->second);
                }
            }

            // Add the pair if not infeasible
            if (!isInfeasible) {
                mhpPaths.addCtxPair(it1->first, it2->first);
            }
        }
    }

    return true;
}



/*
 * Push a Ctx into a CtxPaths.
 * Note that the following two rules should be applied when
 * the Ctx is pushed into the CtxPaths:
 *   (1) the element order of the input Ctx should be reversed;
 *   (2) the first element (i.e., the interested Instruction, which
 *       is not a part of a calling context) should be ignored.
 * @param ctx the input ctx
 * @param splittingSite the thread splitting site
 * @param ctxPaths the output CtxPaths
 */
void MhpPathFinder::pushCtxIntoCtxPaths(const Ctx &ctx,
        const Instruction *splittingSite, CtxPaths &ctxPaths) const {

    // Create a new slot in the ctxPaths
    auto iter = ctxPaths.newCtxPath();

    // Copy the ctx into the ctxPaths in a reversed direction
    Ctx &dst = iter->first;
    dst.reserve(ctx.size());
    auto it = ctx.rbegin();
    auto ie = ctx.rend();
    --ie;
    for (; it != ie; ++it) {
        dst.push(*it);
    }

    // Set the splitting site
    iter->second = splittingSite;
}



/*
  * Check if a given Instruction is trunk reachable from a
  * thread spawn site (or its side effect site) intra-procedurally.
  * @param I the input Instruction
  * @param spawnSite the input thread spawn site
  * @return the splitting point if succeeds
 */
const Instruction *MhpPathFinder::isIntraprocedurallyTrunkReachable(
        const Instruction *I, const Instruction *spawnSite) const {

    // Find all affected sites of spawnSite.
    const Function *F = I->getParent()->getParent();
    const MhpSummary *summary = mhp->getSccSummary(F);
    if (!summary)   return NULL;

    const MhpSummary::InstSet *affectedSites =
            summary->getAffectedSpawnSites(spawnSite);

    // Check if any affected site is forward reachable to I.
    if (!affectedSites)     return NULL;

    CodeSet reachableCode;
    getBackwardReachableCode(I, reachableCode);
    for (auto it = affectedSites->begin(), ie = affectedSites->end();
            it != ie; ++it) {
        const Instruction *affectedSite = *it;
        if (reachableCode.covers(affectedSite)) {
            return affectedSite;
        }
    }

    return NULL;
}



/*
 * Get the intra-procedural backward reachable code from
 * a given Instruction.
 * @param root the input Instruction
 * @param reachableCode the output CodeSet
 */
void MhpPathFinder::getBackwardReachableCode(const Instruction *root,
        CodeSet &reachableCode) const {

    // Identify the backward reachable Instructions in the same BasicBlock.
    const BasicBlock *rootBb = root->getParent();
    const Instruction *I = &rootBb->front();
    while (I != root) {
        reachableCode.insert(I);
        I = I->getNextNode();
    }

    // Prepare BasicBlock reachability analysis
    stack<const BasicBlock*> worklist;
    SmallSet<const BasicBlock*, 16> visited;
    for (auto it = pred_begin(rootBb), ie = pred_end(rootBb);
            it != ie; ++it) {
        const BasicBlock *predBb = *it;
        worklist.push(predBb);
    }

    // Perform backward BasicBlock reachability analysis.
    while (!worklist.empty()) {
        // Get the BasicBlock from the worklist.
        const BasicBlock *bb = worklist.top();
        worklist.pop();

        // Check if we have already visited this BasicBlock
        if (visited.count(bb))   continue;
        visited.insert(bb);

        // Insert the BasicBlock into reachable code
        reachableCode.insert(bb);

        // Handle predecessors
        for (auto it = pred_begin(bb), ie = pred_end(bb);
                it != ie; ++it) {
            const BasicBlock *predBb = *it;
            if (visited.count(predBb))  continue;
            worklist.push(predBb);
        }
    }
}



/*
 * Check if a Ctx path is feasible for a given thread-splitting site.
 * @param branchPath the Ctx path from a branch-reachable Instruction
 * @param splittingSite the corresponding thread-splitting site
 * @return the feasibility
 */
bool MhpPathFinder::isFeasibleBranchReachableCtxPath(const Ctx &branchPath,
        const Instruction *splittingSite) const {

    // Get the thread-splitting Function
    const Function *splittingFunc = splittingSite->getParent()->getParent();

    OperationCollector *oc = OperationCollector::getInstance();

    // Iterate through the Ctx (from callee to caller).
    // For a feasible Ctx path, when it comes up into a thread-splitting Function,
    // it should be passing through the thread-splitting site.
    auto it = branchPath.begin();
    auto ie = branchPath.end();
    while (it != ie) {
        OperationCollector::CsID csId = *it;
        const Instruction *I = oc->getCsInst(csId);
        const Function *F = I->getParent()->getParent();
        if (F == splittingFunc) {
            if (I == splittingSite) {
                return true;
            } else {
                return false;
            }
        }
        ++it;
    }

    return true;
}

