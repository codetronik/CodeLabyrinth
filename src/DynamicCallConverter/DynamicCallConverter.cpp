#include "DynamicCallConverter.hpp"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Demangle/Demangle.h"
#include "Common/LLVMType.hpp"

using namespace llvm;

PreservedAnalyses DynamicCallConverter::run(Module& M, ModuleAnalysisManager& MAM)
{
	this->mod = &M;
	this->moduleContext = &M.getContext();
	GetInstance()->init(moduleContext);
	Init();

	// check if modified
	if (Run() == true)
	{
		PreservedAnalyses::none();
	}
	return PreservedAnalyses::all();
}

void DynamicCallConverter::Init()
{

	Triple triple = Triple(mod->getTargetTriple());

	if (triple.isOSWindows())
	{
		isWindows = true;
		windowsFuncArray.push_back({ "user32.dll", "MessageBoxA" });
		windowsFuncArray.push_back({ "user32.dll", "MessageBoxW" });
		windowsFuncArray.push_back({ "kernel32.dll", "Sleep" });

	}
	else
	{
		windowsFuncArray.push_back({ "", "printf" });
		windowsFuncArray.push_back({ "", "strlen" });

		if (triple.isOSDarwin())
		{
			dlopenFlag = 0x2 | 0x8;
		}
		else if (triple.isAndroid())
		{
			if (triple.isArch64Bit())
			{
				dlopenFlag = 0x00002/*RTLD_NOW*/ | 0x00100;/*RTLD_GLOBAL*/
			}
			else
			{
				dlopenFlag = 0x00000/*RTLD_NOW*/ | 0x00002;/*RTLD_GLOBAL*/
			}
		}
	}
}
  
bool DynamicCallConverter::ChangeDirectToIndirect(Instruction& Inst)
{
	if (false == isa<CallInst>(&Inst))
	{
		return false;
	}
	CallInst* callInst = dyn_cast<CallInst>(&Inst);

	if (!callInst->getCalledFunction())
	{
		return false;
	}
	outs() << "CallInst : " << Inst << "\n";
	
	StringRef loadLib;
	StringRef loadFunc;

	if (isWindows)
	{
		loadLib = "LoadLibraryA";
		loadFunc = "GetProcAddress";
	}
	else
	{
		loadLib = "dlopen";
		loadFunc = "dlsym";
	}

	if (callInst->getCalledFunction()->getName() == loadLib || callInst->getCalledFunction()->getName() == loadFunc)
	{
		return false;
	}

	StringRef name = callInst->getCalledFunction()->getName();
	auto it = std::find_if(windowsFuncArray.begin(), windowsFuncArray.end(), [&name](const std::pair<StringRef, StringRef>& pair) { return pair.second == name; });

	if (it == windowsFuncArray.end())
	{
		outs() << "not found\n";
		return false;
	}

	BasicBlock* EntryBlock = callInst->getParent();
	IRBuilder<> IRB(EntryBlock, EntryBlock->getFirstInsertionPt());

	// dlopen()
	FunctionType* dlopenFnTy;
	FunctionCallee fc;
	if (false == isWindows)
	{
		dlopenFnTy = FunctionType::get(Int64PtrTy, { Int8PtrTy, Int32Ty }, false);
	}
	else
	{
		dlopenFnTy = FunctionType::get(Int64PtrTy, { Int8PtrTy }, false);		
	}

	fc = mod->getOrInsertFunction(loadLib, dlopenFnTy);

	Function* dlopenFn = dyn_cast<Function>(fc.getCallee());
	if (!dlopenFn)
	{
		errs() << callInst->getCalledFunction()->getName() << " failed\n";
		return false;
	}
	Value* handle;
	FunctionType* dlsymFnTy = FunctionType::get(Int64PtrTy, { Int8PtrTy, Int8PtrTy }, false);
	Function* dlsymFn;

	if (false == isWindows)
	{
		handle = IRB.CreateCall(dlopenFn, { Constant::getNullValue(Int8PtrTy), ConstantInt::get(Int32Ty, dlopenFlag) });		
	}
	else
	{
		handle = IRB.CreateCall(dlopenFn, { IRB.CreateGlobalStringPtr(it->first) });		
	}
	dlsymFn = cast<Function>(mod->getOrInsertFunction(loadFunc, dlsymFnTy).getCallee());
	Value* dlsymFnAddress = IRB.CreateCall(dlsymFn, { handle, IRB.CreateGlobalStringPtr(callInst->getCalledFunction()->getName()) });

	Value* calledOperand = callInst->getCalledOperand();
	FunctionType* orgFnType = callInst->getCalledFunction()->getFunctionType();
	Value* dlsymFnCast = IRB.CreateBitCast(dlsymFnAddress, calledOperand->getType());
	// 원래 함수의 호출 규약으로 dlsymFnCast을 호출한다.
	callInst->setCalledFunction(orgFnType, dlsymFnCast);
	outs() << "changed ... " << calledOperand->getName() << "\n";

	return true;
}
bool DynamicCallConverter::Run()
{
	bool success = false;

	for (Function& F : *mod)
	{
		outs() << "Function Name : " << demangle(F.getName().str()) << '\n';

		if (F.isDeclaration() || F.hasAvailableExternallyLinkage())
		{
			continue;
		}
 
		for (BasicBlock& BB : F)
		{
			for (Instruction& Inst : BB)
			{
				success |= ChangeDirectToIndirect(Inst);
			}
		}
	}
   
	return success;
}

