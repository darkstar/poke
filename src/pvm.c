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
    case PVM_VAL_TAG_BOX:
      {
        pvm_val_box box = PVM_VAL_BOX (val);

        switch (PVM_VAL_BOX_TAG (box))
        {
        case PVM_VAL_BOX_TAG_STR:
          free (PVM_VAL_BOX_STR (box));
          break;
        case PVM_VAL_BOX_TAG_ARR:
          free (PVM_VAL_BOX_ARR (box));
          break;
        case PVM_VAL_BOX_TAG_TUP:
          free (PVM_VAL_BOX_TUP (box));
          break;
        case PVM_VAL_BOX_TAG_LONG:
        case PVM_VAL_BOX_TAG_ULONG:
        default:
          break;
        }

        free (box);
      }
      break;

    case PVM_VAL_TAG_BYTE:
    case PVM_VAL_TAG_UBYTE:
    case PVM_VAL_TAG_HALF:
    case PVM_VAL_TAG_UHALF:
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
pvm_make_byte (int8_t value)
{
  return (PVM_VAL_TAG_BYTE << 61) | (value & 0xff);
}

pvm_val
pvm_make_ubyte (uint8_t value)
{
  return (PVM_VAL_TAG_UBYTE << 61) | value;
}

pvm_val
pvm_make_half (int16_t value)
{
  return (PVM_VAL_TAG_HALF << 61) | (value & 0xffff);
}

pvm_val
pvm_make_uhalf (uint16_t value)
{
  return (PVM_VAL_TAG_UHALF << 61) | value;
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
  pvm_val_box box = xmalloc (sizeof (struct pvm_val_box));

  PVM_VAL_BOX_TAG (box) = PVM_VAL_BOX_TAG_LONG;
  PVM_VAL_BOX_LONG (box) = value;

  return (PVM_VAL_TAG_BOX << 61) | ((uint64_t)box >> 3);
}

pvm_val
pvm_make_ulong (uint64_t value)
{
  pvm_val_box box = xmalloc (sizeof (struct pvm_val_box));

  PVM_VAL_BOX_TAG (box) = PVM_VAL_BOX_TAG_ULONG;
  PVM_VAL_BOX_ULONG (box) = value;

  return (PVM_VAL_TAG_BOX << 61) | ((uint64_t)box >> 3);
}

pvm_val
pvm_make_string (const char *str)
{
  pvm_val_box box = xmalloc (sizeof (struct pvm_val_box));

  PVM_VAL_BOX_TAG (box) = PVM_VAL_BOX_TAG_STR;
  PVM_VAL_BOX_STR (box) = xstrdup (str);

  return (PVM_VAL_TAG_BOX << 61) | ((uint64_t)box >> 3);
}

const char *
pvm_error (enum pvm_exit_code code)
{
  static char *pvm_error_strings[]
    = { "ok", "error", "division by zero" };
  
  return pvm_error_strings[code];
}
