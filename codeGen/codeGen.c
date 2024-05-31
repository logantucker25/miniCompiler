#include "codeGen.h"

// GLOBALS
std::map<LLVMValueRef, int> reg_map;
FILE* fp;


std::map<LLVMValueRef, int> getInstIndex(LLVMBasicBlockRef b) {

    std::map<LLVMValueRef, int> res;
    int counter = 0;

    for (LLVMValueRef i = LLVMGetFirstInstruction(b); i; i = LLVMGetNextInstruction(i)) {
        if (!LLVMIsAAllocaInst(i)) {
            res[i] = counter++;
        }
    }

    return res;
}

std::map<LLVMValueRef, pair<int, int>> getLiveRange(LLVMBasicBlockRef b, std::map<LLVMValueRef, int>& inst_index) {

    std::map<LLVMValueRef, std::pair<int, int>> live_range;

    // initialize all instructions range to <start, start>
    for (auto it = inst_index.begin(); it != inst_index.end(); ++it) {
        live_range[it->first] = { it->second, it->second };
    }

    // adjust end ranges
    for (LLVMValueRef i = LLVMGetFirstInstruction(b); i; i = LLVMGetNextInstruction(i)) {

        // ignore allocations and declarations
        if (!LLVMIsAAllocaInst(i) && !LLVMGetFirstUse(i)) {

            int end = inst_index[i];
            // for each use of instruction
            for (LLVMUseRef u = LLVMGetFirstUse(i); u; u = LLVMGetNextUse(u)) {

                // capture greatest index of an instruction that uses the product of i
                end = max(end, inst_index[LLVMGetUser(u)]);
            }

            live_range[i].second = end;
        }

    }
    return live_range;
}

bool isMath(LLVMValueRef i) {
    
    LLVMOpcode o = LLVMGetInstructionOpcode(i);
    return (o == LLVMAdd || o == LLVMSub || o == LLVMMul);
}

bool overlapingLive() {
    return true;
}

LLVMValueRef find_spill(LLVMValueRef i, std::map<LLVMValueRef, int>& inst_index, std::map<LLVMValueRef, std::pair<int, int>> &live_range, std::vector<LLVMValueRef> &sorted_list) {

    for (LLVMValueRef v : sorted_list) {

        if (!overlapingLive()) {
            continue;
        }

        if (reg_map.find(v) != reg_map.end() && reg_map[v] != -1) {
            return v;
        }
    }

    return NULL;
}

std::vector<LLVMValueRef> getSortedInstructions(std::map<LLVMValueRef, pair<int, int>> &live_range) {

    // put every instruction in the list
    std::vector<LLVMValueRef> sorted_list;
    for (auto it = live_range.begin(); it != live_range.end(); ++it) {
        sorted_list.push_back(it->first);
    }

    // sort by end range
    std::sort(sorted_list.begin(), sorted_list.end(), [&live_range](LLVMValueRef a, LLVMValueRef b) { return live_range.at(a).second > live_range.at(b).second;});

    return sorted_list;
}


