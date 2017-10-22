/*
 * RCInstr.cpp
 *
 *  Created on: May 14, 2016
 *      Author: Peng Di
 */

#include "RC/RCInstr.h"
#include "Util/AnalysisUtil.h"

#include <llvm/Support/CommandLine.h>	// for llvm command line options
#include <llvm/IR/InstIterator.h>	// for inst iteration


using namespace llvm;
using namespace analysisUtil;

static cl::opt<bool> DCIAnno("dci", cl::init(false), cl::desc("DCI Annotation"));
static cl::opt<bool> PrintRCPairs("printRC", cl::init(false), cl::desc("Print all RC pairs"));
static cl::opt<bool> UseRCPairs("useRC", cl::init(false), cl::desc("Use RC.pairs as the input pair file"));
static cl::opt<int> CheckingPair("checkingpair", cl::init(-1), cl::desc("Checking i-th pair, default is to check all pairs."));

STATISTIC(NumInstrumentedReads, "Number of instrumented reads");
STATISTIC(NumInstrumentedWrites, "Number of instrumented writes");
STATISTIC(NumAccessesWithBadSize, "Number of accesses with bad size");

static const char *const kRCModuleCtorName = "rc.module_ctor";
static const char *const kRCModuleDtorName = "rc.module_dtor";
static const uint64_t kRCCtorAndDtorPriority = 1;
static const char *const kTSInitName = "__ts_init";
static const char *const kTSFiniName = "__ts_fini";
static const char *const kDCIInitName = "__dci_init";
static const char *const kDCIFiniName = "__dci_fini";


char RCInstr::ID = 0;
static RegisterPass<RCInstr> THREADSCHEDULING("pts", "RCInstr: Dynamic Context Interference.");

/*
 * Get the Pass Name
 */
StringRef RCInstr::getPassName() const {
    return "RCInstr";
}

/*
 * Initialize Callbacks
 */
void RCInstr::initializeCallbacks(Module &M) {
    IRBuilder<> IRB(M.getContext());
    // Initialize the callbacks.

    OrdTy = IRB.getInt32Ty();

    for (size_t i = 0; i < kNumberOfAccessSizes; ++i) {
        const unsigned ByteSize = 1U << i;
        const unsigned BitSize = ByteSize * 8;
        std::string ByteSizeStr = utostr(ByteSize);
        std::string BitSizeStr = utostr(BitSize);

        SmallString<32> ReadName;
        SmallString<32> WriteName;
        SmallString<64> UnalignedReadName;
        SmallString<64> UnalignedWriteName;

        SmallString<32> MemmoveName;
        SmallString<32> MemcpyName;
        SmallString<32> MemsetName;

        SmallString<32> FreeName;

        if (DCIAnno) {
            /// DCI Annotation
            ReadName.assign("__dci_read" + ByteSizeStr);
            WriteName.assign("__dci_write" + ByteSizeStr);
            UnalignedReadName.assign("__dci_unaligned_read" + ByteSizeStr);
            UnalignedWriteName.assign("__dci_unaligned_write" + ByteSizeStr);

            MemmoveName.assign("__dci_memmove");
            MemcpyName.assign("__dci_memcpy");
            MemsetName.assign("__dci_memset");

            FreeName.assign("__dci_free");

        } else {
            /// TS Annotation
            if (RDInsts.size() == 1) {
                ReadName.assign("__ts_self_read" + ByteSizeStr);
                WriteName.assign("__ts_self_write" + ByteSizeStr);
                UnalignedReadName.assign("__ts_self_unaligned_read" + ByteSizeStr);
                UnalignedWriteName.assign("__ts_self_unaligned_write" + ByteSizeStr);

                MemmoveName.assign("__ts_self_memmove");
                MemcpyName.assign("__ts_self_memcpy");
                MemsetName.assign("__ts_self_memset");

                FreeName.assign("__ts_self_free");
            } else {
                ReadName.assign("__ts_read" + ByteSizeStr);
                WriteName.assign("__ts_write" + ByteSizeStr);
                UnalignedReadName.assign("__ts_unaligned_read" + ByteSizeStr);
                UnalignedWriteName.assign("__ts_unaligned_write" + ByteSizeStr);

                MemmoveName.assign("__ts_memmove");
                MemcpyName.assign("__ts_memcpy");
                MemsetName.assign("__ts_memset");

                FreeName.assign("__ts_free");
            }
        }
        Read[i] = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
                      ReadName, IRB.getVoidTy(), IRB.getInt8PtrTy(), IRB.getInt64Ty(), nullptr));
        Write[i] = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
                       WriteName, IRB.getVoidTy(), IRB.getInt8PtrTy(), IRB.getInt64Ty(), nullptr));
        UnalignedRead[i] = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
                               UnalignedReadName, IRB.getVoidTy(), IRB.getInt8PtrTy(), IRB.getInt64Ty(), nullptr));
        UnalignedWrite[i] = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
                                UnalignedWriteName, IRB.getVoidTy(), IRB.getInt8PtrTy(), IRB.getInt64Ty(), nullptr));

        MemmoveFn = checkSanitizerInterfaceFunction(
                        M.getOrInsertFunction(MemmoveName, IRB.getVoidTy(), IRB.getInt8PtrTy(), IRB.getInt8PtrTy(), IntptrTy, IRB.getInt64Ty(), nullptr));
        MemcpyFn = checkSanitizerInterfaceFunction(
                       M.getOrInsertFunction(MemcpyName, IRB.getVoidTy(), IRB.getInt8PtrTy(), IRB.getInt8PtrTy(), IntptrTy, IRB.getInt64Ty(), nullptr));
        MemsetFn = checkSanitizerInterfaceFunction(
                       M.getOrInsertFunction(MemsetName, IRB.getVoidTy(), IRB.getInt8PtrTy(), IntptrTy, IRB.getInt64Ty(), nullptr));

        FreeFn = checkSanitizerInterfaceFunction(
                     M.getOrInsertFunction(FreeName, IRB.getVoidTy(), IRB.getInt8PtrTy(), IRB.getInt64Ty(), nullptr));
    }
}

