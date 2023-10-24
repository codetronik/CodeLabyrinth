#pragma once
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Constants.h>

using namespace llvm;

class BranchAddressEncryptor : public PassInfoMixin<BranchAddressEncryptor>
{
public:
	PreservedAnalyses run(Module& M, ModuleAnalysisManager& MAM);
public:
	bool Run();
	bool runOnFunction(Function& Func);
private:
	Module* mod;
	LLVMContext* moduleContext;
};
