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

/* The pvm_stack opaque type implements the boxed values that are
   native to the poke virtual machine.  */

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

/* The struct pvm_program is defined by Jitter.  Provide a convenient
   opaque type to the PVM users.  */

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
int pvm_execute (pvm_program prog);
pvm_stack pvm_stack_new (void);
void pvm_stack_free (pvm_stack s);

enum pvm_exit_code pvm_exit_code (void);
pvm_stack pvm_result (void);

#endif /* ! PVM_H */
