#pragma once
#include "llvm/IR/PassManager.h"

using namespace llvm;

class FakeInstructionInserter : public PassInfoMixin<FakeInstructionInserter>
{
public: 
	PreservedAnalyses run(Module& M, ModuleAnalysisManager& MAM);
private:
	bool Run();
	bool InsertAsmIntoPrologue(Function &F);
	bool InsertAsmIntoBlock(Function &F);
private:
	Module* mod;
	LLVMContext* moduleContext;
};
