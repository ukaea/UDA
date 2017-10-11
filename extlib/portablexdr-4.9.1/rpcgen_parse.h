
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
     STRUCT = 258,
     ENUM = 259,
     CONST = 260,
     TYPEDEF = 261,
     UNION = 262,
     SWITCH = 263,
     CASE = 264,
     DEFAULT = 265,
     PROGRAM = 266,
     UNSIGNED = 267,
     SIGNED = 268,
     CHAR = 269,
     SHORT = 270,
     INT = 271,
     HYPER = 272,
     DOUBLE = 273,
     STRING = 274,
     OPAQUE = 275,
     BOOL = 276,
     VOID = 277,
     IDENT = 278,
     INTLIT = 279,
     STRLIT = 280
   };
#endif
/* Tokens.  */
#define STRUCT 258
#define ENUM 259
#define CONST 260
#define TYPEDEF 261
#define UNION 262
#define SWITCH 263
#define CASE 264
#define DEFAULT 265
#define PROGRAM 266
#define UNSIGNED 267
#define SIGNED 268
#define CHAR 269
#define SHORT 270
#define INT 271
#define HYPER 272
#define DOUBLE 273
#define STRING 274
#define OPAQUE 275
#define BOOL 276
#define VOID 277
#define IDENT 278
#define INTLIT 279
#define STRLIT 280




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 28 "rpcgen_parse.y"

  char *str;
  struct type *type;
  struct decl *decl;
  struct enum_value *enum_value;
  struct union_case *union_case;
  struct cons *list;



/* Line 1676 of yacc.c  */
#line 113 "rpcgen_parse.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


