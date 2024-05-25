#include "ast.h" 
#include "uniqueNamer.h"
#include <stdio.h>
#include <unordered_map>
#include <unordered_set>
#include <llvm-c/Core.h>
#include <set>
#include <queue>
#include <string>



int deleteSetOfBlocks(LLVMBasicBlockRef b, std::set<LLVMBasicBlockRef> seen) {

    while (b) {

        LLVMBasicBlockRef nextBlock = LLVMGetNextBasicBlock(b); 
        
        // not in seen
        if (seen.find(b) == seen.end()) {
            
            LLVMDeleteBasicBlock(b);
        
        }
        b = nextBlock;
    }

    return 1;

}

void removeZombieBlocks(LLVMValueRef f) {

    // store seen blocks
    std::set<LLVMBasicBlockRef> seen;

    // bfs q
    std::queue<LLVMBasicBlockRef> q;

    LLVMBasicBlockRef entryBB = LLVMGetEntryBasicBlock(f);
    q.push(entryBB);
    seen.insert(entryBB);

    // BFS
    while (!q.empty()) {
        LLVMBasicBlockRef c = q.front();
        q.pop();

        LLVMBasicBlockRef nextBlock = LLVMGetNextBasicBlock(c);
        while (nextBlock) {

            // condensed logic: condition (if next not in seen)
            if (seen.find(nextBlock) == seen.end()) {
                
                // then mark this block for inspection
                seen.insert(nextBlock);
                q.push(nextBlock);
            }

            nextBlock = LLVMGetNextBasicBlock(nextBlock);
        }
    }

    // delete all blocks that did not get reached by BFS
    deleteSetOfBlocks(LLVMGetFirstBasicBlock(f), seen);

}


void scrapeBody(astNode *body,  std::unordered_set<std::string>& varNames) {
    
    switch(body->type) {
        case ast_decl:
            varNames.insert(std::string(body->stmt.decl.name));
            break;
        case ast_block:
            for (auto it = body->stmt.block.stmt_list->begin(); it != body->stmt.block.stmt_list->end(); ++it) {
                scrapeBody(*it, varNames);
                
            }
            break;
        default:
            break;
    }
}

void getVarSet(astNode *node, std::unordered_set<std::string>& varNames) {


    // create empty set
    // std::unordered_set<std::string> varNames;

    // add params of function 
    // CHANGE TO HARDCODE FOR SINGLE PARAM
    /*
    astNode *p = funcNode->func.param;
    while (p != NULL) {
            varNames.insert(param->var.name);
            p = param->next; 
    }
    */
    
    astNode *p = node->func.param;
    varNames.insert(std::string(p->var.name));

    // add vars from func body
    astNode *body = node->func.body;
    scrapeBody(body, varNames);


}

void generateAllocs(std::unordered_map<std::string, LLVMValueRef>& var_map, std::unordered_set<std::string>& varNames, LLVMBuilderRef builder) {
    
    // For each name in the set created above:
    for (auto it = varNames.begin(); it != varNames.end(); ++it) {
        
        // Generate an alloc statement 
        const std::string& name = *it;
        LLVMValueRef a = LLVMBuildAlloca(builder, LLVMInt32Type(), name.c_str());

        // Add name, LLVMvalueRef of alloc statement generated above to var_map  
        var_map[name] = a;

    }
}

