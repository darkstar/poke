/* pvm.h - Poke Virtual Machine.  */

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

#include <config.h>
#include <stdlib.h>
#include "pvm.h"

pvm_stack
pvm_stack_new (void)
{
  pvm_stack s;

  s = xmalloc (sizeof (struct pvm_stack));
  memset (s, 0, sizeof (struct pvm_stack));
  return s;
}

void
pvm_stack_free (pvm_stack s)
{
  switch (PVM_STACK_TYPE (s))
    {
    case PVM_STACK_E_STRING:
      free (s->v.string);
      break;
    default:
      break;
    }
  
  free (s);
}