/*
 * Initialize the pass.
 */
bool RCInstr::doInitialization(Module &M) {

    const DataLayout &DL = M.getDataLayout();
    IntptrTy = DL.getIntPtrType(M.getContext());
    std::tie(CtorFunction, std::ignore) = createSanitizerCtorAndInitFunctions(
            M, kRCModuleCtorName, (DCIAnno)?kDCIInitName:kTSInitName, /*InitArgTypes=*/ {IntegerType::getInt64Ty(M.getContext())},
            /*InitArgs=*/ {ConstantInt::get(IntegerType::getInt64Ty(M.getContext()), RDInsts.size())});

    appendToGlobalCtors(M, CtorFunction, 0);


    Function *Dtor =
        Function::Create(FunctionType::get(Type::getVoidTy(M.getContext()), false),
                         GlobalValue::InternalLinkage, kRCModuleDtorName, &M);
    BasicBlock *RCDtorBB = BasicBlock::Create(M.getContext(), "", Dtor);
    IRBuilder<> IRB_Dtor(ReturnInst::Create(M.getContext(), RCDtorBB));

    DtorFunction = checkSanitizerInterfaceFunction(M.getOrInsertFunction((DCIAnno)?kDCIFiniName:kTSFiniName, IRB_Dtor.getVoidTy(), nullptr));
    DtorFunction->setLinkage(Function::ExternalLinkage);

    IRB_Dtor.CreateCall(DtorFunction, {});
    appendToGlobalDtors(M, DtorFunction, kRCCtorAndDtorPriority);

    annoExtractor.init(&M);  // M is the Module with annotations
    annoExtractor.run();

    if (DCIAnno) {
        for (RCAnnotationExtractor::const_inst_iterator it = annoExtractor.instBegin(), ie = annoExtractor.instEnd(); it != ie; ++it) {
            const Instruction *I = *it;
            RDInsts.insert(I);
        }

        std::set<Pair> pairs;
        for (int i = 0, e = annoExtractor.getNumOfAllPairs(); i != e; ++i) {
            RCAnnotationExtractor::InstPair pair = annoExtractor.getOnePair(i);

            const Instruction *I1 = pair.first;
            const Instruction *I2 = pair.second;

            // Instrument I1 and I2 for RCInstr check

            // For RC usage:
            // Here is how to map from Instructions and their annotation IDs
            size_t id1 = annoExtractor.getAnnotationId(I1);
            size_t id2 = annoExtractor.getAnnotationId(I2);

            // Here is how to map from annotation IDs to Instructions
            assert(I1 == annoExtractor.getInstruction(id1));
            assert(I2 == annoExtractor.getInstruction(id2));

            if (id1 < id2)
                pairs.insert(std::make_pair(id1, id2));
            else
                pairs.insert(std::make_pair(id2, id1));
        }

        FILE *pfile;
        pfile = fopen("RC.pairs", "wb");

        if (pfile == NULL) {
            llvm::outs() << "Failed to open and create RC.pairs\n";
            return EXIT_FAILURE;
        }

        for (std::set<Pair>::iterator it = pairs.begin(), ei = pairs.end(); it != ei; it++) {
            fprintf(pfile, "%zu %zu\n", it->first, it->second);
            if (PrintRCPairs) {
                outs() << "  === " << std::distance(pairs.begin(), it) << "-th RC pairs <" << it->first << ","
                       << it->second << "> (total: " << pairs.size() << ") at:\n";
                outs() << "    |_#1 " << analysisUtil::getSourceLoc(annoExtractor.getInstruction(it->first)) << "\n";
                outs() << "    |_#2 " << analysisUtil::getSourceLoc(annoExtractor.getInstruction(it->second)) << "\n";
            }
        }
    } else {
        std::vector<Pair> pairs;
        FILE *pfile;
        if (UseRCPairs) {
            if ((pfile = fopen("RC.pairs", "r")) == NULL) {
                if ((pfile = fopen("../RC.pairs", "r")) == NULL) {
                    printf("Can't open RC.pairs\n");
                }
            }

        } else {
            if ((pfile = fopen("Ordered.pairs", "r")) == NULL) {
                if ((pfile = fopen("../Ordered.pairs", "r")) == NULL) {
                    printf("Can't open Ordered.pairs\n");
                }
            }
        }
        if (pfile != NULL) {
            unsigned a, b;
            while (!feof(pfile)) {
                if (EOF == fscanf(pfile, "%u %u\n", &a, &b)) {
                    printf("Error reading Ordered.pairs\n");
                }
                pairs.push_back(std::make_pair(a, b));
            }
            fclose(pfile);
        }

        if ((size_t) CheckingPair >= pairs.size() || CheckingPair < 0) {
            outs() << "Wrong checking pair no. " << CheckingPair << " in " << pairs.size() << " pairs.\n";
            return false;
        }

        outs() << "  === Checking " << CheckingPair << "-th instruction pairs (total: " << pairs.size() << ") at:\n";

        size_t id1 = pairs[CheckingPair].first;
        size_t id2 = pairs[CheckingPair].second;

        const Instruction *I1 = annoExtractor.getInstruction(id1);
        const Instruction *I2 = annoExtractor.getInstruction(id2);

        outs() << "    |_#1 " << analysisUtil::getSourceLoc(I1) << "\n";
        outs() << "    |_#2 " << analysisUtil::getSourceLoc(I2) << "\n";
        //... // Instrument I1 and I2 for ThreadScheduling check

        RDInsts.insert(I1);
        RDInsts.insert(I2);
    }



    return true;
}

