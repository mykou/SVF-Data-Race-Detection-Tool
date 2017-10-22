/*
 * DCIRTL.h
 *
 *  Created on: 4 Oct 2016
 *      Author: pengd
 */

#ifndef DCIRTL_H_
#define DCIRTL_H_

#include <atomic>
#include <map>
#include <set>
#include <thread>

#include "../RTLCommon/RTLMutex.h"
#include "RC/RCSparseBitVector.h"

/// Print debug information 0: do not print debug information
#if 0
#define DPLEVEL 0 //Debug print level, 0-5, 0 is highest print level. 5 prints all debug information. Default is 0.
extern BlockingMutex print_mtx;

#define PRINTF(...) print_mtx.Lock(); \
	printf(__VA_ARGS__); \
	print_mtx.Unlock();

#define DBPRINTF(DPL, ...)  if (DPL<DPLEVEL) {\
    print_mtx.Lock(); \
	__VA_ARGS__; \
	print_mtx.Unlock(); }

#else
#define PRINTF(...)
#define DBPRINTF(...)
#endif


#define MAX_THREAD_NUM 16
#define MAX_INSTR_NUM 512// 2 << 8
#define MAX_INSTR_MASK 511 // 1ull << 9

namespace __dci {

typedef __rtl_common::uptr uptr;
typedef std::set<uptr> ThrSet;
typedef u64 Pair;
typedef RCSparseBitVector<MAX_INSTR_NUM> InstIDSet;
typedef std::set<Pair> Pairs;

/*
 * Get current thread
 */
uptr get_counter();

/*!
 * EscapeState is used to record escape state of threads
 * EscapeState (from most significant bit):
 *   instr_num * instr_escape_state
 *   instr_escape_state has 8 bits
 *   ignore          : 1
 *   tid             : 7 kTidBits
 */
class EscapeState {
public:

    /*!
     * Constructor
     */
    EscapeState() {
        SetTid(get_counter());
    }
    EscapeState(uptr thd) {
        SetTid(thd);
    }

    /*!
     * Compare thread id
     */
    void CompareTid(uptr thd) {
        if (GetIgnoreBit())
            return;
        if (thd!=GetTid())
            SetIgnoreBit();
    }

    /*!
     * Set thread id
     */
    void SetTid(uptr thd) {
        x_ = (x_ & ~(kTidMask << kTidShift)) | (u8(thd));
    }

    /*!
     * Get thread id
     */
    uptr GetTid() {
        u8 res = (x_ & ~kIgnoreBit) >> kTidShift;
        return (uptr)res;
    }

    /*!
     * Get thread id with ignore flag
     */
    u64 TidWithIgnore() const {
        u64 res = x_ >> kTidShift;
        return res;
    }

    /*!
     * Set ignore bit
     */
    void SetIgnoreBit() {
        x_ |= kIgnoreBit;
    }

    /*!
     * Clear ignore bit
     */
    void ClearIgnoreBit() {
        x_ &= ~kIgnoreBit;
    }

    /*!
     * Get ignore bit
     */
    bool GetIgnoreBit() const {
        return (s8)x_ < 0;
    }

private:
    static const int kTidBits = 7;
    static const unsigned kMaxTid = 1 << kTidBits;
    static const int kTidShift = 8 - kTidBits - 1;
    static const u8 kIgnoreBit = 1ull << 7;
    static const u8 kTidMask = 127;
    u8 x_;
};

/*!
 * EscapeInfo is used to record escape information
 */
struct EscapeInfo {
    typedef __dci::InstIDSet InstIDSet;
    typedef InstIDSet* InstSetAry;
    typedef std::map<u64,InstSetAry> AddrMap;
    unsigned instr_num;
    AddrMap addrmap;
    EscapeState** escapestate;
};

/// The recorder of escape information
extern EscapeInfo* escapeinfo;

/*!
 * DCI memory accesses information
 * 0-13 the 14 bits of real address
 * 14-15 ksize
 */
struct DCIMem {
    typedef std::map<u16, InstIDSet> InstMap;

    /// Write and Read instructions
    //@{
    InstMap write2inst;
    InstMap read2inst;
    //@}

    /// Constructor
    DCIMem() {
        write2inst.clear();
        read2inst.clear();
    }

    /*!
     * Get recorded address with ksize
     */
    static inline u16 getAbsAddr(void* addr, u16 ksize) {
        return ((ksize) << 14) | ((u64) addr & 0x3fff);
    }

    /*!
     * Add a read instruction
     */
    inline void addReadInst(u16 x_, unsigned instID) {
        read2inst[x_].set(instID);
    }

    /*!
     * Add a write instruction
     */
    inline void addWriteInst(u16 x_, unsigned instID) {
        write2inst[x_].set(instID);
    }
};

/*!
 * DCI information
 */
struct DCIInfo {

    /// DCI memory accesses
    DCIMem *dciinfo[MAX_THREAD_NUM];

    /// All pairs
    Pairs pairs;

    /// The map is used to record all RaceComb pairs
    /// unsigned is the instruction, InstIDSet is its paired instructions
    std::map<unsigned, InstIDSet> instToGroup;


