#include "FakeInstructionInserter.hpp"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/Triple.h"
#include "Common/LLVMType.hpp"
#include "Common/Util.hpp"

PreservedAnalyses FakeInstructionInserter::run(Module& M, ModuleAnalysisManager& FAM)
{
	outs() << "[PASS START] FakeInstructionInserter\n";
	this->mod = &M;
	this->moduleContext = &M.getContext();
	GetInstance()->init(moduleContext);

	// check if modified
	if (Run() == true)
	{
		return PreservedAnalyses::none();
	}
	
	return PreservedAnalyses::all();
}

bool FakeInstructionInserter::InsertAsmIntoBlock(Function &F)
{
	FunctionType *Ty = FunctionType::get(VoidTy, false);

	for (BasicBlock &BB : F)
	{        
		if (BB.size() <= 1)
		{
			return false;
		}
	 
		// insert first non-PHI instruction in this block.
		IRBuilder<> IRB(&BB, BB.getFirstInsertionPt());

		StringRef asmString;
		InlineAsm::AsmDialect ad;

		Triple triple = Triple(F.getParent()->getTargetTriple());
		if (triple.getArch() == Triple::aarch64)
		{
			// 아래의 코드에서 오류가 발견되었다.
			// cmp x3, x8
			// cmp x29, 0x109 : fake
			// b 0x8 : fake
			// deadbeef : fake
			// cset w10, ne ---> ne가 cmp x29, 0x109의 결과값으로 설정되면 안된다.
			// 따라서, 스택에 저장 & 복원하는 형식으로 변경한다.
			
			asmString =	"str x29, [SP, #-16]!\n" // push
						"mrs x29, nzcv\n" // backup flag registers
						"cmp x29, 0x109\n" // it uses flag(ne) register
						"bne 0x8\n"
						".long 0xdeadbeef\n"
						"msr nzcv, x29\n" // restore flag registers
						"ldr x29, [SP], #16"; // pop

			ad = InlineAsm::AD_ATT;

		}
		else if (triple.getArch() == Triple::x86_64)
		{	
			asmString =	"push rax\n"
						"pushfq\n"
						"mov rax, 0x1\n"
						"cmp rax, 0x1000\n"	
						".short 0x0475\n" // jnz +4
						".long 0xf0f0f0f0\n" // bad instruction
						"popfq\n"
						"pop rax\n";			

			ad = InlineAsm::InlineAsm::AD_Intel;						
		}
		else
		{
			errs() << "Unsupported architecture.\n";
			return false;
		}
		InlineAsm* IA = InlineAsm::get(Ty, asmString, "", false, false, ad);
		IRB.CreateCall(IA);
	}
	return true;
}

bool FakeInstructionInserter::InsertAsmIntoPrologue(Function &F)
{
	if (F.hasPrologueData())
	{
		return false;
	}
	StringRef obfusInst;
	Triple triple = Triple(F.getParent()->getTargetTriple());
	if (triple.getArch() == Triple::aarch64)
	{
		/*
		BF 27 04 F1     cmp X29, 0x109
		41 00 00 54     bne 0x8
		CA FE BA BE     just value (no inst.)
		*/		
		
		obfusInst = StringRef("\xBF\x27\x04\xF1\x41\x00\x00\x54\xCA\xFE\xBA\xBE", 12);
	}
	else if (triple.getArch() == Triple::x86_64)
	{
		/*
		49 81 FC 09 01 00 00    cmp r12, 0x109
		75 04                   jnz 0x4
		CA FE BA BE             just value (no inst.)
		*/
		obfusInst = StringRef("\x49\x81\xFC\x09\x01\x00\x00\x75\x04\xCA\xFE\xBA\xBE", 13);
	}
	else
	{
		errs() << "Unsupported architecture.\n";
	}
	Constant* prologue = ConstantDataArray::getRaw(obfusInst, obfusInst.size(), Int8Ty);
	F.setPrologueData(prologue);
	return true;
}

bool FakeInstructionInserter::Run()
{
	bool success = false;

	for (Function& F : *mod)
	{
		outs() << "Function Name : " << demangle(F.getName().str()) << '\n';
		if (F.isDeclaration() || F.hasAvailableExternallyLinkage() || F.getInstructionCount() == 0 || F.getLinkage() != GlobalValue::LinkageTypes::ExternalLinkage)
		{
			continue;
		}
		if (true == hasAnnotation(&F, "nofii"))
		{
			continue;
		}

		success |= InsertAsmIntoPrologue(F); // Binary instructions can be inserted directly into the prologue.
		success |= InsertAsmIntoBlock(F);
	}
	PrintFunction(*mod);
	return success;
}
