#pragma once
#include "llvm/IR/PassManager.h"

using namespace llvm;

typedef std::pair<StringRef, StringRef> PairType;

class DynamicCallConverter : public PassInfoMixin<DynamicCallConverter>
{
public:
	PreservedAnalyses run(Module& M, ModuleAnalysisManager& MAM);

private:
	bool Run();
	void Init();
	bool ChangeDirectToIndirect(Instruction& Inst);

private:
	Module* mod;
	LLVMContext* moduleContext;
	std::set<StringRef> funcArray;
	std::vector <std::pair<StringRef, StringRef>> windowsFuncArray;
	int dlopenFlag;
	bool isWindows = false;
};







