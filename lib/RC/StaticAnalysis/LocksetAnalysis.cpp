/*
 * LocksetAnalysis.cpp
 *
 *  Created on: 08/05/2015
 *      Author: dye
 */

#include "LocksetAnalysis.h"
#include "Util/ThreadCallGraph.h"
#include <llvm/IR/InstIterator.h>

using namespace llvm;
using namespace std;

/*
 * Initialization.
 */
void IntraproceduralLocksetAnalyzer::init() {
    cg = lsa->getCallGraph();
    pta = lsa->getPTA();
}

/*
 * Summarize the memory accesses of Function "F" and its callees recursively.
 */
void IntraproceduralLocksetAnalyzer::bottomUpSummarize(const Function *F,
        LocksetSummary &summary) {
    for (const_inst_iterator it = inst_begin(F), ie = inst_end(F); it != ie;
            ++it) {
        const Instruction *I = const_cast<Instruction*>(&*it);
        if (rcUtil::isMemoryAccess(I)) {
            summary.addMemoryAccess(I);
        }
    }
}

/*
 * Intra-procedural lockset analysis for function F.
 * This analysis is very lightweight race detection,
 * where the lock/unlock side effects across functions are not considered.
 * This algorithm is not sound when a callee is called by multiple callers,
 * some of which have the call sites guarded but others don't.
 */
void IntraproceduralLocksetAnalyzer::run(const Function *F) {
    // Identify lock acquire operations.
    InstSet lockSites;
    for (const_inst_iterator it = inst_begin(F), ie = inst_end(F); it != ie;
            ++it) {
        const Instruction *I = const_cast<Instruction*>(&*it);
        if (analysisUtil::isLockAquireCall(I)) {
            lockSites.insert(I);
        }
    }

    // Return if there isn't any lock operations.
    if (!lockSites.size())    return;

    // Identify the protected Instructions.
    DominatorTree &dt = lsa->getDt(F);
    for (InstSet::const_iterator it = lockSites.begin(), ie = lockSites.end();
            it != ie; ++it) {
        const CallInst *lockSite = dyn_cast<CallInst>(*it);
        assert(lockSite && "Lock acquire instruction must be CallInst");
        const Value *acquiredLockPtr = lockSite->getArgOperand(0);

        // Perform forward traversal
        InstSet visited;
        stack<const Instruction *> worklist;
        worklist.push(*it);
        while (!worklist.empty()) {
            const Instruction *I = worklist.top();
            worklist.pop();
            // Skip the visited Instructions.
            if (visited.count(I))   continue;
            visited.insert(I);

            // Record the result if I is an interested memory access operation.
            if (rcUtil::isMemoryAccess(I)) {
                LocksetSummary::getOrAddProtectingLocks(I).insert(acquiredLockPtr);
            }

            // For function calls, record the callee's summarized memory access operations.
            if (cg->hasCallGraphEdge(I)) {
                for (PTACallGraph::CallGraphEdgeSet::const_iterator it =
                        cg->getCallEdgeBegin(I), ie = cg->getCallEdgeEnd(I);
                        it != ie; ++it) {
                    const PTACallGraphEdge *edge = *it;
                    const Function *callee = edge->getDstNode()->getFunction();
                    if (callee->isDeclaration())    continue;
                    InstSet memoryAccesses;
                    lsa->getMemoryAccesses(callee, memoryAccesses);
                    for (InstSet::iterator it = memoryAccesses.begin(), ie =
                            memoryAccesses.end(); it != ie; ++it) {
                        LocksetSummary::getOrAddProtectingLocks(*it).insert(
                                acquiredLockPtr);
                    }
                }
            }

            // Stop traversing if I is a corresponding unlock operation.
            if (analysisUtil::isLockReleaseCall(I)) {
                const CallInst *releaseInst = dyn_cast<CallInst>(I);
                assert(releaseInst && "Lock release instruction must be CallInst");
                const Value *releasedLockPtr = releaseInst->getArgOperand(0);
                if (rcUtil::alias(acquiredLockPtr, releasedLockPtr, pta))   continue;
            }

            // Push the following Instruction(s) of I into the worklist,
            // if they are dominated by "lockSite".
            if (!I->isTerminator()) {
                worklist.push(I->getNextNode());
            } else {
                const BasicBlock *BB = I->getParent();
                // Visit all successors of BB in the CFG
                for (succ_const_iterator it = succ_begin(BB), ie = succ_end(BB);
                        it != ie; ++it) {
                    const BasicBlock *succBb = *it;
                    if (!dt.dominates(lockSite, succBb))  continue;
                    const Instruction *firstInst = succBb->getFirstNonPHI();
                    if (visited.count(firstInst))   continue;
                    worklist.push(firstInst);
                }
            }
        }
    }
}