/*
 * Check if the instruction I is atomic operation
 */
static bool isAtomic(Instruction *I) {
    if (LoadInst *LI = dyn_cast<LoadInst>(I))
        return LI->isAtomic() && LI->getSynchScope() == CrossThread;
    if (StoreInst *SI = dyn_cast<StoreInst>(I))
        return SI->isAtomic() && SI->getSynchScope() == CrossThread;
    if (isa<AtomicRMWInst>(I))
        return true;
    if (isa<AtomicCmpXchgInst>(I))
        return true;
    if (isa<FenceInst>(I))
        return true;
    return false;
}

/*
 * Running instrumentation on each Function
 */
bool RCInstr::runOnFunction(Function &F) {
    // This is required to prevent instrumenting call to __tsan_init from within
    // the module constructor.
    if (&F == CtorFunction || &F == DtorFunction)
        return false;

    initializeCallbacks(*F.getParent());
    SmallVector<Instruction*, 8> LocalLoadsAndStores;
    SmallVector<Instruction*, 8> InstrumentedInsts;
    SmallVector<Instruction*, 8> MemIntrinCalls;
//    SmallVector<Instruction*, 8> FreeCalls;

    bool Res = false;
    const DataLayout &DL = F.getParent()->getDataLayout();

    // Traverse all instructions, collect loads/stores, check for calls.
    for (auto &BB : F) {
        for (auto &Inst : BB) {
            if (!isAtomic(&Inst)) {
                if (isa<LoadInst>(Inst) || isa<StoreInst>(Inst)) {
                    /// insert Race Detection annotation condition
                    if (RDInsts.find(&Inst) != RDInsts.end()) {
                        LocalLoadsAndStores.push_back(&Inst);
                    }
                }
                else if (isa<CallInst>(Inst) || isa<InvokeInst>(Inst)) {
                    if (isa<MemIntrinsic>(Inst)) {
                        if (RDInsts.find(&Inst) != RDInsts.end()) {
                            MemIntrinCalls.push_back(&Inst);
                        }
                    }
                    if (DCIAnno)
                        chooseInstructionsToInstrument(LocalLoadsAndStores, InstrumentedInsts, DL);
                }
            }
        }
        if (DCIAnno)
            chooseInstructionsToInstrument(LocalLoadsAndStores, InstrumentedInsts, DL);
    }

    // We have collected pair.
    // Instrument memory accesses only if we want to report bugs in the function.

    if (DCIAnno)
        for (auto Inst : InstrumentedInsts) {
            Res |= instrumentLoadOrStore(Inst, DL);
        }
    else
        for (auto Inst : LocalLoadsAndStores) {
            Res |= instrumentLoadOrStore(Inst, DL);
        }

    for (auto Inst : MemIntrinCalls) {
        Res |= instrumentMemIntrinsic(Inst);
    }

    /// So far we do not take use-after-free in account
//    for (auto Inst : FreeCalls) {
//        Res |= instrumentFree(Inst);
//    }
    return Res;
}

