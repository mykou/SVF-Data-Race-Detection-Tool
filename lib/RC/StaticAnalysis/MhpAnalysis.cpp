/*
 * MhpAnalysis.cpp
 *
 *  Created on: 01/05/2015
 *      Author: dye
 */


#include "MhpAnalysis.h"
#include "Util/ThreadCallGraph.h"
#include <llvm/IR/InstIterator.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/Support/CommandLine.h>

using namespace llvm;
using namespace std;
using namespace analysisUtil;

extern cl::opt<bool> RcDetail;

/*
 * Initialization
 * @param cg The callgraph.
 * @param tcg The thread callgraph.
 * @param pta The pointer analysis.
 */
void MhpAnalysis::init(PTACallGraph *cg, ThreadCallGraph *tcg,
        BVDataPTAImpl *pta) {

    this->tcg = tcg;

    SUPER::init(cg, tcg, pta);
}


/*
 * May-happen-in-parallel analysis
 */
void MhpAnalysis::analyze() {

    // Perform bottom-up and top-down analysis for fork-join MHP analysis
    identifySpawnJoinSites();

    SUPER::analyze(SUPER::AO_BOTTOM_UP);

    SUPER::analyze(SUPER::AO_TOP_DOWN);

    // Perform parallel-for MHP analysis
    performParForAnalysis();

    // Compute the back mapping
    computeBackwardReachableInfo();

}


/*
 * Release resource
 */
void MhpAnalysis::release() {
    for (auto it = threadJoinAnalysisMap.begin(), ie =
            threadJoinAnalysisMap.end(); it != ie; ++it) {
        ThreadJoinAnalysis *&p = it->second;
        if (p) {
            delete p;
            p = NULL;
        }
    }
    for (auto it = spawnSiteInfoMap.begin(), ie =
            spawnSiteInfoMap.end(); it != ie; ++it) {
        SpawnJoinSiteInfo *&p = it->second;
        if (p) {
            delete p;
            p = NULL;
        }
    }
    for (auto it = joinSiteInfoMap.begin(), ie =
            joinSiteInfoMap.end(); it != ie; ++it) {
        SpawnJoinSiteInfo *&p = it->second;
        if (p) {
            delete p;
            p = NULL;
        }
    }
}


/*
 * Interface to answer if two Instruction may happen in parallel.
 * Return all the possible mhp spawn sites which are reachable from both Instructions.
 * Performance is low.
 */
MhpAnalysis::InstSet MhpAnalysis::mayHappenInParallel(const Instruction *I1,
        const Instruction *I2) {
    InstSet ret;
    BackwardReachablePoints &brp1 = getBackwardReachablePoints(I1);
    BackwardReachablePoints &brp2 = getBackwardReachablePoints(I2);
    for (BackwardReachablePoints::const_iterator it = brp1.begin(), ie = brp1.end();
            it != ie; ++it) {
        const ReachablePoint &rp1 = *it;
        const Instruction *spawnSite = rp1.getSpawnSite();
        if (ret.count(spawnSite))   continue;

        for (BackwardReachablePoints::const_iterator it = brp2.begin(), ie = brp2.end();
                it != ie; ++it) {
            const ReachablePoint &rp2 = *it;
            // Two Instructions must be backward reachable from the same spawn site.
            if (spawnSite != rp2.getSpawnSite())   continue;

            ReachableType t1 = rp1.getReachableType();
            ReachableType t2 = rp2.getReachableType();
            if (mhpForReachablePair(t1, t2))    ret.insert(spawnSite);
        }
    }
    return ret;
}


/*
 * Get the ReachablePoint pairs of two MHP Instructions
 */
MhpAnalysis::ReachablePointPairs MhpAnalysis::getReachablePointPairs(
        const Instruction *I1, const Instruction *I2) {
    ReachablePointPairs ret;
    BackwardReachablePoints &brp1 = getBackwardReachablePoints(I1);
    BackwardReachablePoints &brp2 = getBackwardReachablePoints(I2);
    for (BackwardReachablePoints::const_iterator it = brp1.begin(), ie =
            brp1.end(); it != ie; ++it) {
        const ReachablePoint &rp1 = *it;
        const Instruction *spawnSite = rp1.getSpawnSite();

        for (BackwardReachablePoints::const_iterator it = brp2.begin(), ie =
                brp2.end(); it != ie; ++it) {
            const ReachablePoint &rp2 = *it;
            // Two Instructions must be backward reachable from the same spawn site.
            if (spawnSite != rp2.getSpawnSite())    continue;

            // Their ReachableType must be risky
            ReachableType t1 = rp1.getReachableType();
            ReachableType t2 = rp2.getReachableType();
            if (!mhpForReachablePair(t1, t2))       continue;

            // Check for the redundancy
            bool isRedundant = false;
            if (I1 == I2) {
                for (int i = 0, e = ret.size(); i != e; ++i) {
                    if (ret[i].first == rp2 && ret[i].second == rp1) {
                        isRedundant = true;
                        break;
                    }
                }
            }
            if (isRedundant)    continue;

            // Push to the ret
            ret.push_back(make_pair(rp1, rp2));
        }
    }
    return ret;
}


/*
 * Determine if a reachable pair (with reachable types t1 and t2) may happen in parallel.
 */
bool MhpAnalysis::mhpForReachablePair(ReachableType t1, ReachableType t2) {
    bool trunk = false;
    bool branch = false;

    // Compute the reachability properties
    switch (t1) {
    case REACHABLE_TRUNK:   trunk = true;   break;
    case REACHABLE_BRANCH:  branch = true;  break;
    default:    break;
    }
    switch (t2) {
    case REACHABLE_TRUNK:   trunk = true;   break;
    case REACHABLE_BRANCH:  branch = true;  break;
    default:    break;
    }

    // Check if they may happen in parallel.
    if (trunk && branch)
        return true;
    else
        return false;
}


/*
 * Identify spawn/join sites according to ThreadCallGraph
 */
