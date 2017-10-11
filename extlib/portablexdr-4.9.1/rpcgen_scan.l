/* -*- C -*-
 * rpcgen - Generate XDR bindings automatically.
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
#include "rpcgen_parse.h"
#include "rpcgen_int.h"
%}

%option noyywrap
%option nounput
%option yylineno

HEXLIT     "0x"[0-9a-fA-F]+
DECLIT     0|[1-9][0-9]*
OCTLIT     0[0-7]+
INTLIT     {HEXLIT}|{DECLIT}|{OCTLIT}
IDENT      [a-zA-Z_][a-zA-Z_0-9]*
WS         [[:space:]]+

%x cstring

%%

 /* # lineno "filename"
  * cpp itself sets file and line numbers using these directives.  We
  * have to parse this ourselves, since these could occur anywhere in
  * the input, eg. in the middle of a struct definition.
  */
^"#".*\n    {
  char *filename;
  int lineno;

  /* Allocate enough space to store the returned filename string. */
  filename = malloc (strlen (yytext));
  if (filename == NULL) perrorf ("malloc");

  if (sscanf (yytext+2, "%d \"%[^\"]\"", &lineno, filename) == 2) {
    yylineno = lineno-1;
    free (input_filename);
    input_filename = filename;
  }
  else free (filename);
 }

 /* Anything on a line beginning with % is passed to the output.  Again
  * we have to handle this within the scanner.
  */
^"%".*\n   fputs (yytext+1, yyout);

 /* C string constants. */
\"         start_string(); BEGIN (cstring);
<cstring>{
  \"       BEGIN (INITIAL); yylval.str = end_string(); return STRLIT;

  \n       error ("unterminated string constant");
  <<EOF>>  error ("unterminated string constant");

  \\[0-7]{1,3} {
    /* octal escape sequence */
    int result;

    (void) sscanf (yytext + 1, "%o", &result);

    if (result > 0xff)
      error ("octal constant is out of range");

    add_char (result);
  }

  /* Does C allow these?? Maybe confusing it with OCaml.
  \\x[0-9a-fA-F]{2} {
    int result;

    (void) sscanf( yytext + 1, "%x", &result );

    add_char (result);
  } */

  \\[0-9]+ {
    /* generate error - bad escape sequence; something
     * like '\48' or '\0777777'
     */
    error ("bad escape sequence: %s\n", yytext);
  }

  \\n  add_char ('\n');
  \\t  add_char ('\t');
  \\r  add_char ('\r');
  \\b  add_char ('\b');
  \\f  add_char ('\f');

  /* Backslash followed by a literal newline character. */
  \\\n add_char ('\n');

  /* Unrecognised escape character - should be an error? */
  \\.  add_char (yytext[1]);

  [^\\\n\"]+ {
    add_string (yytext);
  }
}

 /* Keywords. */
struct     return STRUCT;
enum       return ENUM;
const      return CONST;
typedef    return TYPEDEF;
union      return UNION;
switch     return SWITCH;
case       return CASE;
default    return DEFAULT;
program    return PROGRAM;

unsigned   return UNSIGNED;
signed     return SIGNED;
char       return CHAR;
short      return SHORT;
int        return INT;
hyper      return HYPER;
double     return DOUBLE;
string     return STRING;
opaque     return OPAQUE;
bool       return BOOL;

void       return VOID;

 /* Identifiers. */
{IDENT}    { yylval.str = strdup (yytext); return IDENT; }

 /* Numeric constants are tricky to scan accurately, so keep them as
  * strings and pass them through directly to the C compiler.
  */
{INTLIT}   { yylval.str = strdup (yytext); return INTLIT; }

 /* Single characters with special meaning. */
":"|";"|","|"{"|"}"|"("|")"|"["|"]"|"<"|">"|"="|"*" return yytext[0];

 /* Ignore whitespace. */
{WS}

 /* Anything else is an error. */
.          error ("invalid character in input near '%c'", yytext[0]);
