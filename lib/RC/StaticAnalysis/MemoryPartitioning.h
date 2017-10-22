/*
 * MemoryPartitioning.cpp
 *
 *  Created on: 22/05/2015
 *      Author: dye
 */


#ifndef MEMORYPARTITIONING_H
#define MEMORYPARTITIONING_H

#include "MhpAnalysis.h"
#include "RC/RCSparseBitVector.h"
#include "MemoryModel/PointerAnalysis.h"

#define RISKY_PAIR_BUDGET 3000

class ThreadEscapeAnalysis;
class LocksetAnalysis;
class ThreadJoinRefinement;
class BarrierAnalysis;
class PathRefinement;
class HeapRefinement;
class ContextSensitiveAliasAnalysis;


/*!
 * \brief This memory partitioning process aims to partition
 * the memory objects into groups.
 *
 * It identifies the memory objects that are always accessed by the
 * same set of Instructions according the pointer analysis information.
 * By grouping those memory objects into a single partition, it is able
 * to eliminate redundant analysis to those access-equivalent objects.
 */
class MemoryPartitioning {
public:
    typedef unsigned AccessID;
    typedef unsigned PartID;
    typedef std::vector<AccessID> AccessIdVector;
    typedef RCSparseBitVector<> bitmap;

    /*!
    * Data structure representing a memory partition.
    */
    class Partition {
    public:
      PointsTo objs;                ///< partition => objects
      AccessIdVector accessIds;     ///< partition => memory accesses ids

      /// Get memory access details via access id
      /// @param id the memory access id in Partition::accesses
      //@{
      static inline const llvm::Instruction *getInstruction(AccessID id) {
          return accesses[id].getInstruction();
      }
      static inline const llvm::Value *getPointer(AccessID id) {
          return accesses[id].getPointer();
      }
      static inline bool isWriteAccess(AccessID id) {
          return accesses[id].isWriteAccess();
      }
      static inline bool isPrunedAccess(AccessID id) {
          return PRUNED_ACCESS == id;
      }
      static inline AccessID getPrunedAccessId() {
          return PRUNED_ACCESS;
      }
      static inline const MemoryAccess &getMemoryAccess(AccessID id) {
          return accesses[id];
      }
      //@}

      static std::vector<MemoryAccess> accesses;    ///< all memory accesses
      static const AccessID PRUNED_ACCESS = -1;     ///< id for pruned memory access
    };

    /// Initialization
    /// @param pta Pointer analysis.
    void init(BVDataPTAImpl *pta) {
        this->pta = pta;
    }

    /// Add MemeoryAccesses
    /// @param accesses memory access Instructions to be used to partition memory.
    void addMemoryAccesses(const std::vector<MemoryAccess> &accesses) {
        Partition::accesses.insert(Partition::accesses.end(), accesses.begin(),
                accesses.end());
    }

    /// Perform memory partitioning
    void run();

    /// Print the memory partitioning information
    void print() const;

    /// Print the information of a specific memory partition
    void print(PartID id)  const;

    /// Get pta
    //@{
    inline const BVDataPTAImpl *getPta() const {
        return pta;
    }
    inline BVDataPTAImpl *getPta() {
        return pta;
    }
    //@}

    /// Get the memory objects of a partition.
    /// @param n the memory partition id.
    //@{
    inline const PointsTo &getPartObjs(PartID n) const {
        return parts[n].objs;
    }
    inline PointsTo &getPartObjs(PartID n) {
        return parts[n].objs;
    }
    //@}

    /// Get the memory partition ID that an object belongs to.
    /// @param obj the memory object node id.
    inline PartID getPartId(NodeID obj) const {
        if (obj >= obj2part.size())
            return 0;
        return obj2part[obj];
    }

    /// Get all memory partitions.
    //@{
    inline const std::vector<Partition> &getParts() const {
        return parts;
    }
    inline std::vector<Partition> &getParts() {
        return parts;
    }
    //@}

    /// Get the number of actual memory partitions.
    inline size_t getNumParts() const {
        return parts.size() - 1;
    }

    /// Destructor
    virtual ~MemoryPartitioning() {
        parts.clear();
        obj2part.clear();
    }