/*
 * Check if the instruction I is virtual table access
 */
bool RCInstr::isVtableAccess(Instruction *I) {
    if (MDNode *Tag = I->getMetadata(LLVMContext::MD_tbaa))
        return Tag->isTBAAVtableAccess();
    return false;
}

/*
    * Transform address pointers to constant data
    */
bool RCInstr::addrPointsToConstantData(Value *Addr) {
    // If this is a GEP, just analyze its pointer operand.
    if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(Addr))
        Addr = GEP->getPointerOperand();

    if (GlobalVariable *GV = dyn_cast<GlobalVariable>(Addr)) {
        if (GV->isConstant()) {
            // Reads from constant globals can not race with any writes.
            NumOmittedReadsFromConstantGlobals++;
            return true;
        }
    } else if (LoadInst *L = dyn_cast<LoadInst>(Addr)) {
        if (isVtableAccess(L)) {
            // Reads from a vtable pointer can not race with any writes.
            NumOmittedReadsFromVtable++;
            return true;
        }
    }
    return false;
}

/*
 * Refine the redundant accesses.
 */
void RCInstr::chooseInstructionsToInstrument(
    SmallVectorImpl<Instruction *> &Local, SmallVectorImpl<Instruction *> &All,
    const DataLayout &DL) {
    SmallSet<Value*, 8> WriteTargets;
    // Iterate from the end.
    for (SmallVectorImpl<Instruction*>::reverse_iterator It = Local.rbegin(),
            E = Local.rend(); It != E; ++It) {
        Instruction *I = *It;
        if (StoreInst *Store = dyn_cast<StoreInst>(I)) {
            WriteTargets.insert(Store->getPointerOperand());
        } else {
            LoadInst *Load = cast<LoadInst>(I);
            Value *Addr = Load->getPointerOperand();
            if (WriteTargets.count(Addr)) {
                // We will write to this temp, so no reason to analyze the read.
                NumOmittedReadsBeforeWrite++;
                continue;
            }
            if (addrPointsToConstantData(Addr)) {
                // Addr points to some constant data -- it can not race with any writes.
                continue;
            }
        }
        Value *Addr = isa<StoreInst>(*I)
                      ? cast<StoreInst>(I)->getPointerOperand()
                      : cast<LoadInst>(I)->getPointerOperand();
        if (isa<AllocaInst>(GetUnderlyingObject(Addr, DL)) &&
                !PointerMayBeCaptured(Addr, true, true)) {
            // The variable is addressable but not captured, so it cannot be
            // referenced from a different thread and participate in a data race
            // (see llvm/Analysis/CaptureTracking.h for details).
            NumOmittedNonCaptured++;
            continue;
        }
        All.push_back(I);
    }
    Local.clear();
}

