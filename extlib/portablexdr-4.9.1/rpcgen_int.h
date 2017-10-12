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

#ifndef RPCGEN_INT_H
#define RPCGEN_INT_H

/* Current input file (updated by # line directives in the source). */
extern char *input_filename;

/* Current output file. */
extern const char *output_filename;

/* Current output mode. */
enum output_mode {
  output_c = 0,
  output_h = 1,
};
extern enum output_mode output_mode;

/* Abstract syntax tree types. */
enum type_enum {
  type_char, type_short, type_int, type_hyper,
  type_double,
  type_bool,
  type_ident,
};

struct type {
  enum type_enum type;
  int sgn;			/* true if signed, false if unsigned */
  char *ident;
};

extern struct type *new_type (enum type_enum, int, char *);
extern void free_type (struct type *);

enum decl_type {
  decl_type_string,	        /* string foo<len>; (len is optional) */
  decl_type_opaque_fixed,	/* opaque foo[len]; */
  decl_type_opaque_variable,	/* opaque foo<len>; */
  decl_type_simple,		/* type ident; */
  decl_type_fixed_array,	/* type ident[len]; */
  decl_type_variable_array,	/* type ident<len>; (len is optional) */
  decl_type_pointer,		/* type *ident; */
};

struct decl {
  enum decl_type decl_type;
  struct type *type;		/* NULL for string & opaque types. */
  char *ident;
  char *len;
};

extern struct decl *new_decl (enum decl_type, struct type *, char *, char *);
extern void free_decl (struct decl *);

struct enum_value {
  char *ident;
  char *value;
};

extern struct enum_value *new_enum_value (char *, char *);
extern void free_enum_value (struct enum_value *);

enum union_case_type {
  union_case_normal,		/* case const: decl; */
  union_case_default_void,	/* default: void; */
  union_case_default_decl,	/* default: decl; */
};

struct union_case {
  enum union_case_type type;
  char *const_;
  struct decl *decl;
};

extern struct union_case *new_union_case (enum union_case_type, char *, struct decl *);
extern void free_union_case (struct union_case *);

typedef void (*free_fn) (void *);

struct cons {
  struct cons *next; /* cdr/tail */
  void *ptr; /* car/head */
  free_fn free; /* free the head element */
};

extern struct cons *new_cons (struct cons *, void *, free_fn);
extern struct cons *list_rev (struct cons *);
extern void list_free (struct cons *);

/* Code generator functions. */
extern void gen_prologue (const char *filename);
extern void gen_epilogue (void);
extern void gen_const (const char *name, const char *value);
extern void gen_enum (const char *name, const struct cons *enum_values);
extern void gen_struct (const char *name, const struct cons *decls);
extern void gen_union (const char *name, const struct decl *discrim, const struct cons *union_cases);
extern void gen_typedef (const struct decl *decl);

/* Global functions used by the scanner. */
extern void start_string (void);
extern char *end_string (void);
extern void add_char (int);
extern void add_string (const char *);

/* These functions print an error and then exit. */
extern void error (const char *, ...)
  __attribute__((noreturn, format(printf,1,2)));
extern void perrorf (const char *, ...)
  __attribute__((noreturn, format(printf,1,2)));

/* Symbols exported from the scanner and parser. */
extern FILE *yyin, *yyout;
extern int yyparse (void);
extern int yylineno;
extern int yydebug;

#endif /* RPCGEN_INT_H */
