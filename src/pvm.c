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

static struct pvm_state pvm_state;

void
pvm_init (void)
{
  pvm_initialize ();
  pvm_state_initialize (&pvm_state);
}

void
pvm_shutdown (void)
{
  pvm_stack_free (pvm_state.pvm_state_backing.result_value);
  pvm_state_finalize (&pvm_state);
  pvm_finalize ();
}

pvm_stack
pvm_result (void)
{
  return (pvm_stack) pvm_state.pvm_state_backing.result_value;
}

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
  if (s == NULL)
    return;
  
  switch (PVM_STACK_TYPE (s))
    {
    case PVM_STACK_STR:
      free (s->v.string);
      break;
    default:
      break;
    }
  
  free (s);
}

enum pvm_exit_code
pvm_execute (pvm_program prog, pvm_stack *res)
{
  pvm_stack_free (pvm_state.pvm_state_backing.result_value);
  pvm_state.pvm_state_backing.result_value = NULL;
  pvm_state.pvm_state_backing.exit_code = PVM_EXIT_OK;
    
  pvm_interpret (prog, &pvm_state);

  if (res != NULL)
    *res = pvm_state.pvm_state_backing.result_value;
  
  return pvm_state.pvm_state_backing.exit_code;
}
