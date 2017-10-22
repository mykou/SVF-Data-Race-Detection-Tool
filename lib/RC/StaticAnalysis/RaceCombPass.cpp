/*
 * RaceCombPass.cpp
 *
 *  Created on: 17/10/2014
 *      Author: Ding Ye
 */


#include "RaceComb.h"
#include "RC/RaceCombPass.h"
#include "Util/AnalysisUtil.h"
#include <Util/BreakConstantExpr.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Analysis/ScalarEvolution.h>

using namespace llvm;
using namespace analysisUtil;

char RaceCombPass::ID = 0;

static RegisterPass<RaceCombPass> RaceCombPass("racecomb",
        "RaceComb Data Race Detector");


/*
 * Main analysis of RaceComb
 */
void RaceCombPass::analyze(llvm::Module &M) {

    outs() << pasMsg(getPassName())
           << " is running on the module \""
           << M.getModuleIdentifier() << "\"...\n\n";

    RaceComb *rc = new RaceComb();

    rc->init(&M, this);

    rc->analyze();

    rc->release();

//    delete rc;

    return;
}


/*
 * Analysis usage
 */
void RaceCombPass::getAnalysisUsage(llvm::AnalysisUsage& au) const {
    // do not intend to change the IR in this pass,
    au.setPreservesAll();
    au.addRequired<MergeFunctionRets>();
    au.addRequired<llvm::TargetLibraryInfoWrapperPass>();
    au.addRequired<llvm::AssumptionCacheTracker>();
    au.addRequiredTransitive<llvm::LoopInfoWrapperPass>();

}