    /// Mutexes for instructions' paired group
    /// Multiple instructions share one mutex
    /// Use id & MAX_INST_MASK to identify it.
    BlockingMutex mtx_instToGroup[MAX_INSTR_NUM];

    /// Mutex for threads' DCI information
    BlockingMutex mtx_dciinfo[MAX_THREAD_NUM];

    /// Mutex for pairs
    BlockingMutex mtx_pairs;

    /*!
     * Add a pair(a,b)
     */
    void addPair(unsigned a, unsigned b) {
        u64 pair;
        if (a < b)
            pair = (((u64)a) << 32) | ((u64)b);
        else
            pair = (((u64)b) << 32) | ((u64)a);
        mtx_pairs.Lock();
        pairs.insert(pair);
        mtx_pairs.Unlock();
    }

    /*!
     * Get pair(a,b) from pair
     */
    void getPair(unsigned &a, unsigned &b, Pair pair) {
        a=pair >> 32;
        b=pair & 4294967295;
    }

    /*!
     * Check if one instruction does not have any paired instructions
     */
    inline bool isemptyIG(unsigned id) {
        BlockingMutexLock l(&mtx_instToGroup[id&(MAX_INSTR_MASK)]);
        if (instToGroup.find(id) == instToGroup.end()) return true;
        return instToGroup[id].empty();
    }

    /*!
     * Add a paired instruction pid to id's paired group
     */
    inline void setIG(unsigned id, unsigned pid) {
//        BlockingMutexLock l(&mtx_instToGroup[id&(MAX_INSTR_MASK)]);
        instToGroup[id].set(pid);
    }

    /*!
     * Delete a paired instruction pid from id's paired group
     */
    inline void resetIG(unsigned id, unsigned pid) {
        BlockingMutexLock l(&mtx_instToGroup[id&(MAX_INSTR_MASK)]);
        instToGroup[id].reset(pid);
    }

    /// Lock and unlock for instruction id's paired group
    //@{
    inline void lockIG(unsigned id) {
        mtx_instToGroup[id&(MAX_INSTR_MASK)].Lock();
    }
    inline void unlockIG(unsigned id) {
        mtx_instToGroup[id&(MAX_INSTR_MASK)].Unlock();
    }
    //@}

    /// Lock and unlock for thread's DCI information
    //@{
    inline void lockInfo(unsigned id) {
        mtx_dciinfo[id].Lock();
    }
    inline void unlockInfo(unsigned id) {
        mtx_dciinfo[id].Unlock();
    }
    //@}

    /*!
     * Check write instruction. Remove the pair if it is visited
     */
    void checkWritePair(u16 x_, unsigned id) {

        unsigned cur_thd = (unsigned)get_counter();

        InstIDSet temp;
        for (unsigned i = 0; i < cur_thd; i++) {
            if (dciinfo[i] == NULL)
                break;
            lockInfo(i);
            temp |= dciinfo[i]->read2inst[x_];
            temp |= dciinfo[i]->write2inst[x_];
            unlockInfo(i);
        }

        for (unsigned i = cur_thd+1; i < MAX_THREAD_NUM; i++) {
            if (dciinfo[i] == NULL)
                break;
            lockInfo(i);
            temp |= dciinfo[i]->read2inst[x_];
            temp |= dciinfo[i]->write2inst[x_];
            unlockInfo(i);
        }

        lockIG(id);
        temp &= instToGroup[id];
        unlockIG(id);

        for (InstIDSet::iterator it = temp.begin(), ei = temp.end(); it != ei; it++) {
            addPair(id,*it);
            resetIG(id,*it);
            /// Only delete its instToGroup to reduce unnecessary locks.
            /// resetIG(*it,id);
        }
    }

    /*!
     * Check read instruction. Remove the pair if it is visited
     */
    void checkReadPair(u16 x_, unsigned id) {

        unsigned cur_thd = (unsigned)get_counter();

        InstIDSet temp;
        for (unsigned i = 0; i < cur_thd; i++) {
            if (dciinfo[i] == NULL)
                break;
            lockInfo(i);
            temp |= dciinfo[i]->write2inst[x_];
            unlockInfo(i);
        }

        for (unsigned i = cur_thd+1; i < MAX_THREAD_NUM; i++) {
            if (dciinfo[i] == NULL)
                break;
            lockInfo(i);
            temp |= dciinfo[i]->write2inst[x_];
            unlockInfo(i);
        }

        lockIG(id);
        temp &= instToGroup[id];
        unlockIG(id);
        for (InstIDSet::iterator it = temp.begin(), ei = temp.end(); it != ei; it++) {
            addPair(id,*it);
            resetIG(id,*it);
            /// Only delete its instToGroup to reduce unnecessary locks.
            /// resetIG(*it,id);
        }
    }
};

/// DCI information
extern DCIInfo *dci;

/*!
 * Initialize function
 */
void Initialize(unsigned instr_num);

/*!
 * Finalize function
 */
void Finalize();

/*!
 * Initialize pair information. Store all RC pairs into instToGroup
 */
void InitializePairInfo();

/*!
 * Collect refined pairs and store result into DCI.pairs
 */
void CollectPairs();

}

#endif /* DCIRTL_H_ */
