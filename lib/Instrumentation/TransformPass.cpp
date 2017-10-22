/*
 * Instrument.cpp
 *
 *  Created on: Jun 6, 2013
 *      Author: Yulei Sui
 */

#include "Instrumentation/TransformPass.h"
#include "Instrumentation/BoundsInstr.h"
#include "Instrumentation/SaberInstr.h"
#include "Instrumentation/SaberIntrinsics.h"
#include "Util/AnalysisUtil.h"
#include <llvm/Support/CommandLine.h> // for tool output file
#include <llvm/Support/raw_ostream.h>	// for output

#define LLVM_32_COMPILE 1 //set 0 when compling with version 3.1
using namespace llvm;

static RegisterPass<TransformPass> TransformPass("pinst",
        "Instrumentation Pass");
static cl::opt<bool> SaberInstr("sbInstr", cl::init(true),
                                cl::desc("Print points-to set"));

char TransformPass::ID = 0;

/*!
 * Memory clean up
 */
void TransformPass::destroy() {
    delete instrumentor;
    instrumentor = NULL;
}

/*!
 *
 */
Instrumentor* TransformPass::getInstrumentor(llvm::Module& module) {

    if(instrumentor == NULL) {
        if(SaberInstr) {
            SaberIntrinsics::Create(module);
            return new SaberInstrumentor(&module);
        }
        else
            return new BoundsInstrumentor(&module);
    }
    return instrumentor;
}

/*!
 *
 */
bool TransformPass::runOnModule(llvm::Module& module) {

    instrumentor = getInstrumentor(module);
    // inject metadata manipulated intrisic functions
    instrumentor->instrument();
    // indicate injected function after instrumentation
    setInjectedFuns(instrumentor->getInjectFuns());

    if (Function* pesoduMain = transformMain(module))
        transformedFuns.insert(pesoduMain);

    transformFunctions(module);

    return false;
}

/*!
 *
 */
void TransformPass::setInjectedFuns(const FunSet& funs) {
    for (FunSet::const_iterator iter = funs.begin(); iter != funs.end(); ++iter)
        injectedFuns.insert(*iter);
}

/*!
 *
 */
bool TransformPass::isInjectedFun(Function* fun) {
    return injectedFuns.count(fun);
}

/*!
 *
 */
bool TransformPass::isTransformedFun(Function* fun) {

    return transformedFuns.count(fun);
}

/*!
 *
 */
bool TransformPass::isInterestedTransFun(Function* fun) {
    return instrumentor->isInterestedTransFun(fun);
}

/*!
 *
 */
std::string TransformPass::transformFunctionName(const std::string &str) {
    return "__dvf_" + str;
}

/*!
 *
 */
void TransformPass::transformFunctions(Module& module) {

    std::vector<Function*> funs;

    for (Module::iterator fb = module.begin(); fb != module.end(); ++fb) {
        Function* fun = &*fb;
        //TODO: do we need to place intrinsic condition here?
        if (isInjectedFun(fun) || isTransformedFun(fun) || fun->isIntrinsic())
            continue;

        if (isInterestedTransFun(fun))
            funs.push_back(fun);
    }

    while (!funs.empty()) {
        Function* fun = funs.back();
        funs.pop_back();
        if (Function* newFun = renameFunction(fun))
            transformedFuns.insert(newFun);
    }
}

Function* TransformPass::renameFunction(Function* func) {

    assert(isInterestedTransFun(func)
           && "function should not be transformed!!");

    Type* ret_type = func->getReturnType();
    const FunctionType* fty = func->getFunctionType();
    std::vector<Type*> params;

    if (isTransformedFun(func))
        return NULL;

    int arg_index = 1;

    for (Function::arg_iterator i = func->arg_begin(), e = func->arg_end();
            i != e; ++i, arg_index++) {

        params.push_back(i->getType());
    }

    FunctionType* nfty = FunctionType::get(ret_type, params, fty->isVarArg());
    Function* new_func = Function::Create(nfty, func->getLinkage(),
                                          transformFunctionName(func->getName()));
    new_func->copyAttributesFrom(func);

    Module* module = func->getParent();
    Module::iterator func_iter(func);
    module->getFunctionList().insert(func_iter, new_func);

    // FIXME:: maybe we need to refer to the inject function
    if (!analysisUtil::isExtCall(func)) {
        SmallVector<Value*, 16> call_args;
        new_func->getBasicBlockList().splice(new_func->begin(),
                                             func->getBasicBlockList());
        Function::arg_iterator arg_i2 = new_func->arg_begin();
        for (Function::arg_iterator arg_i = func->arg_begin(), arg_e =
                    func->arg_end(); arg_i != arg_e; ++arg_i) {

            arg_i->replaceAllUsesWith(&*arg_i2);
            arg_i2->takeName(&*arg_i);
            ++arg_i2;
            arg_index++;
        }
    }

    func->replaceAllUsesWith(new_func);
    func->eraseFromParent();

    return new_func;
}

Function* TransformPass::transformMain(Module& module) {

    Function* main_func = module.getFunction("main");

    //
    // If the program doesn't have main then don't do anything
    //
    if (!main_func)
        return NULL;

    Type* ret_type = main_func->getReturnType();
    const FunctionType* fty = main_func->getFunctionType();
    std::vector<Type*> params;

    SmallVector<AttributeSet, 8> param_attrs_vec;
    const AttributeSet& pal = main_func->getAttributes();

    //
    // Get the attributes of the return value
    //
    AttributeSet attrs = pal.getRetAttributes();
    if (attrs.hasAttributes(AttributeSet::ReturnIndex))
        param_attrs_vec.push_back(
            AttributeSet::get(main_func->getContext(), attrs));
    // Get the attributes of the arguments
    int arg_index = 1;
    for (Function::arg_iterator i = main_func->arg_begin(), e =
                main_func->arg_end(); i != e; ++i, arg_index++) {
        params.push_back(i->getType());
        AttributeSet attrs = pal.getParamAttributes(arg_index);
        if (attrs.hasAttributes(arg_index)) {
            AttrBuilder B(attrs, arg_index);
            param_attrs_vec.push_back(
                AttributeSet::get(main_func->getContext(), params.size(),
                                  B));
        }
    }

    FunctionType* nfty = FunctionType::get(ret_type, params, fty->isVarArg());
    Function* new_func = NULL;

    // create the new function
    new_func = Function::Create(nfty, main_func->getLinkage(), "pseudo_main");

    // set the new function attributes
    new_func->copyAttributesFrom(main_func);

    new_func->setAttributes(
        AttributeSet::get(module.getContext(), param_attrs_vec));
    Module::iterator func_iter(main_func);
    main_func->getParent()->getFunctionList().insert(func_iter, new_func);
    main_func->replaceAllUsesWith(new_func);

    //
    // Splice the instructions from the old function into the new
    // function and set the arguments appropriately
    //
    new_func->getBasicBlockList().splice(new_func->begin(),
                                         main_func->getBasicBlockList());
    Function::arg_iterator arg_i2 = new_func->arg_begin();
    for (Function::arg_iterator arg_i = main_func->arg_begin(), arg_e =
                main_func->arg_end(); arg_i != arg_e; ++arg_i) {
        arg_i->replaceAllUsesWith(&*arg_i2);
        arg_i2->takeName(&*arg_i);
        ++arg_i2;
        arg_index++;
    }
    //
    // Remove the old function from the module
    //
    main_func->eraseFromParent();

    return new_func;
}

