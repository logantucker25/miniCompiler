#ifndef LLVM_PARSER_H
#define LLVM_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

LLVMModuleRef createLLVMModel(char * filename);
void walkBBInstructions(LLVMBasicBlockRef bb);
void walkBasicblocks(LLVMValueRef function);
void walkFunctions(LLVMModuleRef module);
void walkGlobalValues(LLVMModuleRef module);

#endif // LLVM_PARSER_H
