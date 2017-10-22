/*
 * IntrinsicInst.h
 *
 *  Created on: Jun 6, 2013
 *      Author: Yulei Sui
 */

#ifndef INTRINSICINST_H_
#define INTRINSICINST_H_

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/ADT/StringMap.h>

/*!
 * Intrinsic functions for the project
 */
class PTAIntrinsics {
public:
    typedef llvm::StringMap<llvm::Function*> IntrinsicFunMap;

public:
    PTAIntrinsics(llvm::Module& M) {
        module = &M;
        voidPtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(M.getContext()));
        int64Ty = llvm::Type::getInt64Ty(M.getContext());
    }
    virtual ~PTAIntrinsics() {
    }

    inline llvm::Type* getInt64Ty() const {
        return int64Ty;
    }
    inline llvm::PointerType* getVoidPtrTy() const {
        return voidPtrTy;
    }
protected:
    /// Set up intrinsic functions
    virtual void setupIntrinsicFun() = 0;
    llvm::PointerType* voidPtrTy;
    llvm::Type* int64Ty;
    llvm::Module *module;
    static IntrinsicFunMap intrinsicFunMap;
};

/// Intrinsics instruction pass
class IntrinsicsPass: public llvm::ModulePass {

public:
    static char ID;
    static bool Initialized;

    static llvm::PointerType* voidPtrTy;
    static llvm::Type* SizeTy;
    static llvm::Module *module;

    static llvm::Function* VirtualVar;
    static llvm::Function* CSUseVirtualVar;
    static llvm::Function* CSDefVirtualVar;
    static llvm::Function* FNUseVirtualVar;
    static llvm::Function* FNDefVirtualVar;

    static llvm::Function* MetadataInst;
    static llvm::Function* MetadataStoreInst;
    static llvm::Function* MetadataLoadInst;
    static llvm::Function* MetadataPropageteInst;
    static llvm::Function* MetadataCheckInst;
    static llvm::Function* MetadataInitialInst;
    static llvm::Function* MetadataLoadCounterInst;
    static llvm::Function* MetadataStoreCounterInst;

    IntrinsicsPass() :
        ModulePass(ID) {
    }

    virtual bool runOnModule(llvm::Module &M);

    virtual const char* getPassName() const {
        return " IntrinsicsPass";
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
        AU.setPreservesAll();
    }

};

/// This is the base class of different kinds of virtual variables extended in LLVM SSA form
class VirtualVar: public llvm::IntrinsicInst {
private:
    VirtualVar();                      ///< place holder
    VirtualVar(const VirtualVar &);  ///< place holder
    void operator=(const VirtualVar &); ///< place holder
public:
    //@{ Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const VirtualVar *) {
        return true;
    }
    static inline bool classof(const llvm::CallInst *ci) {
        assert(
            IntrinsicsPass::Initialized
            && "intrinsic pass should be intialized first!!");
        return ci->getCalledValue() == IntrinsicsPass::VirtualVar
               || ci->getCalledValue() == IntrinsicsPass::CSUseVirtualVar
               || ci->getCalledValue() == IntrinsicsPass::CSDefVirtualVar
               || ci->getCalledValue() == IntrinsicsPass::FNUseVirtualVar
               || ci->getCalledValue() == IntrinsicsPass::FNDefVirtualVar;
    }
    static inline bool classof(const llvm::Value *v) {
        return llvm::isa<llvm::CallInst>(v)
               && classof(llvm::cast<llvm::CallInst>(v));
    }
    static VirtualVar* Create(llvm::ArrayRef<Value *> args,
                              llvm::Instruction *insertBefore, llvm::Function* fn =
                                  IntrinsicsPass::VirtualVar);
    // @}
};

