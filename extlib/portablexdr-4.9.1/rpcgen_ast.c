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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpcgen_int.h"

struct type *
new_type (enum type_enum type, int sgn, char *ident)
{
  struct type *r = malloc (sizeof *r);
  r->type = type;
  r->sgn = sgn;
  r->ident = ident;
  return r;
}

void
free_type (struct type *t)
{
  if (t) {
    free (t->ident);
    free (t);
  }
}

struct decl *
new_decl (enum decl_type decl_type, struct type *type,
	  char *ident, char *len)
{
  struct decl *r = malloc (sizeof *r);
  r->decl_type = decl_type;
  r->type = type;
  r->ident = ident;
  r->len = len;
  return r;
}

void
free_decl (struct decl *d)
{
  if (d) {
    free_type (d->type);
    free (d->ident);
    free (d->len);
    free (d);
  }
}

struct enum_value *
new_enum_value (char *ident, char *value)
{
  struct enum_value *r = malloc (sizeof *r);
  r->ident = ident;
  r->value = value;
  return r;
}

void
free_enum_value (struct enum_value *v)
{
  if (v) {
    free (v->ident);
    free (v->value);
    free (v);
  }
}

struct union_case *
new_union_case (enum union_case_type type, char *const_, struct decl *decl)
{
  struct union_case *r = malloc (sizeof *r);
  r->type = type;
  r->const_ = const_;
  r->decl = decl;
  return r;
}

void
free_union_case (struct union_case *c)
{
  if (c) {
    free (c->const_);
    free_decl (c->decl);
    free (c);
  }
}

struct cons *
new_cons (struct cons *next, void *ptr, free_fn free)
{
  struct cons *r = malloc (sizeof *r);
  r->next = next;
  r->ptr = ptr;
  r->free = free;
  return r;
}

static struct cons *
list_rev_append (struct cons *xs1, struct cons *xs2)
{
  if (!xs1) return xs2;
  else {
    struct cons *tail = xs1->next;
    xs1->next = xs2;
    return list_rev_append (tail, xs1);
  }
}

struct cons *
list_rev (struct cons *xs)
{
  return list_rev_append (xs, NULL);
}

void
list_free (struct cons *xs)
{
  if (xs) {
    struct cons *next = xs->next;
    if (xs->free) xs->free (xs->ptr);
    free (xs);
    list_free (next);
  }
}
