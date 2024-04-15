#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "ast.h"
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <string.h>
#include <vector>

#include <assert.h>

/* Takes in the root node and begins analysis
 * creates data storage
 *       - vector containing vectors of strings corresponding to tokens
 * passes root node into the analysis web
 */
int start(astNode* root);


/* Takes in 
 * 
 *       
 * 
 */
void analysisWeb(astNode* node, astStmt* stmnt, vector<vector<char*>*> *sTable, vector<char*> *sList);

#endif