/*
 * SaberInstr.h
 *
 *  Created on: May 6, 2014
 *      Author: Yulei Sui
 */

#ifndef SABERINSTR_H_
#define SABERINSTR_H_

#include "Instrumentation/Instrumentor.h"
#include "Util/Annotator.h"

/*!
 * Saber instrumentation based on program annotations
 */
class SaberInstrumentor: public Instrumentor {

public:
    /// Constructor
    SaberInstrumentor(llvm::Module* m): Instrumentor(m) {
    }
    /// Destructor
    virtual ~SaberInstrumentor() {
    }

    /// Perform instrumentation, Inject metadata manipulated intrinsic functions
    inline void instrument() {
        visit(getModule());
    }
    /// Transform an existing function to an intrinsic function
    bool isInterestedTransFun(llvm::Function* fun);

    /// Our visit overrides.
    //@{
    // Instructions that cannot be folded away.
    void visitAllocaInst(llvm::AllocaInst &AI) {}
    void visitPHINode(llvm::PHINode &I) {}
    void visitStoreInst(llvm::StoreInst &I) {}
    void visitLoadInst(llvm::LoadInst &I) {}
    void visitGetElementPtrInst(llvm::GetElementPtrInst &I) {}
    void visitReturnInst(llvm::ReturnInst &I) {}
    void visitCastInst(llvm::CastInst &I) {}
    void visitSelectInst(llvm::SelectInst &I) {}
    //@}

    /// handle callsite
    //@{
    void handleExtCall(llvm::CallSite cs) {
        instrumentSrcSnk(cs);
    }
    void handleIndCall(llvm::CallSite cs) {}

    void handleDirectCall(llvm::CallSite cs, const llvm::Function *F) {
        instrumentSrcSnk(cs);
    }
    //@}

    /// Instrument source and sinks
    void instrumentSrcSnk(llvm::CallSite cs);

    /// Provide base case for our instruction visit.
    inline void visitInstruction(llvm::Instruction &I) {
        // If a new instruction is added to LLVM that we don't handle.
        // TODO: ignore here:
    }

private:
    Annotator ann;
};

#endif /* SABERINSTR_H_ */
