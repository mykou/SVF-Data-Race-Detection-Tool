/*
 // Instrumentation
 //
 // Author: Yulei Sui,
 */


#include "Instrumentation/TransformPass.h"
#include "Instrumentation/TSan.h"

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

static cl::opt<std::string> InputFilename(cl::Positional,
        cl::desc("<input bitcode>"), cl::init("-"));

static cl::opt<bool>
StandardCompileOpts("std-compile-opts",
                    cl::desc("Include the standard compile time optimizations"));

static cl::opt<bool> INSTRUMENT("ins", cl::init(false),
                                cl::desc("Instrumentation pass for injecting code into LLVM IR"));

static cl::opt<bool> TSAN("tsan", cl::init(false),
                          cl::desc("Thread Sanitizer Instrumetation Pass"));

static cl::opt<std::string>
DefaultDataLayout("default-data-layout",
                  cl::desc("data layout string to use if not specified by module"),
                  cl::value_desc("layout-string"), cl::init(""));

int main(int argc, char ** argv) {

    sys::PrintStackTraceOnErrorSignal();
    llvm::PrettyStackTraceProgram X(argc, argv);

    LLVMContext &Context = getGlobalContext();

    cl::ParseCommandLineOptions(argc, argv, "On demand value-flow analysis\n");
    sys::PrintStackTraceOnErrorSignal();

    // Initialize passes
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeCore(Registry);
    initializeScalarOpts(Registry);
    initializeIPO(Registry);
    initializeAnalysis(Registry);
    initializeTransformUtils(Registry);
    initializeInstCombine(Registry);
    initializeInstrumentation(Registry);
    initializeTarget(Registry);
    // For codegen passes, only passes that do IR to IR transformation are
    // supported.

    llvm::legacy::PassManager Passes;

    SMDiagnostic Err;

    // Load the input module...
    std::unique_ptr<Module> M1 = parseIRFile(InputFilename, Err, Context);

    if (!M1) {
        Err.print(argv[0], errs());
        return 1;
    }

    std::string OutputFilename;

    std::unique_ptr<tool_output_file> Out;
    std::error_code ErrorInfo;
    StringRef str(InputFilename);
    InputFilename = str.rsplit('.').first;
    OutputFilename = InputFilename + ".instr";

    Out.reset(new tool_output_file(OutputFilename.c_str(), ErrorInfo, sys::fs::F_None));

    if (ErrorInfo) {
        errs() << ErrorInfo.message() << '\n';
        return 1;
    }


    // Add an appropriate DataLayout instance for this module.
    const DataLayout &DL = M1->getDataLayout();
    if (DL.isDefault() && !DefaultDataLayout.empty()) {
        M1->setDataLayout(DefaultDataLayout);
    }

    if(INSTRUMENT)
        Passes.add(new TransformPass());
    else if(TSAN)
        Passes.add(new TSan());

    /*
      TSan tsan;
      tsan.doInitialization(*M1.get());

      for (Module::iterator it = M1->begin(), eit = M1->end(); it != eit; ++it) {
        tsan.runOnFunction(*it);
      }
     */

    Passes.add(createBitcodeWriterPass(Out->os()));
    Passes.run(*M1.get());
    Out->keep();

    PrintStatistics();
    return 0;

}

