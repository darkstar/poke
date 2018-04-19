/* pvm.h - Poke Virtual Machine.  */

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

#include <config.h>

#include <gc.h>
#include <xalloc.h>
#include <string.h>

#include "pvm.h"

/* The following struct defines a Poke Virtual Machine.  */

#define PVM_STATE_RESULT_VALUE(PVM)                     \
  ((PVM)->pvm_state.pvm_state_backing.result_value)
#define PVM_STATE_EXIT_CODE(PVM)                        \
  ((PVM)->pvm_state.pvm_state_backing.exit_code)
#define PVM_STATE_ENV(PVM)                              \
  ((PVM)->pvm_state.pvm_state_runtime.env)

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
  pvm pvm = xmalloc (sizeof (struct pvm));
  memset (pvm, 0, sizeof (struct pvm));
  
  /* Initialize the VM subsystem.  */
  pvm_initialize ();

  /* Start the Boehm garbage collector, which is used to manage PVM
     values and environments.  */
  GC_INIT ();

  /* Initialize the VM state.  */
  PVM_STATE_ENV (pvm) = pvm_env_new ();
  pvm_state_initialize (&pvm->pvm_state);

  return pvm;
}

pvm_env
pvm_get_env (pvm pvm)
{
  return PVM_STATE_ENV (pvm);
}

void
pvm_shutdown (pvm pvm)
{
  /* Finalize the VM state.  */
  pvm_state_finalize (&pvm->pvm_state);

  /* Make the garbage collector to collect the memory used by the
     PVM.  */
  GC_gcollect();

  /* Finalize the VM subsystem.  */
  pvm_finalize ();

  free (pvm);
}

enum pvm_exit_code
pvm_run (pvm pvm, pvm_program prog, pvm_val *res)
{
  PVM_STATE_RESULT_VALUE (pvm) = PVM_NULL;
  PVM_STATE_EXIT_CODE (pvm) = PVM_EXIT_OK;

  pvm_interpret (prog, &pvm->pvm_state);

  if (res != NULL)
    *res = PVM_STATE_RESULT_VALUE (pvm);

  return PVM_STATE_EXIT_CODE (pvm);
}

const char *
pvm_error (enum pvm_exit_code code)
{
  static char *pvm_error_strings[]
    = { "ok", "error", "division by zero" };

  return pvm_error_strings[code];
}

