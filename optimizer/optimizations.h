#ifndef OPTIMIZATION_H
#define OPTIMIZATION_H

#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <tuple>
#include <string>
#include <cstring>

#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

bool eliminateCommonSubExpressions(LLVMBasicBlockRef block);
bool eliminateDeadCode(LLVMBasicBlockRef block);
bool foldConstants(LLVMBasicBlockRef block);

std::unordered_set<LLVMValueRef> getS(LLVMValueRef f);
std::unordered_set<LLVMValueRef> getInSet(std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMValueRef>> blockToOut, std::unordered_set<LLVMValueRef> predsOfB);
std::unordered_set<LLVMValueRef> getOutSet(std::unordered_set<LLVMValueRef> genB, std::unordered_set<LLVMValueRef> inB, std::unordered_set<LLVMValueRef> killB);
std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMBasicBlockRef>> getPredMap(LLVMValueRef f);
bool isAConstantStore(LLVMValueRef i);

std::tuple<
    std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMValueRef>>,
    std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMValueRef>>
> computeInOutSets(LLVMValueRef f);
bool loadHandler(LLVMValueRef f);

LLVMModuleRef optimize(LLVMModuleRef m);

#endif // OPTIMIZATION_H