/*
 * Instrument Load and Store instructions
 */
bool RCInstr::instrumentLoadOrStore(Instruction *I,
                                    const DataLayout &DL) {

    IRBuilder<> IRB(I);

    const size_t id = annoExtractor.getAnnotationId(I);

    bool IsWrite = isa<StoreInst>(*I);
    Value *Addr = IsWrite
                  ? cast<StoreInst>(I)->getPointerOperand()
                  : cast<LoadInst>(I)->getPointerOperand();
    int Idx = getMemoryAccessFuncIndex(Addr, DL);
    if (Idx < 0)
        return false;

    Value* ArgAddr = IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy());
    ConstantInt * ArgInst = IRB.getInt64(id);

    std::vector<Value *> Args;
    Args.push_back(ArgAddr);
    Args.push_back(ArgInst);

    const unsigned Alignment = IsWrite
                               ? cast<StoreInst>(I)->getAlignment()
                               : cast<LoadInst>(I)->getAlignment();
    Type *OrigTy = cast<PointerType>(Addr->getType())->getElementType();
    const uint32_t TypeSize = DL.getTypeStoreSizeInBits(OrigTy);
    Value *OnAccessFunc = nullptr;
    if (Alignment == 0 || Alignment >= 8 || (Alignment % (TypeSize / 8)) == 0) {
        OnAccessFunc = IsWrite ? Write[Idx] : Read[Idx];
    }
    else {
        OnAccessFunc = IsWrite ? UnalignedWrite[Idx] : UnalignedRead[Idx];
    }
    IRB.CreateCall(OnAccessFunc, Args);

    if (IsWrite) NumInstrumentedWrites++;
    else         NumInstrumentedReads++;
    return true;
}

/*
 * Instruments memset intrinsic instructions
 */
bool RCInstr::instrumentMemIntrinsic(Instruction *I) {
    IRBuilder<> IRB(I);
    const size_t id = annoExtractor.getAnnotationId(I);
    if (MemSetInst *M = dyn_cast<MemSetInst>(I)) {
        /// memset: (dst, size, instID)
        IRB.CreateCall(
            MemsetFn,
        {   IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
            IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false),
            IRB.getInt64(id)
        });
        return true;
    } else if (MemTransferInst *M = dyn_cast<MemTransferInst>(I)) {
        /// memcpy and memmove: (dst, size, instID)
        IRB.CreateCall(
            isa<MemCpyInst>(M) ? MemcpyFn : MemmoveFn,
        {   IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
            IRB.CreatePointerCast(M->getArgOperand(1), IRB.getInt8PtrTy()),
            IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false),
            IRB.getInt64(id)
        });
        return true;
    }
    return false;
}

/*
 * Instruments free instructions
 */
bool RCInstr::instrumentFree(Instruction *I) {
    IRBuilder<> IRB(I);
    const size_t id = annoExtractor.getAnnotationId(I);

    if (CallInst *C = dyn_cast<CallInst>(I)) {
        IRB.CreateCall(FreeFn, { IRB.CreatePointerCast(C->getArgOperand(0), IRB.getInt8PtrTy()), IRB.getInt64(id) });
        return true;
    }
    return false;
}

/*
 * Get memory access function index
 */
int RCInstr::getMemoryAccessFuncIndex(Value *Addr,
                                      const DataLayout &DL) {
    Type *OrigPtrTy = Addr->getType();
    Type *OrigTy = cast<PointerType>(OrigPtrTy)->getElementType();
    assert(OrigTy->isSized());
    uint32_t TypeSize = DL.getTypeStoreSizeInBits(OrigTy);
    if (TypeSize != 8  && TypeSize != 16 &&
            TypeSize != 32 && TypeSize != 64 && TypeSize != 128) {
        NumAccessesWithBadSize++;
        // Ignore all unusual sizes.
        return -1;
    }
    size_t Idx = countTrailingZeros(TypeSize / 8);
    assert(Idx < kNumberOfAccessSizes);
    return Idx;
}