/*
 * Dump the information of every protected Instruction with its lockset.
 */
void IntraproceduralLocksetAnalyzer::dump() const {
    for (auto it = inst2LocksetMap.begin(), ie = inst2LocksetMap.end(); it != ie;
            ++it) {
        const Instruction *I = it->first;
        const LockSet &lockset = it->second;
        outs() << rcUtil::getSourceLoc(I) << "  lockset:\n";
        for (LockSet::const_iterator it = lockset.begin(), ie = lockset.end();
                it != ie; ++it) {
            outs() << "\t" << **it << "\n";
        }
        outs() << "\n";
    }
}


/*
 * Initialization
 * @param cg The callgraph.
 * @param pta the pointer analysis
 * @param useIntraproceduralAnalysis whether to perform the unsound
 * but more efficient intra-procedural lockset analysis instead of
 * the inter-procedural one
 */
void LocksetAnalysis::init(PTACallGraph *cg, ThreadCallGraph *tcg,
        BVDataPTAImpl *pta,
        bool useIntraproceduralAnalysis = false) {
    this->tcg = tcg;

    SUPER::init(cg, tcg, pta);

    // Initialize pathCorrelationAnaysis
    pca.init(this);

    // Initialize intraproceduralLocksetAnalyzer if it required.
    if (useIntraproceduralAnalysis) {
        intraproceduralLocksetAnalyzer = new IntraproceduralLocksetAnalyzer(this);
        intraproceduralLocksetAnalyzer->init();
    }
}

/*
 * \brief Perform the lockset analysis.
 *
 * It contains two phases:
 * (1) a bottom-up phase, and (2) a top-down phase.
 */
void LocksetAnalysis::analyze() {
    // Perform bottom-up and top-down analysis by using visitFunction()
    SUPER::analyze(SUPER::AO_BOTTOM_UP);
    SUPER::analyze(SUPER::AO_TOP_DOWN);
}


/*
 * Main Function visitor for inter-procedural analysis.
 */
void LocksetAnalysis::visitFunction(const Function *F) {
    // Call the corresponding function visitor as per the analysis order.
    switch(getCurrentAnalysisOrder()) {
    case SUPER::AO_BOTTOM_UP: {
        bottomUpFunctionVisitor(F);
        break;
    }
    case SUPER::AO_TOP_DOWN: {
        topDownFunctionVisitor(F);
        break;
    }
    default:
        break;
    }
}


/*
 * Summarize the memory access operations for each Function in a bottom-up manner.
 */
void LocksetAnalysis::bottomUpFunctionVisitor(const Function *F) {
    LocksetSummary &summary = getOrAddSccSummary(F);
    // Intra-procedural lockset analysis
    if (intraproceduralLocksetAnalyzer) {
        intraproceduralLocksetAnalyzer->bottomUpSummarize(F, summary);
    }
    // Inter-procedural lockset analysis
    else {
        BottomUpAnalyzer bua(this, F);
        bua.run();
        const ValSet &mustAcquireLockset = bua.getMustAcquireLockset();
        const ValSet &mayReleaseLockset = bua.getMayReleaseLockset();
        summary.addAcquire(mustAcquireLockset.begin(), mustAcquireLockset.end());
        summary.addRelease(mayReleaseLockset.begin(), mayReleaseLockset.end());
    }
}


/*
 * Apply the lock/unlock effects to the accesses for each
 * Function in a top-down manner.
 */
void LocksetAnalysis::topDownFunctionVisitor(const Function *F) {
    if (intraproceduralLocksetAnalyzer) {
        intraproceduralLocksetAnalyzer->run(F);
    } else {
        TopDownAnalyzer tda(this, F);
        tda.run();
    }
}


/*
 * Identify all unlock sites that may release any lock
 * in lockset in Function "this->F".
 * @param lockset the input lockset
 * @param correspondingUnlockSites the output that records the
 * unlock sites that may unlock the input lockset
 */
