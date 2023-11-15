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
	Instruction* GetRetInstruction(Function *function);

private:
	Module* mod;
	LLVMContext* moduleContext;
	
	std::map<AllocaInst*, int> allocSize;
	using PairList = std::map<GlobalVariable*, AllocaInst*>;
	using FunctionToPairsMap = std::map<Function*, PairList>;
	FunctionToPairsMap functionPairMap;

	using InstructionList = std::vector<Instruction*>;
	using FunctionToInstructionMap = std::map<Function*, InstructionList>;
	FunctionToInstructionMap functionInstMap;
};
