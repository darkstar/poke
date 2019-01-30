/* pvm.h - Values for the PVM.  */

/* Copyright (C) 2019 Jose E. Marchesi */

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
#include <gc.h>

#include "pk-term.h"
#include "pvm.h"

pvm_val
pvm_make_int (int32_t value, int size)
{
  return (((((int64_t) value) & 0xffffffff) << 32)
          | (((size - 1) & 0x1f) << 3)
          | PVM_VAL_TAG_INT);
}

pvm_val
pvm_make_uint (uint32_t value, int size)
{
  return (((((uint64_t) value) & 0xffffffff) << 32)
          | (((size - 1) & 0x1f) << 3)
          | PVM_VAL_TAG_UINT);
}

static inline pvm_val
pvm_make_long_ulong (int64_t value, int size, int tag)
{
  uint64_t *ll = GC_MALLOC_UNCOLLECTABLE (sizeof (uint64_t) * 2);
      
  ll[0] = value;
  ll[1] = (size - 1) & 0x3f;
  return ((uint64_t) (uintptr_t) ll) | tag;
}

pvm_val
pvm_make_long (int64_t value, int size)
{
  return pvm_make_long_ulong (value, size, PVM_VAL_TAG_LONG);
}

pvm_val
pvm_make_ulong (uint64_t value, int size)
{
  return pvm_make_long_ulong (value, size, PVM_VAL_TAG_ULONG);
}

static pvm_val_box
pvm_make_box (uint8_t tag)
{
  pvm_val_box box = GC_MALLOC_UNCOLLECTABLE (sizeof (struct pvm_val_box));

  PVM_VAL_BOX_TAG (box) = tag;
  return box;
}

pvm_val
pvm_make_string (const char *str)
{
  pvm_val_box box = pvm_make_box (PVM_VAL_TAG_STR);

  PVM_VAL_BOX_STR (box) = GC_STRDUP (str);
  return PVM_BOX (box);
}

pvm_val
pvm_make_array (pvm_val nelem, pvm_val type)
{
  pvm_val_box box = pvm_make_box (PVM_VAL_TAG_ARR);
  pvm_array arr = GC_MALLOC_UNCOLLECTABLE (sizeof (struct pvm_array));
  size_t nbytes = sizeof (struct pvm_array_elem) * PVM_VAL_ULONG (nelem);
  size_t i;

  arr->offset = PVM_NULL;
  arr->mapper = PVM_NULL;
  arr->writer = PVM_NULL;
  arr->nelem = nelem;
  arr->type = type;
  arr->elems = GC_MALLOC_UNCOLLECTABLE (nbytes);

  for (i = 0; i < PVM_VAL_ULONG (nelem); ++i)
    {
      arr->elems[i].offset = PVM_NULL;
      arr->elems[i].value = PVM_NULL;
    }
  
  PVM_VAL_BOX_ARR (box) = arr;
  return PVM_BOX (box);
}

pvm_val
pvm_make_struct (pvm_val nelem)
{
  pvm_val_box box = pvm_make_box (PVM_VAL_TAG_SCT);
  pvm_struct sct = GC_MALLOC_UNCOLLECTABLE (sizeof (struct pvm_struct));
  size_t nbytes = sizeof (struct pvm_struct_elem) * PVM_VAL_ULONG (nelem);

  sct->offset = PVM_NULL;
  sct->mapper = PVM_NULL;
  sct->writer = PVM_NULL;
  sct->type = PVM_NULL; /* XXX add type in constructor.  */
  sct->nelem = nelem;
  sct->elems = GC_MALLOC_UNCOLLECTABLE (nbytes);
  memset (sct->elems, 0, nbytes);

  PVM_VAL_BOX_SCT (box) = sct;
  return PVM_BOX (box);
}