void MhpAnalysis::identifySpawnJoinSites() {
    // Identify spawn sites
    for (ThreadCallGraph::CallSiteSet::const_iterator it = tcg->forksitesBegin(),
            ie = tcg->forksitesEnd(); it != ie; ++it) {
        const Instruction *spawnSite = *it;
        const Function *spawner = spawnSite->getParent()->getParent();

        // Skip if spawner is a dead function (i.e., not reachable from main)
        if (isDeadFunction(spawner))    continue;

        // Init spawnSiteReachableCodeSetMap for spawnSite
        spawnSiteReachability.addSpawnSite(spawnSite);

        // Add the side effect to spawner
        MhpSummary &summary = getOrAddSccSummary(spawner);
        summary.addAffectedSpawnSite(spawnSite, spawnSite);

        // Create SpawnJoinSiteInfo
        SpawnJoinSiteInfo *spawnJoinSiteInfo = new SpawnJoinSiteInfo(this, spawnSite, true);
        spawnJoinSiteInfo->run();
        spawnSiteInfoMap[spawnSite] = spawnJoinSiteInfo;
    }

    // Identify join sites
    for (ThreadCallGraph::CallSiteSet::const_iterator it = tcg->joinsitesBegin(),
            ie = tcg->joinsitesEnd(); it != ie; ++it) {
        const Instruction *joinSite = *it;
        const Function *F = joinSite->getParent()->getParent();

        // Skip if F is a dead function (i.e., not reachable from main)
        if (isDeadFunction(F))    continue;

        // Add the side effect to spawner
        MhpSummary &summary = getOrAddSccSummary(F);
        summary.insertJoinSite(joinSite);

        // Create SpawnJoinSiteInfo
        SpawnJoinSiteInfo *spawnJoinSiteInfo = new SpawnJoinSiteInfo(this, joinSite, false);
        spawnJoinSiteInfo->run();
        joinSiteInfoMap[joinSite] = spawnJoinSiteInfo;
    }

    // Perform Thread Join Analysis
    for (ThreadCallGraph::CallSiteSet::const_iterator it = tcg->forksitesBegin(),
            ie = tcg->forksitesEnd(); it != ie; ++it) {
        const Instruction *spawnSite = *it;
        const Function *spawner = spawnSite->getParent()->getParent();

        // Skip if spawner is a dead function (i.e., not reachable from main)
        if (isDeadFunction(spawner))    continue;

        ThreadJoinAnalysis *threadJoinAnalysis = new ThreadJoinAnalysis(this, spawnSite);
        threadJoinAnalysis->run();
        threadJoinAnalysisMap[spawnSite] = threadJoinAnalysis;
    }

    // Print the unmatched join sites.
    if (RcDetail) {
        for (auto it = joinSiteInfoMap.begin(), ie = joinSiteInfoMap.end();
                it != ie; ++it) {
            const Instruction *joinSite = it->first;
            if (it->second->getMatchingSpawnSite())     continue;
            outs() << "Join site" << rcUtil::getSourceLoc(joinSite)
                    << " is not matched to any spawn site.\n";
        }
    }
}


/*
 * Check if "F" may have side effect of "spawnSite".
 */
bool MhpAnalysis::mayHaveSideEffect(const Instruction *spawnSite,
        const Function *F) {
    // We do not perform inter-procedural analysis.
    if (F != spawnSite->getParent()->getParent())   return true;

    const ReturnInst *retInst = getRet(F);
    if (!retInst)   return false;

    DominatorTree &dt = getDt(F);
    ThreadJoinAnalysis *threadJoinAnalysis = threadJoinAnalysisMap[spawnSite];
    assert (threadJoinAnalysis);

    // Check if any must-join site dominates "retInst".
    const ValSet &vals = threadJoinAnalysis->getJoinInstsOrBbs(F);
    for (auto it = vals.begin(), ie = vals.end(); it != ie; ++it) {
        const Value *v = *it;
        if (const Instruction *I = dyn_cast<Instruction>(v)) {
            if (dt.dominates(I, retInst))   return false;
        } else if (const BasicBlock *BB = dyn_cast<BasicBlock>(v)) {
            if (dt.dominates(BB, retInst->getParent()))     return false;
        }
    }

    return true;
}


/*
 * Inter-procedural analysis visitor.
 */
