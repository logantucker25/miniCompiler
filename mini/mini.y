%{
// using namespace std;

#include "semantic.h"

#include "ast.h"
#include <iostream> 
#include <stdio.h>
#include <cstddef>
#include <vector>
#include <stack>
#include <cassert>

extern FILE *yyin;
extern int yylineno;
extern char* yytext;

extern int yyparse();
extern int yylex(void);
extern int yylex_destroy();

void yyerror(const char* s) {
    cerr << "Parse error on line " << yylineno << ": " << s << " at '" << yytext << "'" << endl;
}

// save global root to pass to semantic code
astNode* root;
%}


%union {
    char* name;
    int val;
    astNode* node;
    vector<astNode*> *s_list;
}

// holding tokens
%token <name> NAME
%token <val> VALUE

// non terminals aka tree parents
%type<node> program extern func block stmnt dec expr term equ
%type<node> if_s while_s print_s return_s
%type<s_list> stmnts decs

// empty tokens
%token EXTERN VOID INT PRINT READ IF ELSE WHILE RETURN

// left ascociatives (operators)
%left LE GE NE EQ '>' '<' '*' '/' '+' '-' '='

// non conflict operations
%nonassoc UMINUS 
%nonassoc IFX 
%nonassoc ELSE

%start program

%%

program : extern extern func    { 
                                    $$ = createProg($1, $2, $3); 
                                    root = $$; 
                                }                               
        ;

extern  : EXTERN VOID PRINT '(' INT ')' ';'     { $$ = createExtern("print"); } 
        | EXTERN INT READ '(' ')' ';'           { $$ = createExtern("read");  } 


func    : INT NAME '(' INT NAME ')' block   { 
                                                    astNode* var = createVar($5);
                                                    $$ = createFunc($2, var, $7); 
                                                }
        | INT NAME '(' ')' ';' block    {  
                                            $$ = createFunc($2, NULL, $6); 
                                        }
        ;

block   : '{' decs stmnts '}'   {
                                    vector<astNode*>* new_vec = new vector<astNode*>();
                                    new_vec->insert(new_vec->end(), $2->begin(), $2->end());
                                    new_vec->insert(new_vec->end(), $3->begin(), $3->end());
                                    $$ = createBlock(new_vec);
                                }
        | '{' stmnts '}'        {   $$ = createBlock($2); }

decs    : decs dec  {
                        $$ = $1;
                        $$->push_back($2);
                    }
        | dec       {
                        $$ = new vector<astNode*>();
                        $$->push_back($1);
                    }


dec     : INT NAME ';'  { $$ = createDecl($2); }

stmnts  : stmnts stmnt  {
                            $$ = $1;
                            $$->push_back($2);
                        }
        | stmnt         {
                            $$ = new vector<astNode*>();
                            $$->push_back($1);
                        }

stmnt   : NAME '=' expr ';' {
                                astNode* tnptr = createVar($1);
                                $$ = createAsgn(tnptr, $3);
                            }
        | if_s
        | while_s
        | print_s
        | return_s

if_s    : IF '(' equ ')' block ELSE block   { $$ = createIf($3, $5, $7); }    
        | IF '(' equ ')' block %prec IFX    { $$ = createIf($3, $5); }

while_s : WHILE '(' equ ')' block   { $$ = createWhile($3, $5); }

print_s : PRINT '(' expr ')' ';'  { $$ = createCall("print", $3);}

return_s: RETURN '(' expr ')' ';'  { $$ = createRet($3); }


expr	: '-' term %prec UMINUS { $$ = createUExpr($2, uminus); }
        | term '+' term     {$$ = createBExpr($1, $3, add); }
		| term '-' term     {$$ = createBExpr($1, $3, sub); }
		| term '*' term     {$$ = createBExpr($1, $3, mul); }
		| term '/' term     {$$ = createBExpr($1, $3, divide); }
        | term              {$$ = $1; }

equ     : term '<' term     { $$ = createRExpr($1, $3, lt); }
		| term '>' term     { $$ = createRExpr($1, $3, gt); }
		| term LE  term     { $$ = createRExpr($1, $3, le); }
        | term GE  term     { $$ = createRExpr($1, $3, ge); }
        | term NE  term     { $$ = createRExpr($1, $3, neq); }
        | term EQ  term     { $$ = createRExpr($1, $3, eq); }
        | term              {$$ = $1; }

term    : VALUE             {$$ = createCnst($1); }
        | NAME              {$$ = createVar($1); }

%%

int main(int argc, char** argv){

    #ifdef YYDEBUG
        // yydebug = 1;
    #endif

	if (argc == 2){
  	yyin = fopen(argv[1], "r");
	}

    // action
	yyparse();
    start(root);

	if (yyin != stdin)
		fclose(yyin);

	yylex_destroy();
	
	return 0;
}
