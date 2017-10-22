/*
 * BarrierAnalysis.cpp
 *
 *  Created on: 05/07/2016
 *      Author: dye
 */

#include "BarrierAnalysis.h"
#include "Util/ThreadCallGraph.h"

using namespace llvm;
using namespace std;


BarrierSummary::Barrier2EffectMap BarrierSummary::barrier2EffectMap;


/*
 * Initialization
 * @param cg The callgraph.
 * @param tcg The thread callgraph.
 * @param pta The pointer analysis.
 */
void BarrierAnalysis::init(PTACallGraph *cg, ThreadCallGraph *tcg,
        BVDataPTAImpl *pta) {

    this->tcg = tcg;

    SUPER::init(cg, tcg, pta);
}


/*
 * \brief Perform the barrier analysis.
 * It contains two phases:
 * (1) a bottom-up phase, and
 * (2) a top-down phase.
 */
void BarrierAnalysis::analyze() {

    SUPER::analyze(SUPER::AO_BOTTOM_UP);

    SUPER::analyze(SUPER::AO_TOP_DOWN);

}


/*
 * Release the resources
 */
void BarrierAnalysis::release() {

}


/*
 * The interface of the analysis to answer if two given
 * Instructions must be separated by any barrier.
 */
bool BarrierAnalysis::separatedByBarrier(const llvm::Instruction *I1,
        const llvm::Instruction *I2) const {
    auto &barrier2EffectMap = BarrierSummary::getBarrier2EffectMap();
    for (auto it = barrier2EffectMap.begin(), ie = barrier2EffectMap.end();
            it != ie; ++it) {
        auto &effect =(barrier2EffectMap.begin())->second;

//        outs() << rcUtil::getSourceLoc(I1) << "\t"
//                << effect.beforeCode.covers(I1) << " "
//                << effect.afterCode.covers(I1) << "\n";
//        outs() << rcUtil::getSourceLoc(I2) << "\t"
//                << effect.beforeCode.covers(I2) << " "
//                << effect.afterCode.covers(I2) << "\n";

        bool I1Before = effect.beforeCode.covers(I1);
        bool I2Before = effect.beforeCode.covers(I2);
        if (I1Before == I2Before)   continue;

        bool I1After = effect.afterCode.covers(I1);
        if (I1Before == I1After)    continue;

        bool I2After = effect.afterCode.covers(I2);
        if (I2Before == I2After)    continue;

        return true;
    }

    return false;
}


/*
 * Generic Function visitor of inter-procedural analysis.
 */
void BarrierAnalysis::visitFunction(const Function *F) {
    // Call the corresponding function visitor as per the analysis order.
    switch(getCurrentAnalysisOrder()) {
    case SUPER::AO_BOTTOM_UP: {
        buttomUpVisitor(F);
        break;
    }
    case SUPER::AO_TOP_DOWN: {
        topDownVisitor(F);
        break;
    }
    default:
        break;
    }
}


BarrierAnalysis::FunctionAnalyzerBase::UniversalInst2BarriersMap
BarrierAnalysis::FunctionAnalyzerBase::universalBarrierSites;


/*
 * Perform the bottom-up analysis.
 */
void BarrierAnalysis::BottomUpAnalyzer::run() {

    identifyInterestingStuffs();

    // If the Function has a ReturnInst, we summarize its side effects.
    if (retInst) {
        computeMustWaitedBarriers();
    }
}


/*
 * Identify some stuffs including return Instruction and
 * barrier-waiting effects for later analysis.
 */
void BarrierAnalysis::BottomUpAnalyzer::identifyInterestingStuffs() {
    const PTACallGraph *cg = ba->getCallGraph();
    retInst = ba->getRet(F);

    // Iterate every Instruction of "F" to handle interested ones.
    for (const_inst_iterator it = inst_begin(F), ie = inst_end(F); it != ie;
            ++it) {
        const Instruction *I = &*it;

        // Identify direct barrier-waiting operations.
        if (analysisUtil::isBarrierWaitCall(I))  {
            barrierSites[I].insert(rcUtil::getBarrierPtr(I));
        }

        // Handle call sites.
        if (!cg->hasCallGraphEdge(I))    continue;
        for (PTACallGraph::CallGraphEdgeSet::const_iterator it =
                cg->getCallEdgeBegin(I), ie = cg->getCallEdgeEnd(I); it != ie;
                ++it) {
            const PTACallGraphEdge *edge = *it;
            const Function *callee = edge->getDstNode()->getFunction();
            if (callee->isDeclaration())    continue;

            const BarrierSummary *calleeSummary = ba->getSccSummary(callee);
            if (!calleeSummary)     continue;

            // Identify barrier-waiting side effects from callees.
            if (calleeSummary->barrierBegin() != calleeSummary->barrierEnd()) {
                barrierSites[I].insert(calleeSummary->barrierBegin(),
                        calleeSummary->barrierEnd());
            }
        }
    }
}


/*
 * Compute the must-waited barriers of the current Function.
 */