void linearScan(LLVMModuleRef m) {

    // for each basic block
    LLVMValueRef f = LLVMGetFirstFunction(m);
    for (LLVMBasicBlockRef b = LLVMGetFirstBasicBlock(f); b; b = LLVMGetNextBasicBlock(b)) {

        // Initialize the set of available physical registers to (ebx, ecx, edx)
        std::set<int> regs = { 1, 2, 3 };

        std::map<LLVMValueRef, int> inst_index = getInstIndex(b);
        std::map<LLVMValueRef, pair<int, int>> live_range = getLiveRange(b, inst_index);
        std::vector<LLVMValueRef> sorted_list = getSortedInstructions(live_range);

        for (LLVMValueRef i = LLVMGetFirstInstruction(b); i; i = LLVMGetNextInstruction(i)) {

            // Ignore Instr if it is an alloc instruction
            if (LLVMIsAAllocaInst(i)) continue;

            // if instruction has NO RESULT
            if (LLVMIsAStoreInst(i) || LLVMIsABranchInst(i) || (LLVMIsACallInst(i))) {

                // if any operand of the instruction ends its range at the index of the currently inspected instruction
                // mark its register as available
                for (int opi = 0; opi < LLVMGetNumOperands(i); opi++) {

                    LLVMValueRef o = LLVMGetOperand(i, opi);
                    if (reg_map[o] != -1 && live_range[o].second == inst_index[i]) {
                        regs.insert(reg_map[o]);
                    }
                }
                continue;
            }

            // If the following conditions about Instr  hold: a) is of type add/sub/mul   
            // b) the first operand has a physical register R assigned to it in reg_map and  
            // c) liveness range of the first operand ends at Instr, then
            if (isMath(i)) {

                LLVMValueRef o1 = LLVMGetOperand(i, 0);
                if (reg_map[o1] != -1 && live_range[o1].second == inst_index[i]) {

                    // Add the entry Instr -> R to reg_map
                    reg_map[i] = reg_map[o1];

                    LLVMValueRef o2 = LLVMGetOperand(i, 1);
                    if (reg_map[o2] != -1 && live_range[o2].second == inst_index[i]) {
                        regs.insert(reg_map[o2]);
                    }
                }
            }

            // If a physical register R is available:
            else if (!regs.empty()) {
                int reg = *regs.begin();
                regs.erase(regs.begin());
                reg_map[i] = reg;

                // If live range of any operand of Instr ends, 
                // and it has a physical register P assigned to it then add P to available set of registers.
                for (int opi = 0; opi < LLVMGetNumOperands(i); opi++) {
                    LLVMValueRef o = LLVMGetOperand(i, opi);
                    if (reg_map[o] != -1 && live_range[o].second == inst_index[i]) {
                        regs.insert(reg_map[o]);
                    }
                }
            }

            else if (regs.empty()) {

                LLVMValueRef v = find_spill(i, inst_index, live_range, sorted_list);

                // If V has more uses than Instr
                if (v != NULL && live_range[i].second > live_range[v].second) {

                    // Add the entry Instr -> -1 to reg_map to indicate that it does not have a physical register assigned
                    reg_map[i] = -1;
                }
                else if (v != NULL) {

                    int r = reg_map[v];
                    reg_map[i] = r;
                    reg_map[v] = -1;
                }

                // If the live range of any operand of Instr ends, and it has a 
                // physical register P assigned to it, then add P to the available set of registers.
                for (int opi = 0; opi < LLVMGetNumOperands(i); opi++) {
                    LLVMValueRef o = LLVMGetOperand(i, opi);
                    if (reg_map[o] != -1 && live_range[o].second == inst_index[i]) {
                        regs.insert(reg_map[o]);
                    }
                }
            }
        }
    }
}





std::string intToString(int val) {
    
    std::ostringstream os;
    os << val;
    return os.str();
}


std::map<LLVMBasicBlockRef, std::string> createBBLabels(LLVMValueRef f) {

    std::map<LLVMBasicBlockRef, std::string> res;

    int counter = 0;
    for (LLVMBasicBlockRef b = LLVMGetFirstBasicBlock(f); b; b = LLVMGetNextBasicBlock(b)) {

        if (counter == 0) {
            res[b] = ".LFB0";
            counter++;
        }
        else { 
            res[b] = ".L" + intToString(counter++);
        }
    }

    return res;
}

void printDirectives(int stackSize) {

    fprintf(fp, ".globl main\n");
    fprintf(fp, "main:\n");
    fprintf(fp, "\tpushl %%ebp\n");
    fprintf(fp, "\tmovl %%esp, %%ebp\n");
    fprintf(fp,"fn:\n");
    fprintf(fp,"\tsubl\t$%d, %%esp\n", stackSize);


}

void printFunctionEnd() {
    
    fprintf(fp,"\tleave\n");
    fprintf(fp,"\tret\n");
}

