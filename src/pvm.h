/* pvm.h - Poke Virtual Machine.  Definitions.   */

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

#ifndef PVM_H
#define PVM_H

#include <config.h>
#include <string.h>
#include <stdint.h>
#include <xalloc.h>

/* The pvm_val opaque type implements values that are native to the
   poke virtual machine:

   - Signed ("bytes") and unsigned ("ubytes") 8-bit integers.
   - Signed ("halfs") and unsigned ("uhalfs") 16-bit integers.
   - Signed ("ints") and unsigned ("uints") 32-bit integers.
   - Signed ("longs") and unsigned ("ulongs") 64-bit integers.
   - Strings.
   - Arrays.
   - Structs.
   - Offsets.

   It is fundamental for pvm_val values to fit in 64 bits, in order to
   avoid expensive allocations and to also improve the performance of
   the virtual machine.  The 32-bit integers are unboxed.  64-bit
   integers, strings, arrays and structs are boxed.  Both boxed and
   unboxed values are manipulated by the PVM users using the same API,
   defined below in this header file.  */

typedef uint64_t pvm_val;

/* The least-significative bits of pvm_val are reserved for the tag,
   which specifies the type of the value.  */

#define PVM_VAL_TAG(V) ((V) & 0x7)

#define PVM_VAL_TAG_INT   0x0
#define PVM_VAL_TAG_BYTE  0x1
#define PVM_VAL_TAG_UBYTE 0x2
#define PVM_VAL_TAG_HALF  0x3
#define PVM_VAL_TAG_UHALF 0x4
#define PVM_VAL_TAG_UINT  0x5
#define PVM_VAL_TAG_BOX   0x6
/* Note that there is no tag 0x7.  It is used to implement PVM_NULL
   below.  */
/* Note also that the tags below are stored in the box, not in
   PVM_VAL_TAG.  See below in this file.  */
#define PVM_VAL_TAG_LONG  0x8
#define PVM_VAL_TAG_ULONG 0x9
#define PVM_VAL_TAG_STR   0xa
#define PVM_VAL_TAG_ARR   0xb
#define PVM_VAL_TAG_SCT   0xc
#define PVM_VAL_TAG_TYP   0xd
#define PVM_VAL_TAG_MAP   0xe
#define PVM_VAL_TAG_OFF   0xf

/* 8-bit integers (both signed and unsigned) are encoded in the bits
   10..3 of pvm_val.  */

#define PVM_VAL_BYTE(V) ((int8_t) ((V) >> 3))
#define PVM_VAL_UBYTE(V) ((uint8_t) ((V) >> 3))

pvm_val pvm_make_byte (int8_t value);
pvm_val pvm_make_ubyte (uint8_t value);

/* 16-bit integers (both signed and unsigned) are encoded in the bits
   18..3 of pvm_val.  */

#define PVM_VAL_HALF(V) ((int16_t) ((V) >> 3))
#define PVM_VAL_UHALF(V) ((int16_t) ((V) >> 3))

pvm_val pvm_make_half (int16_t value);
pvm_val pvm_make_uhalf (uint16_t value);

/* 32-bit integers (both signed and unsigned) are encoded in the bits
   34..3 bits of pvm_val.  */

#define PVM_VAL_INT(V) ((int32_t) ((V) >> 3))
#define PVM_VAL_UINT(V) ((uint32_t) ((V) >> 3))

pvm_val pvm_make_int (int32_t value);
pvm_val pvm_make_uint (uint32_t value);

/* A pointer to a boxed value is encoded in the most significative 61
   bits of pvm_val.  Note that this assumes all pointers are aligned
   to 8 bytes.  The allocator for the boxed values makes sure this is
   always the case.  */

#define PVM_VAL_BOX(V) ((pvm_val_box) ((V) & ~0x7))

/* A box is a header for a boxed value, plus that value.  It is of
   type `pvm_val_box'.  */

