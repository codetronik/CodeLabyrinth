#include "llvm/IR/PassManager.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "FakeInstructionInserter/FakeInstructionInserter.hpp"
#include "DynamicCallConverter/DynamicCallConverter.hpp"
#include "BranchAddressEncryptor/BranchAddressEncryptor.hpp"

using namespace llvm;

extern "C" PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo()
{   
	// Register LLVM passes
	return
	{
		LLVM_PLUGIN_API_VERSION, "CodeLabyrinth", "v1.0.0", [](PassBuilder &PB)
		{
			PB.registerPipelineStartEPCallback(
				[](ModulePassManager &MPM, OptimizationLevel level)
				{
					// todo : Need to change to cli option
					MPM.addPass(FakeInstructionInserter());
					MPM.addPass(DynamicCallConverter());
					MPM.addPass(BranchAddressEncryptor());
				}
			);
			PB.registerScalarOptimizerLateEPCallback(
				[](FunctionPassManager &FPM, OptimizationLevel level)
				{    
				}
			);
		}
	};
}
