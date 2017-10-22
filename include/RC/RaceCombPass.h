/*
 * RaceCombPasss.h
 *
 *  Created on: 17/10/2014
 *      Author: Ding Ye
 */

#ifndef RACECOMBPASS_H_
#define RACECOMBPASS_H_

#include <llvm/Pass.h>

/*!
 * Analysis Pass for RaceComb -- Static Data Race Error Detector
 */
class RaceCombPass : public llvm::ModulePass {
public:
    /// Pass ID
    static char ID;

    /// Constructor
    RaceCombPass(char id = ID): ModulePass(ID) {
    }

    /// Destructor
    virtual ~RaceCombPass() {
    }

    /// Perform the analysis for the module.
    virtual bool runOnModule(llvm::Module& module) {
        // start analysis
        analyze(module);
        return false;
    }

    /// Get pass name
    virtual llvm::StringRef getPassName() const {
        return "RaceComb Data Race Error Detector";
    }

    /// Pass dependence
    virtual void getAnalysisUsage(llvm::AnalysisUsage& au) const;


protected:
    /// Perform the actual analysis
    void analyze(llvm::Module &M);
};



#endif /* RACECOMBPASS_H_ */