std::map<LLVMValueRef, int> getOffsetMap(LLVMModuleRef m, int &localMem) {

    std::map<LLVMValueRef, int> offset_map;
    localMem = 4;

    LLVMValueRef f = LLVMGetFirstFunction(m);
    LLVMValueRef param = NULL;
    param = LLVMGetParam(f, 0);
    if (param != NULL) {
        offset_map[param] = 8;
    }

    for (LLVMBasicBlockRef b = LLVMGetFirstBasicBlock(f); b; b = LLVMGetNextBasicBlock(b)) {
        for (LLVMValueRef i = LLVMGetFirstInstruction(b); i; i = LLVMGetNextInstruction(i)) {
            
            if (LLVMIsAAllocaInst(i)) {

                localMem += 4;
                offset_map[i] = -localMem;
            }
            else if (LLVMIsAStoreInst(i)) {

                LLVMValueRef op1 = LLVMGetOperand(i, 0);
                LLVMValueRef op2 = LLVMGetOperand(i, 1);
                
                // If the first operand of the store instruction is equal to the function parameter
                if (op1 == param) {

                    // Get the value associated with the first operand in offset_map. Let this be x.
                    // must confirm its in the map before indexing in otherwise wit will create a sudo new value in the map
                    if (offset_map.find(op1) != offset_map.end()) {
                        
                        int x = offset_map[op1];
                        offset_map[op2] = x;
                    }
                }
                else {
                    if (offset_map.find(op2) != offset_map.end()) {
                        
                        int x = offset_map[op2];
                        offset_map[op1] = x;
                    }
                }

            }
            else if (LLVMIsALoadInst(i)) {

                LLVMValueRef op1 = LLVMGetOperand(i, 0);
                if (offset_map.find(op1) != offset_map.end()) {
                        
                    int x = offset_map[op1];
                    offset_map[i] = x;
                }
            }
        }
    }

    return offset_map;
}

void emit(std::string instruction) {
    fprintf(fp, "\t%s\n", instruction.c_str());
}

std::string getRegisterName(int reg) {
    
    std::string res = "__";
    if (reg == 0) {
        res = "ebx";
    }
    else if (reg == 1) {
        res = "ecx";
    }
    else if (reg == 2) {
        res = "edx";
    }
    else {
        res = "edx";
    }
    return res;
}

bool isArithmeticInstruction(LLVMValueRef i) {
    
    LLVMOpcode o = LLVMGetInstructionOpcode(i);
    return (o == LLVMAdd || o == LLVMSub || o == LLVMMul);
}

std::string getOp(LLVMOpcode o) {
    
    switch (o) {
        
        case LLVMAdd: {
            return "addl";
        }

        case LLVMSub: {
            return "subl";
        }

        case LLVMMul: {
            return "imull";
        }

        default: {
            return "addl"; 
        }
    }
}

std::string getComp(LLVMIntPredicate T) {

    switch (T) {

        case LLVMIntULT: {
            return "jl";
        }

        case LLVMIntUGT: {
            return "jg";
        }

        case LLVMIntULE: {
            return "jle";
        }

        case LLVMIntUGE: {
            return "jge";
        }

        case LLVMIntEQ: {
            return "je";
        }

        case LLVMIntSGE: {
            return "jge";
        }
    
        case LLVMIntSLT: {
            return "jl";
            break;
        }
        case LLVMIntSGT: {
            return "jg";
        }
        
        case LLVMIntSLE: {
            return "jle";
        }
    
        default: {     
            break;
        }
    }

    return "noneFound";
}

bool isCompareInstruction(LLVMValueRef i) {
    
    LLVMOpcode o = LLVMGetInstructionOpcode(i);
    return (o == LLVMICmp);  
}


