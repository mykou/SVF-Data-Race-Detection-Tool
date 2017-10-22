/*
 * BoundsInstrumentor.cpp
 *
 *  Created on: Jun 13, 2013
 *      Author: Yulei Sui
 */

#include "Instrumentation/BoundsInstr.h"
#include "Instrumentation/IntrinsicInst.h"
#include "Util/AnalysisUtil.h"

#include <llvm/IR/InstIterator.h>

using namespace llvm;
using namespace std;
using namespace analysisUtil;

/*!
 *
 */
void BoundsInstrumentor::instrument() {
    // inject metadata manipulated intrisic functions
    visit(getModule());
    // inject bound check intrisic functions
    injectBoundCheck();
}

/*!
 *
 */
bool BoundsInstrumentor::isInterestedTransFun(Function* fun) {
    if (analysisUtil::isExtCall(fun)) {
        if (fun->getName() == "malloc")
            return true;
    }

    return false;
}

/*!
 * Instrument Alloca instruction
 */
void BoundsInstrumentor::visitAllocaInst(AllocaInst &inst) {

    DBOUT(DInstrument, errs() << "visit allocation " << inst << "\n");

    Instruction* curInst = &inst;
    Instruction* nextInst = getNextInst(&inst);

    // For alloca instruction, base is bitcast of alloca, bound is bitcast of alloca_ptr + 1

    Value* ptr = castToVoidPtr(&inst, inst.getName(), curInst);

    Value* ptr_base = castToVoidPtr(&inst, curInst);

    Value* intBound = getBoundVal(&inst);

    GetElementPtrInst* gep = GetElementPtrInst::Create(nullptr,&inst, intBound, "mtmp",
                             nextInst);

    Value* ptr_bound = castToVoidPtr(gep, curInst);

    Value* args[] = { ptr, ptr_base, ptr_bound };

    MDInitialInst::Create(args, nextInst);

    addInjectedFun(IntrinsicsPass::MetadataInitialInst);
}

/*!
 * Instrument Phi instruction
 */
void BoundsInstrumentor::visitPHINode(PHINode &inst) {

}

/*!
 * Instrument Store instruction
 */
void BoundsInstrumentor::visitStoreInst(StoreInst &inst) {

    if (!isa<PointerType>(inst.getValueOperand()->getType()))
        return;

    DBOUT(DInstrument, errs() << "visit store " << inst << "\n");

    Instruction* curInst = &inst;
    Instruction* nextInst = getNextInst(&inst);

    Value* vptrStorePtr = castToVoidPtr(inst.getPointerOperand(), curInst);

    Value* vptrStoreVal = castToVoidPtr(inst.getValueOperand(), curInst);

    Value* args[] = { vptrStoreVal, vptrStorePtr };

    MetadataStoreInst::Create(args, nextInst);

    addInjectedFun(IntrinsicsPass::MetadataStoreInst);
}

/*!
 * Instrument Load instruction
 */
void BoundsInstrumentor::visitLoadInst(LoadInst &inst) {

    if (!isa<PointerType>(inst.getType()))
        return;

    DBOUT(DInstrument, errs() << "visit load " << inst << "\n");

    Instruction* curInst = &inst;
    Instruction* nextInst = getNextInst(&inst);

    Value* vptrLoadPtr = castToVoidPtr(inst.getPointerOperand(), curInst);

    Value* vptrLoadRes = castToVoidPtr(&inst, curInst);

    Value* MDLdArgs[] = { vptrLoadPtr, vptrLoadRes };

    MetadataLoadInst::Create(MDLdArgs, nextInst);

    addInjectedFun(IntrinsicsPass::MetadataLoadInst);
}

/*!
 * Instrument Gep instruction
 */
void BoundsInstrumentor::visitGetElementPtrInst(GetElementPtrInst &inst) {

    assert(isa<PointerType>(inst.getType()));

    DBOUT(DInstrument, errs() << "visit Gep " << inst << "\n");
    Instruction* curInst = &inst;
    Instruction* nextInst = getNextInst(&inst);

    Value* args[] = { castToVoidPtr(inst.getPointerOperand(), curInst),
                      castToVoidPtr(&inst, curInst)
                    };

    MetadataPropageteInst::Create(args, nextInst);

    addInjectedFun(IntrinsicsPass::MetadataPropageteInst);
}

/*!
 * Instrument external call instruction
 */
void BoundsInstrumentor::handleExtCall(CallSite cs) {

}

/*!
 * Instrument indirect call
 */
void BoundsInstrumentor::handleIndCall(CallSite cs) {

}

/*!
 * Instrument direct call instruction
 */
void BoundsInstrumentor::handleDirectCall(CallSite cs, const Function *F) {

    assert(F);

}

/*!
 * Instrument return instruction
 */
void BoundsInstrumentor::visitReturnInst(ReturnInst &inst) {

}

/*!
 * Instrument cast instruction
 */
void BoundsInstrumentor::visitCastInst(CastInst &inst) {

    if (!isa<PointerType>(inst.getType())
            || !isa<PointerType>(inst.getOperand(0)->getType()))
        return;

    DBOUT(DInstrument, errs() << "visit Gep " << inst << "\n");
    Instruction* curInst = &inst;
    Instruction* nextInst = getNextInst(&inst);

    Value* args[] = { castToVoidPtr(inst.getOperand(0), curInst), castToVoidPtr(
                          &inst, curInst)
                    };

    MetadataPropageteInst::Create(args, nextInst);

    addInjectedFun(IntrinsicsPass::MetadataPropageteInst);
}

/*!
 * Instrument select instruction
 */
void BoundsInstrumentor::visitSelectInst(SelectInst &inst) {

}

/*!
 * Inject bounds check for memory access instructions
 */
void BoundsInstrumentor::injectBoundCheck() {

    for (Module::iterator fi = getModule()->begin(), fe = getModule()->end(); fi != fe;
            ++fi) {
        for (inst_iterator ii = inst_begin(*fi), ei = inst_end(*fi); ii != ei;
                ++ii) {
            Instruction* inst = &*ii;

            if (LoadInst* load = dyn_cast<LoadInst>(inst)) {

                // get the access size of this pointer when loading memory
                Constant* accessSize = getSizeOfType(
                                           load->getPointerOperand()->getType());

                // if the pointer already be casted during metadata instrumentation
                // then we just use this vptr for checking

                if (getFromValToPtrMap(&*fi, load->getPointerOperand())) {
                    Value* vptrLoadPtr = getFromValToPtrMap(&*fi,
                                                            load->getPointerOperand());
                    Value* MDCheckArgs[] = { vptrLoadPtr, accessSize };
                    MetadataCheckInst::Create(MDCheckArgs, load);
                }
                // if the pointer is not casted to void pointer before
                // then we just casted it and use it for checking
                else {
                    Value* vptrLoadPtr = castToVoidPtr(
                                             load->getPointerOperand(), load);
                    Value* MDCheckArgs[] = { vptrLoadPtr, accessSize };
                    MetadataCheckInst::Create(MDCheckArgs, load);
                }

                addInjectedFun(IntrinsicsPass::MetadataCheckInst);
            }
        }
    }
}

/*!
 * Get bounds of a pointer
 */
Value* BoundsInstrumentor::getBoundVal(Instruction* inst) {
    Value* intBound;
    bool m_is_64_bit = true;
    if (m_is_64_bit) {
        intBound = ConstantInt::get(
                       Type::getInt64Ty(inst->getType()->getContext()), 1, false);
    } else {
        intBound = ConstantInt::get(
                       Type::getInt32Ty(inst->getType()->getContext()), 1, false);
    }

    return intBound;
}