LLVMValueRef genIRExpr(astNode* node, LLVMBuilderRef builder, std::unordered_map<std::string, LLVMValueRef>& var_map, LLVMModuleRef m) {

    switch (node->type) {

        case ast_cnst: {
            
            // Generate an LLVMValueRef using LLVMConstInt using the constant value in the node.
            unsigned long long num = node->cnst.value;
            LLVMValueRef constant = LLVMConstInt(LLVMInt32Type(), num, false);
            return constant;

        }

        case ast_var: {

            // Generate a load instruction that loads from the memory location (alloc instruction in var_map) corresponding to the variable name in the node.
            std::string var(node->var.name);
            LLVMValueRef varAlloc = var_map[var]; 
            LLVMTypeRef varType = LLVMGetElementType(LLVMTypeOf(varAlloc));
            LLVMValueRef instruction = LLVMBuildLoad2(builder, varType, varAlloc, var.c_str());
            return instruction;

        }

        case ast_uexpr: {


        }

        case ast_bexpr: {

            // Generate LLVMValueRef for the lhs and rhs in the binary expression node by calling genIRExpr recursively.
            LLVMValueRef lhs = genIRExpr(node->bexpr.lhs, builder, var_map, m);
            LLVMValueRef rhs = genIRExpr(node->bexpr.rhs, builder, var_map, m);

            // Based on the operator in the binary expression node, generate an addition/subtraction/multiplication 
            // instruction using LLVMValueRef of lhs and rhs as operands.
            switch (node->bexpr.op) {
                case add: {
                    return LLVMBuildAdd(builder, lhs, rhs, "add");
                }
                    
                case sub: {
                    return LLVMBuildSub(builder, lhs, rhs, "sub");
                }

                case divide: {
                    return LLVMBuildUDiv(builder, lhs, rhs, "divide");
                }

                case mul: {
                    return LLVMBuildMul(builder, lhs, rhs, "mul");
                }

                case uminus: {
                    LLVMValueRef expr = genIRExpr(node->uexpr.expr, builder, var_map, m);
                    return LLVMBuildNeg(builder, expr, "uminus");
                }
                    
            }
        }

        case ast_rexpr: {

            LLVMValueRef lhs = genIRExpr(node->rexpr.lhs, builder, var_map, m);
            LLVMValueRef rhs = genIRExpr(node->rexpr.lhs, builder, var_map, m);

            // Based on the operator in the relational expression node, generate a compare instruction with parameter 
            // LLVMIntPredicate Op set to a comparison operator from LLVMIntPredicate enum in Core.h file.
            LLVMIntPredicate p;

            switch (node->rexpr.op) {
                case lt: {
                    p = LLVMIntULT;
                }
                    
                case gt: {
                    p = LLVMIntUGT;
                }

                case le: {
                    p = LLVMIntULE;
                }

                case ge: {
                    p = LLVMIntSGE;
                }

                case eq: {
                    p = LLVMIntEQ;
                }

                case neq: {
                    p = LLVMIntNE;
                }

                default:
                    break;
            }
            return LLVMBuildICmp(builder, p, lhs, rhs, "comparison");
            break;

        }

        case ast_call: {
            
            // Generate a call instruction to read function.
            LLVMTypeRef readType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
            LLVMValueRef read = LLVMGetNamedFunction(m, "print");
                             
            return LLVMBuildCall2(builder, readType, read, NULL, 0, "readCall");
        }



        default: 
            break;
    }
}

