/*
 * RCAnnotaton.h
 *
 *  Created on: 23/06/2015
 *      Author: dye
 */

#ifndef RCANNOTATON_H_
#define RCANNOTATON_H_

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Type.h>
#include <set>

class RCStaticAnalysisResult;


/*!
 * The base class defining the annotation tag names for
 * the RCAnnotator and RCAnnotationExtractor classes.
 */
class RCAnnotationBase {
protected:
    static constexpr const char* const AnnotationPairedInstId = "rc.pii";
    static constexpr const char* const AnnotationPairsInfo = "rc.pairs.info";
    static constexpr const char* const AnnotationRegCheck = "DRCHECK_";
};



/*!
 * \brief The extractor class for extracting the RaceComb's annotations
 * from the LLVM IR.
 *
 * It includes two types of annotations:
 * (1) All racy Instructions for the regular checking purposes (e.g., TSan).
 *
 * (2) The racy Instruction pairs for the paired checking purposes
 * (e.g., ThreadScheduling).
 */
class RCAnnotationExtractor : RCAnnotationBase {
public:

    /// Typedefs of InstPairs
    /// (i.e., Instruction pairs for the paired checks)
    //@{
    typedef std::pair<const llvm::Instruction*, const llvm::Instruction*> InstPair;
    typedef std::vector<InstPair> InstPairs;
    typedef InstPairs::iterator pair_iterator;
    typedef InstPairs::const_iterator const_pair_iterator;
    //@}

    /// Typedefs of MemoryPartitions
    /// (i.e., Grouped memory objects which are access equivalent)
    //@{
    typedef std::vector<InstPairs> InstPairsGroup;
    typedef InstPairsGroup::iterator pairsgroup_iterator;
    typedef InstPairsGroup::const_iterator const_pairsgroup_iterator;
    //@}

    /// Typedefs of InstSet
    /// (i.e., Instructions for the regular checks)
    //@{
    typedef std::set<const llvm::Instruction*> InstSet;
    typedef InstSet::iterator inst_iterator;
    typedef InstSet::const_iterator const_inst_iterator;
    //@}

    /// Constructor
    RCAnnotationExtractor() : M(NULL), numOfAllPairs(0) {
    }

    /// Initialization
    void init(llvm::Module *M);

    /// Perform annotation extraction
    void run();

    /// Release resources
    void release();

    /// Iterators of MPs that consist the racy Instruction pairs
    /// (i.e., Instruction pairs for the paired checks.)
    //@{
    inline pairsgroup_iterator pairsGroupBegin() {
        return pairsGroup.begin();
    }
    inline pairsgroup_iterator pairsGroupEnd() {
        return pairsGroup.end();
    }
    inline const_pairsgroup_iterator pairsGroupBegin() const {
        return pairsGroup.begin();
    }
    inline const_pairsgroup_iterator pairsGroupEnd() const {
        return pairsGroup.end();
    }
    //@}

    /// Number of racy Instruction pairs
    //@{
    /// The number of groups (where Instructions access the same memory partition)
    inline size_t getPairsGroupCount() const {
        return pairsGroup.size();
    }
    /// The number of pairs for a particular group
    inline size_t getNumOfPairsInGroup(size_t groupIdx) const {
        assert(groupIdx < pairsGroup.size() && "groupIdx is out of bounds");
        return pairsGroup[groupIdx].size();
    }
    /// The number of pairs for all groups
    inline size_t getNumOfAllPairs() const {
        return numOfAllPairs;
    }
    //@}

    /// Get one racy Instruction pair
    //@{
    /// Get a pair with universal index
    InstPair getOnePair(size_t i) const;
    /// Get a pair in a group with group-local index
    inline InstPair getOnePairInGroup(size_t groupIdx, size_t pairIdx) const {
        assert(groupIdx < pairsGroup.size() && "groupIdx is out of bounds");
        assert(pairIdx < pairsGroup[groupIdx].size() && "pairIdx is out of bounds");
        return pairsGroup[groupIdx][pairIdx];
    }
    //@}

