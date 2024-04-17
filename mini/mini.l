%{
#include "ast.h"
#include <stdio.h>
#include "y.tab.h"
#include <string.h>
void yyerror(const char *);
int yyerror(char *);
// int yywrap();
%}

/* ---- token rules section ---- */

%%

"extern"    { return EXTERN; }
"void"      { return VOID; }
"int"       { return INT; }

"print"     { return PRINT; }
"read"      { return READ; }


"if"        { return IF; }
"else"      { return ELSE; }
"while"     { return WHILE; }

"return"    { return RETURN;}


[-<>=+/*}{.)(;]  { return yytext[0]; }

"<="        { return LE; }  
">="        { return GE; }  
"!="        { return NE; }  
"=="        { return EQ; } 


[a-zA-Z][a-zA-Z0-9]*  { 
    yylval.name = strdup(yytext);         
    return (NAME); }


[0-9]+  { 
    yylval.val = atoi(yytext);
    return (VALUE); }


[  \t\n]+   {}


.           { yyerror("invalid character"); }

%%

/* ---- code section ---- */

int yywrap(void) { 
    return 1;
}