LLVMBasicBlockRef genIRStmt(astNode *node, LLVMBuilderRef builder, LLVMBasicBlockRef startBB, LLVMModuleRef m, std::unordered_map<std::string, LLVMValueRef>& var_map, LLVMValueRef ret_ref, LLVMBasicBlockRef retBB) {
    
    switch (node->stmt.type) {
        
        case ast_asgn: {
            // Set the position of the builder to the end of startBB
            LLVMPositionBuilderAtEnd(builder, startBB);

            // Generate LLVMValueRef of RHS by calling the genIRExpr subroutine given below.
            LLVMValueRef rhs = genIRExpr(node->stmt.asgn.rhs, builder, var_map, m);

            // Generate an LLVM store instruction to store the RHS LLVMValueRef to the memory location (alloc instruction) corresponding to the LHS
            std::string lhsName(node->stmt.asgn.lhs->var.name);
            LLVMValueRef lhsAlloc = var_map[lhsName];
            LLVMBuildStore(builder, rhs, lhsAlloc);

            return startBB;
            break;
        }

        case ast_call: {

            LLVMPositionBuilderAtEnd(builder, startBB);

            // get the thing to be printed
            LLVMValueRef tbp = genIRExpr(node->stmt.call.param, builder, var_map, m);
            LLVMValueRef args[] = {tbp};

            // Generate a Call instruction to print function
            LLVMTypeRef paramTypes[] = {LLVMInt32Type()};
            LLVMTypeRef printType = LLVMFunctionType(LLVMVoidType(), paramTypes, 1, false);
            
            LLVMValueRef printFunc = LLVMGetNamedFunction(m, "print");
            //LLVMBuildCall(builder, printFunc, tbp, 1, "p");
            LLVMBuildCall2(builder, printType, printFunc, args, 1, "p");
            return startBB;
            break;
        }

        case ast_while: {
            
            LLVMPositionBuilderAtEnd(builder, startBB);

            // Generate a basic block to check the condition of the while loop. Let this be condBB.
            LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(startBB), "cond");

            // Generate an unconditional branch at the end of startBB to condBB.
            LLVMBuildBr(builder, condBB);

            LLVMPositionBuilderAtEnd(builder, condBB);

            // Generate LLVMValueRef of the relational expression (comparison) in the condition of the while loop
            LLVMValueRef comparison = genIRExpr(node->stmt.whilen.cond, builder, var_map, m);

            // Generate two basic blocks, trueBB and falseBB, that will be the successor basic blocks when condition is true or false respectively.
            LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(startBB), "insideWhile");
            LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(startBB), "afterWhile");
            
            // Generate a conditional branch at the end of condBB using the LLVMValueRef of comparison in step E above and setting the successor as 
            // trueBB when the condition is true and the successor as falseBB when the condition is false.
            LLVMBuildCondBr(builder, comparison, trueBB, falseBB);

            // Generate the LLVM IR for the while loop body by calling the genIRStmt subroutine recursively
            LLVMBasicBlockRef trueExitBB = genIRStmt(node->stmt.whilen.body, builder, trueBB, m, var_map, ret_ref, retBB);
            LLVMPositionBuilderAtEnd(builder, trueExitBB);
            LLVMBuildBr(builder, condBB);

            return falseBB;
            break;
        }

        case ast_if: {

            LLVMPositionBuilderAtEnd(builder, startBB);
            LLVMValueRef comparison = genIRExpr(node->stmt.ifn.cond, builder, var_map, m);
            LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(startBB), "insideIf");
            LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(startBB), "afterIf");
            LLVMBuildCondBr(builder, comparison, trueBB, falseBB);

            // If there is no else part to the if statement
            if (node->stmt.ifn.else_body == NULL) {

                // Generate the LLVM IR for the if body by calling the genIRStmt subroutine recursively
                LLVMBasicBlockRef ifExitBB = genIRStmt(node->stmt.ifn.cond, builder, trueBB, m, var_map, ret_ref, retBB);
                
                LLVMPositionBuilderAtEnd(builder, ifExitBB);
                LLVMBuildBr(builder, falseBB);

                return falseBB;
            }
            else {
                LLVMBasicBlockRef ifExitBB = genIRStmt(node->stmt.ifn.if_body, builder, trueBB, m, var_map, ret_ref, retBB);
                LLVMBasicBlockRef elseExitBB = genIRStmt(node->stmt.ifn.else_body, builder, falseBB, m, var_map, ret_ref, retBB);

                LLVMBasicBlockRef endBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(startBB), "end");
                LLVMPositionBuilderAtEnd(builder, ifExitBB);
                LLVMBuildBr(builder, endBB);
                LLVMPositionBuilderAtEnd(builder, elseExitBB);
                LLVMBuildBr(builder, endBB);

                return endBB;
            }
            break;
        }

        case ast_ret: {

            LLVMPositionBuilderAtEnd(builder, startBB);

            // Generate LLVMValueRef of the return value (could be an expression)
            LLVMValueRef retVal = genIRExpr(node->stmt.ret.expr, builder, var_map, m);

            // Generate a store instruction from LLVMValueRef of return value to the memory location pointed by ret_ref alloc statement.
            LLVMBuildStore(builder, retVal, ret_ref);

            LLVMBuildBr(builder, retBB);
            LLVMBasicBlockRef endBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(startBB), "return");
            
            return endBB;
            break;
        }

        case ast_block: {

            LLVMBasicBlockRef prevBB = startBB;

            // for each statement S in the statement list in the block statement:
            vector<astNode*> curr_sList = *(node->stmt.block.stmt_list);
            unsigned int length = curr_sList.size(); 
            for (unsigned int i = 0; i < length; i++) {
                
                // Generate the LLVM IR for S by calling the genIRStmt subroutine recursively
                astNode *s = curr_sList[i];
                prevBB = genIRStmt(s, builder, prevBB, m, var_map, ret_ref, retBB);
            }

            return prevBB;
            break;
        }

        default: 
            break;

    }
 
}

