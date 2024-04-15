%{
#include <stdio.h>
#include "ast.h"
extern int yylex();
extern int yylex_destroy();
extern int yywrap();
void yyerror(const char *);
extern FILE *yyin;
%}

%union{
		int ival;
		char * idname;
	  astNode *nptr;
		vector<astNode *> *svec_ptr;
	}

%token <ival> NUM
%token <idname> ID
%token INT EQ GT LT GE LE

%type <svec_ptr> stmts var_decls
%type <nptr> stmt expr term block_stmt decl

%start block_stmt
%%

block_stmt : '{' var_decls stmts '}' {vector<astNode*>* new_vec = new vector<astNode*>();
																			new_vec->insert(new_vec->end(), $2->begin(), $2->end());
																			new_vec->insert(new_vec->end(), $3->begin(), $3->end());
																			$$ = createBlock(new_vec); 
																			printNode($$);
																		  delete($2);
																			delete($3);
																		} 
					 | '{' stmts '}' {$$ = createBlock($2); 
														printNode($$);}

var_decls	 : var_decls decl {$$ = $1;
														 $$->push_back($2);}
					 | decl {$$ = new vector<astNode*>();
									 $$->push_back($1);}

decl 			 : INT ID ';' {$$ = createDecl($2);}

stmts			 : stmts stmt {$$ = $1;
												 $$->push_back($2);}
					 | stmt {$$ = new vector<astNode*>();
									 $$->push_back($1);}

stmt			 : ID '=' expr ';' {astNode* tnptr = createVar($1);
															$$ = createAsgn(tnptr, $3);}

expr			 : term '+' term {$$ = createBExpr($1, $3, add);}
					 | term '-' term {$$ = createBExpr($1, $3, sub);}
					 | term '*' term {$$ = createBExpr($1, $3, mul);}
					 | term '/' term {$$ = createBExpr($1, $3, divide);}
					 | term {$$ = $1;}

term			 : NUM {$$ = createCnst($1);}
					 | ID {$$ = createVar($1);}
					 | '-' term {$$ = createUExpr($2, uminus);}


%%
int main(int argc, char** argv){
	if (argc == 2){
  	yyin = fopen(argv[1], "r");
	}
	yyparse();
	
	if (yyin != stdin)
		fclose(yyin);
	
	yylex_destroy();
	
	return 0;
}

void yyerror(const char *){
	fprintf(stderr, "Syntax error\n");
}
