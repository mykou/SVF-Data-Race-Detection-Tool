/*
 * RCAnnotaton.cpp
 *
 *  Created on: 23/06/2015
 *      Author: dye
 */

#include "RC/RCAnnotaton.h"
#include "RCResult.h"
#include "RaceComb.h"
#include <llvm/IR/Metadata.h>

using namespace analysisUtil;
using namespace llvm;
using namespace std;

extern cl::opt<bool> RcDetail;

/*
 * Initialization
 */
void RCAnnotationExtractor::init(Module *M) {
    this->M = M;
}


/*
 * Perform annotation extraction
 */
void RCAnnotationExtractor::run() {
    typedef llvm::DenseMap<const llvm::MDNode*, const llvm::Instruction*> MDNode2InstMap;
    MDNode2InstMap idMDNode2InstMap;

    // Initialize the IdMDNode2InstMap and Extract allRacyInsts
    for (auto it = M->begin(), ie = M->end(); it != ie; ++it) {
        const Function &F = *it;
        for (auto it = inst_begin(F), ie = inst_end(F); it != ie; ++it) {
            const Instruction *I = &*it;

            // Instruction for paired check
            if (MDNode *M = I->getMetadata(AnnotationPairedInstId)) {
                // Add to the IdMDNode2Inst mapping
                idMDNode2InstMap[M] = I;

                // Add to the Id2Inst mapping
                size_t id = getAnnotationId(I);
                id2InstMap[id] = I;
            }

            // Instruction for regular check
            if (I->getMetadata(AnnotationRegCheck)) {
                allRacyInsts.insert(I);
            }
        }
    }

    // Extract racy Instruction pair information
    NamedMDNode *MDNodes = M->getOrInsertNamedMetadata(AnnotationPairsInfo);
    if (MDNodes->getNumOperands() <= 1) {
        return;
    }

    // Get the summary metadata
    const MDNode *M = MDNodes->getOperand(0);
    assert(3 == M->getNumOperands());
    string strParts = dyn_cast<MDString>(M->getOperand(1))->getString().str();
    string strPairs = dyn_cast<MDString>(M->getOperand(2))->getString().str();
    string numOfParts = strParts.substr(strParts.find(":") + 1);
    string numOfPairs = strPairs.substr(strPairs.find(":") + 1);

    // Set the number of all pairs
    numOfAllPairs = atoi(numOfPairs.c_str());

    // Reserve space for pairs
    pairsGroup.resize(atoi(numOfParts.c_str()));

    // Get all the racy Instruction pairs
    for (int i = 1, e = MDNodes->getNumOperands(); i != e; ++i) {
        InstPairs &racyInstPairs = pairsGroup[i - 1];

        // Add every consecutive two elements as a pair into racyInstPairs
        const MDNode *M = MDNodes->getOperand(i);
        assert(2 <= M->getNumOperands());
        assert(0 == M->getNumOperands() % 2);

        for (int ii = 0, ee = M->getNumOperands(); ii != ee; ii += 2) {
            const MDNode *M1 = dyn_cast<MDNode>(M->getOperand(ii));
            const MDNode *M2 = dyn_cast<MDNode>(M->getOperand(ii + 1));
            const Instruction *I1 = idMDNode2InstMap[M1];
            const Instruction *I2 = idMDNode2InstMap[M2];
            assert(I1 && I2);

            racyInstPairs.push_back(make_pair(I1, I2));
        }
    }
}


/*
 * Get a pair with universal index
 */
RCAnnotationExtractor::InstPair RCAnnotationExtractor::getOnePair(
        size_t i) const {
    size_t groupIdx = 0;
    size_t n = getNumOfPairsInGroup(groupIdx);
    while (i >= n) {
        i -= n;
        n = getNumOfPairsInGroup(++groupIdx);
    }
    return getOnePairInGroup(groupIdx, i);
}


/*
 * Get the annotation Id of an Instruction
 */
