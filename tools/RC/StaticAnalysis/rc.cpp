/*
 * RaceComb: Data race detector.
 *
 *  Created on: 17/10/2014
 *      Author: Ding Ye,
 */

#include "RC/RaceCombPass.h"

#include <llvm/Support/CommandLine.h>	// for cl
#include <llvm/Support/FileSystem.h>    // for sys::fs::F_None
#include <llvm/Bitcode/BitcodeWriterPass.h>  // for bitcode write
#include <llvm/IR/LegacyPassManager.h>		// pass manager
#include <llvm/Support/Signals.h>	// singal for command line
#include <llvm/IRReader/IRReader.h>	// IR reader for bit file
#include <llvm/Support/ToolOutputFile.h> // for tool output file
#include <llvm/Support/PrettyStackTrace.h> // for pass list
#include <llvm/IR/LLVMContext.h>		// for llvm LLVMContext
#include <llvm/Support/SourceMgr.h> // for SMDiagnostic
#include <llvm/Bitcode/BitcodeWriterPass.h>		// for createBitcodeWriterPass
#include <llvm-c/Core.h>    // for LLVMGetGlobalContext()


using namespace llvm;

extern cl::opt<bool> RcAnno;

static cl::opt<std::string> InputFilename(cl::Positional,
        cl::desc("<input bitcode>"), cl::init("-"));


int main(int argc, char ** argv) {

	llvm::PrettyStackTraceProgram X(argc, argv);
	cl::ParseCommandLineOptions(argc, argv, "Data race detection\n");
	sys::PrintStackTraceOnErrorSignal(argv[0]);

	LLVMOpaqueContext *WrappedContextRef = LLVMGetGlobalContext();
	LLVMContext &Context = *unwrap(WrappedContextRef);
	PassRegistry &Registry = *PassRegistry::getPassRegistry();

	initializeCore(Registry);
	initializeScalarOpts(Registry);
	initializeIPO(Registry);
	initializeAnalysis(Registry);
	initializeIPO(Registry);
	initializeTransformUtils(Registry);
	initializeInstCombine(Registry);
	initializeTarget(Registry);

    llvm::legacy::PassManager Passes;

	SMDiagnostic Err;

    // Load the input module...
    std::unique_ptr<Module> M1 = parseIRFile(InputFilename, Err, Context);

	if (M1.get() == 0) {
		Err.print(argv[0], errs());
		return 1;
	}

    // Add RaceCombPass
    Passes.add(new RaceCombPass());

    // If annotation is not required, return immediately after Passes finish
    if (!RcAnno) {
        Passes.run(*M1.get());
        return 0;
    }
    // Run the Passes and then output IR with annotations.
    else {
        // Setup IR output file.
        std::unique_ptr<tool_output_file> Out;
        std::error_code ErrorInfo;
        StringRef str(InputFilename);
        InputFilename = str.rsplit('.').first;
        std::string OutputFilename = InputFilename + ".rc.bc";
        tool_output_file *outputFile = new tool_output_file(OutputFilename.c_str(),
                ErrorInfo, sys::fs::F_None);
        Out.reset(outputFile);

        if (ErrorInfo) {
            errs() << ErrorInfo.message() << '\n';
            return 1;
        }

        Passes.add(createBitcodeWriterPass(Out->os()));
        Passes.run(*M1.get());
        Out->keep();
    }

	return 0;

}

