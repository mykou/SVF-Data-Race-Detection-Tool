/*
 * IntrinsicInst.cpp
 *
 *  Created on: Jun 6, 2013
 *      Author: Yulei Sui
 */

#include "Instrumentation/IntrinsicInst.h"

#include <llvm/Support/raw_ostream.h>	// for output
using namespace llvm;

static RegisterPass<IntrinsicsPass> InstrinsicPass("pintrinsics",
        "Intrinsics instruction Pass");

PTAIntrinsics::IntrinsicFunMap PTAIntrinsics::intrinsicFunMap;

char IntrinsicsPass::ID = 0;
bool IntrinsicsPass::Initialized = false;

llvm::PointerType*IntrinsicsPass::voidPtrTy = NULL;
llvm::Type*IntrinsicsPass::SizeTy = NULL;
llvm::Module * IntrinsicsPass::module = NULL;
llvm::Function* IntrinsicsPass::VirtualVar = NULL;
llvm::Function* IntrinsicsPass::CSUseVirtualVar = NULL;
llvm::Function* IntrinsicsPass::CSDefVirtualVar = NULL;
llvm::Function* IntrinsicsPass::FNUseVirtualVar = NULL;
llvm::Function* IntrinsicsPass::FNDefVirtualVar = NULL;

llvm::Function* IntrinsicsPass::MetadataInst = NULL;
llvm::Function* IntrinsicsPass::MetadataInitialInst = NULL;
llvm::Function* IntrinsicsPass::MetadataStoreInst = NULL;
llvm::Function* IntrinsicsPass::MetadataLoadInst = NULL;
llvm::Function* IntrinsicsPass::MetadataPropageteInst = NULL;
llvm::Function* IntrinsicsPass::MetadataCheckInst = NULL;
llvm::Function* IntrinsicsPass::MetadataLoadCounterInst = NULL;
llvm::Function* IntrinsicsPass::MetadataStoreCounterInst = NULL;

VirtualVar* VirtualVar::Create(ArrayRef<Value *> args,
                               Instruction *insertBefore, Function* fn) {
    assert(insertBefore && "insert before instruction is null!!");
    assert(
        IntrinsicsPass::Initialized
        && "intrinsic pass should be intialized first!!");
    return llvm::cast<VirtualVar>(
               llvm::CallInst::Create(fn, args, "", insertBefore));
}

MetadataInst* MetadataInst::Create(ArrayRef<Value *> args,
                                   Instruction *insertBefore, Function* fn) {
    assert(insertBefore && "insert before instruction is null!!");
    assert(
        IntrinsicsPass::Initialized
        && "intrinsic pass should be intialized first!!");
    return cast<MetadataInst>(CallInst::Create(fn, args, "", insertBefore));
}

bool IntrinsicsPass::runOnModule(Module &M) {
    module = &M;

    // which void pointer type shall we choose?
    // voidPtrTy = PointerType::getUnqual(Type::getInt8PtrTy(M.getContext()));
    voidPtrTy = PointerType::getUnqual(Type::getInt8Ty(M.getContext()));
    SizeTy = Type::getInt64Ty(M.getContext());

    FunctionType *virtualVarFnTy = FunctionType::get(
                                       Type::getVoidTy(M.getContext()), false);
    FunctionType * MDFnTy = virtualVarFnTy;

    Type *MDInitialTyArgTys[] = { voidPtrTy, voidPtrTy, voidPtrTy };
    FunctionType * MDInitialTy = FunctionType::get(
                                     Type::getVoidTy(M.getContext()), MDInitialTyArgTys, false);

    Type *MDLdStArgTys[] = { voidPtrTy, voidPtrTy };
    FunctionType * MDLoad = FunctionType::get(Type::getVoidTy(M.getContext()),
                            MDLdStArgTys, false);
    FunctionType * MDStore = MDLoad;
    FunctionType * MDProp = MDLoad;

    Type *MDCheckArgTys[] = { voidPtrTy, SizeTy };
    FunctionType * MDCheck = FunctionType::get(Type::getVoidTy(M.getContext()),
                             MDCheckArgTys, false);

    VirtualVar = cast<Function>(
                     module->getOrInsertFunction("VirtualVar", virtualVarFnTy));
    CSUseVirtualVar = cast<Function>(
                          module->getOrInsertFunction("CSUseVirtualVar", virtualVarFnTy));
    CSDefVirtualVar = cast<Function>(
                          module->getOrInsertFunction("CSDefVirtualVar", virtualVarFnTy));
    FNUseVirtualVar = cast<Function>(
                          module->getOrInsertFunction("FNUseVirtualVar", virtualVarFnTy));
    FNDefVirtualVar = cast<Function>(
                          module->getOrInsertFunction("FNDefVirtualVar", virtualVarFnTy));

    MetadataInst = cast<Function>(
                       module->getOrInsertFunction("__metadataInst", MDFnTy));
    MetadataInitialInst = cast<Function>(
                              module->getOrInsertFunction("__metadataInitialInst", MDInitialTy));

    MetadataStoreInst = cast<Function>(
                            module->getOrInsertFunction("__metadataStoreInst", MDStore));
    MetadataLoadInst = cast<Function>(
                           module->getOrInsertFunction("__metadataLoadInst", MDLoad));
    MetadataPropageteInst = cast<Function>(
                                module->getOrInsertFunction("__metadataPropagateInst", MDProp));
    MetadataCheckInst = cast<Function>(
                            module->getOrInsertFunction("__metadataCheckInst", MDCheck));

    MetadataLoadCounterInst = cast<Function>(
                                  module->getOrInsertFunction("__metadataStoreCounter", MDFnTy));
    MetadataStoreCounterInst = cast<Function>(
                                   module->getOrInsertFunction("__metadataLoadCounter", MDFnTy));

    Initialized = true;

    return false;
}
