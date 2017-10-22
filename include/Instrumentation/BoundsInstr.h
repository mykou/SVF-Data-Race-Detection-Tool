/*
 * BoundsInstr.h
 *
 *  Created on: Jun 13, 2013
 *      Author: Yulei Sui
 */

#ifndef BOUNDSINSTRUMENTOR_H_
#define BOUNDSINSTRUMENTOR_H_

#include "Instrumentation/Instrumentor.h"


class BoundsInstrumentor: public Instrumentor {

public:
    /// Constructor
    BoundsInstrumentor(llvm::Module* m) : Instrumentor(m) {
    }
    /// Destructor
    virtual ~BoundsInstrumentor() {
    }
    /// Perform instrumentation, Inject metadata manipulated intrinsic functions
    void instrument();
    /// Transform an existing function to an intrinsic function
    bool isInterestedTransFun(llvm::Function* fun);
    /// Our visit overrides.
    //@{
    // Instructions that cannot be folded away.
    void visitAllocaInst(llvm::AllocaInst &AI);
    void visitPHINode(llvm::PHINode &I);
    void visitStoreInst(llvm::StoreInst &I);
    void visitLoadInst(llvm::LoadInst &I);
    void visitGetElementPtrInst(llvm::GetElementPtrInst &I);
    void visitCallInst(llvm::CallInst &I) {
        visitCallSite(&I);
    }
    void visitInvokeInst(llvm::InvokeInst &II) {
        visitCallSite(&II);
    }
    void visitReturnInst(llvm::ReturnInst &I);
    void visitCastInst(llvm::CastInst &I);
    void visitSelectInst(llvm::SelectInst &I);
    //@}

    /// Provide base case for our instruction visit.
    inline void visitInstruction(llvm::Instruction &I) {
        // If a new instruction is added to LLVM that we don't handle.
        // TODO: ignore here:
    }

private:
    /// get bound and get access size
    //@{
    llvm::Value* getBoundVal(llvm::Instruction* inst);
    //@}

    /// handle callsite
    //@{
    void handleExtCall(llvm::CallSite cs);

    void handleIndCall(llvm::CallSite cs);

    void handleDirectCall(llvm::CallSite cs, const llvm::Function *F);
    //@}

public:
    void injectBoundCheck();

};

#endif /* BOUNDSINSTRUMENTOR_H_ */
