#pragma once
#include "llvm/IR/Type.h"
#include "llvm/IR/LLVMContext.h"

using namespace llvm;

#define VoidTy GetInstance()->getVoidTy()
#define Int8Ty GetInstance()->getInt8Ty()
#define Int8PtrTy GetInstance()->getInt8PtrTy()
#define Int32Ty GetInstance()->getInt32Ty()
#define Int32PtrTy GetInstance()->getInt32PtrTy()
#define Int64Ty GetInstance()->getInt64Ty()
#define Int64PtrTy GetInstance()->getInt64PtrTy()

struct LLVMType
{
	Type* voidTy;
	IntegerType* int8Ty;
	PointerType* int8PtrTy;
	IntegerType* int32Ty;
	PointerType* int32PtrTy;
	IntegerType* int64Ty;
	PointerType* int64PtrTy;
	Type* (*getVoidTy)();
	IntegerType* (*getInt8Ty)();
	PointerType* (*getInt8PtrTy)();
	IntegerType* (*getInt32Ty)();
	PointerType* (*getInt32PtrTy)();
	IntegerType* (*getInt64Ty)();
	PointerType* (*getInt64PtrTy)();
	void (*init)(LLVMContext* moduleContext);
};

static LLVMType* instance = nullptr;

static void initFunction(LLVMContext* moduleContext)
{
	instance->voidTy = Type::getVoidTy(*moduleContext);
	instance->int8Ty = Type::getInt8Ty(*moduleContext);
	instance->int8PtrTy = instance->int8Ty->getPointerTo();
	instance->int32Ty = Type::getInt32Ty(*moduleContext);
	instance->int32PtrTy = instance->int32Ty->getPointerTo();
	instance->int64Ty = Type::getInt64Ty(*moduleContext);
	instance->int64PtrTy = instance->int64Ty->getPointerTo();
}

static Type* getVoidTyFunction()
{
	return instance->voidTy;
}
static IntegerType* getInt8TyFunction()
{
	return instance->int8Ty;
}
static PointerType* getInt8PtrTyFunction()
{
	return instance->int8PtrTy;
}
static IntegerType* getInt32TyFunction()
{
	return instance->int32Ty;
}
static PointerType* getInt32PtrTyFunction()
{
	return instance->int32PtrTy;
}
static IntegerType* getInt64TyFunction()
{
	return instance->int64Ty;
}
static PointerType* getInt64PtrTyFunction()
{
	return instance->int64PtrTy;
}

static LLVMType* GetInstance()
{
	if (instance == nullptr)
	{
		instance = new LLVMType();
		instance->init = &initFunction;
		instance->getVoidTy = &getVoidTyFunction;
		instance->getInt8Ty = &getInt8TyFunction;
		instance->getInt8PtrTy = &getInt8PtrTyFunction;
		instance->getInt32Ty = &getInt32TyFunction;
		instance->getInt32PtrTy = &getInt32PtrTyFunction;
		instance->getInt64Ty = &getInt64TyFunction;
		instance->getInt64PtrTy = &getInt64PtrTyFunction;
	}

	return instance;
}
