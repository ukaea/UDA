/* rpcgen - Generate XDR bindings automatically.    -*- text -*-
 * Copyright (C) 2008 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

%{
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include "rpcgen_int.h"

extern void yyerror (const char *str);
%}

%union {
  char *str;
  struct type *type;
  struct decl *decl;
  struct enum_value *enum_value;
  struct union_case *union_case;
  struct cons *list;
}

%type <type> type_ident
%type <str> const
%type <decl> decl
%type <decl> simple_decl fixed_array_decl variable_array_decl pointer_decl
%type <decl> string_decl opaque_decl
%type <enum_value> enum_value
%type <union_case> union_case
%type <list> decls enum_values union_cases

%token STRUCT
%token ENUM
%token CONST
%token TYPEDEF
%token UNION
%token SWITCH
%token CASE
%token DEFAULT
%token PROGRAM

%token UNSIGNED
%token SIGNED
%token CHAR
%token SHORT
%token INT
%token HYPER
%token DOUBLE
%token STRING
%token OPAQUE
%token BOOL

/* This is sometimes lumped together with the other types, but
 * the special keyword void can only occur after "default:" in
 * union statements.
 */
%token VOID

%token <str> IDENT
%token <str> INTLIT
%token <str> STRLIT

%%

file	: /* empty */
	| stmts
	;

/* Statements. */
stmts	: stmt ';'
	| stmts stmt ';'
	;

stmt	: ENUM IDENT '{' enum_values '}'
	{
	  struct cons *enums = list_rev ($4);
	  gen_enum ($2, enums);
	  free ($2);
	  list_free (enums);
	}
	| STRUCT IDENT '{' decls '}'
	{
	  struct cons *decls = list_rev ($4);
	  gen_struct ($2, decls);
	  free ($2);
	  list_free (decls);
	}
	| UNION IDENT SWITCH '(' decl ')' '{' union_cases '}'
	{
	  struct cons *cases = list_rev ($8);
	  gen_union ($2, $5, cases);
	  free ($2);
	  free_decl ($5);
	  list_free (cases);
	}
	| TYPEDEF decl
	{
	  gen_typedef ($2);
	  free_decl ($2);
	}
	| CONST IDENT '=' const
	{
	  gen_const ($2, $4);
	  free ($2);
	  free ($4);
	}
	| PROGRAM
	{
	  error ("PortableXDR does not support SunRPC program statements");
	}
	;

/* Declarations used inside structs and unions.  eg. "int foo;" */
decls	: decl ';'
	{ $$ = new_cons (NULL, $1, (free_fn) free_decl); }
	| decls decl ';'
	{ $$ = new_cons ($1, $2, (free_fn) free_decl); }
	;

decl	: string_decl
	| opaque_decl
	| simple_decl
	| fixed_array_decl
	| variable_array_decl
	| pointer_decl
	;

string_decl
	: STRING IDENT '<' const '>'
	{
	  $$ = new_decl (decl_type_string, NULL, $2, $4);
	}
	| STRING IDENT '<' '>'
	{
	  $$ = new_decl (decl_type_string, NULL, $2, NULL);
	}
	;

opaque_decl
	: OPAQUE IDENT '[' const ']'
	{
	  $$ = new_decl (decl_type_opaque_fixed, NULL, $2, $4);
	}
	| OPAQUE IDENT '<' const '>'
	{
	  $$ = new_decl (decl_type_opaque_variable, NULL, $2, $4);
	}
	;

simple_decl
	: type_ident IDENT
	{ $$ = new_decl (decl_type_simple, $1, $2, NULL); }
	;

fixed_array_decl
	: type_ident IDENT '[' const ']'
	{ $$ = new_decl (decl_type_fixed_array, $1, $2, $4); }
	;

variable_array_decl
	: type_ident IDENT '<' const '>'
	{ $$ = new_decl (decl_type_variable_array, $1, $2, $4); }
	| type_ident IDENT '<' '>'
	{ $$ = new_decl (decl_type_variable_array, $1, $2, NULL); }
	;

pointer_decl
	: type_ident '*' IDENT
	{ $$ = new_decl (decl_type_pointer, $1, $3, NULL); }
	;

/* Enumerations. */
enum_values
	: enum_value
	{ $$ = new_cons (NULL, $1, (free_fn) free_enum_value); }
	| enum_values ',' enum_value
	{ $$ = new_cons ($1, $3, (free_fn) free_enum_value); }
	;

enum_value
	: IDENT
	{ $$ = new_enum_value ($1, NULL); }
	| IDENT '=' const
	{ $$ = new_enum_value ($1, $3); }
	;

/* Case list inside a union. */
union_cases
	: union_case ';'
	{ $$ = new_cons (NULL, $1, (free_fn) free_union_case); }
	| union_cases union_case ';'
	{ $$ = new_cons ($1, $2, (free_fn) free_union_case); }
	;

union_case
	: CASE const ':' decl
	{ $$ = new_union_case (union_case_normal, $2, $4); }
	| DEFAULT ':' VOID
	{ $$ = new_union_case (union_case_default_void, NULL, NULL); }
	| DEFAULT ':' decl
	{ $$ = new_union_case (union_case_default_decl, NULL, $3); }
	;

/* Constants, which may be integer literals or refer to previously
 * defined constants (using "const" keyword).
 * XXX In future we should probably allow computed constants.
 */
const	: INTLIT
	| IDENT
	;

/* Types.  Note 'string', 'opaque' and 'void' are handled by
 * special cases above.
 */
type_ident
	: CHAR
	/* NB: Unlike SunRPC we make char explicitly signed.  This
	 * will give some warnings in GCC if you mix PortableXDR
	 * code with SunRPC headers or vice versa.
	 */
	{ $$ = new_type (type_char, 1, NULL); }
	| SIGNED CHAR
	{ $$ = new_type (type_char, 1, NULL); }
	| UNSIGNED CHAR
	{ $$ = new_type (type_char, 0, NULL); }
	| SHORT
	{ $$ = new_type (type_short, 1, NULL); }
	| SIGNED SHORT
	{ $$ = new_type (type_short, 1, NULL); }
	| UNSIGNED SHORT
	{ $$ = new_type (type_short, 0, NULL); }
	| INT
	{ $$ = new_type (type_int, 1, NULL); }
	| SIGNED INT
	{ $$ = new_type (type_int, 1, NULL); }
	| UNSIGNED INT
	{ $$ = new_type (type_int, 0, NULL); }
	| HYPER
	{ $$ = new_type (type_hyper, 1, NULL); }
	| SIGNED HYPER
	{ $$ = new_type (type_hyper, 1, NULL); }
	| UNSIGNED HYPER
	{ $$ = new_type (type_hyper, 0, NULL); }
	| SIGNED
	{ $$ = new_type (type_int, 1, NULL); }
	| UNSIGNED
	{ $$ = new_type (type_int, 0, NULL); }
	| DOUBLE
	{ $$ = new_type (type_double, 0, NULL); }
	| BOOL
	{ $$ = new_type (type_bool, 0, NULL); }
	| IDENT
	{ $$ = new_type (type_ident, 0, $1); }
	;

%%

void
yyerror (const char *str)
{
  error ("%s", str);
}
