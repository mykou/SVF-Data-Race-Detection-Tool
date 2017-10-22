/*
 * RCInstr.h
 *
 *  Created on: May 14, 2016
 *      Author: Peng Di
 */

#ifndef RCInstr_H_
#define RCInstr_H_

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

/*!
 * RCInstr: instrument the code in module to find races.
 */
struct RCInstr : public llvm::FunctionPass {
public:
    typedef std::pair<size_t, size_t> Pair;

    RCInstr() : llvm::FunctionPass(ID) {
        NumOmittedReadsBeforeWrite = 0;
        NumOmittedNonCaptured = 0;
        NumOmittedReadsFromConstantGlobals=0;
        NumOmittedReadsFromVtable=0;
    }

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
    static char ID;

private:
    /*!
     * Initialize Callbacks
     */
    void initializeCallbacks(llvm::Module &M);

    /*!
     * Instrument Load and Store instructions
     */
    bool instrumentLoadOrStore(llvm::Instruction *I, const llvm::DataLayout &DL);

    /*!
     * Instrumenting some of the accesses may be proven redundant.
     * Currently handled:
     *  - read-before-write (within same BB, no calls between)
     *  - not captured variables
     *
     * We do not handle some of the patterns that should not survive
     * after the classic compiler optimizations.
     * E.g. two reads from the same temp should be eliminated by CSE,
     * two writes should be eliminated by DSE, etc.
     *
     * 'Local' is a vector of insns within the same BB (no calls between).
     * 'All' is a vector of insns that will be instrumented
     */
    void chooseInstructionsToInstrument(llvm::SmallVectorImpl<llvm::Instruction *> &Local,
                                        llvm::SmallVectorImpl<llvm::Instruction *> &All,
                                        const llvm::DataLayout &DL);

    /*!
     * Transform address pointers to constant data
     */
    bool addrPointsToConstantData(llvm::Value *Addr);

    /*!
     * Check if it is virtual table access
     */
    bool isVtableAccess(llvm::Instruction *I);

    /*!
     * Instrument memory intrinsic instructions
     */
    bool instrumentMemIntrinsic(llvm::Instruction *I);

    /*!
     * Instrument memory intrinsic instructions
     */
    bool instrumentFree(llvm::Instruction *I);

    /*!
     * Get memory access function index
     */
    int getMemoryAccessFuncIndex(llvm::Value *Addr, const llvm::DataLayout &DL);

    /// Type definition
    //@{
    llvm::Type *IntptrTy;
    llvm::IntegerType *OrdTy;
    //@}

    // Accesses sizes are powers of two: 1, 2, 4, 8, 16.
    static const size_t kNumberOfAccessSizes = 5;

    /// Function pointers for instrumented functions
    //@{
    llvm::Function *Read[kNumberOfAccessSizes];
    llvm::Function *Write[kNumberOfAccessSizes];
    llvm::Function *UnalignedRead[kNumberOfAccessSizes];
    llvm::Function *UnalignedWrite[kNumberOfAccessSizes];
    //@}


    /// Constructor function that calls init function of whole program
    llvm::Function *CtorFunction;

    /// Constructor function that calls fini function of whole program
    llvm::Function *DtorFunction;

    /// Memory intrinsic instructions
    //@{
    llvm::Function *MemmoveFn;
    llvm::Function *MemcpyFn;
    llvm::Function *MemsetFn;
    //@}

    /// Free instruction
    llvm::Function *FreeFn;

    /// RaceComb annotation extractor that store the result of static analysis
    RCAnnotationExtractor annoExtractor;

    /// Potential race instructions
    std::set<const llvm::Instruction*> RDInsts;

    /// Statistic
    //@{
    unsigned int NumOmittedReadsBeforeWrite;
    unsigned int NumOmittedNonCaptured;
    unsigned int NumOmittedReadsFromConstantGlobals;
    unsigned int NumOmittedReadsFromVtable;
    //@}
};


#endif /* RCInstr_H_ */