    /// Get memory access details via access id
    /// @param id the memory access id in Partition::accesses
    //@{
    static inline const llvm::Instruction *getInstruction(AccessID id) {
        return Partition::getInstruction(id);
    }
    static inline const llvm::Value *getPointer(AccessID id) {
        return Partition::getPointer(id);
    }
    static inline bool isWriteAccess(AccessID id) {
        return Partition::isWriteAccess(id);
    }
    static inline bool isPrunedAccess(AccessID id) {
        return Partition::isPrunedAccess(id);
    }
    static inline AccessID getPrunedAccessId() {
        return Partition::getPrunedAccessId();
    }
    static inline const MemoryAccess &getMemoryAccess(AccessID id) {
        return Partition::getMemoryAccess(id);
    }
    //@}

protected:
    BVDataPTAImpl *pta;               ///< pointer analysis
    std::vector<PartID> obj2part;     ///< object -> partition
    std::vector<Partition> parts;     ///< memory partitions
};



/*!
 * \brief Extended MemoryPartitioning class to maintain the information
 * of data race error detection for RaceComb.
 *
 * It applies RaceComb's analysis phases, such as ThreadEscapeAnalysis,
 * MhpAnalysis and LocksetAnalysis.
 * It also helps settle the data race error detection results.
 */
class RCMemoryPartitioning : public MemoryPartitioning {
public:
    /*!
     * MemoryAccess that is reachable from a spawn site.
     */
    struct ReachableMemoryAccess {
        /*!
         * Constructor
         * @param id the memory access id
         * @param type the reachability type
         */
        ReachableMemoryAccess(AccessID id, MhpAnalysis::ReachableType type) :
                id(id), type(type) {
        }
        AccessID id;
        MhpAnalysis::ReachableType type;
    };

    typedef llvm::SmallVector<ReachableMemoryAccess, 4> ReachableMemoryAccessVector;
    typedef std::map<const llvm::Instruction*, ReachableMemoryAccessVector> Inst2RmavMap;

    /*!
     * A set of interested Instructions that may be risky.
     * This class is used to record the risky pairs, which can be pruned
     * by analysis such as ThreadEscapeAnalysis, MhpAnalysis and LocksetAnalysis.
     * A risky pair contains at least one write access.
     */
    class RiskyInstructionSet {
    public:
        typedef std::set<std::pair<AccessID, AccessID> > RiskyPairs;

        /// Constructor
        RiskyInstructionSet() : overBudget(false) {
        }

        /// Check if the MhpInstructionSet has any risky memory access pairs.
        inline bool hasRiskyPairs() const {
            return 0 != riskyPairs.size();
        }

        /// Check if the MhpInstructionSet has too many risky memory access pairs.
        inline bool isOverBudget() const {
            return overBudget || (riskyPairs.size() >= riskyPairBudget);
        }

        /// Get the risky memory access pairs.
        //@{
        inline const RiskyPairs &getRiskyPairs() const {
            return riskyPairs;
        }
        inline RiskyPairs &getRiskyPairs() {
            return riskyPairs;
        }
        //@}

        /// Record the Instruction pairs as the combinations from ids1 and ids2.
        void addPairsFrom(const AccessIdVector &ids1, const AccessIdVector &ids2);

        /// Check if a risky pair exists.
        inline bool hasRiskyPair(AccessID id1, AccessID id2) const {
            bool order = id1 < id2;
            AccessID left = order ? id1 : id2;
            AccessID right = order ? id2 : id1;
            return riskyPairs.count(std::make_pair(left, right));
        }

        /// Setter/Getter of computeAllRiskyPairs
        //@{
        static bool needComputeAllRiskyPairs() {
            return computeAllRiskyPairs;
        }
        static void setNeedComputeAllRiskyPairs(bool x) {
            computeAllRiskyPairs = x;
        }
        //@}

    protected:
        /// Record of the risky memory access pairs
        RiskyPairs riskyPairs;

        /// Record if the MhpInstructionSet contains too many pairs.
        bool overBudget;

        /// Check if the MhpInstructionSet has remaining budget for a given number.
        /// Set overBudget to true if it cannot fit the number.
        inline bool hasBudget(int n) {
            if (riskyPairs.size() + n > riskyPairBudget) {
                overBudget = true;
                return false;
            }
            return true;
        }

        /// Maximum number risky memory access pairs for the analysis budget
        static const int riskyPairBudget = RISKY_PAIR_BUDGET;

        /// Indicate whether compute all risky pairs even if out-of-budget
        static bool computeAllRiskyPairs;
    };

    /// Destructor
    virtual ~RCMemoryPartitioning() {
        riskyInstSets.clear();
    }

    /// Perform memory partitioning and setup
    /// RiskyInstructionSet for each memory partition
    void run();

    /// Apply ThreadEscapeAnalysis to roughly prune the memory accesses
    /// that operate on some thread-local objects.
    void applyAnalysis(ThreadEscapeAnalysis *tea);

