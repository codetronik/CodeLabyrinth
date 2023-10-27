#include "StringEncryptor.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Demangle/Demangle.h"
#include <random>
#include "Common/Util.hpp"
#include "Common/LLVMType.hpp"

PreservedAnalyses StringEncryptor::run(Module& M, ModuleAnalysisManager& MAM)
{
	outs() << "[PASS START] StringEncryptor\n";
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

bool StringEncryptor::Run()
{
	bool success = false;

	for (Function& function : *mod)
	{
		outs() << "Function Name : " << demangle(function.getName().str()) << " ... " << function.getLinkage() << "...." << function.isSpeculatable() << '\n';

		if (function.isDeclaration() || function.hasAvailableExternallyLinkage() || function.getInstructionCount() == 0 || function.getLinkage() != GlobalValue::LinkageTypes::ExternalLinkage)
		{
			continue;
		}
		if (true == hasAnnotation(&function, "nose"))
		{
			continue;
		}

		success |= MarkAllStrings(function);

	}
	if (success) EncryptMarkStrings();
	PrintFunction(*mod);

	return success;
}

void StringEncryptor::PrintHex(unsigned char* hex, int size)
{
	std::string line;
	outs() << "\t\t\t\t[" << size << "] ";
	for (int i = 0; i < size; i++)
	{
		char temp[5] = { 0, };
		sprintf(temp, "%02X ", *(hex + i));
		line.append(temp);
		if ((i % 16 == 0 && i != 0) || i == size - 1)
		{
			outs() << line;
			line = "";
		}
	}
	outs() << "\n";
}

bool StringEncryptor::IsStringGV(GlobalVariable* gv, Value* v)
{
	outs() << "\t\t\t\t\tgv->getLinkage() : " << gv->getLinkage() << "\n";
	if (false == gv->hasInitializer())
	{
		outs() << "[INFO] Unsupport GV 1 " << v->getName() << " .. " << gv->getSection() << "\n";
		return false;
	}
	if (false == isa<ConstantDataSequential>(gv->getInitializer()))
	{
		outs() << "[INFO] Unsupport GV 2 " << v->getName() << " .. " << gv->getSection() << "\n";
		return false;
	}

	return true;
}

bool StringEncryptor::MarkAllStrings(Function& function)
{
	bool mark = false;
	// 모든 BasicBlock을 순환하며 GlobalVariable을 마킹한다.
	for (BasicBlock& block : function)
	{
		outs() << "BasicBlock " << block.getName() << " size : " << block.size() << "\n";
		for (Instruction& inst : block)
		{
			outs() << "\tInst " << &inst << " : " << inst << "\n";
			if (isa<PHINode>(inst))
			{
				outs() << "contains Phi node which could raise issues!\n";
				continue;
			}
			for (Value* v : inst.operands())
			{
				outs() << "\t\tvalue " << v << " : " << v->getName() << "\n";
				GlobalVariable* gv = dyn_cast<GlobalVariable>(v->stripPointerCasts());
				if (gv)
				{
					if (true == IsStringGV(gv, v))
					{
						outs() << "\t\t\tInst : " << &inst << " GV value " << gv << " : " << v->getName() << " / " << gv->getSection() << "\n";
						Function* f = block.getParent();
						outs() << "\t\t\t\tparent Function Name : " << gv->getParent() << "..." << demangle(f->getName().str()) << '\n';

						// mark string
						blockPairMap[&block].insert({ gv, nullptr });
						blockInstMap[&block].push_back(&inst);
						mark = true;
					}
				}
			}
		}
	}
	return mark;
}

void StringEncryptor::EncryptMarkStrings()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(0x0, 0xFF);

	for (auto& [block, pairList] : blockPairMap)
	{
		// BB 블록 3분할 후 암/복호화 1블록씩 추가
		// A(첫 명령어) -> 복호화 -> B(몸통) -> 암호화 -> C(마지막 명령어)
		BasicBlock* A = block;
		BasicBlock* B = A->splitBasicBlock(A->getFirstNonPHIOrDbgOrLifetime());
		BasicBlock* C = B->splitBasicBlock(B->getTerminator());
		// B 앞에 복호화 블록을 생성
		BasicBlock* decryptBlock = BasicBlock::Create(*moduleContext, "DecryptBlock", A->getParent(), B);
		// C 앞에 암호화 블록을 생성
		BasicBlock* freeBlock = BasicBlock::Create(*moduleContext, "FreeBlock", A->getParent(), C);
		// A 블록에서 복호화 블록으로 점프
		llvm::ReplaceInstWithInst(A->getTerminator(), BranchInst::Create(decryptBlock));
		// B 블록에서 암호화 블록으로 점프
		llvm::ReplaceInstWithInst(B->getTerminator(), BranchInst::Create(freeBlock));
		IRBuilder<> builderDecryptBlock(decryptBlock);
		IRBuilder<> builderFreeBlock(freeBlock);

		// 각각의 GlobalVariable에 새로운 GlobalVariable 생성후 연결
		for (auto& [originalGV, mallocAddress] : pairList)
		{
			outs() << "mark :: " << originalGV << "\n";
			// 원래 변수
			ConstantDataSequential* cds = dyn_cast<ConstantDataSequential>(originalGV->getInitializer());
			// 사이즈를 구함.
			unsigned strSize = cds->getNumElements() * cds->getElementByteSize();
			const char* str = cds->getRawDataValues().data();
			std::vector<unsigned char> keyArray;
			// 1:1 XOR 키 생성
			for (unsigned j = 0; j < strSize; j++)
			{
				keyArray.push_back(dist(gen));
			}
			outs() << "key : ";
			PrintHex((unsigned char*)keyArray.data(), strSize);
			// 문자열 XOR
			std::vector<unsigned char> encArray;
			for (unsigned j = 0; j < strSize; j++)
			{
				encArray.push_back(str[j] ^ keyArray[j]);
			}
			outs() << "original : " << str << "\n";
			PrintHex((unsigned char*)str, strSize);
			outs() << "encrypt : ";
			PrintHex((unsigned char*)encArray.data(), strSize);
			// 타겟의 data 섹션에 들어가는 변수 생성 
			Constant* encryptConst = ConstantDataArray::get(*moduleContext, ArrayRef<unsigned char>(encArray));
			GlobalVariable* encryptGV = new GlobalVariable(*mod, encryptConst->getType(), false, GlobalValue::LinkageTypes::PrivateLinkage, encryptConst, "EncStr");

			// 최적화 방지
			llvm::appendToCompilerUsed(*encryptGV->getParent(), { encryptGV });

			// malloc 명령어 생성
			Instruction* mallocInst = CallInst::CreateMalloc(
				decryptBlock,
				Type::getInt64Ty(*moduleContext),
				encryptConst->getType(),
				ConstantExpr::getSizeOf(encryptConst->getType()),
				nullptr,
				nullptr);

			// malloc 실행 후 주소 할당 받음
			Value* allocatedMemory = builderDecryptBlock.Insert(mallocInst);

			// 로컬 변수에 저장
			AllocaInst* ai = builderDecryptBlock.CreateAlloca(Type::getInt64Ty(*moduleContext));
			builderDecryptBlock.CreateStore(allocatedMemory, ai, true);
			mallocAddress = ai;

			ArrayType* strType = ArrayType::get(Type::getInt8Ty(*moduleContext), strSize);

			for (unsigned j = 0; j < strSize; j++)
			{
				// 복호화 결과를 malloc 메모리에 저장
				Value* encValue = builderDecryptBlock.CreateGEP(strType, encryptGV, { builderDecryptBlock.getInt32(0),  builderDecryptBlock.getInt32(j) });
				Value* decValue = builderDecryptBlock.CreateGEP(strType, allocatedMemory, { builderDecryptBlock.getInt32(0),  builderDecryptBlock.getInt32(j) });

				// encryptGV에서 한글자 로딩
				LoadInst* loadOneByte = builderDecryptBlock.CreateLoad(builderDecryptBlock.getInt8Ty(), encValue, "encryptInst");
				loadOneByte->setVolatile(true);
				// key와 loadOneByte를 xor
				Value* xorValue = builderDecryptBlock.CreateXor(loadOneByte, keyArray[j]);
				// 복호화 메모리에 저장
				builderDecryptBlock.CreateStore(xorValue, decValue, true);

				// free 하기 전에 메모리를 랜덤 값으로..
				Constant* wipeConstant = ConstantInt::get(Type::getInt8Ty(*moduleContext), dist(gen));
				builderFreeBlock.CreateStore(wipeConstant, decValue, true);
			}
			// free 명령어 생성
			Instruction* freeInst = CallInst::CreateFree(allocatedMemory, freeBlock);
			// free 실행
			builderFreeBlock.Insert(freeInst);

		}
		builderDecryptBlock.CreateBr(B);
		builderFreeBlock.CreateBr(C);
	}

	// GV 교체
	for (const auto& [block, instructions] : blockInstMap)
	{
		PairList gvPair = blockPairMap[block];

		for (const auto& pair : gvPair)
		{
			GlobalVariable* originalGV = pair.first;
			AllocaInst* mallocAddress = pair.second;

			for (const auto& inst : instructions)
			{
				IRBuilder<>* branchInstBuilder = new IRBuilder<>(inst);
				LoadInst* li = branchInstBuilder->CreateLoad(Type::getInt64Ty(*moduleContext), mallocAddress);
				li->setVolatile(true);

				// GV <-> mallocAddress
				// 명령어에서 문자열 변수 교체
				// 예를 들어, 아래와 같이 바뀜
				// %4 = call i32 (ptr, ...) @wprintf(ptr noundef @.str), !dbg !1
				// -> %4 = call i32 (ptr, ...) @wprintf(ptr noundef @-EncryptString), !dbg !11
				inst->replaceUsesOfWith(originalGV, li);
			}
		}
	}
	// 중복 없이 원본 GlobalVariable 가져오기
	std::set<GlobalVariable*> uniqueGVs;
	for (const auto& blockPair : blockPairMap)
	{
		for (const auto& gvPair : blockPair.second)
		{
			uniqueGVs.insert(gvPair.first);
		}
	}

	// wiping GV
	for (const auto& originalGV : uniqueGVs)
	{
		originalGV->removeDeadConstantUsers();
		if (originalGV->getNumUses() == 0)
		{
			originalGV->dropAllReferences();
			originalGV->eraseFromParent();
		}
	}
}