pvm_val
pvm_ref_struct (pvm_val sct, pvm_val name)
{
  size_t nelem, i;
  struct pvm_struct_elem *elems;

  assert (PVM_IS_SCT (sct) && PVM_IS_STR (name));
  
  nelem = PVM_VAL_ULONG (PVM_VAL_SCT_NELEM (sct));
  elems = PVM_VAL_SCT (sct)->elems;
  
  for (i = 0; i < nelem; ++i)
    {
      if (elems[i].name != PVM_NULL
          && strcmp (PVM_VAL_STR (elems[i].name),
                     PVM_VAL_STR (name)) == 0)
        return elems[i].value;
    }
          
  return PVM_NULL;
}

static pvm_val
pvm_make_type (enum pvm_type_code code)
{
  pvm_val_box box = pvm_make_box (PVM_VAL_TAG_TYP);
  pvm_type type = GC_MALLOC_UNCOLLECTABLE (sizeof (struct pvm_type));

  memset (type, 0, sizeof (struct pvm_type));
  type->code = code;

  PVM_VAL_BOX_TYP (box) = type;
  return PVM_BOX (box);
}

pvm_val
pvm_make_integral_type (pvm_val size, pvm_val signed_p)
{
  pvm_val itype = pvm_make_type (PVM_TYPE_INTEGRAL);

  PVM_VAL_TYP_I_SIZE (itype) = size;
  PVM_VAL_TYP_I_SIGNED (itype) = signed_p;
  return itype;
}

pvm_val
pvm_make_string_type (void)
{
  return pvm_make_type (PVM_TYPE_STRING);
}

pvm_val
pvm_make_offset_type (pvm_val base_type, pvm_val unit)
{
  pvm_val otype = pvm_make_type (PVM_TYPE_OFFSET);

  PVM_VAL_TYP_O_BASE_TYPE (otype) = base_type;
  PVM_VAL_TYP_O_UNIT (otype) = unit;
  return otype;
}

pvm_val
pvm_make_array_type (pvm_val type)
{
  pvm_val atype = pvm_make_type (PVM_TYPE_ARRAY);

  PVM_VAL_TYP_A_ETYPE (atype) = type;
  return atype;
}

pvm_val
pvm_make_struct_type (pvm_val nelem,
                      pvm_val *enames, pvm_val *etypes)
{
  pvm_val stype = pvm_make_type (PVM_TYPE_STRUCT);

  PVM_VAL_TYP_S_NELEM (stype) = nelem;
  PVM_VAL_TYP_S_ENAMES (stype) = enames;
  PVM_VAL_TYP_S_ETYPES (stype) = etypes;

  return stype;
}

pvm_val
pvm_make_closure_type (pvm_val rtype,
                       pvm_val nargs, pvm_val *atypes)
{
  pvm_val ctype = pvm_make_type (PVM_TYPE_CLOSURE);

  PVM_VAL_TYP_C_RETURN_TYPE (ctype) = rtype;
  PVM_VAL_TYP_C_NARGS (ctype) = nargs;
  PVM_VAL_TYP_C_ATYPES (ctype) = atypes;

  return ctype;
}

pvm_val
pvm_make_cls (pvm_program program)
{
  pvm_val_box box = pvm_make_box (PVM_VAL_TAG_CLS);
  pvm_cls cls = GC_MALLOC_UNCOLLECTABLE (sizeof (struct pvm_cls));

  cls->program = program;
  cls->entry_point = PVM_PROGRAM_BEGINNING (program);
  cls->env = NULL; /* This should be set by a PEC instruction before
                      using the closure.  */

  PVM_VAL_BOX_CLS (box) = cls;
  return PVM_BOX (box);
}

pvm_val
pvm_make_offset (pvm_val magnitude, pvm_val unit)
{
  pvm_val_box box = pvm_make_box (PVM_VAL_TAG_OFF);
  pvm_off off = GC_MALLOC_UNCOLLECTABLE (sizeof (struct pvm_off));

  off->base_type = pvm_typeof (magnitude);
  off->magnitude = magnitude;
  off->unit = unit;

  PVM_VAL_BOX_OFF (box) = off;
  return PVM_BOX (box);
}

