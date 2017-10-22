/*
 * Instrument.h
 *
 *  Created on: Jun 6, 2013
 *      Author: Yulei Sui
 */

#ifndef INSTRUMENTATION_H_
#define INSTRUMENTATION_H_

#include "IntrinsicInst.h"

#include <set>

class Instrumentor;

/*
 * Instrumentation pass
 */
class TransformPass: public llvm::ModulePass {

public:
    typedef std::set<const llvm::Function*> FunSet;
private:
    //two kinds of functions during our instrumentation
    // injected functions are totally new functions(manipulating meta, perform checks)
    // transformed functions are lib functions in original source code
    // they are renamed and transformed into an internal function
    // transforming functions are done in this pass
    // while injecting functions are done in Intrumentor.cpp
    FunSet injectedFuns;
    FunSet transformedFuns;
    Instrumentor* instrumentor;

    void destroy();
public:
    static char ID;

    /// we start from here
    bool runOnModule(llvm::Module& module);

    TransformPass() :
        ModulePass(ID), instrumentor(NULL) {
    }

    virtual ~TransformPass() {
        destroy();
    }

    Instrumentor* getInstrumentor(llvm::Module& module);

    llvm::Function* transformMain(llvm::Module& module);

    void transformFunctions(llvm::Module& module);

    llvm::Function* renameFunction(llvm::Function* func);

    std::string transformFunctionName(const std::string &str);

    void setInjectedFuns(const FunSet& funs);

    bool isInjectedFun(llvm::Function* fun);

    bool isTransformedFun(llvm::Function* fun);

    bool isInterestedTransFun(llvm::Function* fun);

    virtual const char* getPassName() const {
        return " InstrumentationPass";
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& au) const {
        au.addRequired<IntrinsicsPass>();
    }
};

#endif /* INSTRUMENT_H_ */
