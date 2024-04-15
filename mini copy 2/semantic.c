#include "semantic.h"
#include <iostream>
#include <vector>

void analysisWeb(astNode* node, astStmt* stmnt, vector<vector<char*>*> *sTable, vector<char*> *sList);
int start(astNode* root);
int syntax = 0;

bool isVarDeclared(astNode* node, vector<vector<char*>*> *sTable) {
    // check if the node is a variable node, check if it appears in one of the symbol tables on the stack.    
    for (const auto& sList : *sTable) {
        for (const auto& varName : *sList) {
            if (strcmp(node->var.name, varName) == 0) {
                return true;  
            }
        }
    }
    return false;
}

bool isFirstDecInScope(astStmt* stmnt, vector<char*> *sList) {
    // if the node is a declaration statement, check if the variable is in the symbol table at the top of the stack.
    for (const auto& varName : *sList) {
        if (strcmp(stmnt->decl.name, varName) == 0) {
                return false;  
            }
    }
    return true;
}

void analysisWeb(astNode* node, astStmt* stmnt, vector<vector<char*>*> *sTable, vector<char*> *sList) {
    
    if (stmnt != NULL) {
        switch (stmnt->type) {

            case ast_call: {

                if (stmnt->call.param != NULL) {
                    analysisWeb(stmnt->call.param, NULL, sTable, sList);
                }

                break;
            }

            case ast_ret: {

                analysisWeb(stmnt->ret.expr, NULL, sTable, sList);

                break;
            }

            case ast_block: {

                //create a new symbol table curr_sym_table and push it to the symbol table stack
                vector<char*> *new_sList = new vector<char*> ();
                sTable->push_back(new_sList);

                // visit all nodes in the statement list of block statement
                vector<astNode*> curr_sList = *(stmnt->block.stmt_list);

                unsigned int length = curr_sList.size(); 
                for (unsigned int i = 0; i < length; i++) {
                    analysisWeb(curr_sList[i], NULL, sTable, new_sList);
                }

                // pop top of the stack
                delete new_sList;
                new_sList = NULL;

                if (!sTable->empty()) {
                    sTable->pop_back();
                }

                break;
            }

            case ast_while: {

                analysisWeb(stmnt->whilen.cond, NULL, sTable, sList);
                analysisWeb(stmnt->whilen.body, NULL,  sTable, sList);
            
                break;
            }

            case ast_if: {

                analysisWeb(stmnt->ifn.cond, NULL, sTable, sList);
                analysisWeb(stmnt->ifn.if_body, NULL, sTable, sList);
                if(stmnt->ifn.else_body != NULL) {
                    analysisWeb(stmnt->ifn.else_body, NULL, sTable, sList);
                }

                break;
            }

            case ast_asgn: {

                analysisWeb(stmnt->asgn.lhs, NULL, sTable, sList);
                analysisWeb(stmnt->asgn.rhs, NULL, sTable, sList); 

                break;
            }

            case ast_decl: {

                if (isFirstDecInScope(stmnt, sList)) {
                    sList -> push_back(stmnt->decl.name);
                } else {
                    printf("\nSYNTAX ERROR: multiple declarations of (%s) in a single scope\n", stmnt->decl.name);
                    syntax = 1;
                }

                break;
            }
        }
    } 

    if (node != NULL) {
        switch (node->type) {

            case ast_prog: {

                analysisWeb(node->prog.func, NULL, sTable, sList);

                break;
            }

            case ast_func: {
                // create a new symbol table curr_sym_table and push it to the symbol table stack
                vector<char*> *new_sList = new vector<char*> ();
                sTable->push_back(new_sList);

                // if func node has a parameter add parameter to curr_sym_table
                if (node->func.param != NULL) {
                    new_sList->push_back(node->func.param->var.name);
                }

                // visit the body node of the function node
                analysisWeb(node->func.body, NULL, sTable, new_sList);

                // pop top of the stack
                delete new_sList;
                new_sList = NULL;

                if (!sTable->empty()) {
                    sTable->pop_back();
                }

                break;
            }

            case ast_stmt: {
                
                analysisWeb(NULL, &node->stmt, sTable, sList);

                break;
            }

            case ast_extern: {

                ;

            }

            case ast_var: {

                if (isVarDeclared(node, sTable)) {
                    printf("\nSYNTAX ERROR: use of undeclared variable (%s)\n", node->var.name);
                    syntax = 1; 
                }

                break;
            }

            case ast_cnst: {

                ;

            }

            case ast_rexpr: {

                analysisWeb(node->rexpr.lhs, NULL, sTable, sList);
                analysisWeb(node->rexpr.rhs, NULL, sTable, sList);

                break;
            }

            case ast_bexpr: {

                analysisWeb(node->rexpr.lhs, NULL, sTable, sList);
                analysisWeb(node->rexpr.rhs, NULL, sTable, sList);

                break;
            }

            case ast_uexpr: {

                analysisWeb(node->uexpr.expr, NULL, sTable, sList);

                break;
            }

            default: 
                break;
        }
    }
}

int start(astNode* root){
  
    vector<vector<char*>*> *sTable = new vector<vector<char*>*> (); 
    vector<char*> *sList = new vector<char*> (); 

    sTable -> push_back(sList);

    analysisWeb(root, NULL, sTable, sList);

    delete sList;
    sList = NULL;
    delete sTable; 
    sTable = NULL;

    if (syntax == 0) {
        printf("\n(Valid Syntax)\n");
    } else {
        printf("\n(ERROR: Invalid Syntax)\n");
    }

    freeNode(root);
    return syntax;

}