void LocksetAnalysis::FunctionAnalyzerBase::identifyCorrespondingUnlockSites(
        const LockSet &lockset, InstSet &correspondingUnlockSites) const {
    for (Inst2LocksetMap::const_iterator it = unlockSites.begin(),
            ie = unlockSites.end(); it != ie; ++it) {
        const Instruction *unlockSite = it->first;
        const LockSet &unlockset = it->second;
        if (lsa->hasCommonLock(lockset, unlockset)) {
            correspondingUnlockSites.insert(unlockSite);
        }
    }
}


/*
 * Perform analysis for a single Function.
 */
void LocksetAnalysis::BottomUpAnalyzer::run() {

    identifyInterestingStuffsForFunction();

    identifyInterestingStuffsForScc();

    matchAcquireReleaseLockSites();

    // If the Function has a ReturnInst, we summarize its side effects.
    if (retInst) {
        computeMustAcquiredLockset();
        computeMayReleasedLockset();
    }
}


/*
 * Match the lock acquire/release sites intra-procedurally.
 */
void LocksetAnalysis::BottomUpAnalyzer::matchAcquireReleaseLockSites() {
    for (auto it = lockSites.begin(), ie = lockSites.end(); it != ie; ++it) {
        const Instruction *lockSite = it->first;
        const LockSet &lockset = it->second;
        for (auto it = unlockSites.begin(), ie = unlockSites.end(); it != ie; ++it) {
            const Instruction *unlockSite = it->first;
            const LockSet &unlockset = it->second;
            if (lsa->hasCommonLock(lockset, unlockset)) {
                matchingLockSites[unlockSite].insert(lockSite);
                matchingUnlockSites[lockSite].insert(unlockSite);
            }
        }
    }
}


/*
 * Compute the must-acquired lockset.
 */
void LocksetAnalysis::BottomUpAnalyzer::computeMustAcquiredLockset() {
    // Analyze the identified lock/unlock sites in the CFG.
    DominatorTree &dt = lsa->getDt(F);
    for (Inst2LocksetMap::const_iterator it = lockSites.begin(),
            ie = lockSites.end(); it != ie; ++it) {
        const Instruction *lockSite = it->first;
        const LockSet &lockset = it->second;

        // If csInst holds must-acquire property, it has to satisfy two conditions.
        // (1) csInst dominates the return node in CFG
        if (!dt.dominates(lockSite, retInst))   continue;

        // (2) Backward reachability from retInst to lockSite
        // is not blocked by unlockSites
        const InstSet &correspondingUnlockSites = getMatchingUnlockSites(lockSite);
        if (hasAcquireLockSideEffect(lockSite, correspondingUnlockSites)) {
            mustAcquiredLockset.insert(lockset.begin(), lockset.end());
        }
    }
}


/*
 * Compute the may-released lockset.
 */
void LocksetAnalysis::BottomUpAnalyzer::computeMayReleasedLockset() {
    // If a lock release site is dominated by a matching lock acquire site in CFG,
    // then it would not have any side effect to its callers.
    // FIXME: maybe we are too optimistic here.
    DominatorTree &dt = lsa->getDt(F);
    for (Inst2LocksetMap::const_iterator it = unlockSites.begin(),
            ie = unlockSites.end(); it != ie; ++it) {
        const Instruction *unlockSite = it->first;
        const LockSet &unlockset = it->second;
        const InstSet &correspondingLockSites = getMatchingLockSites(unlockSite);
        if (!correspondingLockSites.empty()) {
            bool hasDominatingLockSite = false;
            for (auto it = correspondingLockSites.begin(), ie =
                    correspondingLockSites.end(); it != ie; ++it) {
                const Instruction *lockSite = *it;
                if (dt.dominates(lockSite, unlockSite)) {
                    hasDominatingLockSite = true;
                    break;
                }
            }
            if (hasDominatingLockSite)      continue;
        }
        mayReleasedLockset.insert(unlockset.begin(), unlockset.end());
    }
}


/*
 * Identify interesting stuffs for the current Function, including ReturnInst,
 * lock/unlock from csInsts.
 */
