#pragma once
#include "llvm/IR/PassManager.h"

using namespace llvm;

class DynamicCallConverter : public PassInfoMixin<DynamicCallConverter>
{
public:
	PreservedAnalyses run(Module& M, ModuleAnalysisManager& MAM);

private:
	bool Run();
	void Init();
	bool Convert(Instruction& Inst);

private:
	Module* mod;
	LLVMContext* moduleContext;
	std::set<StringRef> funcArray;
	std::vector <std::pair<StringRef, StringRef>> windowsFuncArray;
	int dlopenFlag;
	bool isWindows = false;
};







