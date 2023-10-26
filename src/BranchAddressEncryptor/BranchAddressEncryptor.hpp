#pragma once
#include "llvm/IR/PassManager.h"

using namespace llvm;

class BranchAddressEncryptor : public PassInfoMixin<BranchAddressEncryptor>
{
public:
	PreservedAnalyses run(Module& M, ModuleAnalysisManager& MAM);

private:
	bool Run();
	bool EncryptAndIndirect(Function& Func);

private:
	Module* mod;
	LLVMContext* moduleContext;
};