void
pvm_allocate_struct_attrs (pvm_val nelem,
                           pvm_val **enames, pvm_val **etypes)
{
  size_t nbytes = sizeof (pvm_val) * PVM_VAL_ULONG (nelem) * 2;
  *enames = GC_MALLOC_UNCOLLECTABLE (nbytes);
  *etypes = GC_MALLOC_UNCOLLECTABLE (nbytes);
}

void
pvm_allocate_closure_attrs (pvm_val nargs, pvm_val **atypes)
{
  size_t nbytes = sizeof (pvm_val) * PVM_VAL_ULONG (nargs);
  *atypes = GC_MALLOC_UNCOLLECTABLE (nbytes);
}

pvm_val
pvm_elemsof (pvm_val val)
{
  if (PVM_IS_ARR (val))
    return PVM_VAL_ARR_NELEM (val);
  else if (PVM_IS_SCT (val))
    return PVM_VAL_SCT_NELEM (val);
  else if (PVM_IS_STR (val))
    return pvm_make_ulong (strlen (PVM_VAL_STR (val)) + 1, 64);
  else
    return pvm_make_ulong (1, 64);
}

pvm_val
pvm_val_mapper (pvm_val val)
{
  if (PVM_IS_ARR (val))
    return PVM_VAL_ARR_MAPPER (val);
  /* XXX
  if (PVM_IS_SCT (val))
    return PVM_VAL_SCT_MAPPER (val);
  */

  return PVM_NULL;
}

pvm_val
pvm_val_writer (pvm_val val)
{
  if (PVM_IS_ARR (val))
    return PVM_VAL_ARR_WRITER (val);
  /* XXX
  if (PVM_IS_SCT (val))
    return PVM_VAL_SCT_WRITER (val);
  */

  return PVM_NULL;
}

pvm_val
pvm_sizeof (pvm_val val)
{
  if (PVM_IS_INT (val))
    return pvm_make_offset (pvm_make_ulong (PVM_VAL_INT_SIZE (val), 64),
                            pvm_make_ulong (PVM_VAL_OFF_UNIT_BITS, 64));
  else if (PVM_IS_UINT (val))
    return pvm_make_offset (pvm_make_ulong (PVM_VAL_UINT_SIZE (val), 64),
                            pvm_make_ulong (PVM_VAL_OFF_UNIT_BITS, 64));
  else if (PVM_IS_LONG (val))
    return pvm_make_offset (pvm_make_ulong (PVM_VAL_LONG_SIZE (val), 64),
                            pvm_make_ulong (PVM_VAL_OFF_UNIT_BITS, 64));
  else if (PVM_IS_ULONG (val))
    return pvm_make_offset (pvm_make_ulong (PVM_VAL_ULONG_SIZE (val), 64),
                            pvm_make_ulong (PVM_VAL_OFF_UNIT_BITS, 64));
  else if (PVM_IS_STR (val))
    {
      size_t size = (strlen (PVM_VAL_STR (val)) + 1) * 8;

      return pvm_make_offset (pvm_make_ulong (size, 64),
                              pvm_make_ulong (PVM_VAL_OFF_UNIT_BITS, 64));
    }
  else if (PVM_IS_ARR (val))
    {
      size_t nelem, i, size;

      nelem = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (val));

      size = 0;
      for (i = 0; i < nelem; ++i)
        {
          pvm_val off = pvm_sizeof (PVM_VAL_ARR_ELEM_VALUE (val, i));

          size += (PVM_VAL_ULONG (PVM_VAL_OFF_MAGNITUDE (off))
                   * PVM_VAL_ULONG (PVM_VAL_OFF_UNIT (off)));
        }

      return pvm_make_offset (pvm_make_ulong (size, 64),
                              pvm_make_ulong (PVM_VAL_OFF_UNIT_BITS, 64));
    }
  else if (PVM_IS_SCT (val))
    {
      size_t nelem, i, size;

      nelem = PVM_VAL_ULONG (PVM_VAL_SCT_NELEM (val));

      size = 0;
      for (i = 0; i < nelem; ++i)
        {
          pvm_val off = pvm_sizeof (PVM_VAL_SCT_ELEM_VALUE (val, i));

          size += (PVM_VAL_ULONG (PVM_VAL_OFF_MAGNITUDE (off))
                   * PVM_VAL_ULONG (PVM_VAL_OFF_UNIT (off)));
        }

      return pvm_make_offset (pvm_make_ulong (size, 64),
                              pvm_make_ulong (PVM_VAL_OFF_UNIT_BITS, 64));
    }
  else if (PVM_IS_OFF (val))
    return pvm_sizeof (PVM_VAL_OFF_MAGNITUDE (val));
  else if (PVM_IS_TYP (val))
    {
      size_t size;
      
      /* XXX */
      switch (PVM_VAL_TYP_CODE (val))
        {
        case PVM_TYPE_INTEGRAL:
          size = PVM_VAL_ULONG (PVM_VAL_TYP_I_SIZE (val));
          break;
        default:
          assert (0);
        };

      return pvm_make_offset (pvm_make_ulong (size, 64),
                              pvm_make_ulong (PVM_VAL_OFF_UNIT_BITS, 64));
    }
  
  assert (0);
  return 0;
}

