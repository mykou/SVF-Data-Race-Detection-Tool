//===-- TSan.h - race detector -------------------------------===//
//
// This file is a part of TSan, a race detector.
//
// The tool is under development, for the details about previous versions see
// http://code.google.com/p/data-race-test
//
// The instrumentation phase is quite simple:
//   - Insert calls to run-time library before every memory access.
//      - Optimizations may apply to avoid instrumenting some of the accesses.
//   - Insert calls at function entry/exit.
// The rest is handled by the run-time library.
//===----------------------------------------------------------------------===//
#ifndef TSan_H_
#define TSan_H_

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "rc"
#endif

#include "llvm/Transforms/Instrumentation.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "RC/RCAnnotaton.h"

/// TSan: instrument the code in module to find races.
struct TSan : public llvm::FunctionPass {
    TSan() : llvm::FunctionPass(ID) {}

    /*!
     * Get the Pass Name
     */
    virtual llvm::StringRef getPassName() const override;

    /*!
     * Running instrumentation on each Function
     */
    bool runOnFunction(llvm::Function &F) override;

    /*!
     * Initialize the pass.
     */
    bool doInitialization(llvm::Module &M) override;

    // Pass identification, replacement for typeid.
    static char ID;  // Pass identification, replacement for typeid.

private:

    /*!
     * Initialize Callbacks
     */
    void initializeCallbacks(llvm::Module &M);

    /// Instrumenting functions
    //@{
    bool instrumentLoadOrStore(llvm::Instruction *I, const llvm::DataLayout &DL);
    bool instrumentAtomic(llvm::Instruction *I, const llvm::DataLayout &DL);
    bool instrumentMemIntrinsic(llvm::Instruction *I);
    //@}

    /*!
     * Refine redundant accesses
     */
    void chooseInstructionsToInstrument(llvm::SmallVectorImpl<llvm::Instruction *> &Local, llvm::SmallVectorImpl<llvm::Instruction *> &All,
                                        const llvm::DataLayout &DL);

    /*!
     * Transform address pointers to constant data
     */
    bool addrPointsToConstantData(llvm::Value *Addr);

    /*!
     * Get memory access function index
     */
    int getMemoryAccessFuncIndex(llvm::Value *Addr, const llvm::DataLayout &DL);

    /// Type definition
    //@{
    llvm::Type *IntptrTy;
    llvm::IntegerType *OrdTy;
    //@}

    /// Accesses sizes are powers of two: 1, 2, 4, 8, 16.
    static const size_t kNumberOfAccessSizes = 5;

    /// Callbacks to run-time library are computed in doInitialization.
    //@{
    llvm::Function *TsanFuncEntry;
    llvm::Function *TsanFuncExit;
    llvm::Function *TsanRead[kNumberOfAccessSizes];
    llvm::Function *TsanWrite[kNumberOfAccessSizes];
    llvm::Function *TsanUnalignedRead[kNumberOfAccessSizes];
    llvm::Function *TsanUnalignedWrite[kNumberOfAccessSizes];
    llvm::Function *TsanAtomicLoad[kNumberOfAccessSizes];
    llvm::Function *TsanAtomicStore[kNumberOfAccessSizes];
    llvm::Function *TsanAtomicRMW[llvm::AtomicRMWInst::LAST_BINOP + 1][kNumberOfAccessSizes];
    llvm::Function *TsanAtomicCAS[kNumberOfAccessSizes];
    llvm::Function *TsanAtomicThreadFence;
    llvm::Function *TsanAtomicSignalFence;
    llvm::Function *TsanVptrUpdate;
    llvm::Function *TsanVptrLoad;
    llvm::Function *MemmoveFn, *MemcpyFn, *MemsetFn;
    //@}

    /// Constructor function that calls init function of whole program
    llvm::Function *TsanCtorFunction;

    /// RaceComb annotation extractor that store the result of static analysis
    RCAnnotationExtractor annoExtractor;

    /// Potential race instructions
    std::set<const llvm::Instruction*> RDInsts;
};
#endif
