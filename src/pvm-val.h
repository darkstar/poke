/* pvm-val.h - Values for the PVM.  */

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

#ifndef PVM_VAL_H
#define PVM_VAL_H

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <xalloc.h>

/* The pvm_val opaque type implements values that are native to the
   poke virtual machine:

   - Integers up to 32-bit.
   - Long integers wider than 32-bit up to 64-bit.
   - Strings.
   - Arrays.
   - Structs.
   - Offsets.

   It is fundamental for pvm_val values to fit in 64 bits, in order to
   avoid expensive allocations and to also improve the performance of
   the virtual machine.  */

typedef uint64_t pvm_val;

/* The least-significative bits of pvm_val are reserved for the tag,
   which specifies the type of the value.  */

#define PVM_VAL_TAG(V) ((V) & 0x7)

#define PVM_VAL_TAG_INT   0x0
#define PVM_VAL_TAG_UINT  0x1
#define PVM_VAL_TAG_LONG  0x2
#define PVM_VAL_TAG_ULONG 0x3
#define PVM_VAL_TAG_BIG   0x4
#define PVM_VAL_TAG_UBIG  0x5
#define PVM_VAL_TAG_BOX   0x6
/* Note that there is no tag 0x7.  It is used to implement PVM_NULL
   below.  */
/* Note also that the tags below are stored in the box, not in
   PVM_VAL_TAG.  See below in this file.  */
#define PVM_VAL_TAG_STR 0x8
#define PVM_VAL_TAG_OFF 0x9
#define PVM_VAL_TAG_ARR 0xa
#define PVM_VAL_TAG_SCT 0xb
#define PVM_VAL_TAG_TYP 0xc
#define PVM_VAL_TAG_MAP 0xd
#define PVM_VAL_TAG_CLS 0xe

/* Integers up to 32-bit are unboxed and encoded the following way:

              val                   bits  tag
              ---                   ----  ---
      vvvv vvvv vvvv vvvv xxxx xxxx bbbb bttt

   BITS+1 is the size of the integral value in bits, from 0 to 31.

   VAL is the value of the integer, sign- or zero-extended to 32 bits.
   Bits marked with `x' are unused and should be always 0.  */

#define PVM_VAL_INT_SIZE(V) (((int) (((V) >> 3) & 0x1f)) + 1)
#define PVM_VAL_INT(V) (((int32_t) ((V) >> 32))                \
                        << (32 - PVM_VAL_INT_SIZE ((V)))       \
                        >> (32 - PVM_VAL_INT_SIZE ((V))))

#define PVM_VAL_UINT_SIZE(V) (((int) (((V) >> 3) & 0x1f)) + 1)
#define PVM_VAL_UINT(V) (((uint32_t) ((V) >> 32)) \
                         & ((uint32_t) (~( ((~0ul) << ((PVM_VAL_UINT_SIZE ((V)))-1)) << 1 ))))


#define PVM_MAX_UINT(size) ((1U << (size)) - 1)

pvm_val pvm_make_int (int32_t value, int size);
pvm_val pvm_make_uint (uint32_t value, int size);

/* Long integers, wider than 32-bit and up to 64-bit, are boxed.  A
   pointer
                                             ttt
                                             ---
         pppp pppp pppp pppp pppp pppp pppp pttt

   points to a pair of 64-bit words:

                           val
                           ---
   [0]   vvvv vvvv vvvv vvvv vvvv vvvv vvvv vvvv
                                           bits         
                                           ----         
   [1]   xxxx xxxx xxxx xxxx xxxx xxxx xxbb bbbb

   BITS+1 is the size of the integral value in bits, from 0 to 63.

   VAL is the value of the integer, sign- or zero-extended to 64 bits.
   Bits marked with `x' are unused.  */

#define _PVM_VAL_LONG_ULONG_VAL(V) (((int64_t *) ((((uintptr_t) V) & ~0x7)))[0])
#define _PVM_VAL_LONG_ULONG_SIZE(V) ((int) (((int64_t *) ((((uintptr_t) V) & ~0x7)))[1]) + 1)

#define PVM_VAL_LONG_SIZE(V) (_PVM_VAL_LONG_ULONG_SIZE (V))
#define PVM_VAL_LONG(V) (_PVM_VAL_LONG_ULONG_VAL ((V))           \
                         << (64 - PVM_VAL_LONG_SIZE ((V)))      \
                         >> (64 - PVM_VAL_LONG_SIZE ((V))))

#define PVM_VAL_ULONG_SIZE(V) (_PVM_VAL_LONG_ULONG_SIZE (V))
#define PVM_VAL_ULONG(V) (_PVM_VAL_LONG_ULONG_VAL ((V))                 \
                          & ((uint64_t) (~( ((~0ull) << ((PVM_VAL_ULONG_SIZE ((V)))-1)) << 1 ))))

#define PVM_MAX_ULONG(size) ((1LU << (size)) - 1)

pvm_val pvm_make_long (int64_t value, int size);
pvm_val pvm_make_ulong (uint64_t value, int size);

/* Big integers, wider than 64-bit, are boxed.  They are implemented
   using the GNU mp library.  */

/* XXX: implement big integers.  */

/* A pointer to a boxed value is encoded in the most significative 61
   bits of pvm_val (32 bits for 32-bit hosts).  Note that this assumes
   all pointers are aligned to 8 bytes.  The allocator for the boxed
   values makes sure this is always the case.  */