void
pvm_reverse_struct (pvm_val sct)
{
  size_t i, end, nelem;
  struct pvm_struct_elem *elems;

  nelem = PVM_VAL_ULONG (PVM_VAL_SCT_NELEM (sct));
  elems = PVM_VAL_SCT (sct)->elems;

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

/* XXX use similar printers for hexadecimal and octal, so we can use
   _'s */

static void
pvm_print_binary (FILE *out, uint64_t val, int size, int sign)
{
  char b[65];

  if (size != 64 && size != 32 && size != 16 && size != 8
      && size != 4)
    fprintf (out, "(%sint<%d>) ", sign ? "" : "u", size);

  for (int z = 0; z < size; z++) {
    b[size-1-z] = ((val >> z) & 0x1) + '0';
  }
  b[size] = '\0';

  fprintf (out, "0b%s", b);

  if (size == 64)
    {
      if (!sign)
        fputc ('U', out);
      fputc ('L', out);
    }
  else if (size == 16)
    {
      if (!sign)
        fputc ('U', out);
      fputc ('H', out);
    }
  else if (size == 8)
    {
      if (!sign)
        fputc ('U', out);
      fputc ('B', out);
    }
  else if (size == 4)
    {
      {
        if (!sign)
          fputc ('U', out);
      }
      fputc ('N', out);
    }
}

void
pvm_print_val (FILE *out, pvm_val val, int base, int flags)
{
  const char *long64_fmt, *long_fmt;
  const char *ulong64_fmt, *ulong_fmt; 
  const char *int32_fmt, *int16_fmt, *int8_fmt, *int4_fmt, *int_fmt;
  const char *uint32_fmt, *uint16_fmt, *uint8_fmt, *uint4_fmt, *uint_fmt;

  /* Select the appropriate formatting templates for the given
     base.  */
  switch (base)
    {
    case 8:
      long64_fmt = "0o%" PRIo64 "L";
      long_fmt = "(int<%d>) 0o%" PRIo64;
      ulong64_fmt = "0o%" PRIo64 "UL";
      ulong_fmt = "(uint<%d>) 0o%" PRIo64; 
      int32_fmt = "0o%" PRIo32;
      int16_fmt = "0o%" PRIo32 "H";
      int8_fmt = "0o%" PRIo32 "B";
      int4_fmt = "0o%" PRIo32 "N";
      int_fmt = "(int<%d>) 0o%" PRIo32;
      uint32_fmt = "0o%" PRIo32 "U";
      uint16_fmt = "0o%" PRIo32 "UH";
      uint8_fmt = "0o%" PRIo32 "UB";
      uint4_fmt = "0o%" PRIo32 "UN";
      uint_fmt = "(uint<%d>) 0o%" PRIo32;
      break;
    case 10:
      long64_fmt = "%" PRIi64 "L";
      long_fmt = "(int<%d>) %" PRIi64;
      ulong64_fmt = "%" PRIu64 "UL";
      ulong_fmt = "(uint<%d>) %" PRIu64; 
      int32_fmt = "%" PRIi32;
      int16_fmt = "%" PRIi32 "H";
      int8_fmt = "%" PRIi32 "B";
      int4_fmt = "%" PRIi32 "N";
      int_fmt = "(int<%d>) %" PRIi32;
      uint32_fmt = "%" PRIu32 "U";
      uint16_fmt = "%" PRIu32 "UH";
      uint8_fmt = "%" PRIu32 "UB";
      uint4_fmt = "%" PRIu32 "UN";
      uint_fmt = "(uint<%d>) %" PRIu32;
      break;
    case 16:
      long64_fmt = "0x%" PRIx64 "L";
      long_fmt = "(int<%d>) %" PRIx64;
      ulong64_fmt = "0x%" PRIx64 "UL";
      ulong_fmt = "(uint<%d>) %" PRIx64; 
      int32_fmt = "0x%" PRIx32;
      int16_fmt = "0x%" PRIx32 "H";
      int8_fmt = "0x%" PRIx32 "B";
      int4_fmt = "0x%" PRIx32 "N";
      int_fmt = "(int<%d>) 0x%" PRIx32;
      uint32_fmt = "0x%" PRIx32 "U";
      uint16_fmt = "0x%" PRIx32 "UH";
      uint8_fmt = "0x%" PRIx32 "UB";
      uint4_fmt = "0x%" PRIx32 "UN";
      uint_fmt = "(uint<%d>) 0x%" PRIo32;
      break;
    case 2:
      /* This base doesn't use printf's formatting strings, but its
         own printer.  */
      long64_fmt = "";
      long_fmt = "";
      ulong64_fmt = "";
      ulong_fmt = "";
      int32_fmt = "";
      int16_fmt = "";
      int8_fmt = "";
      int4_fmt = "";
      int_fmt = "";
      uint32_fmt = "";
      uint16_fmt = "";
      uint8_fmt = "";
      uint4_fmt = "";
      uint_fmt = "";
      break;
    default:
      assert (0);
      break;
    }

  /* And print out the value in the given stream..  */
  if (val == PVM_NULL)
    fprintf (out, "null");
  else if (PVM_IS_LONG (val))
    {
      int size = PVM_VAL_LONG_SIZE (val);
      int64_t longval = PVM_VAL_LONG (val);
      uint64_t ulongval;

      if (size == 64)
        ulongval = (uint64_t) longval;
      else
        ulongval = (uint64_t) longval & ((((uint64_t) 1) << size) - 1);

      if (base == 2)
        pvm_print_binary (out, ulongval, size, 1);
      else
        {
          if (size == 64)
            fprintf (out, long64_fmt, base == 10 ? longval : ulongval);
          else
            fprintf (out, long_fmt,
                     PVM_VAL_LONG_SIZE (val), base == 10 ? longval : ulongval);
        }
    }
  else if (PVM_IS_INT (val))
    {
      int size = PVM_VAL_INT_SIZE (val);
      int32_t intval = PVM_VAL_INT (val);
      uint32_t uintval;

      if (size == 32)
        uintval = (uint32_t) intval;
      else
        uintval = (uint32_t) intval & ((((uint32_t) 1) << size) - 1);

      if (base == 2)
        pvm_print_binary (out, (uint64_t) uintval, size, 1);
      else
        {
          if (size == 32)
            fprintf (out, int32_fmt, base == 10 ? intval : uintval);
          else if (size == 16)
            fprintf (out, int16_fmt, base == 10 ? intval : uintval);
          else if (size == 8)
            fprintf (out, int8_fmt, base == 10 ? intval : uintval);
          else if (size == 4)
            fprintf (out, int4_fmt, base == 10 ? intval : uintval);
          else
            fprintf (out, int_fmt,
                     PVM_VAL_INT_SIZE (val), base == 10 ? intval : uintval);
        }
    }
  else if (PVM_IS_ULONG (val))
    {
      int size = PVM_VAL_ULONG_SIZE (val);
      uint64_t ulongval = PVM_VAL_ULONG (val);
      
      if (base == 2)
        pvm_print_binary (out, ulongval, size, 0);
      else
        {
          if (size == 64)
            fprintf (out, ulong64_fmt, ulongval);
          else
            fprintf (out, ulong_fmt,
                     PVM_VAL_LONG_SIZE (val), ulongval);
        }
    }
  else if (PVM_IS_UINT (val))
    {
      int size = PVM_VAL_UINT_SIZE (val);
      uint32_t uintval = PVM_VAL_UINT (val);

      if (base == 2)
        pvm_print_binary (out, uintval, size, 0);
      else
        {
          if (size == 32)
            fprintf (out, uint32_fmt, uintval);
          else if (size == 16)
            fprintf (out, uint16_fmt, uintval);
          else if (size == 8)
            fprintf (out, uint8_fmt, uintval);
          else if (size == 4)
            fprintf (out, uint4_fmt, uintval);
          else
            fprintf (out, uint_fmt,
                     PVM_VAL_UINT_SIZE (val), uintval);
        }
    }
  else if (PVM_IS_STR (val))
    {
      const char *str = PVM_VAL_STR (val);
      char *str_printable;
      size_t str_size = strlen (PVM_VAL_STR (val));
      size_t printable_size, i, j;

      /* Calculate the length (in bytes) of the printable string
         corresponding to the string value.  */
      for (printable_size = 0, i = 0; i < str_size; i++)
        {
          switch (str[i])
            {
            case '\n': printable_size += 2; break;
            case '\t': printable_size += 2; break;
            case '\\': printable_size += 2; break;
            default: printable_size += 1; break;
            }
        }

      /* Now build the printable string.  */
      str_printable = xmalloc (printable_size + 1);
      for (i = 0, j = 0; i < printable_size; i++)
        {
          switch (str[i])
            {
            case '\n':
              str_printable[j] = '\\';
              str_printable[j+1] = 'n';
              j += 2;
              break;
            case '\t':
              str_printable[j] = '\\';
              str_printable[j+1] = 't';
              j += 2;
              break;
            case '\\':
              str_printable[j] = '\\';
              str_printable[j+1] = '\\';
              j += 2;
              break;
            default:
              str_printable[j] = str[i];
              j++;
              break;
            }
        }
      str_printable[j] = '\0';
      
      fprintf (out, "\"%s\"", str_printable);
      free (str_printable);
    }
  else if (PVM_IS_ARR (val))
    {
      size_t nelem, idx;
      pvm_val array_offset = PVM_VAL_ARR_OFFSET (val);
      
      nelem = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (val));

      fprintf (out, "[");
      for (idx = 0; idx < nelem; idx++)
        {
          pvm_val elem_offset = PVM_VAL_ARR_ELEM_OFFSET (val, idx);
          pvm_val elem_value = PVM_VAL_ARR_ELEM_VALUE (val, idx);
          
          if (idx != 0)
            fprintf (out, ",");
          pvm_print_val (out, elem_value, base, flags);

          if (flags & PVM_PRINT_F_MAPS && elem_offset != PVM_NULL)
            {
              fputc ('@', out);
              pvm_print_val (out, elem_offset, base, flags);
            }
        }
      fprintf (out, "]");

      if (flags & PVM_PRINT_F_MAPS && array_offset != PVM_NULL)
        {
          fputc ('@', out);
          pvm_print_val (out, array_offset, base, flags);
        }      
    }
  else if (PVM_IS_SCT (val))
    {
      size_t nelem, idx;

      nelem = PVM_VAL_ULONG (PVM_VAL_SCT_NELEM (val));
      fprintf (out, "{");
      for (idx = 0; idx < nelem; ++idx)
        {
          pvm_val name = PVM_VAL_SCT_ELEM_NAME(val, idx);
          pvm_val value = PVM_VAL_SCT_ELEM_VALUE(val, idx);

          if (idx != 0)
            fprintf (out, ",");
          if (name != PVM_NULL)
            fprintf (out, ".%s=", PVM_VAL_STR (name));
          pvm_print_val (out, value, base, flags);
        }
      fprintf (out, "}");
    }
  else if (PVM_IS_TYP (val))
    {
      switch (PVM_VAL_TYP_CODE (val))
        {
        case PVM_TYPE_INTEGRAL:
          {
            if (!(PVM_VAL_UINT (PVM_VAL_TYP_I_SIGNED (val))))
              fprintf (out, "u");
            
            switch (PVM_VAL_ULONG (PVM_VAL_TYP_I_SIZE (val)))
              {
              case 8: fprintf (out, "int8"); break;
              case 16: fprintf (out, "int16"); break;
              case 32: fprintf (out, "int32"); break;
              case 64: fprintf (out, "int64"); break;
              default: assert (0); break;
              }
          }
          break;
        case PVM_TYPE_STRING:
          fprintf (out, "string");
          break;
        case PVM_TYPE_ARRAY:
          pvm_print_val (out, PVM_VAL_TYP_A_ETYPE (val), base, flags);
          fprintf (out, "[]");
          break;
        case PVM_TYPE_OFFSET:
          fprintf (out, "[");
          pvm_print_val (out, PVM_VAL_TYP_O_BASE_TYPE (val), base, flags);
          fputc (' ', out);
          switch (PVM_VAL_ULONG (PVM_VAL_TYP_O_UNIT (val)))
            {
            case PVM_VAL_OFF_UNIT_BITS:
              fputc ('b', out);
              break;
            case PVM_VAL_OFF_UNIT_BYTES:
              fputc ('B', out);
              break;
            case PVM_VAL_OFF_UNIT_KILOBITS:
              fputs ("Kb", out);
              break;
            case PVM_VAL_OFF_UNIT_KILOBYTES:
              fputs ("KB", out);
              break;
            case PVM_VAL_OFF_UNIT_MEGABITS:
              fputs ("Mb", out);
              break;
            case PVM_VAL_OFF_UNIT_MEGABYTES:
              fputs ("MB", out);
              break;
            case PVM_VAL_OFF_UNIT_GIGABITS:
              fputs ("Gb", out);
              break;
            default:
              assert (0);
            }
          fprintf (out, "]");
          break;
        case PVM_TYPE_CLOSURE:
          {
            size_t i, nargs;

            nargs = PVM_VAL_ULONG (PVM_VAL_TYP_C_NARGS (val));

            fprintf (out, "(");
            for (i = 0; i < nargs; ++i)
              {
                pvm_val atype = PVM_VAL_TYP_C_ATYPE (val, i);
                pvm_print_val (out, atype, base, flags);
              }
            pvm_print_val (out, PVM_VAL_TYP_C_RETURN_TYPE (val), 10, flags);
            break;
          }
        case PVM_TYPE_STRUCT:
          {
            size_t i, nelem;

            nelem = PVM_VAL_ULONG (PVM_VAL_TYP_S_NELEM (val));

            fprintf (out, "struct {");
            for (i = 0; i < nelem; ++i)
              {
                pvm_val ename = PVM_VAL_TYP_S_ENAME(val, i);
                pvm_val etype = PVM_VAL_TYP_S_ETYPE(val, i);

                if (i != 0)
                  fprintf (out, " ");
                
                pvm_print_val (out, etype, base, flags);
                if (ename != PVM_NULL)
                  fprintf (out, " %s", PVM_VAL_STR (ename));
                fprintf (out, ";");
              }
            fprintf (out, "}");
          break;
          }
        default:
          assert (0);
        }
    }
  else if (PVM_IS_OFF (val))
    {
      pvm_print_val (out, PVM_VAL_OFF_MAGNITUDE (val), base, flags);
      fprintf (out, CYAN "#" NOATTR);
      switch (PVM_VAL_ULONG (PVM_VAL_OFF_UNIT (val)))
        {
        case PVM_VAL_OFF_UNIT_BITS:
          fprintf (out, CYAN "b" NOATTR);
          break;
        case PVM_VAL_OFF_UNIT_NIBBLES:
          fprintf (out, CYAN "N" NOATTR);
          break;
        case PVM_VAL_OFF_UNIT_BYTES:
          fprintf (out, CYAN "B" NOATTR);
          break;
        case PVM_VAL_OFF_UNIT_KILOBITS:
          fprintf (out, CYAN "Kb" NOATTR);
          break;
        case PVM_VAL_OFF_UNIT_KILOBYTES:
          fprintf (out, CYAN "KB" NOATTR);
          break;
        case PVM_VAL_OFF_UNIT_MEGABITS:
          fprintf (out, CYAN "Mb" NOATTR);
          break;
        case PVM_VAL_OFF_UNIT_MEGABYTES:
          fprintf (out, CYAN "MB" NOATTR);
          break;
        case PVM_VAL_OFF_UNIT_GIGABITS:
          fprintf (out, CYAN "Gb" NOATTR);
          break;
        default:
          /* XXX: print here the name of the base type of the
             offset.  */
          fprintf (out, CYAN "%" PRIu64 NOATTR,
                   PVM_VAL_ULONG (PVM_VAL_OFF_UNIT (val)));
          break;
        }
    }
  else if (PVM_IS_CLS (val))
    {
      fprintf (out, "#<closure>");
    }
  else
    assert (0);
}