void BarrierAnalysis::BottomUpAnalyzer::computeMustWaitedBarriers() {
    // There must be a return Instruction.
    assert(retInst);

    // Analyze the identified barrier sites in the CFG.
    DominatorTree &dt = ba->getDt(F);
    for (Inst2BarriersMap::const_iterator it = barrierSites.begin(),
            ie = barrierSites.end(); it != ie; ++it) {
        const Instruction *waitSite = it->first;
        const ValSet &barriers = it->second;

        // If csInst holds must-wait property,
        // it needs to dominate the return node in CFG
        if (!dt.dominates(waitSite, retInst))   continue;

        mustWaitedBarriers.insert(barriers.begin(), barriers.end());
    }
}


/*
 * Perform the top-down analysis.
 */
void BarrierAnalysis::TopDownAnalyzer::run() {

    // Handle external barriers
    processExternalBarriers();

    // Handle internal barriers
    processInternalBarriers();

}


/*
 * Compute BarrierEffect for relevant external barriers.
 */
void BarrierAnalysis::TopDownAnalyzer::processExternalBarriers() {
    // The external side effect of barriers
    ValSet extBeforeBarriers;
    ValSet extAfterBarriers;

    // Collect all valid call sites calling F
    PTACallGraphEdge::CallInstSet csInsts;
    PTACallGraph *cg = ba->getCallGraph();
    rcUtil::getAllValidCallSitesInvokingCallee(cg, F, csInsts);

    // Get the Barrier2EffectMap
    const BarrierSummary::Barrier2EffectMap &barrier2Effect =
            BarrierSummary::getBarrier2EffectMap();

    // Iterate every call site to analyze the side effect
    bool firstIteration = true;
    ValSet beforeBarriers;
    ValSet afterBarriers;
    for (PTACallGraphEdge::CallInstSet::const_iterator it = csInsts.begin(),
            ie = csInsts.end(); it != ie; ++it) {

        // Get the side effect from csInst
        const Instruction *csInst = *it;
        beforeBarriers.clear();
        afterBarriers.clear();
        for (auto it = barrier2Effect.begin(), ie = barrier2Effect.end();
                it != ie; ++it) {
            const Value *barrier = it->first;
            const BarrierEffect &effect = it->second;
            if (effect.beforeCode.covers(csInst)) {
                beforeBarriers.insert(barrier);
            }
            if (effect.afterCode.covers(csInst)) {
                afterBarriers.insert(barrier);
            }
        }

        // Apply the side effect to the Function
        if (firstIteration) {
            extBeforeBarriers = beforeBarriers;
            extAfterBarriers = afterBarriers;
            firstIteration = false;
        } else {
            extBeforeBarriers = rcUtil::getIntersection(extBeforeBarriers,
                    beforeBarriers);
            extAfterBarriers = rcUtil::getIntersection(extAfterBarriers,
                    afterBarriers);
        }
    }

    // Add the affected code to the result
    for (auto it = extBeforeBarriers.begin(), ie = extBeforeBarriers.end();
            it != ie; ++it) {
        const Value *barrier = *it;
        BarrierEffect &effect = BarrierSummary::getOrAddBarrierEffect(barrier);
        effect.beforeCode.insert(F);
    }
    for (auto it = extAfterBarriers.begin(), ie = extAfterBarriers.end();
            it != ie; ++it) {
        const Value *barrier = *it;
        BarrierEffect &effect = BarrierSummary::getOrAddBarrierEffect(barrier);
        effect.afterCode.insert(F);
    }
}


/*
 * Compute BarrierEffect for relevant internal barriers.
 */
void BarrierAnalysis::TopDownAnalyzer::processInternalBarriers() {
    // Handle barrier wait sites, which split the Function.
     for (Inst2BarriersMap::const_iterator it = barrierSites.begin(),
             ie = barrierSites.end(); it != ie; ++it) {
         const Instruction *waitSite = it->first;
         const ValSet &barriers = it->second;
         for (auto it = barriers.begin(), ie = barriers.end(); it != ie;
                 ++it) {
             const Value *barrier = *it;
             processBarrierEffect(barrier, waitSite);
         }
     }
}


/*
 * Compute the BarrierEffect for a barrier object at a wait site.
 * The result is recorded in BarrierSummary.
 * @param barrier the pointer to the barrier object
 * @param waitSite the barrier-waiting site
 */
void BarrierAnalysis::TopDownAnalyzer::processBarrierEffect(
        const Value *barrier, const Instruction *waitSite) {

    DominatorTree &dt = getDt(F);
    PostDominatorTree &pdt = getPdt(F);
    BarrierEffect &effect = BarrierSummary::getOrAddBarrierEffect(barrier);

    // Identify dominatees/post-dominatees intra-procedurally
    identifyDominatedInstsAndBBs(waitSite, dt, effect.afterCode);
    identifyPostDominatedInstsAndBBs(waitSite, pdt, effect.beforeCode);

}