void LocksetAnalysis::BottomUpAnalyzer::identifyInterestingStuffsForFunction() {
    const PTACallGraph *cg = lsa->getCallGraph();
    retInst = lsa->getRet(F);

    // Iterate every Instruction of "F" to handle interested ones.
    for (const_inst_iterator it = inst_begin(F), ie = inst_end(F); it != ie;
            ++it) {
        const Instruction *I = &*it;

        // Identify direct lock/unlock operations.
        if (analysisUtil::isLockAquireCall(I))  {
            lockSites[I].insert(rcUtil::getLockPtr(I));
        } else if (analysisUtil::isLockReleaseCall(I)) {
            unlockSites[I].insert(rcUtil::getLockPtr(I));
        }

        // Handle call sites.
        if (!cg->hasCallGraphEdge(I))    continue;
        for (PTACallGraph::CallGraphEdgeSet::const_iterator it =
                cg->getCallEdgeBegin(I), ie = cg->getCallEdgeEnd(I); it != ie;
                ++it) {
            const PTACallGraphEdge *edge = *it;
            const Function *callee = edge->getDstNode()->getFunction();
            if (callee->isDeclaration())    continue;
            const LocksetSummary *calleeSummary = lsa->getSccSummary(callee);
            if (!calleeSummary)     continue;

            // Identify lock/unlock side effects from callees.
            if (calleeSummary->acquire_begin() != calleeSummary->acquire_end()) {
                lockSites[I].insert(calleeSummary->acquire_begin(),
                        calleeSummary->acquire_end());
            }
            if (calleeSummary->release_begin() != calleeSummary->release_end()) {
                unlockSites[I].insert(calleeSummary->release_begin(),
                        calleeSummary->release_end());
            }
        }
    }
}


/*
 *  Identify interesting stuffs for the current SCC.
 *  This includes the reachable Function set (the current SCC and its iterative callees).
 *  The summary will be used by refinement analysis in a later phase.
 */
void LocksetAnalysis::BottomUpAnalyzer::identifyInterestingStuffsForScc() {
    const SCC *scc = lsa->getCurrentScc();
    if (F != scc->getRep())  return;

    const PTACallGraph *cg = lsa->getCallGraph();
    LocksetSummary &summary = lsa->getOrAddSccSummary(scc);

    // Add all Functions of current SCC into the reachableFuncs set.
    summary.addReachableFunction(scc->begin(), scc->end());

    // Collect callers' SCCs
    SmallSet<const SCC*, 16> callerSccs;
    for (auto it = scc->begin(), ie = scc->end(); it != ie; ++it) {
        const Function *F = *it;
        const PTACallGraphNode *cgNode = cg->getCallGraphNode(F);
        for (auto it = cgNode->getInEdges().begin(), ie = cgNode->getInEdges().end();
                it != ie; ++it) {
            const PTACallGraphEdge *edge = *it;
            const Function *caller = edge->getSrcNode()->getFunction();
            if (lsa->isDeadFunction(caller))    continue;
            callerSccs.insert(lsa->getScc(caller));
        }
    }

    // Propagate reachableFuncs set to callers' SccSummary
    for (auto it = callerSccs.begin(), ie = callerSccs.end(); it != ie; ++it) {
        const SCC *callerScc = *it;
        LocksetSummary &callerSummary = lsa->getOrAddSccSummary(callerScc);
        callerSummary.addReachableFunction(summary.reachableFuncBegin(),
                summary.reachableFuncEnd());
    }
}


/*
 * Check if "lockSite" is not blocked for every path to the return site in the CFG.
 * @param lockSite the given lock site
 * @param correspondingUnlockSites the unlock sites that may release
 * the acquired locks in lockSite
 */
