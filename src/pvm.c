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
#include <assert.h>

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
  pvm_state_finalize (&pvm_state);
  pvm_finalize ();
}

void
pvm_val_free (pvm_val val)
{
  switch (PVM_VAL_TAG (val))
    {
    case PVM_VAL_TAG_LONG:
    case PVM_VAL_TAG_ULONG:
    case PVM_VAL_TAG_STR:
      free (PVM_VAL_PTR (val));
      break;
    case PVM_VAL_TAG_ARR:
      assert (0); /* XXX */
      break;
    case PVM_VAL_TAG_TUP:
      assert (0); /* XXX */
      break;
    case PVM_VAL_TAG_INT:
    case PVM_VAL_TAG_UINT:
    default:
      break;
    }
}

enum pvm_exit_code
pvm_run (pvm_program prog, pvm_val *res)
{
  pvm_state.pvm_state_backing.result_value = PVM_NULL;
  pvm_state.pvm_state_backing.exit_code = PVM_EXIT_OK;
    
  pvm_interpret (prog, &pvm_state);

  if (res != NULL)
    *res = pvm_state.pvm_state_backing.result_value;
  
  return pvm_state.pvm_state_backing.exit_code;
}

pvm_val
pvm_make_int (int32_t value)
{
  return (PVM_VAL_TAG_INT << 61) | (value & 0xffffffff);
}

pvm_val
pvm_make_uint (uint32_t value)
{
  return (PVM_VAL_TAG_UINT << 61) | value;
}

pvm_val
pvm_make_long (int64_t value)
{
  int64_t *p = xmalloc (sizeof (int64_t));
  *p = value;
  return (PVM_VAL_TAG_LONG << 61) | ((uint64_t)p >> 3);
}

pvm_val
pvm_make_ulong (uint64_t value)
{
  uint64_t *p = xmalloc (sizeof (uint64_t));
  *p = value;
  return (PVM_VAL_TAG_ULONG << 61) | ((uint64_t)p >> 3);
}

pvm_val
pvm_make_string (const char *str)
{
  char *p = xmalloc (strlen (str) + 1);
  memcpy (p, str, strlen (str) + 1);
  return (PVM_VAL_TAG_STR << 61) | ((uint64_t)p >> 3);
}