#define PVM_VAL_BOX_TAG(B) ((B)->tag)
#define PVM_VAL_BOX_LONG(B) ((B)->v.l)
#define PVM_VAL_BOX_ULONG(B) ((B)->v.ul)
#define PVM_VAL_BOX_STR(B) ((B)->v.string)
#define PVM_VAL_BOX_ARR(B) ((B)->v.array)
#define PVM_VAL_BOX_SCT(B) ((B)->v.sct)
#define PVM_VAL_BOX_TYP(B) ((B)->v.type)
#define PVM_VAL_BOX_MAP(B) ((B)->v.map)
#define PVM_VAL_BOX_OFF(B) ((B)->v.offset)

struct pvm_val_box
{
  uint8_t tag;
  union
  {
    int64_t l;
    uint64_t ul;
    char *string;
    struct pvm_array *array;
    struct pvm_struct *sct;
    struct pvm_type *type;
    struct pvm_map *map;
    struct pvm_off *offset;
  } v;
};

typedef struct pvm_val_box *pvm_val_box;

/* 64-bit integers (both signed and unsigned) are boxed.  */

#define PVM_VAL_LONG(V) (PVM_VAL_BOX_LONG (PVM_VAL_BOX ((V))))
#define PVM_VAL_ULONG(V) (PVM_VAL_BOX_ULONG (PVM_VAL_BOX ((V))))

pvm_val pvm_make_long (int64_t value);
pvm_val pvm_make_ulong (uint64_t value);

/* Strings are also boxed.  */

#define PVM_VAL_STR(V) (PVM_VAL_BOX_STR (PVM_VAL_BOX ((V))))

pvm_val pvm_make_string (const char *value);

/* Arrays are also boxed.  */

#define PVM_VAL_ARR(V) (PVM_VAL_BOX_ARR (PVM_VAL_BOX ((V))))
#define PVM_VAL_ARR_TYPE(V) (PVM_VAL_ARR(V)->type)
#define PVM_VAL_ARR_ARRAYOF(V) (PVM_VAL_ARR(V)->type)
#define PVM_VAL_ARR_NELEM(V) (PVM_VAL_ARR(V)->nelem)
#define PVM_VAL_ARR_ELEM(V,I) (PVM_VAL_ARR(V)->elems[(I)])

struct pvm_array
{
  pvm_val type;
  pvm_val nelem;
  pvm_val *elems;
};

typedef struct pvm_array *pvm_array;

pvm_val pvm_make_array (pvm_val nelem, pvm_val type);

/* Structs are also boxed.  */

#define PVM_VAL_SCT(V) (PVM_VAL_BOX_SCT (PVM_VAL_BOX ((V))))
#define PVM_VAL_SCT_NELEM(V) (PVM_VAL_SCT((V))->nelem)
#define PVM_VAL_SCT_ELEM(V,I) (PVM_VAL_SCT((V))->elems[(I)])

struct pvm_struct
{
  pvm_val nelem;
  struct pvm_struct_elem *elems;
};

#define PVM_VAL_SCT_ELEM_NAME(V,I) (PVM_VAL_SCT_ELEM((V),(I)).name)
#define PVM_VAL_SCT_ELEM_VALUE(V,I) (PVM_VAL_SCT_ELEM((V),(I)).value)

struct pvm_struct_elem
{
  pvm_val name;
  pvm_val value;
};

typedef struct pvm_struct *pvm_struct;

pvm_val pvm_make_struct (pvm_val nelem);
void pvm_reverse_struct (pvm_val sct);
pvm_val pvm_ref_struct (pvm_val sct, pvm_val name);

/* Types are also boxed.  */

#define PVM_VAL_TYP(V) (PVM_VAL_BOX_TYP (PVM_VAL_BOX ((V))))