bool LocksetAnalysis::BottomUpAnalyzer::hasAcquireLockSideEffect(
    const Instruction *lockSite, const InstSet &correspondingUnlockSites) const {
    // If there isn't any corresponding unlock site to block the lockSite,
    // then the lock side effect must happen.
    if (correspondingUnlockSites.empty())   return true;

    // Examine the reachability
    const BasicBlock *retBB = retInst->getParent();
    const BasicBlock *lockSiteBB = lockSite->getParent();

    // (1) If "retInst" and "lockSite" are in the same BasicBlock,
    // then check if there is any corresponding unlock site between.
    if (retBB == lockSiteBB) {
        const Instruction *I = lockSite->getNextNode();
        while (I != retInst) {
            if (correspondingUnlockSites.count(I))  return false;
            I = I->getNextNode();
        }
        return true;
    }

    // (2) If "retInst" and "lockSite" are in different BasicBlocks,
    // then perform a reachability analysis.
    const Instruction *I = NULL;

    // (2-1) If there is any corresponding unlock site in "retBB",
    // then it does not have a must-happen side effect.
    I = retBB->getFirstNonPHI();
    while (I != retInst) {
        if (correspondingUnlockSites.count(I))  return false;
        I = I->getNextNode();
    }

    // (2-2) If there is any corresponding unlock site after "lockSite"
    // in "lockSiteBB", then it does not have a must-happen side effect.
    I = lockSite->getNextNode();
    while (!I->isTerminator()) {
        if (correspondingUnlockSites.count(I))  return false;
        I = I->getNextNode();
    }

    // (2-3) If all corresponding unlock sites are located outside of "retBB"
    // and "lockSiteBB", then we perform a BasicBlock level reachability analysis.
    BbSet reachable;
    reachable.insert(lockSiteBB);
    stack<const BasicBlock*> worklist;
    worklist.push(retBB);
    while (!worklist.empty()) {
        const BasicBlock *BB = worklist.top();
        worklist.pop();
        if (reachable.count(BB))    continue;
        reachable.insert(BB);
        for (const_pred_iterator it = pred_begin(BB), ie = pred_end(BB);
                it != ie; ++it) {
            const BasicBlock *predBB = *it;
            if (reachable.count(predBB))    continue;
            worklist.push(predBB);
        }
    }
    for (InstSet::const_iterator it = correspondingUnlockSites.begin(),
            ie = correspondingUnlockSites.end(); it != ie; ++it) {
        const Instruction *unlockSite = *it;
        const BasicBlock *unlockSiteBB = unlockSite->getParent();
        if (reachable.count(unlockSiteBB))  return false;
    }
    return true;
}


/*
 * Perform analysis for a single Function.
 */
void LocksetAnalysis::TopDownAnalyzer::run() {

    identifyEntryLockset();

    performReachabilityAnalysis();
}


/*
 * Identify the side effects of acquired lockset from callers.
 * We only consider must-acquire lockset, which contains
 * the acquired lock from all call sites that "this-F" is called.
 */
void LocksetAnalysis::TopDownAnalyzer::identifyEntryLockset() {
    PTACallGraphEdge::CallInstSet csInsts;
    PTACallGraph *cg = lsa->getCallGraph();
    rcUtil::getAllValidCallSitesInvokingCallee(cg, F, csInsts);
    bool firstIteration = true;
    for (PTACallGraphEdge::CallInstSet::const_iterator it = csInsts.begin(),
            ie = csInsts.end(); it != ie; ++it) {

        // Compute the intersection of all valid call sites
        const Instruction *csInst = *it;
        const LockSet *lockset = LocksetSummary::getProtectingLocks(csInst);
        if (!lockset) {
            entryLockset.clear();
            return;
        }

        if (firstIteration) {
            entryLockset = *lockset;
            firstIteration = false;
        } else {
            entryLockset = rcUtil::getIntersection(entryLockset, *lockset);
        }
    }
}


/*
 * Perform reachability analysis to identify protected Instructions.
 */
void LocksetAnalysis::TopDownAnalyzer::performReachabilityAnalysis() {
    // Forward reachability analysis from Function entry.
    if (!entryLockset.empty()) {
        const Instruction *firstInst = &*F->getEntryBlock().getFirstInsertionPt();
        identifyProtectedInstructions(firstInst, entryLockset);
    }

    // Forward reachability analysis from every lock site.
    for (Inst2LocksetMap::const_iterator it = lockSites.begin(),
            ie = lockSites.end(); it != ie; ++it) {
        const Instruction *lockSite = it->first;
        const LockSet &lockset = it->second;
        identifyProtectedInstructions(lockSite->getNextNode(), lockset);
    }
}


/*
 * Identify protected Instructions of a given lockset from a relevant acquire site.
 * This algorithm has some path sensitivity.
 * @param root the root Instruction (usually the immediate
 * following Instruction of a lock acquire site) to start traversal
 * @param lockset the lockset acquired
 */
