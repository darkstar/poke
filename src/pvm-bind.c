/* pvm-bind.c - Poke Virtual Machine.  Binding scopes.  */

/* Copyright (C) 2018 Jose E. Marchesi */

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

#include <config.h>
#include <xalloc.h>
#include "pvm-bind.h"

static int
hash_string (const char *name)
{
  size_t len;
  int hash;
  int i;

  len = strlen (name);
  hash = len;
  for (i = 0; i < len; i++)
    hash = ((hash * 613) + (unsigned)(name[i]));

#define HASHBITS 30
  hash &= (1 << HASHBITS) - 1;
  hash %= HASH_TABLE_SIZE;
#undef HASHBITS

  return hash;
}

pvm_scope
pvm_push_scope (pvm_scope scope)
{
  pvm_scope new = xmalloc (sizeof (struct pvm_scope));

  memset (new, 0, sizeof (struct pvm_scope));
  new->parent = scope;
  return new;
}

pvm_scope
pvm_pop_scope (pvm_scope scope)
{
  size_t i;
  pvm_scope parent;
  
  if (scope == NULL)
    return NULL;

  for (i = 0; i < HASH_TABLE_SIZE; ++i)
    if (scope->bindings[i])
      {
        pvm_bind b, n;
        for (b = scope->bindings[i]; b; b = n)
          {
            n = PVM_BIND_CHAIN (b);
            free (PVM_BIND_SYMBOL (b));
          }
      }

  parent = scope->parent;
  free (scope);

  return parent;
}

int
pvm_get_bind (pvm_scope scope, const char *symbol,
              pvm_reg *reg, size_t *frame)
{

  return 0;
}
