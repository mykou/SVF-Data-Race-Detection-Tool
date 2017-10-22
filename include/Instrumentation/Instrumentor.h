/*
 * Instrumentor.h
 *
 *  Created on: Jun 13, 2013
 *      Author: Yulei Sui
 */

#ifndef INSTRUMENTOR_H_
#define INSTRUMENTOR_H_

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>

#include <llvm/IR/InstVisitor.h>	// for instruction visitor
#include <map>
#include <set>

/*!
 * Base instrumentor class as interface for derived classes
 */
class Instrumentor: public llvm::InstVisitor<Instrumentor> {

public:
    typedef std::set<const llvm::Function*> FunSet;
    typedef std::map<const llvm::Value*, llvm::BitCastInst *> ValToVPtrMap;
    typedef std::map<const llvm::Function*, llvm::BranchInst*> FunToDummyBBMap;
    typedef std::map<const llvm::Function*, ValToVPtrMap> FunToValVPtrMap;

private:
    llvm::Module* module;		///< llvm module
    llvm::Type* voidPtrTy;		///< llvm void ptr type
    FunToValVPtrMap valToVPtrMap;	///< map a pointer to its void ptr type
    FunToDummyBBMap funToBBMap;		///< map a function to its dummy entry basic block
    FunSet injectedFuns;	///< store functions which are injected during instrumentation

public:
    /// Perform instrumentation
    virtual void instrument() = 0;
    /// Whether an existing function needs to be transformed (rename to an instrinsic function)
    virtual bool isInterestedTransFun(llvm::Function* fun) = 0;
    /// Our visit overrides.
    //@{
    // Instructions that cannot be folded away.
    virtual void visitAllocaInst(llvm::AllocaInst &AI) = 0;
    virtual void visitPHINode(llvm::PHINode &I) = 0;
    virtual void visitStoreInst(llvm::StoreInst &I) = 0;
    virtual void visitLoadInst(llvm::LoadInst &I) = 0;
    virtual void visitGetElementPtrInst(llvm::GetElementPtrInst &I) = 0;
    virtual void visitReturnInst(llvm::ReturnInst &I) = 0;
    virtual void visitCastInst(llvm::CastInst &I) = 0;
    virtual void visitSelectInst(llvm::SelectInst &I) = 0;
    inline void visitCallInst(llvm::CallInst &I) {
        visitCallSite(&I);
    }
    inline void visitInvokeInst(llvm::InvokeInst &II) {
        visitCallSite(&II);
    }
    void visitCallSite(llvm::CallSite cs);

    //@}

    /// Provide base case for our instruction visit.
    inline void visitInstruction(llvm::Instruction &I) {
        // If a new instruction is added to LLVM that we don't handle.
        // TODO: ignore here:
    }

    /// handle callsite
    //@{
    virtual void handleExtCall(llvm::CallSite cs) = 0;

    virtual void handleIndCall(llvm::CallSite cs) = 0;

    virtual void handleDirectCall(llvm::CallSite cs, const llvm::Function *F) = 0;
    //@}

protected:

    /// Cast a pointer to a void pointer type
    //@{
    llvm::Value* castToVoidPtr(llvm::Value* pointer,
                               llvm::Instruction* insert_before);

    llvm::Value* castToVoidPtr(llvm::Value* pointer, llvm::StringRef NameStr,
                               llvm::Instruction* insert_before);
    //@}

    /// Get and set method for valToVPtrMap and funToBBMap
    //@{
    inline llvm::BitCastInst* getFromValToPtrMap(const llvm::Function* fun,
            const llvm::Value* val) const {
        assert(isObversedFun(fun) && "not an observed function!");
        FunToValVPtrMap::const_iterator fit = valToVPtrMap.find(fun);
        if (fit != valToVPtrMap.end()) {
            ValToVPtrMap::const_iterator vit = fit->second.find(val);
            if (vit!=fit->second.end())
                return vit->second;
            else
                return NULL;
        } else
            return NULL;
    }

    inline void addToValToPtrMap(const llvm::Function* fun, const llvm::Value* val,
                                 llvm::BitCastInst* bitcast) {
        assert(isObversedFun(fun) && "not an observed function!");
        assert(valToVPtrMap[fun].count(val) == false
               && "not allowed to re-associate!!");
        valToVPtrMap[fun][val] = bitcast;
    }

    inline llvm::BranchInst* getFunEntryDummyBB(const llvm::Function* fun) const {
        assert(isObversedFun(fun) && "not an observed function!");
        FunToDummyBBMap::const_iterator it = funToBBMap.find(fun);
        if (it!=funToBBMap.end())
            return it->second;
        else
            return NULL;
    }

    inline void addFunEntryDummyBB(const llvm::Function* fun, llvm::BranchInst* br) {
        assert(isObversedFun(fun) && "not an observed function!");
        assert(funToBBMap.find(fun) == funToBBMap.end() && "not allowed to re-associate!!");
        funToBBMap[fun] = br;
    }
    //@}

    /// whether the function is in module
    inline bool isObversedFun(const llvm::Function *F) const {
        for (llvm::Module::const_iterator fi = module->begin(), fe = module->end();
                fi != fe; ++fi) {
            const llvm::Function* fun = &*fi;
            if (fun == F)
                return true;
        }
        return false;
    }

    /// Helper functions
    //@{
    llvm::Constant* getSizeOfType(llvm::Type* input_type);

    llvm::Instruction* getNextInst(llvm::Instruction* inst);
    //@}
public:

    /// Constructor
    Instrumentor(llvm::Module* m);
    /// Destructor
    virtual ~Instrumentor() {
    }

    /// Get/Set injected functions
    ///@{
    inline const FunSet& getInjectFuns() const {
        return injectedFuns;
    }
    inline bool addInjectedFun(const llvm::Function* fun) {
        return injectedFuns.insert(fun).second;
    }
    //@}

    inline llvm::Module* getModule() const {
        return module;
    }
};

#endif /* INSTRUMENTOR_H_ */
