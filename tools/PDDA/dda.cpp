/*
 // Parallel On Demand Value Flow Analysis
 //
 // Author: Yulei Sui,
 */

//#include "AliasUtil/AliasAnalysisCounter.h"
//#include "MemoryModel/ComTypeModel.h"
#include "DDA/DDAPass.h"
#include "DDA/FlowDDA.h"
#include "DDA/ContextDDA.h"
#include "DDA/PathDDA.h"
#include "DDA/DDAClient.h"
#include "tbb/task_group.h"
#include "tbb/tbb.h"
#include <queue>
#include <vector>
#include <cstdio>
#include <sstream>
#include <limits.h>
#include <sys/resource.h>               /// increase stack size

#include <llvm/Support/CommandLine.h>	// for cl
#include <llvm/Support/FileSystem.h>	// for sys::fs::F_None
#include <llvm/Bitcode/BitcodeWriterPass.h>  // for bitcode write
#include <llvm/IR/LegacyPassManager.h>		// pass manager
#include <llvm/Support/Signals.h>	// singal for command line
#include <llvm/IRReader/IRReader.h>	// IR reader for bit file
#include <llvm/Support/ToolOutputFile.h> // for tool output file
#include <llvm/Support/PrettyStackTrace.h> // for pass list
#include <llvm/IR/LLVMContext.h>		// for llvm LLVMContext
#include <llvm/Support/SourceMgr.h> // for SMDiagnostic
#include <llvm/Bitcode/ReaderWriter.h>		// for createBitcodeWriterPass

using namespace llvm;
using namespace tbb;

static cl::opt<std::string> InputFilename(cl::Positional,
        cl::desc("<input bitcode>"), cl::init("-"));

static cl::opt<bool> StandardCompileOpts("std-compile-opts",
        cl::desc("Include the standard compile time optimizations"));

//static cl::list<const PassInfo*, bool, PassNameParser>
//PassList(cl::desc("Optimizations available:"));

static cl::opt<bool> PFLOW("pdfs", cl::init(false),
                           cl::desc("enable flow-sensitivity for demand-driven analysis"));

static cl::opt<bool> PCONTEXT("pcxt", cl::init(false),
                              cl::desc("enable context-sensitivity for demand-driven analysis"));

static cl::opt<bool> PPATH("pdps", cl::init(false),
                           cl::desc("enable path-sensitivity for demand-driven analysis"));

static cl::opt<unsigned> tdnum("tdnum", cl::init(1),
                               cl::desc("thread number for parallel DDA"));

static cl::opt<unsigned> maxPathLen1("-maxpath", cl::init(100000),
                                     cl::desc("Maximum path limit for DDA"));

static cl::opt<unsigned> maxContextLen1("-maxcxt", cl::init(100000),
                                        cl::desc("Maximum context limit for DDA"));

static cl::opt<string> userInputQuery1("pquery", cl::init("all"),
                                       cl::desc("Please specify queries by inputing their pointer ids"));

static cl::opt<bool> printCPts1("pcpts", cl::init(false),
                                cl::desc("Dump conditional points-to set "));

tbb::concurrent_queue<NodeID> candidateQueries;

static double pRunTime[16];
static u32_t pQueryNum[16];

class ParallelDDA {
private:
    u32_t _i;
    std::vector<PointerAnalysis*> ptaVec;
public:
    ParallelDDA(u32_t i, std::vector<PointerAnalysis*>& vec) :
        _i(i), ptaVec(vec) {
    }
    void operator()() const {
        PointerAnalysis* pta = ptaVec[_i];
        NodeID i = 0;
        while (candidateQueries.try_pop(i)) {
            PAGNode* node = pta->getPAG()->getPAGNode(i);
            if (pta->getPAG()->isValidTopLevelPtr(node)) {
                double start = DDAStat::getClk();
                pta->computeDDAPts(node->getId());
                pRunTime[_i] += DDAStat::getClk() - start;
                pQueryNum[_i]++;
            }
        }
    }

    void allocateBufQueries() {
        PointerAnalysis* pta = ptaVec[_i];
        NodeID i = 0;
        u32_t bufsize = 0;
        NodeBS buffer;
        do {
            bufsize = 0;
            while (candidateQueries.try_pop(i)) {
                buffer.set(i);
                if (++bufsize > 10)
                    break;
            }

            if (buffer.empty())
                break;

            for (NodeBS::iterator it = buffer.begin(), eit = buffer.end();
                    it != eit; ++it) {

                PAGNode* node = pta->getPAG()->getPAGNode(*it);
                if (pta->getPAG()->isValidTopLevelPtr(node)) {
                    pta->computeDDAPts(node->getId());
                }
            }
            buffer.clear();
        } while (true);
    }
};

/*!
 * Initialize queries
 */