size_t RCAnnotationExtractor::getAnnotationId(const Instruction *I) const {
    MDNode *M = I->getMetadata(AnnotationPairedInstId);
    assert(M && "Paired annotation not found for the Instruction");
    string s = dyn_cast<MDString>(M->getOperand(0))->getString().str();
    size_t id = atoi(s.c_str());
    return id;
}


/*
 * Initialization
 */
void RCAnnotator::init(Module *M, RCStaticAnalysisResult *result) {

    this->M = M;

    this->raceResult = result;

}


/*
 * Perform annotation
 */
void RCAnnotator::run() {

    performAnnotationForRegularCheck();

    performAnnotationForPairedCheck();

}


/*
 * Release resources
 */
void RCAnnotator::release() {

}


/*
 * Print annotation result
 */
void RCAnnotator::print() const {
    outs() << pasMsg(" --- IR Annotation ---\n");
    outs() << getNumOfRacyInsts() << " Instructions annotated for regular check.\n";
    outs() << getNumOfPairs() << " Instruction pairs (including "
            << getNumOfPairedInsts() << " Instructions) annotated for paired check.\n";
    outs() << "\n";
}


/*
 * Annotate all racy Instructions for regular check.
 */
void RCAnnotator::performAnnotationForRegularCheck() {

    DenseSet<const Instruction*> insts;

    // Collect interested Instructions
    for (auto it = raceResult->begin(), ie = raceResult->end(); it != ie;
            ++it) {
        const RCStaticAnalysisResult::DataRaceWarning &warning = *it;
        for (auto it = warning.instBegin(), ie = warning.instEnd(); it != ie;
                ++it) {
            const Instruction *I = *it;
            annotateInst(I, AnnotationRegCheck, NULL);
            insts.insert(I);
        }
    }

    // Set the number of Instructions annotated
    numOfRacyInsts = insts.size();
}


/*
 * Annotate the racy Instruction pairs for the paired check.
 * The combinatorial exploded ones are not annotated.
 */
void RCAnnotator::performAnnotationForPairedCheck() {

    pairedInstNumbering();

    annotateRacyPairs();

}


/*
 * Annotate a given Instruction with a single metadata
 * @param I the Instruction to annotate
 * @param annoStr the annotation type id
 * @param metadataStr the metadata content
 * @return false if the kind of annotation already exists; true otherwise
 */
bool RCAnnotator::annotateInst(const Instruction *I, const char *annoStr,
        const char *metadataStr) {
    Instruction *inst = const_cast<Instruction*>(I);

    // Return false if the kind of annotation already exists
    if (inst->getMetadata(annoStr))     return false;

    // Perform the annotation
    LLVMContext &C = I->getContext();
    if (metadataStr) {
        MDString *S = MDString::get(C, metadataStr);
        Metadata *M[] = {S};
        inst->setMetadata(annoStr, MDNode::get(C, M));
    } else {
        inst->setMetadata(annoStr, MDNode::get(C, NULL));
    }

    return true;
}


/*
 * Setting Ids for the paired Instructions (begins with 0).
 */
