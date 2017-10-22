/*
 * SaberInstr.cpp
 *
 *  Created on: May 6, 2014
 *      Author: Yulei Sui
 */

#include "Instrumentation/SaberInstr.h"
#include "Instrumentation/SaberIntrinsics.h"
using namespace llvm;

/*!
 *
 */
bool SaberInstrumentor::isInterestedTransFun(llvm::Function* fun) {
    return false;
}

/*!
 * Instrumentation according to annotation
 */
void SaberInstrumentor::instrumentSrcSnk(CallSite cs) {

    Instruction* inst = cs.getInstruction();
    Instruction* nextInst = getNextInst(inst);

    Value* accessSize = ConstantInt::get(Type::getInt64Ty(inst->getContext()), 0, false);
    /// TODO: if this is wrapper source, we need to find its malloc allocation site
    if (ann.hasSBSourceFlag(inst)) {

        if (cs.arg_size() == 1) {
            if (Value* ag = cs.getArgument(0)) {

                assert(ag->getType()->isIntegerTy()
                       && "first argument not an integer type??");

                accessSize = ag;
                /// if this is not a interger type with 64 bits width
                if (ag->getType()->isIntegerTy(64) == false)
                    accessSize = new SExtInst(ag, Type::getInt64Ty(ag->getContext()), "cast2sext", nextInst);
            }
        }

        Value* ptr = castToVoidPtr(inst, inst);
        Value* args[] = { ptr, accessSize };
        SaberMallocInst::Create(args, nextInst);
    }
}
