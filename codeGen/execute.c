#include <stdio.h>
#include <llvm-c/Core.h>
#include "llvm_parser.h"
#include "optimizations.h" 
#include "codeGen.h"       

int main() {
    char* ll = "test.ll";
    LLVMModuleRef m = createLLVMModel(ll);
    if (!m) {
        fprintf(stderr, ".ll --> MOD FAILURE\n");
        return 1;
    }

    // LLVMModuleRef om = optimize(m);
    //if (!om) {
        //fprintf(stderr, "m --> om FAILURE\n");
        //LLVMDisposeModule(m); 
        //return 1;
    //}


    // code gen
    FILE* fptr = fopen("output.s", "w+");
    codeGen(m, fptr);


    // LLVMDumpModule(om);
    LLVMDisposeModule(m);
    return 0;
}
