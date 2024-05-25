#ifndef NAMER_H
#define NAMER_H

std::unordered_map<char*, LLVMValueRef> makeUnique(astNode *root, std::stack<std::unordered_map<std::string, std::string>>& stack);

#endif
