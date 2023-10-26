#include "llvm/IR/Function.h"

using namespace llvm;

bool hasAnnotation(Function* f, StringRef str);
void PrintFunction(Module& mod);