pvm_val
pvm_typeof (pvm_val val)
{
  pvm_val type;
  
  if (PVM_IS_INT (val))
    type = pvm_make_integral_type (pvm_make_ulong (PVM_VAL_INT_SIZE (val), 64),
                                   pvm_make_uint (1, 32));
  else if (PVM_IS_UINT (val))
    type = pvm_make_integral_type (pvm_make_ulong (PVM_VAL_UINT_SIZE (val), 64),
                                   pvm_make_uint (0, 32));
  else if (PVM_IS_LONG (val))
    type = pvm_make_integral_type (pvm_make_ulong (PVM_VAL_LONG_SIZE (val), 64),
                                   pvm_make_uint (1, 32));
  else if (PVM_IS_ULONG (val))
    type = pvm_make_integral_type (pvm_make_ulong (PVM_VAL_ULONG_SIZE (val), 64),
                                   pvm_make_uint (0, 32));
  else if (PVM_IS_STR (val))
    type = pvm_make_string_type ();
  else if (PVM_IS_OFF (val))
    type = pvm_make_offset_type (PVM_VAL_OFF_BASE_TYPE (val),
                                 PVM_VAL_OFF_UNIT (val));
  else if (PVM_IS_ARR (val))
    type = pvm_make_array_type (PVM_VAL_ARR_TYPE (val));
  else if (PVM_IS_SCT (val))
    {
      size_t i;
      pvm_val *enames = NULL, *etypes = NULL;
      pvm_val nelem = PVM_VAL_SCT_NELEM (val);

      if (PVM_VAL_ULONG (nelem) > 0)
        {
          enames = GC_MALLOC_UNCOLLECTABLE (PVM_VAL_ULONG (nelem) * sizeof (pvm_val));
          etypes = GC_MALLOC_UNCOLLECTABLE (PVM_VAL_ULONG (nelem) * sizeof (pvm_val));
      
          for (i = 0; i < PVM_VAL_ULONG (nelem); ++i)
            {
              enames[i] = PVM_VAL_SCT_ELEM_NAME (val, i);
              etypes[i] = pvm_typeof (PVM_VAL_SCT_ELEM_VALUE (val, i));
            }
        }
      
      type = pvm_make_struct_type (nelem, enames, etypes);
    }
  else
    assert (0);

  return type;
}

void
pvm_print_string (FILE *out, pvm_val string)
{
  fputs (PVM_VAL_STR (string), out);
}