void codeGen(LLVMModuleRef m, FILE* fptr) {

    // set global out file pointer
    fp = fptr;

    // create global reg_map
    linearScan(m);

    LLVMValueRef f = LLVMGetFirstFunction(m);

    std::map<LLVMBasicBlockRef, std::string> bbLabels = createBBLabels(f);
    
    printDirectives(12);

    int localMem;
    std::map<LLVMValueRef, int> offset_map = getOffsetMap(m, localMem);

    emit("subl $" + std::to_string(localMem) + ", %esp");
    emit("pushl %ebx");

    LLVMValueRef param = NULL;
    param = LLVMGetParam(f, 0);
    for (LLVMBasicBlockRef b = LLVMGetFirstBasicBlock(f); b; b = LLVMGetNextBasicBlock(b)) {

        // print basic block label
        const std::string label = bbLabels[b] + ":\n";
        fprintf(fp, "%s", label.c_str());

        for (LLVMValueRef i = LLVMGetFirstInstruction(b); i; i = LLVMGetNextInstruction(i)) {

            if (LLVMIsAReturnInst(i)) {

                // If A is a constant
                LLVMValueRef a = LLVMGetOperand(i, 0);
                if (LLVMIsAConstantInt(a)) {

                    emit("movl $" + std::to_string(LLVMConstIntGetZExtValue(a)) + ", %eax");
                }

                // If A is a temporary variable and is in memory
                else if (offset_map.find(a) != offset_map.end()) {
                    
                    int k = offset_map[a];
                    emit("movl " + std::to_string(k) + "(%ebp), %eax");



                } 
                
                // If A is a temporary variable and has a physical register %exx assigned to it 
                else if (reg_map.find(a) != reg_map.end() && reg_map[a] != -1) {
                    
                    int reg = reg_map[a];
                    emit("movl %" + getRegisterName(reg) + ", %eax");
                }

                emit("popl %ebx");
                printFunctionEnd();
            }

            else if (LLVMIsALoadInst(i)) {
                
                // if %a has been assigned a physical register %exx:
                LLVMValueRef b = LLVMGetOperand(i, 0);
                if (reg_map.find(i) != reg_map.end() && reg_map[i] != -1) {
                    
                    // get offset c for %b
                    int c = offset_map[b];
                    int reg = reg_map[i];
                    emit("movl " + std::to_string(c) + "(%ebp), %" + getRegisterName(reg));
                }
            }

            else if (LLVMIsAStoreInst(i)) {

                LLVMValueRef A = LLVMGetOperand(i, 0);
                LLVMValueRef b = LLVMGetOperand(i, 1);

                if (A == param) {
                    continue;
                }

                if (LLVMIsAConstant(A)) {

                    int c = offset_map[b];
                    emit("movl $" + std::to_string(LLVMConstIntGetZExtValue(A)) + ", " + std::to_string(c) + "(%ebp)");
                }
                
                // If A is a temporary variable %a
                else {
                    
                    // if %a has a physical register %exx assigned to it
                    if (reg_map.find(i) != reg_map.end() && reg_map[i] != -1) {

                        int c = offset_map[b];
                        int reg = reg_map[i];
                        emit("movl %" + getRegisterName(reg) + ", " + std::to_string(c) + "(%ebp)");
                    }

                    else {
                        
                        int c1 = offset_map[A];
                        emit("movl " + std::to_string(c1) + "(%ebp), %eax");
                        int c2 = offset_map[b];
                        emit("movl " + std::to_string(c2) + "(%ebp), %eax");
                    }
                }
            }

            else if (LLVMIsACallInst(i)) {
                emit("pushl %ecx");
                emit("pushl %edx");

                if (param != NULL && LLVMIsAConstant(param)) {
                    emit("pushl $" + std::to_string(LLVMConstIntGetZExtValue(param)));
                }
                
                // if P is a temporary variable %p  
                // if %p has a physical register %exx assigned
                else if (reg_map.find(param) != reg_map.end() && reg_map[param] != -1) {
                            
                    int reg = reg_map[param];
                    emit("pushl %" + getRegisterName(reg));
                }
                
                // if %p is in memory
                else if (offset_map.find(param) != offset_map.end()) {
                    int k = offset_map[param];
                    emit("pushl " + std::to_string(k) + "(%ebp)");
                }

                // emit call func
                emit("call " + std::string(LLVMGetValueName(f)));

                if (param != NULL) {
                    emit("addl $4, %esp");
                }

                emit("popl %edx");
                emit("popl %ecx");

                // if Instr is of the form (%a = call type @func())
                if (LLVMGetFirstUse(i)) {
                    
                    // if %a has a physical register %exx assigned to it
                    if (reg_map.find(i) != reg_map.end() && reg_map[i] != -1) {
                        
                        emit("movl %eax, %" + getRegisterName(reg_map[i]));
                    }
                    // if %a is in memory
                    else if (offset_map.find(i) != offset_map.end()) {
                        int k = offset_map[i];
                        emit("movl %eax, " + std::to_string(k) + "(%ebp)");
                    }
                } 
            }

            else if (LLVMIsABranchInst(i)) {

                // if the branch is unconditional (br label %b)
                if (LLVMGetNumOperands(i) == 1) {

                    // get label L of %b from bb_labels
                    LLVMValueRef b = LLVMGetOperand(i, 0);
                    const char* L = bbLabels[LLVMValueAsBasicBlock(b)].c_str();

                    emit(std::string("jmp ") + L);
                }

                // if the branch is conditional (br i1 %a, label %b, label %c)
                else {

                    LLVMValueRef b = LLVMGetOperand(i, 2);
                    const char* L1 = bbLabels[LLVMValueAsBasicBlock(b)].c_str();
                    LLVMValueRef c = LLVMGetOperand(i, 1);
                    const char* L2 = bbLabels[LLVMValueAsBasicBlock(c)].c_str();

                    // get the predicate T of comparison from instruction %a (<, >, <=, >=, ==)
                    LLVMValueRef a = LLVMGetOperand(i, 0);
                    LLVMIntPredicate T = LLVMGetICmpPredicate(a);
                    std::string comp = getComp(T);

                    emit(std::string(comp) + " " + L1);
                    emit(std::string("jmp ") + L2);
                }
            }

            else if (isArithmeticInstruction(i)) {

                // Let R be the register for %a. If %a has a physical register %exx assigned to it, 
                // then R  is %exx, else R is %eax
                std::string R = "%eax";
                if (reg_map.find(i) != reg_map.end() && reg_map[i] != -1) {
                    
                    R = '%' + getRegisterName(reg_map[i]);
                }

                LLVMValueRef A = LLVMGetOperand(i, 0); 
                if (LLVMIsAConstant(A)) {
                    emit("movl $" + std::to_string(LLVMConstIntGetZExtValue(A)) + ", " + R);
                }

                // if A is a temporary variable
                else {
                    
                    // if A has physical register %eyy assigned to it
                    if (reg_map.find(A) != reg_map.end() && reg_map[A] != -1) {

                        emit("movl %" + getRegisterName(reg_map[A]) + ", " + R);
                    }

                    // if A is in memory
                    else {

                        int n = offset_map[A];
                        emit("movl " + std::to_string(n) + "(%ebp), " + R);
                    }

                }

                LLVMValueRef B = LLVMGetOperand(i, 1);
                if (LLVMIsAConstant(B)) {
                    
                    emit(getOp(LLVMGetInstructionOpcode(i)) + " $" + std::to_string(LLVMConstIntGetZExtValue(B)) + ", " + R);
                }
        
                // if B is a temporary variable and has physical register %ezz assigned to it:
                else if (reg_map.find(B) != reg_map.end() && reg_map[B] != -1) {

                    int reg = reg_map[B];
                    emit(getOp(LLVMGetInstructionOpcode(i)) + " %" + getRegisterName(reg) +  ", " + R);
                }

                // if B is a temporary variable and  does not have a physical register assigned to it:
                else {

                    int m = offset_map[B];
                    emit(getOp(LLVMGetInstructionOpcode(i)) + " " + std::to_string(m) + "(%ebp), " + R);
                }

                // if %a is in memory
                if (reg_map.find(i) != reg_map.end() && reg_map[i] == -1) {
                    
                    int k = reg_map[i];
                    emit("movl %eax, " + std::to_string(k) + "(%ebp)");
                }
                
            }

            else if (isCompareInstruction(i)) {

                // Let R be the register for %a. If %a has a physical register %exx assigned to it, 
                // then R  is %exx, else R is %eax.
                std::string R = "%eax";
                if (reg_map.find(i) != reg_map.end() && reg_map[i] != -1) {
                    
                    R = '%' + getRegisterName(reg_map[i]);
                }
                
                LLVMValueRef A = LLVMGetOperand(i, 0); 
                if (LLVMIsAConstant(A)) {
                    emit("movl $" + std::to_string(LLVMConstIntGetZExtValue(A)) + ", " + R);
                }

                else {

                    // if A has physical register %eyy assigned to it
                    if (reg_map.find(A) != reg_map.end() && reg_map[A] != -1) {

                        emit("movl %" + getRegisterName(reg_map[A]) + ", " + R);
                    }

                    // if A is in memory
                    else {

                        int n = offset_map[A];
                        emit("movl " + std::to_string(n) + "(%ebp), " + R);
                    }

                }

                LLVMValueRef B = LLVMGetOperand(i, 1);
                if (LLVMIsAConstant(B)) {
                    
                    emit(getOp(LLVMGetInstructionOpcode(i)) + " $" + std::to_string(LLVMConstIntGetZExtValue(B)) + ", " + R);
                }

                // if B is a temporary variable and has physical register %ezz assigned to it:
                else if (reg_map.find(B) != reg_map.end() && reg_map[B] != -1) {

                    int reg = reg_map[B];
                    emit("cmpl %" + getRegisterName(reg) +  ", " + R);
                }

                // if B is a temporary variable and  does not have a physical register assigned to it:
                else {

                    int m = offset_map[B];
                    emit("cmpl " + std::to_string(m) + "(%ebp), " + R);
                }
            }
        }
    }
}








