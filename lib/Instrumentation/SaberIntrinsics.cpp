/*
 * SaberIntrinsics.cpp
 *
 *  Created on: May 6, 2014
 *      Author: Yulei Sui
 */

#include "Instrumentation/SaberIntrinsics.h"
#include "Util/BasicTypes.h"
#include <vector>
using namespace llvm;

/// Static member
SaberIntrinsics* SaberIntrinsics::SBIntrinsic = NULL;

static const unsigned maxNumOfArgs = 10;

struct InstrinsicEntry {
    const char *n;
    u32_t args[maxNumOfArgs];
};

static const InstrinsicEntry ientrys[]= {
    {"__saber_malloc", {SaberIntrinsics::VoidPtrTyArg, SaberIntrinsics::Int64TyArg} },
    {"__saber_free", {SaberIntrinsics::VoidPtrTyArg, SaberIntrinsics::Int64TyArg} },

    //This must be the last entry.
    {0, {} }
};


/*!
 * initialize intrinsic functions
 */
void SaberIntrinsics::setupIntrinsicFun() {

    for(const InstrinsicEntry *p= ientrys; p->n; ++p) {
        std::vector<Type*> types;
        for(u32_t i = 0; i < maxNumOfArgs; i++) {
            if(p->args[i] == VoidPtrTyArg)
                types.push_back(getVoidPtrTy());
            else if(p->args[i] == Int64TyArg)
                types.push_back(getInt64Ty());
            else
                assert(p->args[i]==0 && "other arg types not supported!");
        }

        if(types.empty())
            continue;

        ArrayRef<Type*> tys(types);
        FunctionType * funType = FunctionType::get(Type::getVoidTy(module->getContext()),tys, false);
        intrinsicFunMap[p->n] = cast<Function>(module->getOrInsertFunction(p->n, funType));
    }
}

SaberInst* SaberInst::Create(ArrayRef<Value *> args,
                             Instruction *insertBefore, Function* fn) {
    assert(insertBefore && "insert before instruction is null!!");
    assert(fn && "please specify intrinsic function!!");
    return cast<SaberInst>(CallInst::Create(fn, args, "", insertBefore));
}

