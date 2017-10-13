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
#include <string.h>

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
        case PVM_VAL_TAG_STR:
          free (PVM_VAL_BOX_STR (box));
          break;
        case PVM_VAL_TAG_ARR:
          free (PVM_VAL_BOX_ARR (box));
          break;
        case PVM_VAL_TAG_TUP:
          free (PVM_VAL_BOX_TUP (box));
          break;
        case PVM_VAL_TAG_LONG:
        case PVM_VAL_TAG_ULONG:
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

  PVM_VAL_BOX_TAG (box) = PVM_VAL_TAG_LONG;
  PVM_VAL_BOX_LONG (box) = value;

  return (PVM_VAL_TAG_BOX << 61) | ((uint64_t)box >> 3);
}

pvm_val
pvm_make_ulong (uint64_t value)
{
  pvm_val_box box = xmalloc (sizeof (struct pvm_val_box));

  PVM_VAL_BOX_TAG (box) = PVM_VAL_TAG_ULONG;
  PVM_VAL_BOX_ULONG (box) = value;

  return (PVM_VAL_TAG_BOX << 61) | ((uint64_t)box >> 3);
}

pvm_val
pvm_make_string (const char *str)
{
  pvm_val_box box = xmalloc (sizeof (struct pvm_val_box));

  PVM_VAL_BOX_TAG (box) = PVM_VAL_TAG_STR;
  PVM_VAL_BOX_STR (box) = xstrdup (str);

  return (PVM_VAL_TAG_BOX << 61) | ((uint64_t)box >> 3);
}

pvm_val
pvm_make_array (int type, int arrayof, size_t nelem)
{
  pvm_val_box box = xmalloc (sizeof (struct pvm_val_box));
  pvm_array arr = xmalloc (sizeof (struct pvm_array));

  arr->nelem = nelem;
  arr->type = type;
  arr->arrayof = arrayof;
  arr->elems = xmalloc (sizeof (pvm_val) * nelem);
  memset (arr->elems, 0, sizeof (pvm_val) * nelem);
  
  PVM_VAL_BOX_TAG (box) = PVM_VAL_TAG_ARR;
  PVM_VAL_BOX_ARR (box) = arr;

  return (PVM_VAL_TAG_BOX << 61) | ((uint64_t)box >> 3);
}

pvm_val
pvm_make_tuple (size_t nelem)
{
  pvm_val_box box = xmalloc (sizeof (struct pvm_val_box));
  pvm_tuple tuple = xmalloc (sizeof (struct pvm_tuple));

  tuple->nelem = nelem;
  tuple->elems = xmalloc (sizeof (struct pvm_tuple_elem) * nelem);
  memset (tuple->elems, 0, sizeof (struct pvm_tuple_elem) * nelem);

  PVM_VAL_BOX_TAG (box) = PVM_VAL_TAG_TUP;
  PVM_VAL_BOX_TUP (box) = tuple;

  return (PVM_VAL_TAG_BOX << 61) | ((uint64_t)box >> 3);
}

pvm_val
pvm_ref_tuple (pvm_val tuple, pvm_val name)
{
  size_t nelem, i;
  struct pvm_tuple_elem *elems;

  assert (PVM_IS_TUP (tuple) && PVM_IS_STRING (name));
  
  nelem = PVM_VAL_TUP_NELEM (tuple);
  elems = PVM_VAL_TUP (tuple)->elems;
  
  for (i = 0; i < nelem; ++i)
    {
      if (elems[i].name != PVM_NULL
          && strcmp (PVM_VAL_STR (elems[i].name),
                     PVM_VAL_STR (name)) == 0)
        return elems[i].value;
    }
          
  return PVM_NULL;
}

pvm_val
pvm_elemsof (pvm_val val)
{
  if (PVM_IS_ARR (val))
    return PVM_VAL_ARR_NELEM (val);
  else if (PVM_IS_TUP (val))
    return PVM_VAL_TUP_NELEM (val);
  else
    return 1;
}

pvm_val
pvm_sizeof (pvm_val val)
{
  if (PVM_IS_BYTE (val) || PVM_IS_UBYTE (val))
    return 1;
  else if (PVM_IS_HALF (val) || PVM_IS_UHALF (val))
    return 2;
  else if (PVM_IS_INT (val) || PVM_IS_UINT (val))
    return 4;
  else if (PVM_IS_LONG (val) || PVM_IS_ULONG (val))
    return 8;
  else if (PVM_IS_STRING (val))
    return strlen (PVM_VAL_STR (val)) + 1;
  else if (PVM_IS_ARR (val))
    {
      size_t nelem, i, size;

      nelem = PVM_VAL_ARR_NELEM (val);

      size = 0;
      for (i = 0; i < nelem; ++i)
        size += pvm_sizeof (PVM_VAL_ARR_ELEM (val, i));

      return size;
    }
  else if (PVM_IS_TUP (val))
    {
      size_t nelem, i, size;

      nelem = PVM_VAL_TUP_NELEM (val);

      size = 0;
      for (i = 0; i < nelem; ++i)
        size += pvm_sizeof (PVM_VAL_TUP_ELEM_VALUE (val, i));

      return size;
    }

  assert (0);
  return 0;
}

void
pvm_reverse_tuple (pvm_val tuple)
{
  size_t i, end, nelem;
  struct pvm_tuple_elem *elems;

  nelem = PVM_VAL_TUP_NELEM (tuple);
  elems = PVM_VAL_TUP (tuple)->elems;

  end = nelem - 1;
  for (i = 0; i < nelem / 2; ++i)
    {
      pvm_val tmp_value = elems[i].value;
      pvm_val tmp_name = elems[i].name;

      elems[i].name = elems[end].name;
      elems[i].value = elems[end].value;

      elems[end].name = tmp_name;      
      elems[end].value = tmp_value;
      --end;
    }
}

const char *
pvm_error (enum pvm_exit_code code)
{
  static char *pvm_error_strings[]
    = { "ok", "error", "division by zero" };
  
  return pvm_error_strings[code];
}