/*
 * Perform intra-procedural dominant analysis.
 * @param rootInst the root Instruction to start traversal;
 * it is excluded before the traversal happens,
 * but it may be included later if there is a cycle.
 * @param dt the DominatorTree
 * @param dominatee the output of the dominated code.
 */
void BarrierAnalysis::TopDownAnalyzer::identifyDominatedInstsAndBBs(
        const Instruction *rootInst, DominatorTree &dt, CodeSet &dominatee) {
    // If "rootInst" has already been covered,
    // then there is nothing to discover further.
    if (dominatee.covers(rootInst))     return;

    // Collect the Instructions after "rootInst" in the BasicBlock.
    const Instruction *I = rootInst->getNextNode();
    identifyForwardReachableInstructions(I, dominatee);

    // Perform forward traversal in the CFG.
    // Initialize worklist with all dominated successors of rootBB in the CFG
    stack<const BasicBlock*> worklist;
    const BasicBlock *rootBB = rootInst->getParent();
    for (succ_const_iterator it = succ_begin(rootBB), ie = succ_end(
            rootBB); it != ie; ++it) {
        const BasicBlock *bb = *it;
        if (dt.dominates(rootBB, bb)) {
            worklist.push(*it);
        }
    }

    // Traverse CFG
    while (!worklist.empty()){
        const BasicBlock* BB = worklist.top();
        worklist.pop();

        // Skip previously visited node
        if (dominatee.count(BB))   continue;

        // Put "BB" into "reachable"
        dominatee.insert(BB);

        // Visit all successors of BB in the CFG
        for (succ_const_iterator it = succ_begin(BB), ie = succ_end(BB);
                it != ie; ++it) {
            const BasicBlock *bb = *it;
            if (dt.dominates(rootBB, bb)) {
                worklist.push(*it);
            }
        }
    }
}


/*
 * Perform intra-procedural post-dominant analysis.
 * @param rootInst the root Instruction to start traversal;
 * it is excluded before the traversal happens,
 * but it may be included later if there is a cycle.
 * @param pdt the PostDominatorTree
 * @param postDominatee the output of the post-dominated code.
 */
void BarrierAnalysis::TopDownAnalyzer::identifyPostDominatedInstsAndBBs(
        const Instruction *rootInst, PostDominatorTree &pdt,
        CodeSet &postDominatee) {
    // If "rootInst" has already been covered, there is nothing to discover further.
    if (postDominatee.covers(rootInst))     return;

    // Collect the Instructions before "rootInst" in the BasicBlock.
    const Instruction *I = rootInst->getPrevNode();
    identifyBackwardReachableInstructions(I, postDominatee);

    // Perform forward traversal in the CFG.
    // Initialize worklist with all post-dominated predecessors of rootBB in the CFG
    stack<const BasicBlock*> worklist;
    const BasicBlock *rootBB = rootInst->getParent();
    for (const_pred_iterator it = pred_begin(rootBB), ie = pred_end(
            rootBB); it != ie; ++it) {
        const BasicBlock *bb = *it;
        if (pdt.dominates(rootBB, bb)) {
            worklist.push(*it);
        }
    }

    // Traverse CFG
    while (!worklist.empty()){
        const BasicBlock* BB = worklist.top();
        worklist.pop();

        // Skip previously visited node
        if (postDominatee.count(BB))   continue;

        // Put "BB" into "reachable"
        postDominatee.insert(BB);

        // Visit all predecessors of BB in the CFG
        for (const_pred_iterator it = pred_begin(BB), ie = pred_end(BB);
                it != ie; ++it) {
            const BasicBlock *bb = *it;
            if (pdt.dominates(rootBB, bb)) {
                worklist.push(*it);
            }
        }
    }
}


/*
 * Identify intra-procedural forward reachable code.
 */
void BarrierAnalysis::TopDownAnalyzer::identifyForwardReachableInstructions(
        const Instruction *I, CodeSet &reachable) {
    while (!I->isTerminator()) {
        if (BarrierAnalysis::isInterstedInst(I)) {
            reachable.insert(I);
        }
        I = I->getNextNode();
    }
}


/*
 * Identify intra-procedural backward reachable code.
 */
void BarrierAnalysis::TopDownAnalyzer::identifyBackwardReachableInstructions(
        const Instruction *I, CodeSet &reachable) {
    while (I != nullptr) {
        if (BarrierAnalysis::isInterstedInst(I)) {
            reachable.insert(I);
        }
        I = I->getPrevNode();
    }
}


/*
 * In the bottom-up phase, the visitor summarizes the
 * barrier objects that must be waited by the Function.
 */
void BarrierAnalysis::buttomUpVisitor(const Function *F) {
    BottomUpAnalyzer bua(this, F);
    bua.run();
    const ValSet &barriers = bua.getMustWaitedBarriers();
    BarrierSummary &summary = getOrAddSccSummary(F);
    summary.addBarriers(barriers);
}


/*
 * In the top-down phase, the visitor compute the BarrierEffect
 * for all barrier objects involved.
 */
void BarrierAnalysis::topDownVisitor(const Function *F) {
    TopDownAnalyzer tda(this, F);
    tda.run();
}

