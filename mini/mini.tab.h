/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NAME = 258,
     VALUE = 259,
     EXTERN = 260,
     VOID = 261,
     INT = 262,
     PRINT = 263,
     READ = 264,
     IF = 265,
     ELSE = 266,
     WHILE = 267,
     RETURN = 268,
     EQ = 269,
     NE = 270,
     GE = 271,
     LE = 272,
     UMINUS = 273,
     IFX = 274
   };
#endif
/* Tokens.  */
#define NAME 258
#define VALUE 259
#define EXTERN 260
#define VOID 261
#define INT 262
#define PRINT 263
#define READ 264
#define IF 265
#define ELSE 266
#define WHILE 267
#define RETURN 268
#define EQ 269
#define NE 270
#define GE 271
#define LE 272
#define UMINUS 273
#define IFX 274




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 27 "mini.y"
{
    char* name;
    int val;
    astNode* node;
    vector<astNode*> *s_list;
}
/* Line 1529 of yacc.c.  */
#line 94 "mini.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

