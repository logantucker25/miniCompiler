#ifndef CODEGEN_H
#define CODEGEN_H

#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <algorithm>

#include <stdbool.h>
#include <string.h>

#include <map>
#include <set>
#include <vector>


void codeGen(LLVMModuleRef m, FILE* fptr);

#endif