void processFunctionNodes(astNode *root, LLVMModuleRef m) {

    astNode *funcNode = root->prog.func;
    while (funcNode != NULL) {

        // Generate a LLVM builder
        LLVMBuilderRef builder = LLVMCreateBuilder();

        // Generate an entry basic block, and let entryBB be the reference to this basic block
        LLVMTypeRef paramTypes[] = { LLVMInt32Type() };
        LLVMTypeRef newFuncType = LLVMFunctionType(LLVMInt32Type(), paramTypes, 1, 0);
        LLVMValueRef newFunc = LLVMAddFunction(m, funcNode->func.name, newFuncType);
        LLVMBasicBlockRef entryBB = LLVMAppendBasicBlock(newFunc, "entryBB");

        // Create a set with names of all parameters and local variables
        std::unordered_set<std::string> varNames;
        getVarSet(funcNode, varNames);

        // Set the position of the builder to the end of entryBB
        LLVMPositionBuilderAtEnd(builder, entryBB);

        // Initialize var_map to a new map: and fill it with proper allocation
        std::unordered_map<std::string, LLVMValueRef> var_map;
        generateAllocs(var_map, varNames, builder);

        // Generate an alloc instruction for the return value and keep the LLVMValueRef, 
        // ret_ref, of this instruction available everywhere in your program
        LLVMValueRef ret_ref = LLVMBuildAlloca(builder, LLVMInt32Type(), "ret_ref");

        // Generate a store instruction to store the function parameter (use LLVMGetParam) into 
        // the memory location associated with (alloc instruction) the parameter name in the function ast node. 
        LLVMValueRef pAlloc = LLVMBuildAlloca(builder, LLVMInt32Type(), "pAlloc");
        LLVMValueRef p = LLVMGetParam(newFunc, 0); 
        LLVMBuildStore(builder, p, pAlloc);

        // Generate a return basic block and keep the LLVMBasicBlockRef, 
        // retBB, of this basic block available everywhere in your program
        // set pos at end of retBB
        LLVMBasicBlockRef retBB = LLVMAppendBasicBlock(newFunc, "retBB");
        LLVMPositionBuilderAtEnd(builder, retBB);

        // add instructions to retBB
        LLVMValueRef returnVal = LLVMBuildLoad2(builder, LLVMInt32Type(), ret_ref, "returnVal");
        LLVMBuildRet(builder, returnVal);

        // gen IR
        LLVMBasicBlockRef exitBB =  genIRStmt(funcNode->func.body, builder, entryBB, m, var_map, ret_ref, retBB);

        // handle terminator
        if (!LLVMGetBasicBlockTerminator(exitBB)) {
            
            // Set the position of the builder to the end of exitBB
            LLVMPositionBuilderAtEnd(builder, exitBB);
            
            // Generate an unconditional branch to retBB
            LLVMBuildBr(builder, retBB);
        }

        // Remove all basic blocks that do not have any predecessor basic blocks 
        removeZombieBlocks(newFunc);

        // MEMORY CLEAN UP

        //funcNode = funcNode->next;
    }

}


//******************************************************************************************************************
//******************************************************************************************************************
// ******************************************************************************************************************
/// THIS IS THE MOTHER FUNCTION

int convertAST(astNode *root1) {


    yyin = fopen(argv[1], "r");
    if (yyparse() != 0) {
        fprintf(stderr, "Parsing failed\n");
        return 1;
    }

    // make unique names for variables in tree
    std::stack<std::unordered_map<std::string, std::string>> stack;
    makeUnique(root1, stack);

    // Generate a module, set the target architecture
    LLVMModuleRef m = LLVMModuleCreateWithName("test");
	LLVMSetTarget(m, "x86_64-pc-linux-gnu");

    // Generate LLVM functions without bodies for print and read extern function declarations
    LLVMTypeRef paramTypes[] = { LLVMInt32Type() };
    LLVMTypeRef printFuncType = LLVMFunctionType(LLVMVoidType(), paramTypes, 1, 0);
    LLVMAddFunction(m, "print", printFuncType);

    LLVMTypeRef readFuncType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMAddFunction(m, "read", readFuncType);

    // traverse tree from func node down
    processFunctionNodes(root1, m);

    LLVMDumpModule(m);
    return 0;

}