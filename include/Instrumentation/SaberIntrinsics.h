/*
 * SaberIntrinsicInst.h
 *
 *  Created on: May 6, 2014
 *      Author: rockysui
 */

#ifndef SABERINTRINSICINST_H_
#define SABERINTRINSICINST_H_

#include "Instrumentation/IntrinsicInst.h"

/*!
 * Saber Intrinsic Instructions/Functions
 */
class SaberIntrinsics: public PTAIntrinsics {

public:
    /// argument type of intrinsic function
    enum IntrinsicArgType {
        DummyTyArg = 0,
        VoidPtrTyArg,
        Int64TyArg
    };

private:
    /// Static member
    static SaberIntrinsics* SBIntrinsic;
    /// initialize intrinsic functions
    void setupIntrinsicFun();

    /// Constructor
    SaberIntrinsics(llvm::Module& M) : PTAIntrinsics(M) {
        setupIntrinsicFun();
    }

public:
    /// Destructor
    virtual ~SaberIntrinsics() {

    }

    /// Singleton design here to make sure we only have one instance during whole analysis
    static inline SaberIntrinsics* Create(llvm::Module& M) {
        if (SBIntrinsic == NULL) {
            SBIntrinsic = new SaberIntrinsics(M);
        }
        return SBIntrinsic;
    }

    /// Return intrinsic function
    static inline llvm::Function* getIntrinsicFun(std::string name) {
        IntrinsicFunMap::const_iterator it = intrinsicFunMap.find(name);
        assert(it!=intrinsicFunMap.end() && "intrinsic function not found!");
        return it->second;
    }
};

/*!
 * Saber IntrinsicInst
 */
class SaberInst: public llvm::IntrinsicInst {
private:
    SaberInst();                      ///< place holder
    SaberInst(const SaberInst &);  ///< place holder
    void operator=(const SaberInst &); ///< place holder
public:
    //@{ Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const SaberInst *) {
        return true;
    }
    static inline bool classof(const CallInst* ci) {
        return ci->getCalledValue() == SaberIntrinsics::getIntrinsicFun("__saber_malloc")
               || ci->getCalledValue() == SaberIntrinsics::getIntrinsicFun("__saber_free");
    }
    static inline bool classof(const llvm::Value *v) {
        return llvm::isa<llvm::CallInst>(v)
               && classof(llvm::cast<llvm::CallInst>(v));
    }
    static SaberInst* Create(llvm::ArrayRef<Value *> args,
                             llvm::Instruction *insertBefore, llvm::Function* fn = NULL);
    // @}
};


INITIALIZE_INTRINSICS_CLASS(SaberMallocInst, SaberInst,
                            SaberIntrinsics::getIntrinsicFun("__saber_malloc"))
INITIALIZE_INTRINSICS_CLASS(SaberFreeInst, SaberInst,
                            SaberIntrinsics::getIntrinsicFun("__saber_free"))
#endif /* SABERINTRINSICINST_H_ */