class MetadataInst: public llvm::IntrinsicInst {
private:
    MetadataInst();                      ///< place holder
    MetadataInst(const MetadataInst &);  ///< place holder
    void operator=(const MetadataInst &); ///< place holder
public:
    //@{ Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const MetadataInst *) {
        return true;
    }
    static inline bool classof(const llvm::CallInst *ci) {
        assert(
            IntrinsicsPass::Initialized
            && "intrinsic pass should be intialized first!!");
        return ci->getCalledValue() == IntrinsicsPass::MetadataInst
               || ci->getCalledValue() == IntrinsicsPass::MetadataInitialInst
               || ci->getCalledValue() == IntrinsicsPass::MetadataStoreInst
               || ci->getCalledValue() == IntrinsicsPass::MetadataLoadInst
               || ci->getCalledValue() == IntrinsicsPass::MetadataPropageteInst
               || ci->getCalledValue() == IntrinsicsPass::MetadataCheckInst
               || ci->getCalledValue()
               == IntrinsicsPass::MetadataLoadCounterInst
               || ci->getCalledValue()
               == IntrinsicsPass::MetadataStoreCounterInst;
    }
    static inline bool classof(const llvm::Value *v) {
        return llvm::isa<llvm::CallInst>(v)
               && classof(llvm::cast<llvm::CallInst>(v));
    }
    static MetadataInst* Create(llvm::ArrayRef<Value *> args,
                                llvm::Instruction *insertBefore, llvm::Function* fn =
                                    IntrinsicsPass::MetadataInst);
    // @}
};

//#define MDInitialInst "InitialInst"

#define INITIALIZE_INTRINSICS_CLASS(className, baseClass, IntrinsicFun) \
  class className  : public baseClass { \
	private:	\
	className();   \
    className(const className &);  \
    void operator=(const className &); \
	public:	\
	static inline bool classof(const className *) { return true; }	\
	static inline bool classof(const llvm::Value *v) {	\
        return llvm::isa<llvm::CallInst>(v) && classof(llvm::cast<llvm::CallInst>(v));	\
    }	\
    static inline bool classof(const llvm::CallInst *ci) {	\
        return ci->getCalledValue() == IntrinsicFun;	\
    }	\
    static inline bool classof(const llvm::IntrinsicInst *ci) {	\
        return ci->getCalledValue() == IntrinsicFun;	\
    }	\
    static className* Create(llvm::ArrayRef<Value *> args, llvm::Instruction *insertBefore){	\
    	assert(IntrinsicFun->getArgumentList().size() == args.size() && "argument size not match!!");	\
    	return llvm::cast<className>(baseClass::Create(args,insertBefore,IntrinsicFun));	\
    }	\
  };

//FIXME: remember each time add the class into MetadataInst/VirtualVar for identifying its uniqueness.
INITIALIZE_INTRINSICS_CLASS(CSUseVirtualVar, VirtualVar,
                            IntrinsicsPass::CSUseVirtualVar)
INITIALIZE_INTRINSICS_CLASS(CSDefVirtualVar, VirtualVar,
                            IntrinsicsPass::CSDefVirtualVar)
INITIALIZE_INTRINSICS_CLASS(FNUseVirtualVar, VirtualVar,
                            IntrinsicsPass::FNUseVirtualVar)
INITIALIZE_INTRINSICS_CLASS(FNDefVirtualVar, VirtualVar,
                            IntrinsicsPass::FNDefVirtualVar)

INITIALIZE_INTRINSICS_CLASS(MDInitialInst, MetadataInst,
                            IntrinsicsPass::MetadataInitialInst)
INITIALIZE_INTRINSICS_CLASS(MetadataStoreCounterInst, MetadataInst,
                            IntrinsicsPass::MetadataStoreCounterInst)
INITIALIZE_INTRINSICS_CLASS(MetadataLoadCounterInst, MetadataInst,
                            IntrinsicsPass::MetadataLoadCounterInst)
INITIALIZE_INTRINSICS_CLASS(MetadataStoreInst, MetadataInst,
                            IntrinsicsPass::MetadataStoreInst)
INITIALIZE_INTRINSICS_CLASS(MetadataLoadInst, MetadataInst,
                            IntrinsicsPass::MetadataLoadInst)
INITIALIZE_INTRINSICS_CLASS(MetadataPropageteInst, MetadataInst,
                            IntrinsicsPass::MetadataPropageteInst)
INITIALIZE_INTRINSICS_CLASS(MetadataCheckInst, MetadataInst,
                            IntrinsicsPass::MetadataCheckInst)

#endif /* INTRINSICINST_H_ */
