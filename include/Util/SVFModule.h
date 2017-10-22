//===- SVFModule.h -- SVFModule class-----------------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2017>  <Yulei Sui>
// 

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

/*
 * SVFModule.h
 *
 *  Created on: Aug 4, 2017
 *      Author: Xiaokang Fan
 */

#ifndef SVFMODULE_H_
#define SVFMODULE_H_

#include "Util/BasicTypes.h"
#include <llvm/IR/LLVMContext.h>		// for llvm LLVMContext
#include <llvm/IR/Module.h>

class SVFModule {
public:
    typedef std::vector<llvm::Function*> FunctionSetType;
    typedef std::vector<llvm::GlobalVariable*> GlobalSetType;
    typedef std::vector<llvm::GlobalAlias*> AliasSetType;

    typedef std::map<const llvm::Function*, llvm::Function*> FunDeclToDefMapTy;
    typedef std::map<const llvm::Function*, FunctionSetType> FunDefToDeclsMapTy;
    typedef std::map<const llvm::GlobalVariable*, llvm::GlobalVariable*> GlobalDefToRepMapTy;

    /// Iterators type def
    typedef FunctionSetType::iterator iterator;
    typedef FunctionSetType::const_iterator const_iterator;
    typedef GlobalSetType::iterator global_iterator;
    typedef GlobalSetType::const_iterator const_global_iterator;
    typedef AliasSetType::iterator alias_iterator;
    typedef AliasSetType::const_iterator const_alias_iterator;

private:
    u32_t moduleNum;
    llvm::LLVMContext *cxts;
    std::unique_ptr<llvm::Module> *modules;

    FunctionSetType FunctionSet;  ///< The Functions in the module
    GlobalSetType GlobalSet;      ///< The Global Variables in the module
    AliasSetType AliasSet;        ///< The Aliases in the module

    /// Function declaration to function definition map
    FunDeclToDefMapTy FunDeclToDefMap;
    /// Function definition to function declaration map
    FunDefToDeclsMapTy FunDefToDeclsMap;
    /// Global definition to a rep definition map
    GlobalDefToRepMapTy GlobalDefToRepMap;

    static SVFModule *svfModule;

public:
    /// Constructor
    SVFModule(const std::vector<std::string> &moduleNameVec);
    SVFModule() {}

    void build(const std::vector<std::string> &moduleNameVec);

    u32_t getModuleNum() const {
        return moduleNum;
    }

    llvm::Module *getModule(u32_t idx) const {
        assert(idx < moduleNum && "Out of range.");
        return modules[idx].get();
    }

    llvm::Module &getModuleRef(u32_t idx) const {
        assert(idx < moduleNum && "Out of range.");
        return *(modules[idx].get());
    }

    // Dump modules to files
    void dumpModulesToFile(const std::string suffix);

    /// Fun decl --> def
    bool hasDefinition(const llvm::Function *fun) const {
        assert(fun->isDeclaration() && "not a function declaration?");
        FunDeclToDefMapTy::const_iterator it = FunDeclToDefMap.find(fun);
        return it != FunDeclToDefMap.end();
    }

    llvm::Function *getDefinition(const llvm::Function *fun) const {
        assert(fun->isDeclaration() && "not a function declaration?");
        FunDeclToDefMapTy::const_iterator it = FunDeclToDefMap.find(fun);
        assert(it != FunDeclToDefMap.end() && "has no definition?");
        return it->second;
    }

    /// Fun def --> decl
    bool hasDeclaration(const llvm::Function *fun) const {
        assert(!fun->isDeclaration() && "not a function definition?");
        FunDefToDeclsMapTy::const_iterator it = FunDefToDeclsMap.find(fun);
        return it != FunDefToDeclsMap.end();
    }

    const FunctionSetType &getDeclaration(const llvm::Function *fun) const {
        assert(!fun->isDeclaration() && "not a function definition?");
        FunDefToDeclsMapTy::const_iterator it = FunDefToDeclsMap.find(fun);
        assert(it != FunDefToDeclsMap.end() && "has no declaration?");
        return it->second;
    }

    /// Global to rep
    bool hasGlobalRep(const llvm::GlobalVariable *val) const {
        GlobalDefToRepMapTy::const_iterator it = GlobalDefToRepMap.find(val);
        return it != GlobalDefToRepMap.end();
    }

    llvm::GlobalVariable *getGlobalRep(const llvm::GlobalVariable *val) const {
        GlobalDefToRepMapTy::const_iterator it = GlobalDefToRepMap.find(val);
        assert(it != GlobalDefToRepMap.end() && "has no rep?");
        return it->second;
    }

    /// Iterators
    ///@{
    iterator begin() {
        return FunctionSet.begin();
    }
    const_iterator begin() const {
        return FunctionSet.begin();
    }
    iterator end() {
        return FunctionSet.end();
    }
    const_iterator end() const {
        return FunctionSet.end();
    }

    global_iterator global_begin() {
        return GlobalSet.begin();
    }
    const_global_iterator global_begin() const {
        return GlobalSet.begin();
    }
    global_iterator global_end() {
        return GlobalSet.end();
    }
    const_global_iterator global_end() const {
        return GlobalSet.end();
    }

    alias_iterator alias_begin() {
        return AliasSet.begin();
    }
    const_alias_iterator alias_begin() const {
        return AliasSet.begin();
    }
    alias_iterator alias_end() {
        return AliasSet.end();
    }
    const_alias_iterator alias_end() const {
        return AliasSet.end();
    }
    ///@}

    static inline SVFModule *getSVFModule() {
        if (svfModule == NULL)
            svfModule = new SVFModule;
        return svfModule;
    }

    static void releaseSVFModule() {
        if (svfModule)
            delete svfModule;
        svfModule = NULL;
    }

private:
    void loadModules(const std::vector<std::string> &moduleNameVec);
    void initialize();

    void buildFunToFunMap();
    void buildGlobalDefToRepMap();
};


#endif /* SVFMODULE_H_ */
