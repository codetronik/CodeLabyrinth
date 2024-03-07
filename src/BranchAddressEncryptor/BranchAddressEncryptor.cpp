#include "BranchAddressEncryptor.hpp"
#include "llvm/ADT/Triple.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include <random>
#include "Common/LLVMType.hpp"
#include "Common/Util.hpp"

PreservedAnalyses BranchAddressEncryptor::run(Module& M, ModuleAnalysisManager& MAM)
{
    outs() << "[PASS START] BranchAddressEncryptor\n";
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

bool BranchAddressEncryptor::Run()
{
    bool success = false;

    for (Function& F : *mod)
    {
        outs() << "Function Name : " << demangle(F.getName().str()) << "..." << F.getLinkage() << '\n';

        if (F.isDeclaration() || F.hasAvailableExternallyLinkage() || F.getInstructionCount() == 0 || F.getLinkage() != GlobalValue::LinkageTypes::ExternalLinkage)
        {
            continue;
        }
        if (true == hasAnnotation(&F, "nobae"))
        {
            continue;
        }
        success |= EncryptAndIndirect(F);

    }
    PrintFunction(*mod);
    return success;
}

bool BranchAddressEncryptor::EncryptAndIndirect(Function& Func)
{
    outs() << " -" << demangle(Func.getName().str()) << "\n";

    std::vector<BranchInst*> conditionBrInsts;
    std::vector<BranchInst*> unconditionBrInsts;

    for (Instruction& Inst : instructions(Func))
    {
        if (isa<BranchInst>(&Inst))
        {
            BranchInst* br = dyn_cast<BranchInst>(&Inst);

            if (true == br->isConditional())
            {
                conditionBrInsts.push_back(br);
            }
            else
            {
                unconditionBrInsts.push_back(br);
            }
        }
    }

    // 비조건 브랜치를 조건 브랜치처럼 난독화하면 최적화 되어 버린다.
    // rand()와 비교문을 추가하여 강제로 조건 브랜치로 변경하면 최적화 대상에서 제외된다.
    for (BranchInst* branchInst : unconditionBrInsts)
    {
        BasicBlock* trueBlock = branchInst->getSuccessor(0);

        IRBuilder<>* branchInstBuilder = new IRBuilder<>(branchInst);
        FunctionCallee randFunc = mod->getOrInsertFunction("rand", Int32Ty);
        CallInst* callRand = branchInstBuilder->CreateCall(randFunc, {});
        // 항상 false가 되는 구문을 작성해야함
        // if (rand() > -1)
        Value* cmpResult = branchInstBuilder->CreateICmpSGT(callRand, ConstantInt::get(Int32Ty, -1));

        // 최적화를 피하기 위해 추가함. 실행되면 안되는 코드
        BasicBlock* falseBlock = BasicBlock::Create(*moduleContext, "falseBlock", &Func);
        IRBuilder<> builder(falseBlock);
        builder.CreateBr(falseBlock);

        BranchInst* bi = BranchInst::Create(trueBlock, falseBlock, cmpResult);
        llvm::ReplaceInstWithInst(branchInst, bi);

        conditionBrInsts.push_back(bi);
    }


    Value* zero = ConstantInt::get(Int64Ty, 0);
    for (BranchInst* branchInst : conditionBrInsts)
    {
        outs() << "BI : " << *branchInst << "\n";

        IRBuilder<>* branchInstBuilder = new IRBuilder<>(branchInst);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0x100000, INT32_MAX);

        // type은 정수 타입 (Int32Ty일 경우 0x1이면 메모리에서 0x8임)
        ConstantInt* keyOffset = ConstantInt::get(Int64Ty, dist(gen));

        BasicBlock* trueBlock = branchInst->getSuccessor(0);
        BasicBlock* falseBlock = branchInst->getSuccessor(1);
        Triple triple = Triple(mod->getTargetTriple());

        // 분기 주소 + 오프셋
        // 최적화를 피할 수 있는 유일한 주소 연산 방법 (이라고 믿고 있음)
        Type* dynamicTy;
        if (triple.isArch64Bit())
        {
            dynamicTy = Int64Ty;
        }
        else
        {
            dynamicTy = Int32Ty;
        }

        auto trueBlockPtr = ConstantExpr::getGetElementPtr(dynamicTy, BlockAddress::get(trueBlock), keyOffset);
        auto trueBlockRealEncAddr = ConstantExpr::getPtrToInt(trueBlockPtr, dynamicTy);
        auto falseBlockPtr = ConstantExpr::getGetElementPtr(dynamicTy, BlockAddress::get(falseBlock), keyOffset);
        auto falseBlockRealEncAddr = ConstantExpr::getPtrToInt(falseBlockPtr, dynamicTy);

        // 순서는 false, true 순 (고정)
        std::vector<Constant*> trueFalseBlocks;
        trueFalseBlocks.push_back(falseBlockRealEncAddr);
        trueFalseBlocks.push_back(trueBlockRealEncAddr);

        ArrayType* arrayType = ArrayType::get(dynamicTy, trueFalseBlocks.size());
        Constant* trueFalseArray = ConstantArray::get(arrayType, ArrayRef<Constant*>(trueFalseBlocks));

        // 최적화를 피하기 위해 global section을 사용한다.
        GlobalVariable* trueFalseGV = new GlobalVariable(*mod, arrayType, false, GlobalValue::LinkageTypes::PrivateLinkage, trueFalseArray, "EncBlockTable");

        appendToCompilerUsed(*Func.getParent(), { trueFalseGV });

        // 어디로 분기할지 동적으로 구하는 코드
        // 메모리에 0 or 1로 저장됨. 이것을 index로 활용한다.
        Value* condition = branchInst->getCondition();
        // 큰 타입으로 변환 (상위 비트는 0으로 채움)
        Value* index = branchInstBuilder->CreateZExt(condition, Int64Ty); // 0 or 1

        // true or false 주소를 담고 있는 메모리의 주소를 가져옴
        Value* branchGV = branchInstBuilder->CreateGEP(arrayType, trueFalseGV, { zero, index });
        // encBranchAddrPtr =  *branchGV
        LoadInst* encBranchAddrPtr = branchInstBuilder->CreateLoad(Int64PtrTy, branchGV);
        // 더했던 만큼 뺌
        Value* offset = branchInstBuilder->CreateSub(zero, keyOffset);

        // 분기 주소 - 오프셋
        Value* realBranchAddr = branchInstBuilder->CreateGEP(encBranchAddrPtr->getType(), encBranchAddrPtr, offset);
        IndirectBrInst* indirectBr = branchInstBuilder->CreateIndirectBr(realBranchAddr, trueFalseBlocks.size());
        indirectBr->addDestination(falseBlock);
        indirectBr->addDestination(trueBlock);

        // 원본 명령어는 삭제
        branchInst->eraseFromParent();
    }

    return true;
}