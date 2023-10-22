#include "Util.hpp"

bool hasAnnotation(Function* f, StringRef str)
{
	GlobalVariable* Annotations = f->getParent()->getGlobalVariable("llvm.global.annotations");
	if (!Annotations)
	{
		return false;
	}
	ConstantArray* arrays = dyn_cast<ConstantArray>(Annotations->getInitializer());
	if (!arrays)
	{
		return false;
	}

	for (auto& op : arrays->operands())
	{
		ConstantStruct* constStruct = dyn_cast<ConstantStruct>(op);
		if (!constStruct)
		{
			continue;
		}
		Function* func = dyn_cast<Function>(constStruct->getOperand(0)->stripPointerCasts());
		if (!func)
		{
			continue;
		}
		if (func != f)
		{
			continue;
		}
		GlobalValue* value = dyn_cast<GlobalValue>(constStruct->getOperand(1)->stripPointerCasts());
		if (!value)
		{
			continue;
		}
		ConstantDataSequential* data = dyn_cast<ConstantDataSequential>(value->getOperand(0));
		if (!data)
		{
			continue;
		}
		if (data->getAsCString().contains(str))
		{
			outs() << "found Annotation\n";
			return true;
		}
	}

	return false;
}