/*
 * RCResult.cpp
 *
 *  Created on: 22/06/2016
 *      Author: dye
 */


#include "RCResult.h"
#include "LocksetAnalysis.h"

using namespace llvm;
using namespace std;
using namespace rcUtil;
using namespace analysisUtil;


extern cl::opt<bool> RcDetail;


/*
 * Compare DataRaceWarning according to the object's source location.
 */
bool RCStaticAnalysisResult::compare(const DataRaceWarning &warning1,
        const DataRaceWarning &warning2) {
    // Use the first object of MP
    string str1 = rcUtil::getSourceLoc(warning1.getObj(0));
    string str2 = rcUtil::getSourceLoc(warning2.getObj(0));

    // Extract filename:line substring
    size_t pos;
    pos = str1.find("(");
    if (string::npos != pos)
        str1 = str1.substr(pos + 1, str1.length() - pos - 2);
    pos = str2.find("(");
    if (pos != string::npos)
        str2 = str2.substr(pos + 1, str2.length() - pos - 2);

    // Split strings by delim (i.e., ':');
    const char delim = ':';
    vector<string> substrings1, substrings2;
    split(str1, delim, substrings1);
    split(str2, delim, substrings2);

    // Compare the filenames and line numbers
    const size_t lIdx = 0;
    const size_t fIdx = 1;
    if (substrings1.size() && substrings2.size()) {
        // Same filename
        if (substrings1[fIdx] != substrings2[fIdx]) {
            return substrings1[fIdx] < substrings2[fIdx];
        }
        // Different filenames
        else {
            int line1 = atoi(substrings1[lIdx].c_str());
            int line2 = atoi(substrings2[lIdx].c_str());
            if (line1 == line2) {
                return substrings1[fIdx - 1] < substrings2[fIdx - 1];
            } else {
                return line1 < line2;
            }
        }
    }
    return true;
}


/*
 * Initialization
 */
void RCStaticAnalysisResult::init(BVDataPTAImpl *pta) {
    this->pta = pta;
}


/*
 * Sort DataRaceWarnings in ascending order of objects' source locations.
 */
void RCStaticAnalysisResult::sort() {
    std::sort(warnings.begin(), warnings.end(), compare);
}


/*
 * Print the data races of RCStaticAnalysisResult to outs().
 */
void RCStaticAnalysisResult::print() const {
    outs() << pasMsg(" --- Data Race Results ---\n");

    // Iterate over warnings
    for (int i = 0, e = warnings.size(); i != e; ++i) {
        if (RcDetail) {
            outs() << "============== Possible race in MP "
                    << warnings[i].getPartId() << "\t";
        }
        for (auto it = warnings[i].objBegin(), ie = warnings[i].objEnd();
                                it != ie; ++it) {
            NodeID obj = *it;
            outs() << rcUtil::getSourceLoc(obj) << "\t";
        }
        outs() << "\n";

        // Stop printing further details unless RcDetailed is set by cl.
        if (!RcDetail)    continue;

        // Print memory accesses
        outs() << "dereferenced at " << warnings[i].getNumInstructions()
                << " places:\n";
        for (auto it = warnings[i].instBegin(), ie = warnings[i].instEnd();
                        it != ie; ++it) {
            const Value *deref = *it;
            outs() << "\t" << rcUtil::getSourceLoc(deref) << "\n";

            // Print protecting lockset
            const Instruction *I = dyn_cast<Instruction>(deref);
            if (!I)     continue;
            const LocksetSummary::LockSet *lockset =
                    LocksetSummary::getProtectingLocks(I);
            if (!lockset)   continue;
            for (LocksetSummary::LockSet::const_iterator it = lockset->begin(),
                    ie = lockset->end(); it != ie; ++it) {
                const Value *lockPtr = *it;
                NodeID id = pta->getPAG()->getValueNode(lockPtr);
                PointsTo &pts = pta->getPts(id);

                outs() << "\t\tLocked --- " << rcUtil::getSourceLoc(lockPtr) << " --> ";
                if (pts.empty())
                    outs() << "empty lock set\n";
                else {
                    outs() << "{ ";
                    for (auto it = pts.begin(), ie = pts.end(); it != ie; ++it) {
                        outs() << rcUtil::getSourceLoc(*it);
                        auto tmpIter = it;
                        if (++tmpIter != ie)    outs() << ", ";
                    }
                    outs() << " }\n";
                }
            }
        }
    }

    // Print the data race warning count
    size_t numWarning = warnings.size();
    if (0 == numWarning) {
        outs() << pasMsg("No race found.\n");
    } else {
        string warningCountMsg = to_string(numWarning);
        if (1 == numWarning) 
             warningCountMsg += " race in total\n";
        else 
             warningCountMsg += " races in total\n";
        outs() << errMsg(warningCountMsg);
    }
    outs() << "\n";
}