    /// Apply MhpAnalysis to prune the non-risky memory accesses
    /// that must not happen in parallel;
    /// and apply ThreadEscapeAnalysis to precisely prune some
    /// thread-local objects.
    void applyAnalysis(MhpAnalysis *mph, ThreadEscapeAnalysis *tea,
            ContextSensitiveAliasAnalysis *csaa);

    /// Apply BarrierAnalysis to prune the memory accesses
    /// that must not happen in parallel;
    void applyAnalysis(BarrierAnalysis *ba);

    /// Apply LocksetAnalysis to prune the properly protected
    /// memory accesses.
    void applyAnalysis(LocksetAnalysis *lsa);

    /// Apply ThreadJoinRefinement to prune some conservative results
    /// of course-grain MhpAnalysis, where join is imprecisely handled;
    /// apply PathRefinement to prune the memory accesses
    /// under exclusive conditions;
    /// apply ContextSensitiveAliasAnalysis to prune some infeasible
    /// alias results.
    void applyAnalysis(ThreadJoinRefinement *joinRefine,
            PathRefinement *pathRefine);

    /// Apply HeapRefinement to prune the memory accesses
    /// on different heap instances.
    void applyAnalysis(HeapRefinement *heapRefine);

    /// Apply context-sensitive CFL-reachability-based alias analysis
    /// to prune infeasible pairs.
    void applyAnalysis(ContextSensitiveAliasAnalysis *csaa);

    /// Prune the non-risky memory accesses according to the
    /// RiskyInstructionSet information
    void pruneNonRiskyMemoryAccess();

    /// Get RiskyInstructionSet for a partion
    //@{
    inline const RiskyInstructionSet &getRiskyInstSet(PartID partId) const {
        return riskyInstSets[partId];
    }
    inline RiskyInstructionSet &getRiskyInstSet(PartID partId) {
        return riskyInstSets[partId];
    }
    //@}

    /// Setter/Getter of RiskyInstructionSet::computeAllPairs, which
    /// indicates whether compute all risky pairs even if out-of-budget.
    //@{
    inline bool needComputeAllRiskyPairs() const {
        return RiskyInstructionSet::needComputeAllRiskyPairs();
    }
    inline void setNeedComputeAllRiskyPairs(bool x) {
        RiskyInstructionSet::setNeedComputeAllRiskyPairs(x);
    }
    //@}


protected:

    /// Get the risky Instructions of a given memory partition
    template<typename SetType>
    inline void getRiskyAccessIds(PartID partId, SetType &riskyIds) const {
        const RCMemoryPartitioning::RiskyInstructionSet &riskyInstSet =
                getRiskyInstSet(partId);
        const RCMemoryPartitioning::RiskyInstructionSet::RiskyPairs &racyPairs =
                riskyInstSet.getRiskyPairs();
        for (auto it = racyPairs.begin(), ie = racyPairs.end(); it != ie;
                ++it) {
            riskyIds.insert(it->first);
            riskyIds.insert(it->second);
        }
    }


    /*!
     * Check if a memory access must not access a given memory partition.
     * @param accessId the memory access id
     * @param rp the ReachablePoint of the access
     * @param partId the memory partition id
     * @param csaa the ContextSensitiveAliasAnalysis
     * @return whether the memory access must not read/write the partition
     */
    bool mustNotAccessPart(AccessID accessId,
            const MhpAnalysis::ReachablePoint &rp, PartID partId,
            ContextSensitiveAliasAnalysis *csaa) const;

    /*!
     * Compute all reachable memory accesses that access a given memory
     * partition for every spawn site.
     * @param partId the input memory partition id
     * @param mhp the input MhpAnalysis
     * @param tea the input ThreadEscapeAnalysis
     * @param csaa the input ContextSensitiveAliasAnalysis
     * @param reachable the output reachability information
     */
    void computeReachabilityMapForPartition(PartID partId, MhpAnalysis *mhp,
            ThreadEscapeAnalysis *tea, ContextSensitiveAliasAnalysis *csaa,
            Inst2RmavMap &reachable) const;

    /*!
     * Apply reachability information to identify risky memory access pairs
     * of a memory partition.
     * @param partId the input memory partition id
     * @param reachable the input reachability information
     * @param riskyInstSet the output risky access pairs
     */
    void identifyRiskyInstSetForPartition(PartID partId,
            Inst2RmavMap &reachable, RiskyInstructionSet &riskyInstSet) const;

    /// risky Instruction set of each memory partition
    std::vector<RiskyInstructionSet> riskyInstSets;

};


#endif
