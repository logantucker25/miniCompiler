#include "ast.h" 
#include "uniqueNamer.h"
#include <stdio.h>
#include <unordered_map>
#include <unordered_set>
#include <llvm-c/Core.h>
#include <set>
#include <queue>
#include <string>


int counter = 0;
// std::stack<std::unordered_map<std::string, std::string>> stack;


std::unordered_map<char*, LLVMValueRef> makeUnique(astNode *root, std::stack<std::unordered_map<std::string, std::string>>& stack) {

    // check: empty node
    if (node == NULL) {
        return;
    }

    // if encounter declaration of a variable. 
    // Give it a unique name using a counter
    // map the og name to the unique in the map (for this scope)

    if (node->type == ast_decl) {

        std::string og(node->stmt.decl.name);
        std::string new = og + "_" + std::to_string(++counter);
        stack.top()[og] = new;

        node->stmt.decl.name = strdup(new.c_str());
    }

    // find this variables new name which was chenged when we inspected its upstream declaration
    // set the name of thsi vraiable to its new unique name
    else if (node->type == ast_var) {

        std::string og(node->var.name);

        // find the new name we gave to this variable at its declaration
        // by iterating through each map in stack in DOUBLE REVERSE ORDER we are able to 
        // find the most recent declaration of this variable INSTEAD OF the first declaration of this variable
        std::string new;

        // for each scopes map 
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {

            // for each og name in scopes map
            if (it->find(og) != it->end()) {

                // if og name is found get its new unique name
                new = (*it)[og];
            }
        }

        node->var.name = strdup(new.c_str());
    }

    // if entering new declaration scope
    if (node->type == ast_func || node->type == ast_block || || node->type == ast_while || node->type == ast_if;) {

        // add a new empty map to top of the stack of maps
        stack.push(std::unordered_map<std::string, std::string>());
    }

    // pass in all children nodes to function for recursive traveral of whole tree
    for (auto child : node->children) {
        makeUnique(child, stack);
    }

    // if entering new declaration POST RECURSIVE DFS FROM SOME POINT it means that you are in fact exiting a scope 
    if (node->type == ast_func || node->type == ast_block || || node->type == ast_while || node->type == ast_if;) {

        // remove map at top of stack
       stack.pop();
    }

}