#define PVM_VAL_BOX(V) ((pvm_val_box) ((((uintptr_t) V) & ~0x7)))

/* This constructor should be used in order to build boxes.  */

#define PVM_BOX(PTR) (((uint64_t) (uintptr_t) PTR) | PVM_VAL_TAG_BOX)

/* A box is a header for a boxed value, plus that value.  It is of
   type `pvm_val_box'.  */

#define PVM_VAL_BOX_TAG(B) ((B)->tag)
#define PVM_VAL_BOX_STR(B) ((B)->v.string)
#define PVM_VAL_BOX_ARR(B) ((B)->v.array)
#define PVM_VAL_BOX_SCT(B) ((B)->v.sct)
#define PVM_VAL_BOX_TYP(B) ((B)->v.type)
#define PVM_VAL_BOX_MAP(B) ((B)->v.map)
#define PVM_VAL_BOX_CLS(B) ((B)->v.cls)
#define PVM_VAL_BOX_OFF(B) ((B)->v.offset)

struct pvm_val_box
{
  uint8_t tag;
  union
  {
    char *string;
    struct pvm_array *array;
    struct pvm_struct *sct;
    struct pvm_type *type;
    struct pvm_map *map;
    struct pvm_off *offset;
    struct pvm_cls *cls;
  } v;
};

typedef struct pvm_val_box *pvm_val_box;

/* Strings are boxed.  */

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

/* Closures are also boxed.  */

#define PVM_VAL_CLS(V) (PVM_VAL_BOX_CLS (PVM_VAL_BOX ((V))))

#define PVM_VAL_CLS_PROGRAM(V) (PVM_VAL_CLS((V))->program)
#define PVM_VAL_CLS_ENTRY_POINT(V) (PVM_VAL_CLS((V))->entry_point)
#define PVM_VAL_CLS_ENV(V) (PVM_VAL_CLS((V))->env)

struct pvm_cls
{
  /* Note we have to use explicit pointers here due to the include
     mess induced by jitter's combined header files :/ */
  struct jitter_program *program;
  const void *entry_point;
  struct pvm_env *env;
};

typedef struct pvm_cls *pvm_cls;

pvm_val pvm_make_cls (struct jitter_program *program);

/* Offsets are boxed values.  */

#define PVM_VAL_OFF(V) (PVM_VAL_BOX_OFF (PVM_VAL_BOX ((V))))

#define PVM_VAL_OFF_MAGNITUDE(V) (PVM_VAL_OFF((V))->magnitude)
#define PVM_VAL_OFF_UNIT(V) (PVM_VAL_OFF((V))->unit)
#define PVM_VAL_OFF_BASE_TYPE(V) (PVM_VAL_OFF((V))->base_type)

#define PVM_VAL_OFF_UNIT_BITS 1
#define PVM_VAL_OFF_UNIT_NIBBLES 4
#define PVM_VAL_OFF_UNIT_BYTES (2 * PVM_VAL_OFF_UNIT_NIBBLES)
#define PVM_VAL_OFF_UNIT_KILOBITS (1024 * PVM_VAL_OFF_UNIT_BITS)
#define PVM_VAL_OFF_UNIT_KILOBYTES (1024 * PVM_VAL_OFF_UNIT_BYTES)
#define PVM_VAL_OFF_UNIT_MEGABITS (1024 * PVM_VAL_OFF_UNIT_KILOBITS)
#define PVM_VAL_OFF_UNIT_MEGABYTES (1024 * PVM_VAL_OFF_UNIT_KILOBYTES)
#define PVM_VAL_OFF_UNIT_GIGABITS (1024 * PVM_VAL_OFF_UNIT_MEGABITS)

struct pvm_off
{
  pvm_val base_type;
  pvm_val magnitude;
  pvm_val unit;
};

typedef struct pvm_off *pvm_off;

pvm_val pvm_make_offset (pvm_val magnitude, pvm_val unit);

/* PVM_NULL is an invalid pvm_val.  */

#define PVM_NULL (0x7ULL << 61)

/* Public interface.  */

#define PVM_IS_INT(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_INT)
#define PVM_IS_UINT(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_UINT)
#define PVM_IS_LONG(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_LONG)
#define PVM_IS_ULONG(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_ULONG)
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
#define PVM_IS_CLS(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_CLS)
#define PVM_IS_OFF(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_OFF)


#define PVM_IS_INTEGRAL(V)                                      \
  (PVM_IS_INT (V) || PVM_IS_UINT (V)                            \
   || PVM_IS_LONG (V) || PVM_IS_ULONG (V))

#define PVM_VAL_INTEGRAL(V)                      \
  (PVM_IS_INT ((V)) ? PVM_VAL_INT ((V))          \
   : PVM_IS_UINT ((V)) ? PVM_VAL_UINT ((V))      \
   : PVM_IS_LONG ((V)) ? PVM_VAL_LONG ((V))      \
   : PVM_IS_ULONG ((V)) ? PVM_VAL_ULONG ((V))    \
   : 0)

/* Return an offset with the size of VAL.  */
pvm_val pvm_sizeof (pvm_val val);

/* For arrays and structs, return the number of elements stored.
   Return 1 otherwise.  */
pvm_val pvm_elemsof (pvm_val val);

/* Print a pvm_val to the given file descriptor. */
void pvm_print_val (FILE *out, pvm_val val, int base);

#endif /* !PVM_VAL_H */
