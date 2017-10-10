/* pvm.h - Poke Virtual Machine.  Definitions.   */

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

#ifndef PVM_H
#define PVM_H

#include <config.h>
#include <string.h>
#include <stdint.h>
#include <xalloc.h>

#ifndef PVM_VM_H_
# include "pvm-vm.h"
#endif

/* The pvm_val opaque type implements values that are native to the
   poke virtual machine:

   - Signed 32-bit integers ("ints").
   - Unsigned 32-bit integers ("uints").
   - Signed 64-bit integers ("longs").
   - Unsigned 64-bit integers ("ulongs").
   - Strings.
   - Arrays.
   - Tuples.

   It is fundamental for pvm_val values to fit in 64-bit, in order to
   avoid expensive allocations and to also improve the performance of
   the virtual machine.  The 32-bit integers are unboxed.  64-bit
   integers, strings, arrays and tuples are boxed.  Both boxed and
   unboxed values are manipulated by the PVM users using the same API,
   defined below in this header file.  */

typedef uint64_t pvm_val;

/* The most-significative bits of pvm_val are reserved for the tag,
   which specifies the type of the value.  */

#define PVM_VAL_TAG(V) (((V) >> 61) & 0x3)

#define PVM_VAL_TAG_INT  0x0UL
#define PVM_VAL_TAG_UINT 0x1UL
#define PVM_VAL_TAG_LONG 0x2UL
#define PVM_VAL_TAG_ULONG 0x3UL
#define PVM_VAL_TAG_STR  0x4UL
#define PVM_VAL_TAG_ARR  0x5UL
#define PVM_VAL_TAG_TUP  0x6UL

/* 32-bit integers (both signed and unsigned) are encoded in the
   least-significative 32 bits of pvm_val.  */

#define PVM_VAL_INT(V) ((int32_t) ((V) & 0xffffffff))
#define PVM_VAL_UINT(V) ((uint32_t) ((V) & 0xffffffff))

pvm_val pvm_make_int (int32_t value);
pvm_val pvm_make_uint (uint32_t value);

/* A pointer to a boxed value is encoded in the least significative 61
   bits of pvm_val.  Note that this assumes all pointers are aligned
   to 8 bytes.  The allocator for the boxed values makes sure this is
   always the case.  */

#define PVM_VAL_PTR(V) ((uint64_t *)((V) << 3))

/* 64-bit integers (both signed and unsigned) are pointed by
   PVM_VAL_PTR.  */

#define PVM_VAL_LONG(V) ((int64_t) *PVM_VAL_PTR((V)))
#define PVM_VAL_ULONG(V) ((uint64_t) *PVM_VAL_PTR((V)))

pvm_val pvm_make_long (int64_t value);
pvm_val pvm_make_ulong (uint64_t value);

/* Strings are also pointed by PVM_VAL_PTR.  */

#define PVM_VAL_STR(V) ((char *) PVM_VAL_PTR((V)))

pvm_val pvm_make_string (const char *value);

/* Arrays are stored in pvm_array structs pointed by PVM_VAL_PTR.  */

#define PVM_VAL_ARR(V) ((struct pvm_array *) PVM_VAL_PTR((V)))
#define PVM_VAL_ARR_TYPE(V) (PVM_VAL_ARR(V)->type)
#define PVM_VAL_ARR_NELEMS(V) (PVM_VAL_ARR(V)->nelems)
#define PVM_VAL_ARR_ELEM(V,I) (PVM_VAL_ARR(V)->elems[(I)])

struct pvm_array
{
  char type;
  size_t nelems;
  pvm_val *elems;
};

typedef struct pvm_arr *pvm_arr;

pvm_val pvm_make_array (size_t nelems, int type);

/* Tuples are stored in pvm_tuple structs pointed by PVM_VAL_PTR.  */

#define PVM_VAL_TUP(V) ((pvm_tuple) PVM_VAL_PTR((V)))
#define PVM_VAL_TUP_NELEMS(V) (PVM_VAL_TUP(V)->nelems)
#define PVM_VAL_TUP_ELEM(V,I) (PVM_VAL_ARR(V)->elems[(I)])

struct pvm_tuple
{
  size_t nelems;
  pvm_val *elems;
};

typedef struct pvm_tuple *pvm_tuple;

pvm_val pvm_make_tuple (size_t nelems, int types[]);

/* PVM_NULL is an invalid pvm_val.  */

#define PVM_NULL (0x7UL << 61)

/* Clients must call `pvm_val_free' in a PVM value when it stops
   working with it.  */

void pvm_val_free (pvm_val val);

/* Public interface.  */

#define PVM_IS_INT(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_INT)
#define PVM_IS_UINT(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_UINT)
#define PVM_IS_LONG(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_LONG)
#define PVM_IS_ULONG(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_ULONG)
#define PVM_IS_STRING(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_STR)
#define PVM_IS_ARRAY(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_ARR)
#define PVM_IS_TUPLE(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_TUP)

#define PVM_IS_NUMBER(V)                                        \
  (PVM_IS_INT(V) || PVM_IS_UINT(V)                              \
   || PVM_IS_LONG(V) || PVM_IS_ULONG(V))

#define PVM_VAL_NUMBER(V)                       \
  (PVM_IS_INT ((V)) ? PVM_VAL_INT ((V))         \
   : PVM_IS_UINT ((V)) ? PVM_VAL_UINT ((V))     \
   : PVM_IS_LONG ((V)) ? PVM_VAL_LONG ((V))     \
   : PVM_IS_ULONG ((V)) ? PVM_VAL_ULONG ((V))   \
   : 0)

/* A PVM program can be executed in the virtual machine at any time.
   The struct pvm_program is provided by Jitter, but we provide here
   an opaque type to the PVM users.  */

typedef struct pvm_program *pvm_program;

/* The following enumeration contains every possible exit code
   resulting from the execution of a program in the PVM.  */

enum pvm_exit_code
  {
    PVM_EXIT_OK,
    PVM_EXIT_ERROR
  };

/* Public functions.  */

void pvm_init (void);
void pvm_shutdown (void);
enum pvm_exit_code pvm_run (pvm_program prog, pvm_val *res);

#endif /* ! PVM_H */