void RCAnnotator::pairedInstNumbering() {
    typedef SmallSet<const llvm::Instruction*, 8> InstSet;
    map<const Function*, InstSet> funcToInstMap;

    // Collect interested Instructions (i.e., the Instructions
    // which form racy pairs).
    size_t numOfPairs = 0;
    for (auto it = raceResult->begin(), ie = raceResult->end();
            it != ie; ++it) {
        const RCStaticAnalysisResult::DataRaceWarning &warning = *it;
        for (auto it = warning.instBegin(), ie = warning.instEnd();
                it != ie; ++it) {
            const Instruction *I = *it;
            const Function *F = I->getParent()->getParent();
            funcToInstMap[F].insert(I);
        }

        // Sum the number of pairs
        numOfPairs += warning.getRiskyInstSet()->getRiskyPairs().size();
    }

    // Number every collected Instruction
    size_t counter = 0;
    for (auto it = M->begin(), ie = M->end(); it != ie; ++it) {
        const Function &F = *it;
        auto iter = funcToInstMap.find(&F);

        // Skip the Function without any interested Instructions
        if (iter == funcToInstMap.end())    continue;

        // Number the collected Instructions in ascending order
        // of the inst_iterator
        auto &insts = iter->second;
        for (auto it = inst_begin(F), ie = inst_end(F); it != ie; ++it) {
            const Instruction *I = &*it;

            // Skip the uninterested Instruction
            if (!insts.count(I))    continue;

            // Number and annotate the Instruction
            inst2IdMap[I] = counter++;
        }
    }

    // Set the number of paired Instructions and pairs annotated
    numOfPairedInsts = inst2IdMap.size();
    numOfInstPairs = numOfPairs;
}


/*
 * Annotate the racy pair information on the LLVM-IR.
 */
void RCAnnotator::annotateRacyPairs() {
    // Annotate individual paired Instruction with its Id
    for (auto it = inst2IdMap.begin(), ie = inst2IdMap.end();
            it != ie; ++it) {
        const Instruction *I = it->first;
        size_t id = it->second;
        const char *metadataStr = to_string(id).c_str();
        annotateInst(I, AnnotationPairedInstId, metadataStr);
    }

    // Global annotation of all racy pairs
    NamedMDNode *MDNodes = M->getOrInsertNamedMetadata(AnnotationPairsInfo);
    LLVMContext &C = M->getContext();

    // Add number of paired racy Instructions
    vector<Metadata*> metadataArray;
    string numOfInstsStr = string("No. of Instructions: ") + to_string(numOfPairedInsts);
    string numOfPartsStr = string("No. of Memory Partitions: ") + to_string(raceResult->size());
    string numOfPairsStr = string("No. of Pairs: ") + to_string(numOfInstPairs);
    MDString *S1 = MDString::get(C, numOfInstsStr.c_str());
    MDString *S2 = MDString::get(C, numOfPartsStr.c_str());
    MDString *S3 = MDString::get(C, numOfPairsStr.c_str());
    metadataArray.push_back(S1);
    metadataArray.push_back(S2);
    metadataArray.push_back(S3);
    MDNode *M = MDNode::get(C, metadataArray);
    MDNodes->addOperand(M);

    // Generate metadata for risky pairs
    for (auto it = raceResult->begin(), ie = raceResult->end();
            it != ie; ++it) {
        const RCStaticAnalysisResult::DataRaceWarning &warning = *it;
        auto *riskyInstSet = warning.getRiskyInstSet();

        // Print warning message for MP that has out-of-budget pair numbers
        if (riskyInstSet->isOverBudget() && RcDetail) {
            outs() << bugMsg1("IR annotation (paired checking)")
                    << " for out-of-budget MP " << warning.getPartId() << "\n";
        }

        // Add all pairs for the MP into medatadaArray
        auto &riskyPairs = riskyInstSet->getRiskyPairs();
        metadataArray.clear();
        for (auto it = riskyPairs.begin(), ie = riskyPairs.end();
                it != ie; ++it) {
            RCMemoryPartitioning::AccessID a1 = it->first;
            RCMemoryPartitioning::AccessID a2 = it->second;
            const Instruction *I1 = RCMemoryPartitioning::getInstruction(a1);
            const Instruction *I2 = RCMemoryPartitioning::getInstruction(a2);

            MDNode *M1 = I1->getMetadata(AnnotationPairedInstId);
            MDNode *M2 = I2->getMetadata(AnnotationPairedInstId);
            assert(M1 && M2 && "The paired Instructions must have their Ids.");

            metadataArray.push_back(M1);
            metadataArray.push_back(M2);
        }

        // Generate MDNode for this MP
        M = MDNode::get(C, metadataArray);
        MDNodes->addOperand(M);
    }

}


