/*
 * RCResult.h
 *
 *  Created on: 22/06/2016
 *      Author: dye
 */

#ifndef RCRESULT_H_
#define RCRESULT_H_

#include "MemoryPartitioning.h"


/*!
 * The records of the static data race detection results.
 */
class RCStaticAnalysisResult {
public:
    typedef RCMemoryPartitioning::RiskyInstructionSet RiskyInstructionSet;

    /*!
     * A data race warning, including the memory objects that
     * may be accessed as well as the corresponding access operations.
     */
    class DataRaceWarning {
    public:
        typedef llvm::SmallVector<const llvm::Instruction*, 8> InstVec;
        typedef llvm::SmallVector<NodeID, 8> IdSet;
        typedef InstVec::iterator inst_iterator;
        typedef InstVec::const_iterator const_inst_iterator;
        typedef IdSet::iterator id_iterator;
        typedef IdSet::const_iterator const_id_iterator;

        /// Constructor
        //@{
        DataRaceWarning() : partId(0), riskyInstSet(NULL) {
        }
        DataRaceWarning(MemoryPartitioning::PartID part,
                const RiskyInstructionSet *riskyInstSet) :
                partId(part), riskyInstSet(riskyInstSet) {
        }
        //@}

        /// Destructor
        virtual ~DataRaceWarning() {
        }

        /// Iterators
        //@{
        inline inst_iterator instBegin() {
            return accesses.begin();
        }
        inline inst_iterator instEnd() {
            return accesses.end();
        }
        inline const_inst_iterator instBegin() const {
            return accesses.begin();
        }
        inline const_inst_iterator instEnd() const {
            return accesses.end();
        }
        inline id_iterator objBegin() {
            return objs.begin();
        }
        inline id_iterator objEnd() {
            return objs.end();
        }
        inline const_id_iterator objBegin() const {
            return objs.begin();
        }
        inline const_id_iterator objEnd() const {
            return objs.end();
        }
        //@}

        /// Getter/setter of partId
        //@{
        inline MemoryPartitioning::PartID getPartId() const {
            return partId;
        }
        inline void setPartId(MemoryPartitioning::PartID part) {
            partId = part;
        }
        //@}

        /// Getter/setter of riskyInstSet
        //@{
        inline const RiskyInstructionSet *getRiskyInstSet() const {
            return riskyInstSet;
        }
        inline void setRisktyPairs(const RiskyInstructionSet *riskyInstSet) {
            this->riskyInstSet = riskyInstSet;
        }
        //@}

        /// Obj
        //@{
        inline NodeID getObj(size_t i) const {
            return objs[i];
        }
        inline void addObj(NodeID obj) {
            objs.push_back(obj);
        }
        inline size_t getNumObjs() const {
            return objs.size();
        }
        //@}

        /// Racy memory access Instruction
        //@{
        inline const llvm::Instruction *getInstruction(size_t i) const {
            return accesses[i];
        }
        inline void addInstruction(const llvm::Instruction *I) {
            accesses.push_back(I);
        }
        inline size_t getNumInstructions() const {
            return accesses.size();
        }
        //@}

    private:
        MemoryPartitioning::PartID partId;
        IdSet objs;
        InstVec accesses;
        const RiskyInstructionSet *riskyInstSet;
    };

    typedef std::vector<DataRaceWarning> WarningVector;
    typedef WarningVector::iterator iterator;
    typedef WarningVector::const_iterator const_iterator;

    /// Constructor
    RCStaticAnalysisResult() : pta(NULL) {
    }

    /// Destructor
    virtual ~RCStaticAnalysisResult() {
    }

    /// Initialisation
    void init(BVDataPTAImpl *pta);

    /// Iterators
    //@{
    inline const_iterator begin() const {
        return warnings.begin();
    }
    inline const_iterator end() const {
        return warnings.end();
    }
    inline iterator begin() {
        return warnings.begin();
    }
    inline iterator end() {
        return warnings.end();
    }
    //@}

    /// Size
    inline size_t size() const {
        return warnings.size();
    }

    /// Add a DataRaceWarning, and return its reference.
    //@{
    inline DataRaceWarning &addWarning() {
        warnings.push_back(DataRaceWarning());
        return warnings.back();
    }
    inline DataRaceWarning &addWarning(MemoryPartitioning::PartID part,
            const RiskyInstructionSet *riskyInstSet) {
        warnings.push_back(DataRaceWarning(part, riskyInstSet));
        return warnings.back();
    }
    //@}

    /// Sort DataRaceWarnings in ascending order of objects' source locations.
    void sort();

    /// Print the data races of DataRaceResults to outs().
    void print() const;

private:
    /// Check if a pointer has an empty points-to set
    inline bool hasEmptyPts(const llvm::Value *p) const {
        NodeID id = pta->getPAG()->getValueNode(p);
        return pta->getPts(id).empty();
    }

    WarningVector warnings;    ///< Records of data race warnings
    BVDataPTAImpl *pta;

    /// Comparison function used to sort the DataRaceWarnings.
    static bool compare(const DataRaceWarning &warning1,
            const DataRaceWarning &warning2);
};


#endif /* RCRESULT_H_ */
