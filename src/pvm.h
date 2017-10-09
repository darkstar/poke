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

typedef int64_t pvm_int;
typedef uint64_t pvm_uint;

enum pvm_stack_type
  {
    PVM_STACK_INT,
    PVM_STACK_STR,
  };

#define PVM_STACK_TYPE(S) ((S)->type)
#define PVM_STACK_INTEGER(S) ((S)->v.integer)
#define PVM_STACK_STRING(S) ((S)->v.string)

struct pvm_stack
{
  enum pvm_stack_type type;

  union
  {
    pvm_int integer;
    char *string;
  } v;
};

typedef struct pvm_stack *pvm_stack;
typedef struct pvm_program *pvm_program;

pvm_stack pvm_stack_new (void);
void pvm_stack_free (pvm_stack s);

void pvm_init (void);
void pvm_shutdown (void);

int pvm_execute (pvm_program prog);

/* Instructions.  */

void pvm_op_add (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_sub (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_mul (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_div (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_mod (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_ieq (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_ine (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_seq (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_sne (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_ilt (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_ile (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_igt (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_ige (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_slt (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_sle (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_sgt (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_sge (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_and (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_or (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_not (pvm_stack res, pvm_stack a);
void pvm_op_bxor (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_bior (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_band (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_bnot (pvm_stack res, pvm_stack a);
void pvm_op_bsl (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_bsr (pvm_stack res, pvm_stack a, pvm_stack b);
void pvm_op_neg (pvm_stack res, pvm_stack a);
void pvm_op_preinc (pvm_stack res, pvm_stack a);
void pvm_op_predec (pvm_stack res, pvm_stack a);
void pvm_op_sconc (pvm_stack res, pvm_stack a, pvm_stack b);

#endif /* ! PVM_H */
