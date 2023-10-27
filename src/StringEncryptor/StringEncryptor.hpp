#pragma once
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Instructions.h"
#include <map>
#include <vector>

using namespace llvm;

class StringEncryptor : public PassInfoMixin<StringEncryptor>
{
public:
	PreservedAnalyses run(Module& M, ModuleAnalysisManager& MAM);

private:
	bool Run();
	void PrintHex(unsigned char* hex, int size);
	bool MarkAllStrings(Function& function);
	void EncryptMarkStrings();
	bool IsStringGV(GlobalVariable* gv, Value* v);

private:
	Module* mod;
	LLVMContext* moduleContext;

	using PairList = std::map<GlobalVariable*, AllocaInst*>;
	using BasicBlockToPairsMap = std::map<BasicBlock*, PairList>;
	BasicBlockToPairsMap blockPairMap;

	using InstructionList = std::vector<Instruction*>;
	using BasicBlockToInstructionMap = std::map<BasicBlock*, InstructionList>;
	BasicBlockToInstructionMap blockInstMap;
};