#define PVM_VAL_TYP_CODE(V) (PVM_VAL_TYP((V))->code)
#define PVM_VAL_TYP_I_SIZE(V) (PVM_VAL_TYP((V))->val.integral.size)
#define PVM_VAL_TYP_I_SIGNED(V) (PVM_VAL_TYP((V))->val.integral.signed_p)
#define PVM_VAL_TYP_A_NELEM(V) (PVM_VAL_TYP((V))->val.array.nelem)
#define PVM_VAL_TYP_A_ETYPE(V) (PVM_VAL_TYP((V))->val.array.etype)
#define PVM_VAL_TYP_S_NELEM(V) (PVM_VAL_TYP((V))->val.sct.nelem)
#define PVM_VAL_TYP_S_ENAMES(V) (PVM_VAL_TYP((V))->val.sct.enames)
#define PVM_VAL_TYP_S_ETYPES(V) (PVM_VAL_TYP((V))->val.sct.etypes)
#define PVM_VAL_TYP_S_ENAME(V,I) (PVM_VAL_TYP_S_ENAMES((V))[(I)])
#define PVM_VAL_TYP_S_ETYPE(V,I) (PVM_VAL_TYP_S_ETYPES((V))[(I)])
#define PVM_VAL_TYP_O_UNIT(V) (PVM_VAL_TYP((V))->val.off.unit)
#define PVM_VAL_TYP_O_BASE_TYPE(V) (PVM_VAL_TYP((V))->val.off.base_type)

enum pvm_type_code
{
  PVM_TYPE_INTEGRAL,
  PVM_TYPE_STRING,
  PVM_TYPE_ARRAY,
  PVM_TYPE_STRUCT,
  PVM_TYPE_OFFSET,
  PVM_TYPE_MAP
};

struct pvm_type
{
  enum pvm_type_code code;

  union
  {
    struct
    {
      pvm_val size;
      pvm_val signed_p;
    } integral;

    struct
    {
      pvm_val nelem;
      pvm_val etype;
    } array;

    struct
    {
      pvm_val nelem;
      pvm_val *enames;
      pvm_val *etypes;
    } sct;

    struct
    {
      pvm_val base_type;
      pvm_val unit;
    } off;
  } val;
};

typedef struct pvm_type *pvm_type;

pvm_val pvm_make_integral_type (pvm_val size, pvm_val signed_p);
pvm_val pvm_make_string_type (void);
pvm_val pvm_make_map_type (void);
pvm_val pvm_make_array_type (pvm_val nelem, pvm_val type);
pvm_val pvm_make_struct_type (pvm_val nelem, pvm_val *enames, pvm_val *etypes);
pvm_val pvm_make_offset_type (pvm_val base_type, pvm_val unit);

void pvm_allocate_struct_attrs (pvm_val nelem, pvm_val **enames, pvm_val **etypes);

pvm_val pvm_dup_type (pvm_val type);
pvm_val pvm_type_equal (pvm_val t1, pvm_val t2);
pvm_val pvm_typeof (pvm_val val);

/* Mappings are also boxed.  */

#define PVM_VAL_MAP(V) (PVM_VAL_BOX_MAP (PVM_VAL_BOX ((V))))

#define PVM_VAL_MAP_TYPE(V) (PVM_VAL_MAP((V))->type)
#define PVM_VAL_MAP_OFFSET(V) (PVM_VAL_MAP((V))->offset)

struct pvm_map
{
  pvm_val type;
  pvm_val offset;
};

typedef struct pvm_map *pvm_map;

pvm_val pvm_make_map (pvm_val type, pvm_val offset);

/* Offsets are boxed values.  */

#define PVM_VAL_OFF(V) (PVM_VAL_BOX_OFF (PVM_VAL_BOX ((V))))

#define PVM_VAL_OFF_MAGNITUDE(V) (PVM_VAL_OFF((V))->magnitude)
#define PVM_VAL_OFF_UNIT(V) (PVM_VAL_OFF((V))->unit)
#define PVM_VAL_OFF_BASE_TYPE(V) (PVM_VAL_OFF((V))->base_type)

