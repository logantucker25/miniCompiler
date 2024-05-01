#include "optimizations.h"





// ####################################### local optimization helpers section #######################################




// determine if the operatnds of 2 instructions match
bool opMatch(LLVMValueRef a, LLVMValueRef b) {
    if ((LLVMGetInstructionOpcode(a) == LLVMGetInstructionOpcode(b) && LLVMGetNumOperands(a) == LLVMGetNumOperands(b))) {

        for (int i = 0; i < LLVMGetNumOperands(a); i++)
        {
            if (LLVMGetOperand(a, i) != LLVMGetOperand(b, i))
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

// check if there is a store to dangerous memory between instructions a and b
bool interveningStoreInstructions(LLVMValueRef a, LLVMValueRef b)
{

    // get adress of a
    LLVMValueRef aAddress = LLVMGetOperand(a, 1);

    // check every instruction between a and b
    for (LLVMValueRef curr = LLVMGetNextInstruction(a); curr != b; curr = LLVMGetNextInstruction(curr))
    {
        // if the instruction touches memory
        if (LLVMIsAStoreInst(curr))
        {

            // check: instruction mods a adress
            if (LLVMGetOperand(curr, 1) == aAddress)
            {
                return true;
            }
        }
    }
    return false;
}

// determine if an instruction is worth deleting
bool isUselessInstruction(LLVMValueRef instruction)
{

    // check: instruction has "side effects"
    if (LLVMIsAStoreInst(instruction))
    {
        return false;
    }
    if (LLVMIsAAllocaInst(instruction))
    {
        return false;
    }
    if (LLVMIsATerminatorInst(instruction))
    {
        return false;
    }

    // special check to not remove prints
    if (LLVMIsACallInst(instruction))
    {
        return false;  
    } 


    // check: instruction is used somewhere else in block
    if (LLVMGetFirstUse(instruction) != NULL) {
        return false;
    }
    return true;
}

// determin if an instruction has been replaced in all its uses
bool isReplacedInstruction(LLVMValueRef i, std::unordered_set<char*> *replaced) {
    char* iStr = LLVMPrintValueToString(i);
    for (auto it = replaced->begin(); it != replaced->end(); ++it) {
        
        if (strcmp(*it, iStr) == 0) {
            return true;
        }
    }
    return false;
}





// ####################################### local optimization section #######################################





bool eliminateCommonSubExpressions(LLVMBasicBlockRef block, std::unordered_set<char*> *replaced)
{

    bool changeMade = false;

    // loop through all instructions. source instruction A
    for (LLVMValueRef iA = LLVMGetFirstInstruction(block); iA; iA = LLVMGetNextInstruction(iA))
    {
        
        // check to see if this instruction is a ghost
        if (isReplacedInstruction(iA, replaced)) {
            continue;
        }

        // check to see if this instruction allocates memory and should be left alone
        if (LLVMIsAAllocaInst(iA)) {
            continue;
        }


        // loop through all instructions ahead of instruction A. source instruction B
        for (LLVMValueRef iB = LLVMGetNextInstruction(iA); iB; iB = LLVMGetNextInstruction(iB))
        {

            // check matching ops
            if (opMatch(iA, iB))
            {
        
                // non load
                if (!LLVMIsALoadInst(iA) && !LLVMIsALoadInst(iB))
                {

                    // replace
                    LLVMReplaceAllUsesWith(iB, iA);

                    // mark A as replaced
                    replaced->insert(LLVMPrintValueToString(iA));
                    
                    changeMade = true;
                }

                // dual load
                if (LLVMIsALoadInst(iA) && LLVMIsALoadInst(iB))
                {
                    // check if replace is memory safe
                    if (!interveningStoreInstructions(iA, iB))
                    {
                        
                        // replace
                        LLVMReplaceAllUsesWith(iB, iA);

                        // mark A as replaced
                        replaced->insert(LLVMPrintValueToString(iA));

                        changeMade = true;
                    }
                }
            }
        }
    }
    return changeMade;
}

bool eliminateDeadCode(LLVMBasicBlockRef block)
{

    bool changeMade = false;

    // loop through all instructions in the block
    for (LLVMValueRef i = LLVMGetFirstInstruction(block); i != NULL; i = LLVMGetNextInstruction(i))
    {

        // check if instruction is useless
        if (isUselessInstruction(i))
        {
            // remove
            LLVMInstructionEraseFromParent(i);

            changeMade = true;
        }
    }

    return changeMade;
}

bool foldConstants(LLVMBasicBlockRef block)
{

    bool changeMade = false;

    // loop through all instructions in the block
    for (LLVMValueRef i = LLVMGetFirstInstruction(block); i != NULL; i = LLVMGetNextInstruction(i))
    {
        LLVMValueRef next = LLVMGetNextInstruction(i);
        LLVMOpcode op = LLVMGetInstructionOpcode(i);
        LLVMValueRef op1 = LLVMGetOperand(i, 0);
        LLVMValueRef op2 = LLVMGetOperand(i, 1);

        // check: both possible values are constants
        if ((op1 != NULL && op2 != NULL) && LLVMIsAConstant(op1) && LLVMIsAConstant(op2))
        {
            LLVMValueRef newConst = NULL;

            // check: operation performs arithmatic
            switch (op) {
                case LLVMAdd:
                    newConst = LLVMConstAdd(op1, op2);
                    break;
                case LLVMSub:
                    newConst = LLVMConstSub(op1, op2);
                    break;
                case LLVMMul:
                    newConst = LLVMConstMul(op1, op2);
                    break;
                default:
                    break;
            }

            if (newConst != NULL) {
                LLVMReplaceAllUsesWith(i, newConst);
                changeMade = true;
            }
        }
    }

    return changeMade;
}





// ####################################### helper fucntions for global optimization section #######################################





// Compute the set "S" of all store instructions in the given function before computing KILL for all basic blocks
std::unordered_set<LLVMValueRef> getS(LLVMValueRef f)
{
    std::unordered_set<LLVMValueRef> s;
    // loop through all basic blocks
    for (LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(f); block; block = LLVMGetNextBasicBlock(block))
    {
        // loop through all instructions
        for (LLVMValueRef i = LLVMGetFirstInstruction(block); i; i = LLVMGetNextInstruction(i))
        {

            if (LLVMIsAStoreInst(i))
            {
                s.insert(i);
            }
        }
    }
    return s;
}

std::unordered_set<LLVMValueRef> getInSet(std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMValueRef>> blockToOut, std::unordered_set<LLVMBasicBlockRef> predsOfB)
{

    std::unordered_set<LLVMValueRef> in;

    // loop through each predeseccor of block
    for (auto it = predsOfB.begin(); it != predsOfB.end(); ++it)
    {
        LLVMBasicBlockRef pred = *it;

        // get out[predescessor] and add it to result set (in)
        std::unordered_set<LLVMValueRef> outP = blockToOut[pred];
        in.insert(outP.begin(), outP.end());
    }

    return in;
}

std::unordered_set<LLVMValueRef> getOutSet(std::unordered_set<LLVMValueRef> genB, std::unordered_set<LLVMValueRef> inB, std::unordered_set<LLVMValueRef> killB)
{

    // create copy of inB to the subtract values from it
    std::unordered_set<LLVMValueRef> in_kill;
    in_kill = inB;

    // remove values from inB that are in killB (in - kill)
    for (auto it = killB.begin(); it != killB.end(); ++it)
    {
        LLVMValueRef elem = *it;
        in_kill.erase(elem);
    }

    // create copy of genb to then add in_kill to it
    std::unordered_set<LLVMValueRef> genAndIn_kill;
    genAndIn_kill = genB;

    // union gen and (in-kill)
    genAndIn_kill.insert(in_kill.begin(), in_kill.end());

    return genAndIn_kill;
}

std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMBasicBlockRef>> getPredMap(LLVMValueRef f)
{

    std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMBasicBlockRef>> sucToPred;
    for (LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(f); block; block = LLVMGetNextBasicBlock(block))
    {
        LLVMValueRef t = LLVMGetBasicBlockTerminator(block);

        // for each SUCCESSOR that a predesessor (block) has add successor --> predescessor to map
        for (unsigned int i = 0; i < LLVMGetNumSuccessors(t); i++)
        {
            LLVMBasicBlockRef s = LLVMGetSuccessor(t, i);
            sucToPred[s].insert(block);
        }
    }
    return sucToPred;
}

bool isAConstantStore(LLVMValueRef i)
{
    LLVMValueRef storedValue = LLVMGetOperand(i, 0);
    return LLVMIsAConstant(storedValue);
}





// ####################################### global optimization section #######################################





std::unordered_set<LLVMValueRef> computeGenSet(LLVMBasicBlockRef block)
{

    // Initialize the set GEN[B] to empty set
    std::unordered_set<LLVMValueRef> gen;

    // Iterate over all the instructions in basic block
    for (LLVMValueRef i = LLVMGetFirstInstruction(block); i != NULL; i = LLVMGetNextInstruction(i))
    {

        // If instruction is a store instruction, add it to the set
        if (LLVMIsAStoreInst(i))
        {

            // kill all q memory stores
            LLVMValueRef KillAddress = LLVMGetOperand(i, 1);
            for (auto it = gen.begin(); it != gen.end();)
            {

                // if an instruction shares the kill adress, remove it
                if (LLVMGetOperand(*it, 1) == KillAddress)
                {
                    it = gen.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            gen.insert(i);
        }
    }
    return gen;
}

std::unordered_set<LLVMValueRef> computeKillSet(LLVMBasicBlockRef block, LLVMValueRef f)
{

    // get s (all stores in func)
    std::unordered_set<LLVMValueRef> s = getS(f);

    // Initialize KILL[B] = empty set
    std::unordered_set<LLVMValueRef> kill;

    // for each instruction I in basic block B
    for (LLVMValueRef i = LLVMGetFirstInstruction(block); i != NULL; i = LLVMGetNextInstruction(i))
    {

        // If I is a store instruction add all the instructions in S that get killed by I to KILL
        if (LLVMIsAStoreInst(i))
        {
            LLVMValueRef KillAddress = LLVMGetOperand(i, 1);
            for (auto it = s.begin(); it != s.end(); it++)
            {

                // check: same adress. note: instruction cant kill self
                if ((*it != i) && LLVMGetOperand(*it, 1) == KillAddress)
                {
                    kill.insert(*it);
                }
            }
        }
    }
    return kill;
}

std::tuple<
    std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMValueRef>>,
    std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMValueRef>>>
computeInOutSets(LLVMValueRef f)
{

    std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMBasicBlockRef>> sucToPred = getPredMap(f);

    // Initialize set IN[B] of each basic block B to empty set
    // For each basic block B set OUT[B] = GEN[B]
    std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMValueRef>> blockToIn;
    std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMValueRef>> blockToOut;

    for (LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(f); block; block = LLVMGetNextBasicBlock(block))
    {
        blockToIn[block] = std::unordered_set<LLVMValueRef>();
        blockToOut[block] = computeGenSet(block);
    }

    // change = True /* Check for fix-point */
    bool change = true;
    while (change)
    {

        change = false;

        for (LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(f); block; block = LLVMGetNextBasicBlock(block))
        {
            // IN[B] = union(OUT[P1], OUT[P2],.., OUT[PN]), where P1, P2, .. PN are predecessors of basic block B in the control flow graph.
            blockToIn[block] = getInSet(blockToOut, sucToPred[block]);

            std::unordered_set<LLVMValueRef> oldOut;
            oldOut = blockToOut[block];

            // OUT[B] = GEN[B] union (in[B] - kill[B])
            blockToOut[block] = getOutSet(computeGenSet(block), blockToIn[block], computeKillSet(block, f));

            if (blockToOut[block] != oldOut)
            {
                change = true;
            }
        }
    }
    return std::make_tuple(blockToIn, blockToOut);
}

bool loadHandler(LLVMValueRef f)
{

    bool changeMade = false;

    // compute ins and outs
    std::tuple<
        std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMValueRef>>,
        std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMValueRef>>>
        inout = computeInOutSets(f);

    std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMValueRef>> &in = std::get<0>(inout);
    // std::unordered_map<LLVMBasicBlockRef, std::unordered_set<LLVMValueRef>> &out = std::get<1>(inout);

    for (LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(f); block; block = LLVMGetNextBasicBlock(block))
    {

        // instantiate set r
        std::unordered_set<LLVMValueRef> r;
        r = in[block];

        // create list of instructions to delete
        std::unordered_set<LLVMValueRef> toDelete;

        // loop through all instructions in block
        for (LLVMValueRef i = LLVMGetFirstInstruction(block); i != NULL; i = LLVMGetNextInstruction(i))
        {

            // If instruction is a store instruction, add it to r
            if (LLVMIsAStoreInst(i))
            {
                
                LLVMValueRef KillAddress = LLVMGetOperand(i, 1);

                for (auto it = r.begin(); it != r.end();)
                {

                    // if an instruction shares the kill adress, remove it
                    if (LLVMGetOperand(*it, 1) == KillAddress)
                    {
                        it = r.erase(it);
                        // changeMade = true;
                    }
                    else
                    {
                        ++it;
                    }
                }
                r.insert(i);
            }
            else if (LLVMIsALoadInst(i))
            {

                LLVMValueRef t = LLVMGetOperand(i, 0);

                // find all the store instructions in R that write to address represented by %t.
                // bool all storesRconstant
                // sconstant store value
                bool allStoresRConstant = true;
                long constant;
                bool firstConstantFound = false;

                for (auto it = r.begin(); it != r.end(); it++)
                {
                    // ***** BREAKDOWN OF THE DENSE LOGIC BLOCK BELOW *****

                    // if instruction is a store
                    // if stores to adress t
                    // if store is a constant store
                    // if storeVal == Null;
                    // storeVal = r.storeval
                    // else if (r.storeVal != storeVal)
                    //  stroresConstants = false;
                    // break;

                    // else
                    //  stroresConstants = false;
                    // break;

                    if (LLVMIsAStoreInst(*it))
                    {
                        if (LLVMGetOperand(*it, 1) == t)
                        {
                            if (isAConstantStore(*it))
                            {

                                LLVMValueRef ValueOp = LLVMGetOperand(*it, 0);
                                long storedValue = LLVMConstIntGetSExtValue(ValueOp);
                                
                                if (!firstConstantFound) 
                                {
                                    firstConstantFound = true;
                                    constant = storedValue;
                                }
                                if (storedValue != constant)
                                {
                                    allStoresRConstant = false;
                                    break;
                                }
                            }
                            else
                            {
                                allStoresRConstant = false;
                                break;
                            }
                        }
                    }
                }

                // i have now confirmed if all stores in R to %t r constant stores that write the same value (storesRcomnstant)

                // if storesRconstant
                // LLVMReplaceAllUsesWith()
                // save this current instruction i in a list to be deleted

                if (allStoresRConstant)
                {
                    toDelete.insert(i);
                    LLVMReplaceAllUsesWith(i, LLVMConstInt(LLVMInt32Type(), constant, 1));

                    changeMade = true;
                }
            }
        }

        // go through and delete all instructions in list of to delete later
        for (auto it = toDelete.begin(); it != toDelete.end(); it++)
        {
            LLVMInstructionEraseFromParent(*it);
        }
    }
    return changeMade;
}





// ####################################### module section #######################################





LLVMModuleRef optimize(LLVMModuleRef m) {

    bool changeMade = true;

    // if anything on local or global is optimized. repeat the proscess and optimize the module again
    while (changeMade) {
        changeMade = false;

        for (LLVMValueRef f = LLVMGetFirstFunction(m); f; f = LLVMGetNextFunction(f)) {
            
            // run constant propogation
            if (loadHandler(f)) {
                changeMade = true;
            }


            // run local optimizations in a loop until they longer create any changes
            std::unordered_set<char*> replacedInstructions;
            for (LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(f); block != NULL; block = LLVMGetNextBasicBlock(block)) {
                bool localChange = false;
                do {
                    localChange = eliminateCommonSubExpressions(block, &replacedInstructions);
                    localChange |= eliminateDeadCode(block);
                    localChange |= foldConstants(block);
                    changeMade |= localChange;
                } while (localChange);
            }
        }
    } 
    
    return m;
}
