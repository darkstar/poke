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

/* The pvm_val opaque type implements the boxed values that are
   native to the poke virtual machine.  */

typedef int64_t pvm_int;
typedef uint64_t pvm_uint;

enum pvm_val_type
  {
    PVM_VAL_INT,
    PVM_VAL_STR,
  };

#define PVM_VAL_TYPE(S) ((S)->type)
#define PVM_VAL_INTEGER(S) ((S)->v.integer)
#define PVM_VAL_STRING(S) ((S)->v.string)

struct pvm_val
{
  enum pvm_val_type type;

  union
  {
    pvm_int integer;
    char *string;
  } v;
};

typedef struct pvm_val *pvm_val;

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
enum pvm_exit_code pvm_execute (pvm_program prog, pvm_val *res);
pvm_val pvm_val_new (void);
void pvm_val_free (pvm_val s);

#endif /* ! PVM_H */