#define PVM_VAL_OFF_UNIT_BITS 0
#define PVM_VAL_OFF_UNIT_BYTES 1

struct pvm_off
{
  pvm_val base_type;
  pvm_val magnitude;
  pvm_val unit;
};

typedef struct pvm_off *pvm_off;

pvm_val pvm_make_offset (pvm_val base_type, pvm_val magnitude, pvm_val unit);

/* PVM_NULL is an invalid pvm_val.  */

#define PVM_NULL (0x7UL << 61)

/* Public interface.  */

#define PVM_IS_BYTE(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_BYTE)
#define PVM_IS_UBYTE(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_UBYTE)
#define PVM_IS_HALF(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_HALF)
#define PVM_IS_UHALF(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_UHALF)
#define PVM_IS_INT(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_INT)
#define PVM_IS_UINT(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_UINT)
#define PVM_IS_LONG(V)                                                  \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_LONG)
#define PVM_IS_ULONG(V)                                                 \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_ULONG)
#define PVM_IS_ULONG(V)                                                 \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_ULONG)
#define PVM_IS_STR(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_STR)
#define PVM_IS_ARR(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_ARR)
#define PVM_IS_SCT(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_SCT)
#define PVM_IS_TYP(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_TYP)
#define PVM_IS_MAP(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_MAP)
#define PVM_IS_OFF(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_OFF)


#define PVM_IS_NUMBER(V)                                        \
  (PVM_IS_BYTE(V) || PVM_IS_UBYTE(V)                            \
   || PVM_IS_HALF(V) || PVM_IS_UHALF(V)                         \
   || PVM_IS_INT(V) || PVM_IS_UINT(V)                           \
   || PVM_IS_LONG(V) || PVM_IS_ULONG(V))

#define PVM_VAL_NUMBER(V)                        \
  (PVM_IS_BYTE ((V)) ? PVM_VAL_BYTE ((V))        \
   : PVM_IS_UBYTE ((V)) ? PVM_VAL_UBYTE ((V))    \
   : PVM_IS_HALF ((V)) ? PVM_VAL_HALF ((V))      \
   : PVM_IS_UHALF ((V)) ? PVM_VAL_UHALF ((V))    \
   : PVM_IS_INT ((V)) ? PVM_VAL_INT ((V))        \
   : PVM_IS_UINT ((V)) ? PVM_VAL_UINT ((V))      \
   : PVM_IS_LONG ((V)) ? PVM_VAL_LONG ((V))      \
   : PVM_IS_ULONG ((V)) ? PVM_VAL_ULONG ((V))    \
   : 0)

/* The following enumeration contains every possible exit code
   resulting from the execution of a program in the PVM.  */

enum pvm_exit_code
  {
    PVM_EXIT_OK,
    PVM_EXIT_ERROR,
    PVM_EXIT_EDIVZ
  };

/* Note that the jitter-generated header should be included this late
   in the file because it uses some stuff defined above.  */
#include "pvm-vm.h"

/* A PVM program can be executed in the virtual machine at any time.
   The struct pvm_program is provided by Jitter, but we provide here
   an opaque type to be used by the PVM users.  */

typedef struct pvm_program *pvm_program;

/* Public functions.  */

void pvm_init (void);
void pvm_shutdown (void);
enum pvm_exit_code pvm_run (pvm_program prog, pvm_val *res);
const char *pvm_error (enum pvm_exit_code code);

/* Return the size of VAL in bytes.  */
size_t pvm_sizeof (pvm_val val);

/* For arrays and structs, return the number of elements stored.
   Return 1 otherwise.  */
size_t pvm_elemsof (pvm_val val);

/* Print a pvm_val to the given file descriptor. */
void pvm_print_val (FILE *out, pvm_val val, int base);

#endif /* ! PVM_H */
