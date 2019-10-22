/* pvm.h - Poke Virtual Machine.  */

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

#include <xalloc.h>
#include <string.h>
#include <assert.h>

#include "pvm.h"

/* The following struct defines a Poke Virtual Machine.  */

#define PVM_STATE_RESULT_VALUE(PVM)                     \
  ((PVM)->pvm_state.pvm_state_backing.result_value)
#define PVM_STATE_EXIT_CODE(PVM)                        \
  ((PVM)->pvm_state.pvm_state_backing.exit_code)
#define PVM_STATE_ENV(PVM)                              \
  ((PVM)->pvm_state.pvm_state_runtime.env)
#define PVM_STATE_ENDIAN(PVM)                           \
  ((PVM)->pvm_state.pvm_state_runtime.endian)
#define PVM_STATE_NENC(PVM)                             \
  ((PVM)->pvm_state.pvm_state_runtime.nenc)
#define PVM_STATE_PRETTY_PRINT(PVM)                     \
  ((PVM)->pvm_state.pvm_state_runtime.pretty_print)

struct pvm
{
  /* Note that the contents of the struct pvm_state are defined in the
     state-struct-backing-c and state-struct-runtime-c entries in
     pvm.jitter.  */
  struct pvm_state pvm_state;
};

pvm
pvm_init (void)
{
  pvm apvm = xmalloc (sizeof (struct pvm));
  memset (apvm, 0, sizeof (struct pvm));

  /* Initialize the memory allocation subsystem.  */
  pvm_alloc_initialize ();

  /* Initialize the VM subsystem.  */
  pvm_initialize ();

  /* Initialize the VM state.  */
  pvm_state_initialize (&apvm->pvm_state);

  /* Register GC roots.  */
  pvm_alloc_add_gc_roots (&PVM_STATE_ENV (apvm), 1);
  pvm_alloc_add_gc_roots
    (apvm->pvm_state.pvm_state_backing.jitter_stack_stack_backing.memory,
     apvm->pvm_state.pvm_state_backing.jitter_stack_stack_backing.element_no);
  pvm_alloc_add_gc_roots
    (apvm->pvm_state.pvm_state_backing.jitter_stack_returnstack_backing.memory,
     apvm->pvm_state.pvm_state_backing.jitter_stack_returnstack_backing.element_no);
  pvm_alloc_add_gc_roots
    (apvm->pvm_state.pvm_state_backing.jitter_stack_exceptionstack_backing.memory,
     apvm->pvm_state.pvm_state_backing.jitter_stack_exceptionstack_backing.element_no);

  /* Initialize the global environment.  Note we do this after
     registering GC roots, since we are allocating memory.  */
  PVM_STATE_ENV (apvm) = pvm_env_new ();

  return apvm;
}

pvm_env
pvm_get_env (pvm apvm)
{
  return PVM_STATE_ENV (apvm);
}

void
pvm_shutdown (pvm apvm)
{
  /* Deregister GC roots.  */
  pvm_alloc_remove_gc_roots (&PVM_STATE_ENV (apvm), 1);
  pvm_alloc_remove_gc_roots
    (apvm->pvm_state.pvm_state_backing.jitter_stack_stack_backing.memory,
     apvm->pvm_state.pvm_state_backing.jitter_stack_stack_backing.element_no);
  pvm_alloc_remove_gc_roots
    (apvm->pvm_state.pvm_state_backing.jitter_stack_returnstack_backing.memory,
     apvm->pvm_state.pvm_state_backing.jitter_stack_returnstack_backing.element_no);
  pvm_alloc_remove_gc_roots
    (apvm->pvm_state.pvm_state_backing.jitter_stack_exceptionstack_backing.memory,
     apvm->pvm_state.pvm_state_backing.jitter_stack_exceptionstack_backing.element_no);

  /* Finalize the VM state.  */
  pvm_state_finalize (&apvm->pvm_state);

  /* Finalize the VM subsystem.  */
  pvm_finalize ();

  free (apvm);

  /* Finalize the memory allocator.  */
  pvm_alloc_finalize ();
}

enum pvm_exit_code
pvm_run (pvm apvm, pvm_program prog, pvm_val *res)
{
  PVM_STATE_RESULT_VALUE (apvm) = PVM_NULL;
  PVM_STATE_EXIT_CODE (apvm) = PVM_EXIT_OK;

  pvm_interpret (prog, &apvm->pvm_state);

  if (res != NULL)
    *res = PVM_STATE_RESULT_VALUE (apvm);

  return PVM_STATE_EXIT_CODE (apvm);
}

enum ios_endian
pvm_endian (pvm apvm)
{
  return PVM_STATE_ENDIAN (apvm);
}

void
pvm_set_endian (pvm apvm, enum ios_endian endian)
{
  PVM_STATE_ENDIAN (apvm) = endian;
}

enum ios_nenc
pvm_nenc (pvm apvm)
{
  return PVM_STATE_NENC (apvm);
}

void
pvm_set_nenc (pvm apvm, enum ios_nenc nenc)
{
  PVM_STATE_NENC (apvm) = nenc;
}

int
pvm_pretty_print (pvm apvm)
{
  return PVM_STATE_PRETTY_PRINT (apvm);
}

void
pvm_set_pretty_print (pvm apvm, int flag)
{
  PVM_STATE_PRETTY_PRINT (apvm) = flag;
}

void
pvm_assert (int expression)
{
  assert (expression);
}