void LocksetAnalysis::TopDownAnalyzer::identifyProtectedInstructions(
        const Instruction *root, const LockSet &lockset) {
    // Skip NULL root
    if (!root)  return;

    DominatorTree &dt = lsa->getDt(F);
    PTACallGraph *cg = lsa->getCallGraph();
    PathCorrelationAnaysis &pca = lsa->getPathCorrelationAnaysis();

    // Get corresponding unlock sites of "lockset".
    InstSet correspondingUnlockSites;
    identifyCorrespondingUnlockSites(lockset, correspondingUnlockSites);

    // Protected frontiers
    InstSet protectedFrontiers;

    // Perform forward traversal.
    InstSet visited;
    stack<const Instruction*> worklist;
    worklist.push(root);
    while (!worklist.empty()) {
        const Instruction *I = worklist.top();
        worklist.pop();

        // Skip the visited Instructions.
        if (visited.count(I))   continue;

        // Check if I is protected with some path sensitivity.
        bool isProtectedInst = pca.subsumeOnGuards(I, root);

        // Process lockset protection if I is protected
        if (isProtectedInst) {
            protectedFrontiers.clear();
            protecting(I, lockset, correspondingUnlockSites, dt, cg, visited,
                    protectedFrontiers);
            for (auto it = protectedFrontiers.begin(),
                    ie = protectedFrontiers.end(); it != ie; ++it) {
                worklist.push(*it);
            }
        }
        // Continue forward traversal
        else {
            visited.insert(I);

            const BasicBlock *BB = I->getParent();
            // Visit all successors of "BB" in the CFG
            for (succ_const_iterator it = succ_begin(BB), ie = succ_end(BB);
                    it != ie; ++it) {
                const BasicBlock *succBb = *it;
                const Instruction *firstInst = succBb->getFirstNonPHI();
                if (visited.count(firstInst))   continue;
                worklist.push(firstInst);
            }
        }
    }
}


/*
 * Identify the protected Instructions .
 * @param root the root Instruction to start traversal
 * @param lockset the protecting lockset
 * @param unlockSites the corresponding lock release sites
 * @param dt DominatorTree
 * @param cg CallGraph
 * @param visited the visited Instructions
 * @param protectedFrontiers the Instructions becoming unprotected
 */
void LocksetAnalysis::TopDownAnalyzer::protecting(const llvm::Instruction *root,
        const LockSet &lockset, const InstSet &unlockSites, DominatorTree &dt,
        PTACallGraph *cg, InstSet &visited, InstSet &protectedFrontiers) {
    stack<const Instruction*> worklist;
    worklist.push(root);
    while (!worklist.empty()) {
        const Instruction *I = worklist.top();
        worklist.pop();

        // Skip the visited Instructions.
        if (visited.count(I))   continue;
        visited.insert(I);

        // Record the result if "I" is an interested memory access operation.
        if (rcUtil::isMemoryAccess(I)) {
            LocksetSummary::getOrAddProtectingLocks(I).insert(
                    lockset.begin(), lockset.end());
        }

        // Function calls.
        if (cg->hasCallGraphEdge(I)) {
            // If the call may release the lock, then stop traversing,
            if (unlockSites.count(I))  continue;
            // else record the side effect from F to callee via the csInst
            LocksetSummary::getOrAddProtectingLocks(I).insert(
                    lockset.begin(), lockset.end());
        }

        // Push the following Instruction(s) of "I" into the worklist,
        // if they are dominated by "I".
        if (!I->isTerminator()) {
            worklist.push(I->getNextNode());
        } else {
            const BasicBlock *BB = I->getParent();
            // Visit all successors of "BB" in the CFG
            for (succ_const_iterator it = succ_begin(BB), ie = succ_end(BB);
                    it != ie; ++it) {
                const BasicBlock *succBb = *it;
                const Instruction *firstInst = succBb->getFirstNonPHI();
                if (!dt.dominates(root, succBb)) {
                    protectedFrontiers.insert(firstInst);
                    continue;
                }
                if (visited.count(firstInst))   continue;
                worklist.push(firstInst);
            }
        }
    }
}



LocksetSummary::Inst2LocksetMap LocksetSummary::inst2LocksetMap;

LocksetAnalysis::FunctionAnalyzerBase::UniversalInst2LocksetMap
LocksetAnalysis::FunctionAnalyzerBase::universalLockSites;

LocksetAnalysis::FunctionAnalyzerBase::UniversalInst2LocksetMap
LocksetAnalysis::FunctionAnalyzerBase::universalUnlockSites;

LocksetAnalysis::InstSet LocksetAnalysis::BottomUpAnalyzer::emptyInstSet;