void MhpAnalysis::visitFunction(const Function *F) {
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
 * Bottom-up Function visitor.
 * Compute the thread spawn site's side effects.
 * This analysis performs for each Function in a bottom-up manner
 * to summarize the side effects.
 */
void MhpAnalysis::bottomUpFunctionVisitor(const Function *F) {
    // We only do the summary once for each SCC.
    // So we perform the analysis when visiting the rep of the SCC.
    const SCC *scc = topoSccOrder.getScc(F);
    if (scc->getRep() != F)   return;

    // Skip if we do not have anything to propagate.
    MhpSummary *summary = getSccSummary(F);
    if (!summary)   return;

    // Do the summary work for every spawn site.
    PTACallGraph *cg = getCallGraph();
    PTACallGraphEdge::CallInstSet callInsts;
    for (MhpSummary::const_iterator it = summary->spawnBegin(), ie = summary->spawnEnd();
            it != ie; ++it) {
        // Skip if the spawn site does not have side effect to its callers.
        const Instruction *spawnSite = it->first;
        if (!mayHaveSideEffect(spawnSite, F))   continue;

        // Collect all callInst that calls any Function in "scc".
        callInsts.clear();
        for (SCC::const_iterator it = scc->begin(), ie = scc->end(); it != ie; ++it) {
            const Function *F = *it;
            rcUtil::getAllValidCallSitesInvokingCallee(cg, F, callInsts);
        }

        // Propagate the side effect to the callers' summaries
        for (PTACallGraphEdge::CallInstSet::const_iterator it =
                callInsts.begin(), ie = callInsts.end(); it != ie; ++it) {
            const Instruction *I = *it;
            const Function *caller = I->getParent()->getParent();
            MhpSummary &callerSummary = getOrAddSccSummary(caller);
            callerSummary.addAffectedSpawnSite(spawnSite, I);
        }
    }

}


/*
 * Top-down Function visitor.
 * Compute the thread spawn site's reachability.
 * This analysis performs for each Function in a top-down manner
 * to compute the followers/spawnees.
 */
void MhpAnalysis::topDownFunctionVisitor(const Function *F) {
    // We only do the analysis once for each SCC.
    // We choose the rep Function to perform the analysis.
    const SCC *scc = topoSccOrder.getScc(F);
    if (scc->getRep() != F)   return;

    // Skip if we do not have anything to propagate.
    const MhpSummary *summary = getSccSummary(F);
    if (!summary)   return;

    // Identify the followers/spawnees for every spawn site.
    for (MhpSummary::const_iterator it = summary->spawnBegin(), ie = summary->spawnEnd();
            it != ie; ++it) {
        const Instruction *spawnSite = it->first;
        const MhpSummary::InstSet &sideEffectSites = it->second;
        identifyReachable(scc, spawnSite, sideEffectSites);
    }
}


/*
 * Identify the reachable code of "spawnSite".
 */
void MhpAnalysis::identifyReachable(const SCC *scc, const Instruction *spawnSite,
            const MhpSummary::InstSet &sideEffectSites) {
    // Regular reachability analysis
    if (!refinementMode) {
        SpawnSiteReachableCodeSet &reachableCodeSet =
                spawnSiteReachability.getReachableCodeSet(spawnSite);
        CodeSet &trunkCode = reachableCodeSet.getTrunkReachableCodeSet();
        CodeSet &branchCode = reachableCodeSet.getBranchReachableCodeSet();

        // If scc is recursive, then the whole scc are followers.
        if (scc->isRecursive()) {
            reachableCodeSet.setRecursive(true);

            // TODO: Set the whole scc as followers
            if (RcDetail) {
                outs() << bugMsg1("Miss-handled cycle")
                        << " in MhpAnalysis::identifyReachable()\n";
            }
        }

        // Initialize the ReachabilityAnalysis
        ThreadJoinAnalysis *threadJoinAnalysis = threadJoinAnalysisMap[spawnSite];
        assert(threadJoinAnalysis);
        ReachabilityAnalysis reachabilityAnalysis(trunkCode, branchCode, threadJoinAnalysis);
        reachabilityAnalysis.solveTrunkReachability(sideEffectSites, *scc, tcg);
        reachabilityAnalysis.solveBranchReachability(spawnSite, tcg, branchCode);
        return;
    }

    // Refinement reachability analysis
    for (auto it = refinedSpawnSiteReachableCodeSetMap.begin(), ie =
            refinedSpawnSiteReachableCodeSetMap.end(); it != ie; ++it) {
        if (it->first != spawnSite)     continue;

        for (auto itt = it->second.begin(), iee = it->second.end();
                itt != iee; ++itt) {
            const Instruction *excludedSpawnSite = itt->first;
            SpawnSiteReachableCodeSet &reachableCodeSet = itt->second;
            CodeSet &trunkCode = reachableCodeSet.getTrunkReachableCodeSet();
            CodeSet &branchCode = reachableCodeSet.getBranchReachableCodeSet();
            ThreadJoinAnalysis *threadJoinAnalysis = threadJoinAnalysisMap[spawnSite];
            assert(threadJoinAnalysis);
            ReachabilityAnalysis reachabilityAnalysis(trunkCode, branchCode, threadJoinAnalysis);
            reachabilityAnalysis.solveTrunkReachability(sideEffectSites, *scc,
                    tcg, excludedSpawnSite);
            reachabilityAnalysis.solveBranchReachability(spawnSite,
                    tcg, threadJoinAnalysisMap[excludedSpawnSite]);
        }
    }

    return;
}


/*
 * Compute the backward reachability information that tells
 * which spawn site(s) can be backwards reached from a given
 * Instruction/BasicBlock/Function.
 */
void MhpAnalysis::computeBackwardReachableInfo() {
    // Process spawn sites
    for (auto it = spawnSiteReachability.begin(), ie = spawnSiteReachability.end();
            it != ie; ++it) {
        const Instruction *spawnSite = it->first;
        SpawnSiteReachableCodeSet &reachableCodeSet = it->second;
        CodeSet &trunkCode = reachableCodeSet.getTrunkReachableCodeSet();
        CodeSet &branchCode = reachableCodeSet.getBranchReachableCodeSet();

        // Do redundancy removal here before adding ReachablePoints.
        trunkCode.removeRedundamcy();
        branchCode.removeRedundamcy();

        // Compute the Function level backward reachability information
        for (auto it = trunkCode.func_begin(), ie = trunkCode.func_end();
                it != ie; ++it) {
            const Function *F = *it;
            addReachablePoint(F, spawnSite, REACHABLE_TRUNK);
        }
        for (auto it = branchCode.func_begin(), ie = branchCode.func_end();
                it != ie; ++it) {
            const Function *F = *it;
            addReachablePoint(F, spawnSite, REACHABLE_BRANCH);
        }

        // Compute the BasicBlock level backward reachability information
        for (auto it = trunkCode.bb_begin(), ie = trunkCode.bb_end(); it != ie;
                ++it) {
            const BasicBlock *BB = *it;
            addReachablePoint(BB, spawnSite, REACHABLE_TRUNK);
        }

        // Compute the Instruction level backward reachability information
        for (auto it = trunkCode.inst_begin(), ie = trunkCode.inst_end();
                it != ie; ++it) {
            const Instruction *I = *it;
            addReachablePoint(I, spawnSite, REACHABLE_TRUNK);
        }
    }

    // Process parallel-for sites
    for (auto it = parForReachabilityMap.begin(), ie = parForReachabilityMap.end();
            it != ie; ++it) {
        const Instruction *parForSite = it->first;
        const CodeSet &code = it->second;

        assert(code.inst_begin() == code.inst_end());
        assert(code.bb_begin() == code.bb_end());

        // Compute the Function level backward reachability information
        for (auto it = code.func_begin(), ie = code.func_end();
                it != ie; ++it) {
            const Function *F = *it;
            addReachablePoint(F, parForSite, REACHABLE_TRUNK);
        }
        for (auto it = code.func_begin(), ie = code.func_end();
                it != ie; ++it) {
            const Function *F = *it;
            addReachablePoint(F, parForSite, REACHABLE_BRANCH);
        }
    }
}


/*
 * Check if the MHP relations between two given Instructions from a spawn
 * site is refined (proven to be infeasible by refinement analysis' result).
 */
bool MhpAnalysis::branchJoinRefined(const Instruction *spawnSite,
        const Instruction *I1, const Instruction *I2) const {
    assert(refinementMode && "Refinement analysis must have been performed.");

    auto iter = refinedSpawnSiteReachableCodeSetMap.find(spawnSite);
    if (iter == refinedSpawnSiteReachableCodeSetMap.end())  return false;
    for (auto it = iter->second.begin(), ie = iter->second.end(); it != ie;
            ++it) {
        const SpawnSiteReachableCodeSet &reachableCodeSet = it->second;
        if (reachableCodeSet.isTrunkReachable(I1))      return false;
        if (reachableCodeSet.isTrunkReachable(I2))      return false;
        if (reachableCodeSet.isBranchReachable(I1))     return false;
        if (reachableCodeSet.isBranchReachable(I2))     return false;
    }

    return true;
}


/*
 * Initialize refinement analysis.
 */
void MhpAnalysis::initRefinementAnalysis() {
    // Activate refinement mode
    refinementMode = true;

    // Identify the join sites that were not handled by the default MhpAnalysis
    for (auto it = joinSiteInfoMap.begin(), ie = joinSiteInfoMap.end();
            it != ie; ++it) {
        const Instruction *joinSite = it->first;
        SpawnJoinSiteInfo *joinSiteInfo = it->second;
        const Instruction *matchingSpawnSite = joinSiteInfo->getMatchingSpawnSite();
        if (!matchingSpawnSite || isTrunkReachable(matchingSpawnSite, joinSite)
                || isBranchReachable(matchingSpawnSite, joinSite)) {
            continue;
        }

        const BackwardReachablePoints &brp = getBackwardReachablePoints(joinSite);
        for (auto it = brp.begin(), ie = brp.end(); it != ie; ++it) {
            const Instruction *reachableSpawnSite = (*it).getSpawnSite();
            refinedSpawnSiteReachableCodeSetMap[reachableSpawnSite][matchingSpawnSite];
        }
    }
}


/*
 * Perform parallel-for MHP analysis
 */
void MhpAnalysis::performParForAnalysis() {
    // Iterate every parallel-for site
    for (ThreadCallGraph::CallSiteSet::const_iterator it = tcg->parForSitesBegin(),
            ie = tcg->parForSitesEnd(); it != ie; ++it) {
        const Instruction *parForSite = *it;
        assert(parForReachabilityMap.find(parForSite) == parForReachabilityMap.end());
        ReachabilityAnalysis::solveBranchReachability(parForSite, tcg,
                parForReachabilityMap[parForSite]);
    }
}


/*
 * Perform refinement analysis.
 */
void MhpAnalysis::performRefinementAnalysis() {

    initRefinementAnalysis();

    SUPER::analyze(SUPER::AO_TOP_DOWN);

}


/*
 * Print BoundCond information
 */
void MhpAnalysis::BoundCond::print() const {
    if (!isValid()) {
        outs() << " --- Invalid BoundCond ---\n";
        return;
    }

    outs() << " --- BoundCond ---\n";
    outs() << indVar->getName()
            << " " << rcUtil::getPredicateString(pred) << " "
            << bound->getName() << "\n";
    outs() << "\t" << *CI << "\n";
    outs() << "\t" << *indVar << "\n";
    outs() << "\t" << *bound << "\n";
}


/*
 * Compute the information of a spawn/join site.
 */
void MhpAnalysis::SpawnJoinSiteInfo::run() {
    const BasicBlock *BB = site->getParent();
    const Function *F = BB->getParent();

    // Compute tidPtr
    if (isSpawnSite) {
        tidPtr = site->getOperand(0);
    } else {
        Value *tid = site->getOperand(0);
        LoadInst *LI = dyn_cast<LoadInst>(tid);
        if (!LI) {
            // May be parameters passed to some thread_join wrappers in this case.
            return;
        }
        tidPtr = LI->getPointerOperand();
    }

    // Compute tidPtrBase
    tidPtrBase = getBasePtr(tidPtr);

    // Get other loop-related information
    ScalarEvolution &SE = mhp->getSe(F);
    const SCEV *siteScev = SE.getSCEV(tidPtr);
    const SCEV *offsetPatternScev = getSCEVMinusExpr(siteScev, SE.getSCEV(tidPtrBase), &SE);
    const SCEVAddRecExpr *addRec = dyn_cast<SCEVAddRecExpr>(offsetPatternScev);
    if (!addRec)    return;

    if (const SCEVUnknown *startExpr = dyn_cast<SCEVUnknown>(addRec->getStart())) {
        offsetStart = startExpr->getValue();
    } else if (const SCEVConstant *startExpr = dyn_cast<SCEVConstant>(addRec->getStart())) {
        offsetStart = startExpr->getValue();
    }
    if (const SCEVUnknown *strideExpr = dyn_cast<SCEVUnknown>(addRec->getStepRecurrence(SE))) {
        offsetStride = strideExpr->getValue();
    } else if (const SCEVConstant *strideExpr = dyn_cast<SCEVConstant>(addRec->getStepRecurrence(SE))) {
        offsetStride = strideExpr->getValue();
    }
    if (!offsetStart || !offsetStride)    return;

    // Check if it is in a loop
    L = getLi(F)->getLoopFor(BB);
    if (!L)     return;

    // Get canonical induction variable if available
    canonicalIndVar = L->getCanonicalInductionVariable();

    // Identify the unique exiting BasicBlock if available,
    // where the exitingBlock must dominate the exitBlock.
    auto pair = getUniqueExitBasicBlock();
    exitingBb = pair.first;
    exitBb = pair.second;
    if (!exitingBb || !exitBb)     return;
    if (!getDt(F).dominates(exitingBb, exitBb))    return;

    // Compute BoundCond
    const TerminatorInst *Term = exitingBb->getTerminator();
    if (const BranchInst *BI = dyn_cast<BranchInst>(Term)) {
        assert(BI->isConditional() && "If unconditional, it can't be in loop!");
        const CmpInst *CI = dyn_cast<CmpInst>(BI->getCondition());
        if (!CI)  return;

        CmpInst::Predicate pred = CI->getPredicate();
        const Value *lhs = rcUtil::stripAllCasts(CI->getOperand(0));
        const Value *rhs = rcUtil::stripAllCasts(CI->getOperand(1));
        const Value *indVar = L->getCanonicalInductionVariable();
        const Value *bound = rhs;

        if (lhs != indVar && rhs != indVar)   return;
        if (rhs == indVar) {
            bound = lhs;
            pred = rcUtil::getMirrorPredicate(pred);
        }

        BasicBlock *falseBb = BI->getSuccessor(1);
        if (L->contains(falseBb))
            pred = rcUtil::getInversePredicate(pred);

        bc.reset(CI, indVar, bound, pred);
    }
}


/*
 * Get the unique exiting and exit BasicBlocks
 * if this spawn/join site is in a loop.
 */
pair<const BasicBlock*, const BasicBlock*>
MhpAnalysis::SpawnJoinSiteInfo::getUniqueExitBasicBlock() {
    assert(L && "This spawn/join site must be in a loop.");

    // Get exiting BasicBlocks
    SmallVector<Loop::Edge, 4> exitEdges;
    L->getExitEdges(exitEdges);

    // Get the conservative execution count (e.g., ignoring the loop exit
    // caused by the failure of thread spawn).
    if (isSpawnSite) {
        // There is a single exit edge
        if (1 == exitEdges.size()) {
            return exitEdges.front();
        }
        // There are multiple exit edges
        else {
            // Identify the spawn failure branch.
            const BasicBlock *failureBranch =
                    ThreadJoinAnalysis::getSpawnFailureBranch(site);
            if (!failureBranch) {
                return make_pair((const BasicBlock*) NULL,
                        (const BasicBlock*) NULL);
            }

            for (auto it = exitEdges.begin(), ie = exitEdges.end();
                    it != ie; ++it) {
                BasicBlock *exitingBb = const_cast<BasicBlock*>(it->first);
                BasicBlock *exitBb = const_cast<BasicBlock*>(it->second);
                // We do not care the spawn failure branch.
                if (exitBb != failureBranch) {
                    return make_pair(exitingBb, exitBb);
                }
            }
        }
    }
    // Get the exact execution count
    else {
        if (executeBackedgeTakenCountTimes(site->getParent(), L)) {
            return exitEdges.front();
        }
    }

    return make_pair((const BasicBlock*)NULL, (const BasicBlock*)NULL);
}


/*
 * Check if a BasicBlock executes backedgeTakenCount times in a loop.
 */
bool MhpAnalysis::SpawnJoinSiteInfo::executeBackedgeTakenCountTimes(
        const BasicBlock *BB, const Loop *L) {
    // Identify the source BasicBlock of the backedge
    BasicBlock *headerBB = L->getHeader();
    BasicBlock *backedgeSrcBB = NULL;
    int numBackedges = 0;
    for (pred_iterator it = pred_begin(headerBB), ie = pred_end(headerBB);
            it != ie; ++it) {
        BasicBlock *predBB = *it;
        if (!L->contains(predBB))   continue;
        backedgeSrcBB = predBB;
        ++numBackedges;
    }

    // We only handle the loops with a single backedge.
    if (1 != numBackedges)  return false;

    // Check if "BB" dominates "backedgeSrcBB".
    DominatorTree &dt = FunctionPassPool::getDt(BB->getParent());
    return dt.dominates(BB, backedgeSrcBB);
}


/*
 * Print the information of this spawn/join site.
 */
void MhpAnalysis::SpawnJoinSiteInfo::print() const {
    outs() << " ----- SpawnJoinSiteInfo ----\n";

    // Spawn/join site source code location
    if (isSpawnSite) {
        outs() << "Spawn site:\t";
    } else {
        outs() << "Join site:\t";
    }
    outs() << rcUtil::getSourceLoc(site) << "\n";

    if (!isSpawnSite) {
        outs() << "Matching spawn site:\t"
                << rcUtil::getSourceLoc(matchingSite) << "\n";
    }

    // BoundCond
    bc.print();
}


/*
 * Get the base pointer from any GEP.
 */
Value *MhpAnalysis::SpawnJoinSiteInfo::getBasePtr(
        Value *v) {
    GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(v);
    while (GEP) {
        v = GEP->getOperand(0);
        GEP = dyn_cast<GetElementPtrInst>(v);
    }
    return v;
}


/*
 * Compute a SCEV that represents the subtraction of two given SCEVs.
 */
const SCEV *MhpAnalysis::SpawnJoinSiteInfo::getSCEVMinusExpr(const SCEV *s1,
        const SCEV *s2, ScalarEvolution *SE) {
    if (SE->getCouldNotCompute() == s1 || SE->getCouldNotCompute() == s2)
      return SE->getCouldNotCompute();

    Type *t1 = SE->getEffectiveSCEVType(s1->getType());
    Type *t2 = SE->getEffectiveSCEVType(s2->getType());
    if (t1 != t2)  {
      if (SE->getTypeSizeInBits(t1) < SE->getTypeSizeInBits(t2))
        s1 = SE->getSignExtendExpr(s1, t2);
      else
        s2 = SE->getSignExtendExpr(s2, t1);
    }

    return SE->getMinusSCEV(s1, s2);
}


/*
 * Check if a given SpawnJoinSiteInfo matches this in terms of:
 * (1) equal loop BoundCond;
 * (2) same access pattern of tid pointers in loop.
 */
bool MhpAnalysis::SpawnJoinSiteInfo::match(
        const SpawnJoinSiteInfo *siteInfo) const {
    // Check if both siteInfo have equal BoundCond
    if (bc != siteInfo->bc)   return false;

    // Check if both siteInfo have the same offset pattern.
    if (offsetStart != siteInfo->offsetStart)   return false;
    if (offsetStride != siteInfo->offsetStride)   return false;

    return true;
}


/*
 * Get the BasicBlock of the failure branch of "this->spawnSite".
 *  Return NULL if it does not find any.
 */
const BasicBlock *MhpAnalysis::ThreadJoinAnalysis::getSpawnFailureBranch(
        const Instruction *spawnSite) {
    // The BasicBlock of "spawnSite" must have multiple successors.
    const BasicBlock *spawnSiteBB = spawnSite->getParent();
    const BranchInst *BI = dyn_cast<BranchInst>(spawnSiteBB->getTerminator());
    if (!BI || BI->isUnconditional())   return NULL;

    // Get the condition Instruction
    const Value *cond = BI->getCondition();
    const CmpInst *CI = dyn_cast<CmpInst>(cond);
    if (!CI)    return NULL;

    // Examine the operands of CI
    const Value *opndLHS = rcUtil::stripAllCasts(CI->getOperand(0));
    const Value *opndRHS = rcUtil::stripAllCasts(CI->getOperand(1));

    if (opndLHS == spawnSite) {
        if (const ConstantInt *constInt = dyn_cast<ConstantInt>(opndRHS)) {
            if (!constInt->isZero())    return NULL;
        }
    } else if (opndRHS == spawnSite) {
        if (const ConstantInt *constInt = dyn_cast<ConstantInt>(opndLHS)) {
            if (!constInt->isZero())    return NULL;
        }
    } else {
        return NULL;
    }

    // Determine spawn failure branch.
    // Thread spawn is successful when LHS == RHS,
    // so CI evaluates to "true" if there is a spawn failure.
    bool trueWhenSpawnFails = CI->isFalseWhenEqual();
    const BasicBlock *failureBranch =
            trueWhenSpawnFails ? BI->getSuccessor(0) : BI->getSuccessor(1);
    return failureBranch;
}


/*
 * Check if the two SCEVExprs are two SCEVAddRecExpr with
 * equivalent starts and step recurrences according to "SE".
 */
bool MhpAnalysis::ThreadJoinAnalysis::haveSameStartAndStep(const SCEV *se1,
        const SCEV *se2) const {
    // We only handle AddRec here
    const SCEVAddRecExpr *addRec1 = dyn_cast<SCEVAddRecExpr>(se1);
    const SCEVAddRecExpr *addRec2 = dyn_cast<SCEVAddRecExpr>(se2);
    if (!addRec1 || !addRec2)   return false;

    // Compare the starts
    bool sameStart = (getStart(addRec1->getStart()) == getStart(addRec2->getStart()));
    if (!sameStart)     return false;

    // Compare the steps
    bool sameStep = (addRec1->getStepRecurrence(SE)
            == addRec2->getStepRecurrence(SE));
    if (!sameStep)      return false;

    return true;
}


/*
 * Get the known start value of a given SCEV
 */
const SCEV *MhpAnalysis::ThreadJoinAnalysis::getStart(const SCEV *scev) const {
    if (scev->isZero() || startFromZero.count(scev))
        return SE.getConstant(scev->getType(), 0);

    switch (scev->getSCEVType()) {
    case scAddRecExpr: {
        const SCEVAddRecExpr *AR = cast<SCEVAddRecExpr>(scev);
        const SCEV *start = AR->getStart();
        return getStart(start);
    }
    case scAddExpr: {
        const SCEVAddExpr *A = cast<SCEVAddExpr>(scev);
        const SCEV *res = getStart(A->getOperand(0));
        for (uint32_t i = 1; i < A->getNumOperands(); ++i) {
            const SCEV* opnd = getStart(A->getOperand(i));
            res = SE.getAddExpr(res, opnd);
        }
        return res;
    }
    case scMulExpr: {
        const SCEVMulExpr *M = cast<SCEVMulExpr>(scev);
        const SCEV *res = getStart(M->getOperand(0));
        for (uint32_t i = 1; i < M->getNumOperands(); ++i) {
            const SCEV* opnd = getStart(M->getOperand(i));
            res = SE.getMulExpr(res, opnd);
        }
        return res;
    }
    case scTruncate: {
        const SCEVTruncateExpr *T = cast<SCEVTruncateExpr>(scev);
        const SCEV *opnd = getStart(T->getOperand());
        return SE.getTruncateExpr(opnd, T->getType());
    }
    case scZeroExtend: {
        const SCEVZeroExtendExpr *zExt = cast<SCEVZeroExtendExpr>(scev);
        const SCEV *opnd = getStart(zExt->getOperand());
        return SE.getZeroExtendExpr(opnd, zExt->getType());
    }
    case scSignExtend: {
        const SCEVSignExtendExpr *sExt = cast<SCEVSignExtendExpr>(scev);
        const SCEV *opnd = getStart(sExt->getOperand());
        return SE.getSignExtendExpr(opnd, sExt->getType());
    }
    case scConstant:
    case scUnknown: {
        return scev;
    }
    }
    return scev;
}


/*
 * Check if the SCEVExprs computed by SCEV for spawn and join sites
 * have the same execution count.
 */
bool MhpAnalysis::ThreadJoinAnalysis::haveSameExecutionCount(
        const SCEV *spawnSiteExecutionCount, const SCEV *joinSiteExecutionCount,
        const Loop *spawnSiteLoop) {

    // Return false if we cannot get much information from joinSiteExecutionCount.
    if (SE.getCouldNotCompute() == joinSiteExecutionCount)  return false;

    // Return true if they have exactly the same execution count computed by SCEV.
    if (spawnSiteExecutionCount == joinSiteExecutionCount)  return true;

    // We now go a bit deeper into joinSiteExecutionCount to check weather
    // it equals the loop iteration counter of spawnSiteLoop.
    stack<const SCEV*> worklist;
    worklist.push(joinSiteExecutionCount);
    while (!worklist.empty()) {
        const SCEV *scev = worklist.top();
        worklist.pop();
        // (1) SCEVAddRecExpr
        if (const SCEVAddRecExpr *addRec = dyn_cast<SCEVAddRecExpr>(scev)) {
            const SCEV *start = addRec->getStart();
            const SCEV *stride = addRec->getStepRecurrence(SE);
            const Loop *L = addRec->getLoop();
            if ((start->isZero() || startFromZero.count(start)) && stride->isOne()
                    && L == spawnSiteLoop) {
                return true;
            }
        }
        // (2) SCEVSMaxExpr
        if (const SCEVSMaxExpr *sMax = dyn_cast<SCEVSMaxExpr>(scev)) {
            if (2 != sMax->getNumOperands())    continue;
            const SCEV *opnd1 = sMax->getOperand(0);
            const SCEV *opnd2 = sMax->getOperand(1);
            const SCEV *bound = NULL;
            if (opnd1->isZero()) {
                bound = opnd2;
            } else if (opnd2->isZero()) {
                bound = opnd1;
            }
            if (!bound)     continue;
            worklist.push(bound);
        }
        // (3) SCEVUnknown
        if (const SCEVUnknown *unknown = dyn_cast<SCEVUnknown>(scev)) {
            Value *v = unknown->getValue();
            if (PHINode *phi = dyn_cast<PHINode>(v)) {
                bool hasZero = false;
                for (int i = 0, e = phi->getNumIncomingValues(); i != e; ++i) {
                    const SCEV *incomingValueScev = SE.getSCEV(phi->getIncomingValue(i));
                    worklist.push(incomingValueScev);
                    if (incomingValueScev->isZero()
                            || startFromZero.count(incomingValueScev)) {
                        hasZero = true;
                    }
                }
                if (hasZero)    {
                    startFromZero.insert(scev);
                }
            }
        }
    }

    return false;
}


/*
 * Check if "BB" executes backedgeTakenCount times in the loop "L".
 */
bool MhpAnalysis::ThreadJoinAnalysis::executeBackedgeTakenCountTimes(
        const BasicBlock *BB, Loop *L) const {
    // Identify the source BasicBlock of the backedge
    BasicBlock *headerBB = L->getHeader();
    BasicBlock *backedgeSrcBB = NULL;
    int numBackedges = 0;
    for (pred_iterator it = pred_begin(headerBB), ie = pred_end(headerBB);
            it != ie; ++it) {
        BasicBlock *predBB = *it;
        if (!L->contains(predBB))   continue;
        backedgeSrcBB = predBB;
        ++numBackedges;
    }

    // We only handle the loops with a single backedge.
    if (1 != numBackedges)  return false;

    // Check if "BB" dominates "backedgeSrcBB".
    DominatorTree &dt = FunctionPassPool::getDt(F);
    return dt.dominates(BB, backedgeSrcBB);
}


/*
 * Identify the corresponding must-join Instructions or BasicBlocks.
 * TODO: ModRef analysis to ensure there is no modification to the tid object
 *       between spawn and join sites.
 */
void MhpAnalysis::ThreadJoinAnalysis::run() {
    // Get information from spawnSiteInfo
    const Value *tidPtr = spawnSiteInfo->getTidPointer();
    const Loop *spawnSiteLoop = spawnSiteInfo->getLoop();

    // We only handle singleton at this moment.
    PointerAnalysis *pta = mhp->getPTA();
//    if (!rcUtil::isSingletonMemObj(tidPtr, pta))   return;

    // Iterate each join site in the ThreadCallGraph
    for (auto it = mhp->joinSiteInfoMap.begin(), ie =
            mhp->joinSiteInfoMap.end(); it != ie; ++it) {
        SpawnJoinSiteInfo *joinSiteInfo = it->second;

        // Skip if spawn and join sites operate on different threads.
        const Value *joinSiteTidPtr = joinSiteInfo->getTidPointer();
        if (!joinSiteTidPtr || !rcUtil::alias(tidPtr, joinSiteTidPtr, pta))
            continue;

        // We firstly try the SCEV way to intra-procedurally handle
        // spawn-join site matching.
        const Instruction *joinSite = joinSiteInfo->getSite();
        const Function *joinSiteFun = joinSite->getParent()->getParent();
        const Loop *joinSiteLoop = joinSiteInfo->getLoop();
        bool matched = false;
        if (F == joinSiteFun) {
            // Check if the spawn and join sites execute in different loops.
            if (spawnSiteLoop == joinSiteLoop) {
                joinInstsOrBbsMap[joinSiteFun].insert(joinSite);
                matched = true;

            } else {
                const BasicBlock *exitBlock = handleLoopJoin(joinSite);
                if (exitBlock) {
                    joinInstsOrBbsMap[joinSiteFun].insert(exitBlock);
                    matched = true;
                }
            }
        }
        if (matched) {
            joinSiteInfo->setMatchingSpawnSite(spawnSite);
            continue;
        }

        // If the previous attempt fails, we then try the more aggressive method.
        if (rcUtil::isSingletonMemObj(tidPtr, pta)) {
            joinInstsOrBbsMap[joinSiteFun].insert(joinSite);
            matched = true;

        } else {
            if (spawnSiteInfo->match(joinSiteInfo)) {
                const BasicBlock *exitBlock = joinSiteInfo->getExitBasicBlock();
                joinInstsOrBbsMap[joinSiteFun].insert(exitBlock);
                matched = true;
            }
        }
        if (matched) {
            joinSiteInfo->setMatchingSpawnSite(spawnSite);
            continue;
        }
    }

    // Compute blockingCodeInfo
    initBlockingCodeInfo();
}


/*
 * Check if "joinSite" must join the threads created in
 * "this->spawnSite" in loops.
 * If so, return the Exit BasicBlock of the loop where
 * "joinSite" is located; otherwise return NULL.
 */
const BasicBlock *MhpAnalysis::ThreadJoinAnalysis::handleLoopJoin(
        const Instruction *joinSite) {
    const BasicBlock *spawnSiteBB = spawnSite->getParent();
    const BasicBlock *joinSiteBB = joinSite->getParent();

    // Get the tid pointers
    Value *spawnSiteTidPtr = spawnSite->getOperand(0);
    Value *joinSiteTidValue = joinSite->getOperand(0);
    LoadInst *ld = dyn_cast<LoadInst>(joinSiteTidValue);
    if (!ld)    return NULL;
    Value *joinSiteTidPtr = ld->getPointerOperand();

    // Get loops
    Loop *spawnSiteLoop = LI->getLoopFor(spawnSiteBB);
    Loop *joinSiteLoop = LI->getLoopFor(joinSiteBB);
    if (!spawnSiteLoop || !joinSiteLoop)    return NULL;

    // Check execution counts
    const SCEV *spawnSiteExecutionCount = getExecutionCount(spawnSiteBB,
            spawnSiteLoop, false);
    const SCEV *joinSiteExecutionCount = getExecutionCount(joinSiteBB,
            joinSiteLoop, true);
    bool sameExecutionCount = haveSameExecutionCount(spawnSiteExecutionCount,
            joinSiteExecutionCount, spawnSiteLoop);
    if (!sameExecutionCount)    return NULL;

    // Check the value ranges of tid pointers
    const SCEV *spawnSiteTidPtrSCEV = SE.getSCEV(spawnSiteTidPtr);
    const SCEV *joinSiteTidPtrSCEV = SE.getSCEV(joinSiteTidPtr);
    bool sameRange = haveSameStartAndStep(spawnSiteTidPtrSCEV, joinSiteTidPtrSCEV);
    if (!sameRange)   return NULL;

    // Check if the exitingBlock dominates the exitBlock in CFG
    const BasicBlock *exitingBb = joinSiteLoop->getExitingBlock();
    const BasicBlock *exitBb = joinSiteLoop->getExitBlock();
    if (getDt(exitBb->getParent()).dominates(exitingBb, exitBb))    return exitBb;
    return NULL;
}


/*
 * Get the execution count of a BasicBlock in a loop.
 * @param BB the BasicBlock for execution count
 * @param L the loop
 * @param exact If true, return the exact count if possible;
 * otherwise, return the conservative count (ignoring the loop exit
 * caused by the failure of thread spawn).
 * @return the number of the execution count of BB in L
 */
const SCEV *MhpAnalysis::ThreadJoinAnalysis::getExecutionCount(
        const BasicBlock *BB, Loop *L, bool exact) {
    const SCEV *ret = SE.getCouldNotCompute();

    // Get the exact execution count
    if (exact) {
        if (executeBackedgeTakenCountTimes(BB, L)) {
            ret = SE.getBackedgeTakenCount(L);
        }
    }
    // Get the conservative execution count (ignoring the loop exit
    // caused by the failure of thread spawn).
    else {
        // Get exiting BasicBlocks
        SmallVector<Loop::Edge, 4> exitEdges;
        L->getExitEdges(exitEdges);

        // There is a single exit edge
        if (1 == exitEdges.size()) {
            ret = SE.getBackedgeTakenCount(L);
        }
        // There are multiple exit edges
        else {
            // Identify the spawn failure branch.
            const BasicBlock *failureBranch = getSpawnFailureBranch(spawnSite);
            if (!failureBranch)     return ret;
            for (auto it = exitEdges.begin(), ie = exitEdges.end();
                    it != ie; ++it) {
                BasicBlock *exitingBb = const_cast<BasicBlock*>(it->first);
                BasicBlock *exitBb = const_cast<BasicBlock*>(it->second);
                // We do not care the spawn failure branch.
                if (exitBb != failureBranch) {
                    ret = SE.getExitCount(L, exitingBb);
                    break;
                }
            }
        }
    }

    return ret;
}


/*
 * Compute the BlockingCodeInfo for each involved Functions.
 */
void MhpAnalysis::ThreadJoinAnalysis::initBlockingCodeInfo() {
    const JoinInstsOrBbsMap &joinInstsOrBbsMap = getJoinInstsOrBbsMap();
    // Compute BlockingCodeInfo for each Function
    for (auto it = joinInstsOrBbsMap.begin(), ie = joinInstsOrBbsMap.end();
            it != ie; ++it) {
        const Function *F = it->first;
        const ValSet &joinInstsOrBbs = it->second;
        BlockingCodeInfo &blockingCodeInfo = blockingCodeInfoMap[F];
        blockingCodeInfo.reset(joinInstsOrBbs);
    }
}


/*
 * Reset BlockingCodeInfo to identify the blocked CodeSet with blockers.
 * @param blockers it contains the join site Instructions
 * or BasicBlocks that finish the thread join effect in a loop.
 */
void MhpAnalysis::ThreadJoinAnalysis::BlockingCodeInfo::reset(
        const ValSet &blockers) {
    // Identify the dominatees of the blockers sites
    for (ValSet::const_iterator it = blockers.begin(), ie = blockers.end();
            it != ie; ++it) {
        const Value *v = *it;
        resolveBlocker(v);
    }
}


/*
 * Identify the CodeSet blocked by a given Value intra-procedurally.
 * @param v it can be either an Instruction or a BasicBlock
 */
void MhpAnalysis::ThreadJoinAnalysis::BlockingCodeInfo::resolveBlocker(
        const Value *v) {
    const Instruction *I = dyn_cast<Instruction>(v);
    const BasicBlock *BB = I ? I->getParent() : dyn_cast<BasicBlock>(v);
    assert(BB);
    const Function *F = BB->getParent();

    // Collect dominated BasicBlocks
    DominatorTree &dt = getDt(F);
    SmallVector<BasicBlock*, 16> dominateeBbs;
    dt.getDescendants(const_cast<BasicBlock*>(BB), dominateeBbs);
    for (int i = 0, e = dominateeBbs.size(); i != e; ++i) {
        if (I && dominateeBbs[i] == BB)  continue;
        blockedCode.insert(dominateeBbs[i]);
    }

    // Collect dominated Instructions in the same BasicBlock
    if (I) {
        ReachabilityAnalysis::identifyReachableInstructions(I->getNextNode(),
                emptyBlockingCodeInfo, blockedCode);
        blockingBbs.insert(BB);
    }
}


/*
 * Perform reachability analysis for trunk reachable code
 */
void MhpAnalysis::ReachabilityAnalysis::solveTrunkReachability(
        const InstSet &startingPoints, const SCC &scc,
        const ThreadCallGraph *tcg, const Instruction *excludedSpawnSite) {

    // Worklists
    std::stack<const Instruction*> instWorklist;
    std::stack<const Function*> funcWorklist;

    // Perform reachability analysis
    for (auto it = startingPoints.begin(), ie = startingPoints.end();
            it != ie; ++it) {
        instWorklist.push(*it);
    }

    // Identify follower Instruction and BasicBlocks
    while (!instWorklist.empty()) {
        const Instruction *rootInst = instWorklist.top();
        instWorklist.pop();
        const Function *F = rootInst->getParent()->getParent();
        if (trunkReachable.count(F))    continue;
        const BlockingCodeInfo &blockingCodeInfo =
                threadJoinAnalysis->getBlockingCodeInfo(F);
        identifyReachableInstsAndBBs(rootInst, blockingCodeInfo, trunkReachable);
    }

    // Identify Function followers/spawnees (inter-procedurally).
    // Collect all call site Instructions of all Functions in "scc".
    InstSet csInsts;
    scc.collectCallSites(csInsts);

    // Classify the trunk-reachable callees
    FuncSet callees;
    for (InstSet::const_iterator it = csInsts.begin(), ie = csInsts.end();
            it != ie; ++it) {
        const Instruction *csInst = *it;
        if (!trunkReachable.covers(csInst))     continue;
        if (csInst == excludedSpawnSite)        continue;

        callees.clear();
        rcUtil::getCallees<FuncSet>(tcg, csInst, callees);
        for (auto it = callees.begin(), ie = callees.end(); it != ie; ++it) {
            const Function *callee = *it;
            if (trunkReachable.count(callee))   continue;
            funcWorklist.push(callee);
        }
    }

    // Identify followers
    while (!funcWorklist.empty()) {
        const Function* F = funcWorklist.top();
        funcWorklist.pop();

        // Skip un-interested functions
        if (!F || F->isDeclaration()) continue;

        // Skip previously visited node
        if (trunkReachable.count(F) || trunkPartialReachableFuncs.count(F))     continue;

        // If there are blockers in F, use fine-grain analysis mode.
        const BlockingCodeInfo &blockingCodeInfo =
                threadJoinAnalysis->getBlockingCodeInfo(F);
        bool fineGrainMode = false;
        if (!blockingCodeInfo.empty()) {
            fineGrainMode = true;
            const Instruction *rootInst = &F->getEntryBlock().front();
            identifyReachableInstsAndBBs(rootInst, blockingCodeInfo, trunkReachable);
            trunkPartialReachableFuncs.insert(F);
        } else {
            trunkReachable.insert(F);
        }

        // Add feasible callees of F into worklist
        for (const_inst_iterator it = inst_begin(F), ie = inst_end(F);
                it != ie; ++it) {
            const Instruction *I = &*it;
            // Skip non-callsite Instructions
            if (!isa<CallInst>(I) && !isa<InvokeInst>(I))   continue;

            // If we are in the fine-grain mode, skip the blocked Instruction
            if (fineGrainMode && !trunkReachable.covers(I))     continue;

            // Skip the excluded spawn sites
            if (I == excludedSpawnSite)       continue;

            // Add all callees into worklist
            callees.clear();
            rcUtil::getCallees<FuncSet>(tcg, I, callees);
            for (auto it = callees.begin(), ie = callees.end(); it != ie; ++it) {
                const Function *callee = *it;
                if (callee->isDeclaration() || trunkReachable.count(callee))   continue;
                funcWorklist.push(callee);
            }
        }
    }
}


/*
 * Perform reachability analysis for branch reachable code
 * for refinement analysis
 */
void MhpAnalysis::ReachabilityAnalysis::solveBranchReachability(
        const Instruction *spawnSite, const ThreadCallGraph *tcg,
        const ThreadJoinAnalysis *threadJoinAnalysis) {
    // Check if we have already solved the barnch reachability.
    if (!branchReachable.empty())   return;

    // Identify spawnees
    FuncSet callees;
    rcUtil::getCallees<FuncSet>(tcg, spawnSite, callees);

    // Prepare worklist
    std::stack<const llvm::Function*> funcWorklist;
    for (auto it = callees.begin(), ie = callees.end(); it != ie; ++it) {
        const Function *callee = *it;
        if (!callee || callee->isDeclaration())     continue;
        funcWorklist.push(callee);
    }

    // Identify reachability
    while (!funcWorklist.empty()) {
        const Function* F = funcWorklist.top();
        funcWorklist.pop();

        // Skip un-interested functions
        if (!F || F->isDeclaration()) continue;

        // Skip previously visited node
        if (branchReachable.count(F))   continue;

        // If there are blockers in F, use fine-grain analysis mode.
        const BlockingCodeInfo &blockingCodeInfo =
                threadJoinAnalysis->getBlockingCodeInfo(F);
        bool fineGrainMode = false;
        if (!blockingCodeInfo.empty()) {
            fineGrainMode = true;
            const Instruction *rootInst = &F->getEntryBlock().front();
            identifyReachableInstsAndBBs(rootInst, blockingCodeInfo, branchReachable);
        } else {
            branchReachable.insert(F);
        }

        // Add feasible callees of F into worklist
        for (const_inst_iterator it = inst_begin(F), ie = inst_end(F);
                it != ie; ++it) {
            const Instruction *I = &*it;
            // Skip non-callsite Instructions
            if (!isa<CallInst>(I) && !isa<InvokeInst>(I))   continue;

            // If we are in the fine-grain mode, skip the blocked Instruction
            if (fineGrainMode && !branchReachable.covers(I))     continue;

            // Add all callees into worklist
            callees.clear();
            rcUtil::getCallees<FuncSet>(tcg, I, callees);
            for (auto it = callees.begin(), ie = callees.end(); it != ie; ++it) {
                const Function *callee = *it;
                if (callee->isDeclaration() || branchReachable.count(callee))   continue;
                funcWorklist.push(callee);
            }
        }
    }
}


/*
 * Perform reachability analysis for branch reachable code
 * for regular analysis.
 */
void MhpAnalysis::ReachabilityAnalysis::solveBranchReachability(
        const Instruction *spawnSite, const ThreadCallGraph *tcg,
        CodeSet &branchReachable) {
    // Check if we have already solved the branch reachability.
    if (!branchReachable.empty())   return;

    FuncSet callees;
    rcUtil::getCallees<FuncSet>(tcg, spawnSite, callees);
    identifyReachableFunctions(callees, branchReachable, tcg);
}


/*
 * Identify the forward reachable Functions from a set of starting Functions
 * @param start the Functions to start traversal
 * @param reachable the output to record the results of reachable Functions
 * @param cg the call graph used for traversal
 */
void MhpAnalysis::ReachabilityAnalysis::identifyReachableFunctions(
        const FuncSet &start, CodeSet &reachable, const ThreadCallGraph *cg) {

    // Set the traversal roots
    stack<const Function*> worklist;
    for (FuncSet::const_iterator it = start.begin(), ie = start.end(); it != ie;
            ++it) {
        worklist.push(*it);
    }

    // Traverse cg
    while (!worklist.empty()) {
        const Function* F = worklist.top();
        worklist.pop();

        // Skip un-interested functions
        if (!F || F->isDeclaration()) continue;

        // Skip previously visited node
        if (reachable.count(F))   continue;

        reachable.insert(F);

        // Visit all callees of F in the CallGraph
        PTACallGraphNode* cgNode = cg->getCallGraphNode(F);
        for (PTACallGraphNode::const_iterator it = cgNode->OutEdgeBegin(),
                ie = cgNode->OutEdgeEnd(); it != ie; ++it) {
            // Put callee into worklist
            const PTACallGraphEdge *edge = *it;
            const Function* callee = edge->getDstNode()->getFunction();
            if (reachable.count(callee))   continue;
            worklist.push(callee);
        }
    }
}


/*
 * Identify the forward reachable code from a given Instruction.
 * @param I the Instruction to start traversal (with itself included).
 * @param blockingCodeInfo the BlockingCodeInfo under which the traversal stops
 * @param reachable the output to record the reachable code.
 */
void MhpAnalysis::ReachabilityAnalysis::identifyReachableInstructions(
        const Instruction *I, const BlockingCodeInfo &blockingCodeInfo,
        CodeSet &reachable) {
    if (!I)     return;

    while (!I->isTerminator()) {
        if (MhpAnalysis::isInterstedInst(I)) {
            if (!blockingCodeInfo.isBlocked(I))
                reachable.insert(I);
        }
        I = I->getNextNode();
    }
}


/*
 * Perform intra-procedural reachability analysis.
 * @param rootInst the root Instruction to start traversal;
 * it is excluded before the traversal happens,
 * but it may be included later if there is a cycle.
 * @param blockingCodeInfo the BlockingCodeInfo under which the traversal stops
 * @param reachable the output to record the results of reachable code.
 */
void MhpAnalysis::ReachabilityAnalysis::identifyReachableInstsAndBBs(
        const Instruction *rootInst, const BlockingCodeInfo &blockingCodeInfo,
        CodeSet &reachable) {
    // If "rootInst" has already been reachable, there is nothing to discover further.
    if (reachable.covers(rootInst)) return;

    // Collect the Instructions after "rootInst" in the BasicBlock.
    const Instruction *I = rootInst->getNextNode();
    identifyReachableInstructions(I, blockingCodeInfo, reachable);

    // If "rootBB" contains a blocking Instruction, then we do nothing further.
    const BasicBlock *rootBB = rootInst->getParent();
    if (blockingCodeInfo.isBlockingBasicBlock(rootBB))  return;

    // Perform forward traversal in the CFG.
    // Initialize worklist with all successors of rootBB in the CFG
    stack<const BasicBlock*> worklist;
    for (succ_const_iterator it = succ_begin(rootBB), ie = succ_end(
            rootBB); it != ie; ++it) {
        worklist.push(*it);
    }

    // Traverse CFG
    while (!worklist.empty()){
        const BasicBlock* BB = worklist.top();
        worklist.pop();

        // Skip previously visited node
        if (reachable.count(BB) || blockingCodeInfo.isBlocked(BB))   continue;

        // If BB contains a blocking Instruction, then we just
        // collect the Instructions before that blocking Instruction.
        if (blockingCodeInfo.isBlockingBasicBlock(BB)) {
            const Instruction *I = BB->getFirstNonPHIOrDbg();
            identifyReachableInstructions(I, blockingCodeInfo, reachable);
            continue;
        }

        // Put "BB" into "reachable"
        reachable.insert(BB);

        // Visit all successors of BB in the CFG
        for (succ_const_iterator it = succ_begin(BB), ie = succ_end(BB);
                it != ie; ++it) {
            worklist.push(*it);
        }
    }
}


MhpAnalysis::ThreadJoinAnalysis::BlockingCodeInfo
MhpAnalysis::ThreadJoinAnalysis::BlockingCodeInfo::emptyBlockingCodeInfo;