    /// Mapping between Instructions to their annotation IDs
    //@{
    /// Instruction -> annotation ID
    size_t getAnnotationId(const llvm::Instruction *I) const;
    /// Annotation ID -> Instruction
    const llvm::Instruction *getInstruction(size_t id) const {
        auto it = id2InstMap.find(id);
        assert(it != id2InstMap.end() && "No Instruction found for the annotation ID");
        return it->second;
    }
    //@}

    /// Iterators of all racy Instructions
    /// (i.e., Instructions for the regular checks)
    //@{
    inline inst_iterator instBegin() {
        return allRacyInsts.begin();
    }
    inline inst_iterator instEnd() {
        return allRacyInsts.end();
    }
    inline const_inst_iterator instBegin() const {
        return allRacyInsts.begin();
    }
    inline const_inst_iterator instEnd() const {
        return allRacyInsts.end();
    }
    //@}

    /// Number of all racy Instructions
    /// (i.e., those for the regular check)
    inline size_t getNumOfRacyInsts() const {
        return allRacyInsts.size();
    }

    /// Check if a given Instruction is racy
    /// (i.e., should perform a regular check).
    inline bool isRacyInst(const llvm::Instruction *I) const {
        return allRacyInsts.count(I);
    }


protected:

    typedef llvm::DenseMap<size_t, const llvm::Instruction*> Id2InstMap;

    llvm::Module *M;
    Id2InstMap id2InstMap;
    InstSet allRacyInsts;
    InstPairsGroup pairsGroup;
    size_t numOfAllPairs;
};



/*!
 * \brief RaceComb's annotator to perform the IR-level annotations.
 *
 * It is used by the static analyzer to embed the knowledge of possible
 * data races (i.e., the static analysis result) into LLVM IR.
 *
 * The the information appears in the following two forms:
 *
 * (1) individual memory access Instructions that may race
 * with other Instruction(s);
 *
 * (2) memory access Instruction pairs that may cause any data race.
 */
class RCAnnotator : RCAnnotationBase {
public:

    typedef llvm::DenseMap<const llvm::Instruction*, size_t> Inst2IdMap;

    /// Constructor
    RCAnnotator() :
            M(NULL), raceResult(NULL), numOfRacyInsts(0), numOfPairedInsts(0),
            numOfInstPairs() {
    }

    /// Initialization
    void init(llvm::Module *M, RCStaticAnalysisResult *result);

    /// Perform annotation
    void run();

    /// Release resources
    void release();

    /// Print annotation result
    void print() const;

    /// Get the number of all racy Instructions annotated
    inline size_t getNumOfRacyInsts() const {
        return numOfRacyInsts;
    }

    /// Get the number of paired Instructions annotated
    inline size_t getNumOfPairedInsts() const {
        return numOfPairedInsts;
    }

    /// Get the number of Instruction pairs
    inline size_t getNumOfPairs() const {
        return numOfInstPairs;
    }

protected:

    /// Annotate all racy Instructions for regular check.
    void performAnnotationForRegularCheck();

    /// Annotate the racy Instruction pairs for the paired check.
    void performAnnotationForPairedCheck();

    /**
     * Annotate a given Instruction with a single metadata
     * @param I the Instruction to annotate
     * @param annoStr the annotation kind
     * @param metadataStr
     * @return false if the kind of annotation already exists; true otherwise
     */
    static bool annotateInst(const llvm::Instruction *I, const char *annoStr,
            const char *metadataStr);

private:
    /// Setting Ids for the paired Instructions (begins with 0).
    void pairedInstNumbering();

    /// Annotate the racy pair information on the LLVM-IR.
    void annotateRacyPairs();

    llvm::Module *M;
    RCStaticAnalysisResult *raceResult;
    Inst2IdMap inst2IdMap;
    size_t numOfRacyInsts;
    size_t numOfPairedInsts;
    size_t numOfInstPairs;
};



#endif /* RCANNOTATON_H_ */
