/*
 // RaceComb Instrumentor
 //
 // Author: Peng Di,
 */

#include <llvm/IR/Module.h>
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
#include <llvm/Bitcode/BitcodeWriterPass.h>		// for createBitcodeWriterPass
#include <llvm/IR/DataLayout.h>		// data layout
#include <llvm-c/Core.h>    // for LLVMGetGlobalContext()

#include "RC/TSan.h"
#include "RC/RCInstr.h"
#include "RC/Reorder.h"

using namespace llvm;

static cl::opt<std::string> InputFilename(cl::Positional,
        cl::desc("<input bitcode>"), cl::init("-"));

static cl::opt<bool> TSANAnno("tsan", cl::init(false), cl::desc("TSan Annotation"));
static cl::opt<bool> REORDER("reorder", cl::init(false), cl::desc("Reorder Pairs"));

static cl::opt<std::string>
DefaultDataLayout("default-data-layout",
                  cl::desc("data layout string to use if not specified by module"),
                  cl::value_desc("layout-string"), cl::init(""));
/*
 * rcinstr instruments bc file created by RaceComb to create new instrumented bc file
 * Using the following flags:
 * -tsan: Create bc file for TSan checking
 * -tsan -rc: Speed-oriented mode; Use RaceComb result to optimise TSan bc file
 * -reorder: Reorder pairs in RC.pairs and create new pair file DCI.pairs
 * -dci: Create bc file with DCI instrumentation
 * -dci -printRC: print all RC pairs
 * -ts -checkingpair=i: Create bc file with the i-th pair TS instrumentation
 */

int main(int argc, char ** argv) {

    sys::PrintStackTraceOnErrorSignal(argv[0]);
    llvm::PrettyStackTraceProgram X(argc, argv);

    LLVMOpaqueContext *WrappedContextRef = LLVMGetGlobalContext();
    LLVMContext &Context = *unwrap(WrappedContextRef);

    std::string OutputFilename;

    cl::ParseCommandLineOptions(argc, argv, "RaceComb Instrumentor\n");
    sys::PrintStackTraceOnErrorSignal(argv[0]);

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
    OutputFilename = InputFilename + ".rcinstr.bc";

    Out.reset(
        new tool_output_file(OutputFilename.c_str(), ErrorInfo,
                             sys::fs::F_None));

    if (ErrorInfo) {
        errs() << ErrorInfo.message() << '\n';
        return 1;
    }

    // Add an appropriate DataLayout instance for this module.

    if (REORDER) {
        Reorder::run();
        return 0;
    } else if (TSANAnno) {
        Passes.add(new TSan());
    } else {
        Passes.add(new RCInstr());
    }
    Passes.add(createBitcodeWriterPass(Out->os()));

    Passes.run(*M1.get());
    Out->keep();

    return 0;

}

