/*
 * Instrumentor.cpp
 *
 *  Created on: Jun 13, 2013
 *      Author: Yulei Sui
 */

#include "Instrumentation/Instrumentor.h"
#include "Util/AnalysisUtil.h"

#include <llvm/IR/InstIterator.h>

using namespace llvm;
using namespace std;
using namespace analysisUtil;

/*!
 * Constructor
 */
Instrumentor::Instrumentor(Module* m) :
    module(m) {
    voidPtrTy = PointerType::getUnqual(Type::getInt8Ty(m->getContext()));
}

/*!
 * Cast a c pointer into a void ptr by creating a new BitCastInst instruction
 */
Value* Instrumentor::castToVoidPtr(Value* pointer, Instruction* curInst) {
    return castToVoidPtr(pointer, "cast2VPtr", curInst);
}

/*!
 * if we create a new bitcast instruction,
 * then we record it for later use,
 * this vptr is unique during whole program metadata propagation
 * FIXME:: do we need to create this bitcast instruction at the pointer definition?
 * for global variable we may create this bitcast at function entry.
 */
Value* Instrumentor::castToVoidPtr(Value* pointer, StringRef NameStr,
                                   Instruction* curInst) {

    Function* fun = curInst->getParent()->getParent();

    // TODO: we only map observed functions
    if (getFromValToPtrMap(fun, pointer))
        return getFromValToPtrMap(fun, pointer);

    BitCastInst* vptr = NULL;
    BranchInst* dummyEntryBB = NULL;

    if (getFunEntryDummyBB(fun)) {
        dummyEntryBB = getFunEntryDummyBB(fun);
    } else {
        BasicBlock &entrybb = fun->getEntryBlock();
        BasicBlock *newbb = BasicBlock::Create(fun->getContext(), "dummyEntry",
                                               fun, &entrybb);
        dummyEntryBB = BranchInst::Create(&entrybb, newbb);
        addFunEntryDummyBB(fun, dummyEntryBB);
    }

    /// here we only associate vptr pointers with the definition of a value.
    /// a definition of a value can be Instruction, Argument (formal param), global.
    /// to be noted that every pointer (whatever it is a void* or not) needs to be casted to void*
    if (Instruction* def = dyn_cast<Instruction>(pointer)) {
        vptr = new BitCastInst(pointer, voidPtrTy, NameStr);
        vptr->insertAfter(def);
        addToValToPtrMap(fun, def, vptr);
    } else if (Argument* def = dyn_cast<Argument>(pointer)) {
        vptr = new BitCastInst(pointer, voidPtrTy, NameStr, dummyEntryBB);
        addToValToPtrMap(fun, def, vptr);
    } else if (GlobalVariable* def = dyn_cast<GlobalVariable>(pointer)) {
        vptr = new BitCastInst(pointer, voidPtrTy, NameStr, dummyEntryBB);
        addToValToPtrMap(fun, def, vptr);
    }
    /// the definition can be a function itself, like we pass function name to a function pointer.
    else if (Function* def = dyn_cast<Function>(pointer)) {
        vptr = new BitCastInst(pointer, voidPtrTy, NameStr, dummyEntryBB);
        addToValToPtrMap(fun, def, vptr);
    } else if (ConstantExpr* def = dyn_cast<ConstantExpr>(pointer)) {
        vptr = new BitCastInst(pointer, voidPtrTy, NameStr, curInst);
        addToValToPtrMap(fun, def, vptr);
    } else if (Constant* def = dyn_cast<Constant>(pointer)) {
        vptr = new BitCastInst(pointer, voidPtrTy, NameStr, curInst);
        addToValToPtrMap(fun, def, vptr);
    } else {
        errs() << "ptr :  " << pointer << *pointer << "\n";
        assert(false && "pointer not an instruction or argument??");
    }

    return vptr;
}

/*!
 * Instrument call instruction
 */
void Instrumentor::visitCallSite(CallSite cs) {

    // skip llvm debug info intrinsic
    if(isInstrinsicDbgInst(cs.getInstruction()))
        return;

    const Function *callee = getCallee(cs);

    if (callee) {
        if (isExtCall(callee))
            handleExtCall(cs);
        else
            handleDirectCall(cs, callee);
    } else {
        //If the callee was not identified as a function (null F), this is indirect.
        handleIndCall(cs);
    }

}

/*!
 * Get the next instruction in the basic block
 */
Instruction* Instrumentor::getNextInst(Instruction* inst) {
    if (isa<TerminatorInst>(inst)) {
        return inst;
    } else {
        BasicBlock::iterator i(inst);
        i++;
        Instruction* next = dyn_cast<Instruction>(i);
        assert(next && "can not find the next instruction??");
        return next;
    }
}

/*!
 * Get the machine size of a type
 */
Constant* Instrumentor::getSizeOfType(Type* input_type) {

    // Create a Constant Pointer Null of the input type.  Then get a
    // getElementPtr of it with next element access cast it to unsigned
    // int

    if (const PointerType* ptr_type = dyn_cast<PointerType>(input_type))
        if (isa<FunctionType>(ptr_type->getElementType())) {
            return ConstantInt::get(Type::getInt64Ty(ptr_type->getContext()), 0);
        }

    const SequentialType* seq_type = dyn_cast<SequentialType>(input_type);
    assert(seq_type && "pointer dereference and it is not a sequential type\n");

    StructType* struct_type = dyn_cast<StructType>(input_type);

    if (struct_type && struct_type->isOpaque()) {
        return ConstantInt::get(Type::getInt64Ty(struct_type->getContext()), 0);
    }

    if (!seq_type->getElementType()->isSized()) {
        return ConstantInt::get(Type::getInt64Ty(seq_type->getContext()), 0);
    }

    return ConstantExpr::getSizeOf(seq_type->getElementType());
}
