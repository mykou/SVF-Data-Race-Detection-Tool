/*
 * ResultValidator.h
 *
 *  Created on: 26/10/2015
 *      Author: dye
 */

#ifndef RESULTVALIDATOR_H_
#define RESULTVALIDATOR_H_

#include "MemoryPartitioning.h"
#include "RCResultValidator.h"


/*!
 * ResultValidator is a subclass of RCResultValidator.
 * It defines a concrete result validator for the regression test
 * programs in the RaceComb project.
 */
class ResultValidator: public RCResultValidator {
public:
    typedef MemoryPartitioning::AccessID AccessID;
    typedef MemoryPartitioning::PartID PartID;
    typedef MemoryPartitioning::AccessIdVector AccessIdVector;
    typedef std::map<const llvm::Instruction*, std::vector<AccessID> > Inst2Ids;

    /// Constructor
    ResultValidator(MemoryPartitioning *mp, ThreadEscapeAnalysis *tea,
            MhpAnalysis *mhp, LocksetAnalysis *lsa, BarrierAnalysis *ba,
            ThreadJoinRefinement *joinRefine, PathRefinement *pathRefine,
            HeapRefinement *heapRefine, ContextSensitiveAliasAnalysis *csaa) :
            mp(mp), tea(tea), mhp(mhp), lsa(lsa), ba(ba), joinRefine(joinRefine),
            pathRefine(pathRefine), heapRefine(heapRefine), csaa(csaa) {
    }

    /// Perform the validation.
    void analyze();

    /// Override the interface methods.
    //@{
    bool mayAccessAliases(const llvm::Instruction *I1,
            const llvm::Instruction *I2);
    bool mayHappenInParallel(const llvm::Instruction *I1,
            const llvm::Instruction *I2);
    bool protectedByCommonLocks(const llvm::Instruction *I1,
            const llvm::Instruction *I2);
    bool mayHaveDataRace(const llvm::Instruction *I1,
            const llvm::Instruction *I2);
    //@}

private:
    /*!
     * Check if two Instructions access different heap instances.
     * @param partId memory partition id
     * @param I1 memory access instruction
     * @param I2 memory access instruction
     * @return true if (1) partId contains exactly one heap object,
     *         and (2) I1 and I2 must access different instances of the object.
     */
    bool heapRefined(PartID partId, const llvm::Instruction *I1,
            const llvm::Instruction *I2);

    MemoryPartitioning *mp;
    ThreadEscapeAnalysis *tea;
    MhpAnalysis *mhp;
    LocksetAnalysis *lsa;
    BarrierAnalysis *ba;
    ThreadJoinRefinement *joinRefine;
    PathRefinement *pathRefine;
    HeapRefinement *heapRefine;
    ContextSensitiveAliasAnalysis *csaa;
    Inst2Ids inst2accessIds;
    Inst2Ids inst2partIds;
};


#endif /* RESULTVALIDATOR_H_ */
