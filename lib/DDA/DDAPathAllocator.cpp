/*
 * DDAPathAllocator.cpp
 *
 *  Created on: Sep 9, 2014
 *      Author: Yulei Sui
 */

#include "DDA/DDAPathAllocator.h"

using namespace llvm;
using namespace analysisUtil;


/**
 * Allocate conditions for a basic block and propagate its condition to its successors.
 */
void DDAPathAllocator::allocateForBB(const BasicBlock & bb)
{

    u32_t pred_number = getBBPredecessorNum(&bb);

    // if successor number greater than 1, allocate new decision variable for successors
    if(pred_number > 1) {

        //allocate log2(num_succ) decision variables
        double num = log(pred_number)/log(2);
        u32_t bit_num = (u32_t)ceil(num);
        u32_t pred_index = 0;
        std::vector<Condition*> condVec;
        for(u32_t i = 0 ; i < bit_num; i++) {
            condVec.push_back(newCond(&bb.front()));
        }

        // iterate each successor
        for (const_pred_iterator pred_it = pred_begin(&bb);
                pred_it != pred_end(&bb);
                pred_it++, pred_index++) {

            const BasicBlock* pred = *pred_it;

            Condition* path_cond = getTrueCond();

            ///TODO: handle BranchInst and SwitchInst individually here!!

            // for each predecessor decide its bit representation
            // decide whether each bit of succ_index is 1 or 0, if (three predecessor) pred_index is 000 then use C1^C2^C3
            // if 001 use C1^C2^negC3
            for(u32_t j = 0 ; j < bit_num; j++) {
                //test each bit of this successor's index (binary representation)
                u32_t tool = 0x01 << j;
                if(tool & pred_index) {
                    path_cond = condAnd(path_cond, condVec.at(j));
                }
                else {
                    path_cond = condAnd(path_cond, (condNeg(condVec.at(j))));
                }
            }
            setBranchCond(&bb,pred,path_cond);
        }

    }
}

DDAPathAllocator::Condition* DDAPathAllocator::getCFGuard(const llvm::BasicBlock* bb, CFGEdgeSet& edgeSet) {

    DominatorTree* dt = getDT(bb->getParent());
    if(dt->dominates(getEndBB(),bb))
        return getTrueCond();

    BBPairToConditionMap::const_iterator it = _cachedCond.find(std::make_pair(bb,getEndBB()));
    if(it!=_cachedCond.end())
        return it->second;

    Condition* guard = getFalseCond();

    for (const_pred_iterator pred_it = pred_begin(bb); pred_it != pred_end(bb); pred_it++) {
        const BasicBlock* pred = *pred_it;

        if(!edgeSet.hasCFGEdge(bb,pred)) {

            Condition* brCond;
            /// drop the second test instance
            if(edgeSet.hasOutgoingCFGEdge(bb))
                brCond = getTrueCond();
            /// get the first test instance
            else
                brCond = getBranchCond(bb, pred);

            edgeSet.addCFGEdge(bb,pred);
            brCond = condAnd(brCond,getCFGuard(pred,edgeSet));
            edgeSet.rmCFGEdge(bb,pred);
            guard = condOr(guard,brCond);
        }
    }

    _cachedCond[std::make_pair(bb,getEndBB())] = guard;

    return guard;
}

DDAPathAllocator::Condition* DDAPathAllocator::ComputeIntraVFGGuard(const llvm::BasicBlock* srcBB, const llvm::BasicBlock* dstBB) {

    assert(srcBB->getParent() == dstBB->getParent() && "two basic blocks are not in the same function??");

    setEndBB(srcBB);

    cfgEdges.clear();
    _cachedCond.clear();

    return getCFGuard(dstBB,cfgEdges);
}

/*!
 * Compute calling inter-procedural guards between two SVFGNodes (from caller to callee)
 * src --c1--> callBB --true--> funEntryBB --c2--> dst
 * the InterCallVFGGuard is c1 ^ c2
 */
DDAPathAllocator::Condition* DDAPathAllocator::ComputeInterCallVFGGuard(const llvm::BasicBlock* srcBB, const llvm::BasicBlock* dstBB, const BasicBlock* callBB) {
    const BasicBlock* funEntryBB = &dstBB->getParent()->getEntryBlock();

    Condition* c1 = ComputeIntraVFGGuard(srcBB,callBB);
    Condition* c2 = ComputeIntraVFGGuard(funEntryBB,dstBB);
    return condAnd(c1,c2);
}

/*!
 * Compute return inter-procedural guards between two SVFGNodes (from callee to caller)
 * src --c1--> funExitBB --true--> retBB --c2--> dst
 * the InterRetVFGGuard is c1 ^ c2
 */
DDAPathAllocator::Condition* DDAPathAllocator::ComputeInterRetVFGGuard(const llvm::BasicBlock*  srcBB, const llvm::BasicBlock*  dstBB, const BasicBlock* retBB) {
    const BasicBlock* funExitBB = getFunExitBB(srcBB->getParent());

    Condition* c1 = ComputeIntraVFGGuard(srcBB,funExitBB);
    Condition* c2 = ComputeIntraVFGGuard(retBB,dstBB);
    return condAnd(c1,c2);
}


/*!
 * Get complement phi condition
 * e.g., B0: dstBB; B1:incomingBB; B2:complementBB
 * Assume B0 (phi node) is the successor of both B1 and B2.
 * If B1 dominates B2, and B0 not dominate B2 then condition from B1-->B0 = neg(B2-->B0)^(B1-->B0)
 */
DDAPathAllocator::Condition* DDAPathAllocator::getPHIComplementCond(const BasicBlock* BB1, const BasicBlock* BB2, const BasicBlock* BB0) {
    assert(BB1 && BB2 && "expect NULL BB here!");

    DominatorTree* dt = getDT(BB1->getParent());
    /// avoid both BB0 and BB1 dominate BB2 (e.g., while loop), then BB2 is not necessaryly a complement BB
    if(dt->dominates(BB1,BB2) && !dt->dominates(BB0,BB2)) {
        Condition* cond =  ComputeIntraVFGGuard(BB2,BB0);
        return condNeg(cond);
    }

    return trueCond();
}

/*!
 * Get a branch condition
 */
DDAPathAllocator::Condition* DDAPathAllocator::getBranchCond(const llvm::BasicBlock * bb, const llvm::BasicBlock *pred) const {
    if(bb->getSinglePredecessor() == pred) {
        return getTrueCond();
    }
    else {
        u32_t pos = getBBPredecessorPos(bb,pred);
        BBCondMap::const_iterator it = bbConds.find(bb);
        assert(it!=bbConds.end() && "basic block does not have branch and conditions??");
        CondPosMap::const_iterator cit = it->second.find(pos);
        assert(cit!=it->second.end() && "no condition on the branch??");
        return cit->second;
    }
}

/*!
 * Set branch condition
 */
void DDAPathAllocator::setBranchCond(const llvm::BasicBlock *bb, const llvm::BasicBlock *pred, Condition* cond) {
    /// we only care about basic blocks have more than one predecessors
    assert(bb->getSinglePredecessor() == NULL && "not more than one predecessor??");
    u32_t pos = getBBPredecessorPos(bb,pred);
    CondPosMap& condPosMap = bbConds[bb];

    /// FIXME: llvm getNumSuccessors allows duplicated block in the successors, it makes this assertion fail
    /// In this case we may waste a condition allocation, because the overwrite of the previous cond
    ///assert(condPosMap.find(pos) == condPosMap.end() && "this branch has already been set ");
    condPosMap[pos] = cond;
}
