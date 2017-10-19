/* pvm-bind.h - Poke Virtual Machine.  Binding scopes.  */

/* Copyright (C) 2017 Jose E. Marchesi */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PVM_BINDING_H
#define PVM_BINDING_H

#include <config.h>
#include "pvm.h"

typedef size_t pvm_reg;

/* A `binding' is the association between a symbol (a NULL-terminated
   string) and a Jitter register.  The jitter register contains a PVM
   value, which can be a type.  */

#define PVM_BIND_SYMBOL(B) ((B)->symbol)
#define PVM_BIND_REG(B) ((B)->reg)
#define PVM_BIND_CHAIN(B) ((B)->chain)

struct pvm_bind
{
  char *symbol;
  pvm_reg reg;

  /* For buckets.  */
  struct pvm_bind *chain;
};

typedef struct pvm_bind *pvm_bind;

/* A `binding scope' is a set of bindings.  They can be nested.  At
   any scope, the set of valid bindings are the set of all bindings
   defined in the scope and all its parents.  Bindings in nested
   scopes "ghost" bindings in outer scopes featuring the same
   symbols.  */

#define HASH_TABLE_SIZE 1008

struct pvm_scope
{
  /* Hash containing the symbol bindings defined in this scope.  Each
     entry binds a symbol (identifier) to a jitter register.  The
     jitter register contains a PVM value, which can be a type.  */
  pvm_bind bindings[HASH_TABLE_SIZE];
  
  /* Index of the lowest jitter register index to be used next.
     Incices are allocated sequentially, so the first variable will be
     assigned to 0, the second to 1 and so on.  */
  size_t next_register_index;

  /* This is NULL for the global binding level.  */
  struct pvm_scope *parent;
};

typedef struct pvm_scope *pvm_scope;

/* Initialize and return a new binding scope, nested to SCOPE.  Note
   that SCOPE may be NULL.  */
pvm_scope pvm_push_scope (pvm_scope scope);

/* Destroy SCOPE, freeing all used resources, and return its parent.
   If SCOPE is NULL then do nothing and return NULL.  */
pvm_scope pvm_pop_scope (pvm_scope scope);

/* Get the register bound to a given symbol in SCOPE.  The register is
   set in REG, and the frame in FRAME.  If no such binding exist in
   either SCOPE or its parents, return 0.  Otherwise return 1. */
int pvm_get_bind (pvm_scope scope, const char *symbol,
                  pvm_reg *reg, size_t *frame);

/* Add a new binding to SCOPE.  Return the register bound to SYMBOL.
   If symbol is already bound in SCOPE then replace it.  */
pvm_reg pvm_bind_symbol (pvm_scope scope, const char *symbol);

#endif /* !PVM_BINDING_H */