void answerQueries(PointerAnalysis* pta) {
    if (!userInputQuery1.empty()) {
        /// solve function pointer
        if (userInputQuery1 == "funptr") {
            u32_t limitnum = 0;
            for (PAG::CallSiteToFunPtrMap::const_iterator it =
                        pta->getPAG()->getIndirectCallsites().begin(), eit =
                        pta->getPAG()->getIndirectCallsites().end(); it != eit;
                    ++it,++limitnum) {
                candidateQueries.push(it->second);
            }
        }
        /// or use whole program top-level pointers
        else if (userInputQuery1 == "all") {
            for (NodeBS::iterator it = pta->getAllValidPtrs().begin(), eit =
                        pta->getAllValidPtrs().end(); it != eit; ++it)
                candidateQueries.push(*it);
        }
        /// allow user specify queries
        else {
            u32_t buf; // Have a buffer
            stringstream ss(userInputQuery1); // Insert the user input string into a stream
            while (ss >> buf)
                candidateQueries.push(buf);
        }
    }
}

void statQueryCount() {
    double totalQueryTime = 0;
    for (u32_t i = 0; i < tdnum; i++) {
        std::cout << "---Thread " << i << " Query num: " << pQueryNum[i]<< " time: ";
        std::cout << pRunTime[i]/TIMEINTERVAL << "\n";
        totalQueryTime+=pRunTime[i];
    }
    std::cout << "====Total Query: " << candidateQueries.unsafe_size() << "\n";
}

/// select a client to initialize queries
DDAClient* selectClient(llvm::Module& module) {

    DDAClient* _client = NULL;
    if (!userInputQuery1.empty()) {
        /// solve function pointer
        if(userInputQuery1 == "funptr") {
            _client = new FunptrDDAClient();
        }
        /// check casting instructions
        else if (userInputQuery1 == "cast") {
            _client = new CastDDAClient();
        }
        /// check use after free
        else if (userInputQuery1 == "free") {
            _client = new FreeDDAClient();
        }
        /// check uninitialised variables
        else if (userInputQuery1 == "uninit") {
            _client = new UninitDDAClient();
        }
        /// allow user specify queries
        else {
            _client = new DDAClient();
            if (userInputQuery1 != "all") {
                u32_t buf; // Have a buffer
                stringstream ss(userInputQuery1); // Insert the user input string into a stream
                while (ss >> buf)
                    _client->setQuery(buf);
            }
        }
    }
    else {
        assert(false && "Please specify query options!");
    }

    _client->initialise(module);

    return _client;
}

int main(int argc, char ** argv) {

    sys::PrintStackTraceOnErrorSignal();
    llvm::PrettyStackTraceProgram X(argc, argv);

    LLVMContext &Context = getGlobalContext();

    std::string OutputFilename;

    cl::ParseCommandLineOptions(argc, argv, "On demand value-flow analysis\n");
    sys::PrintStackTraceOnErrorSignal();

    PassRegistry &Registry = *PassRegistry::getPassRegistry();

    initializeCore(Registry);
    initializeScalarOpts(Registry);
    initializeIPO(Registry);
    initializeAnalysis(Registry);
    initializeTransformUtils(Registry);
    initializeInstCombine(Registry);
    initializeInstrumentation(Registry);
    initializeTarget(Registry);

    llvm::legacy::PassManager Passes;

    SMDiagnostic Err;

    // Load the input module...
    std::unique_ptr<Module> M1 = parseIRFile(InputFilename, Err, Context);

    if (!M1) {
        Err.print(argv[0], errs());
        return 1;
    }


    std::unique_ptr<tool_output_file> Out;
    std::error_code ErrorInfo;

    StringRef str(InputFilename);
    InputFilename = str.rsplit('.').first;
    OutputFilename = InputFilename + ".dvf";

    Out.reset(
        new tool_output_file(OutputFilename.c_str(), ErrorInfo,
                             sys::fs::F_None));

    if (ErrorInfo) {
        errs() << ErrorInfo.message() << '\n';
        return 1;
    }

    typedef std::vector<PointerAnalysis*> PTAVECTOR;
    PTAVECTOR ptaVec;

    VFPathCond::setMaxPathLen(maxPathLen1);
    ContextCond::setMaxCxtLen(maxContextLen1);

    DDAClient* client = selectClient(*M1.get());
    /// Initialize pointer analysis.
    if (PFLOW) {
        for (u32_t n = 0; n < tdnum; n++)
            ptaVec.push_back(new FlowDDA(&*M1.get(),client));
    } else if (PCONTEXT) {
        for (u32_t n = 0; n < tdnum; n++)
            ptaVec.push_back(new ContextDDA(&*M1.get(),client));
    } else if (PPATH) {
        for (u32_t n = 0; n < tdnum; n++)
            ptaVec.push_back(new PathDDA(&*M1.get(),client));
    } else {
        assert(false && "please specify pointer analysis implementation");
    }

    ///initialize
    for (PTAVECTOR::iterator it = ptaVec.begin(), eit = ptaVec.end(); it != eit;
            ++it)
        (*it)->initialize(*M1.get());
    PAG* pag = ptaVec[0]->getPAG();
    NodeBS queries = client->collectCandidateQueries(pag);
    for (NodeBS::iterator it = queries.begin(), eit = queries.end(); it != eit; ++it)
        candidateQueries.push(*it);

    const rlim_t kStackSize = 256L * 1024L * 1024L;   // min stack size = 256 Mb
    task_scheduler_init init(tdnum,kStackSize);
    task_group g;
    for (u32_t n = 0; n < tdnum; n++)
        g.run(ParallelDDA(n, ptaVec)); // spawn a task
    g.wait();                // wait for both tasks to complete

    return 0;
}

