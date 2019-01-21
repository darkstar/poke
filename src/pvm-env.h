/* pvm-env.h - Run-time environment for Poke.  */

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

#ifndef PVM_ENV_H
#define PVM_ENV_H

#include <config.h>
#include <assert.h>

#include "pvm-val.h"

/* The poke virtual machine (PVM) maintains a data structure called
   the run-time environment.  This structure contains run-time frames,
   which in turn store the variables of PVM programs.

   A set of PVM instructions are provided to allow programs to
   manipulate the run-time environment.  These are implemented in
   pvm.jitter in the "Environment instructions" section, and
   summarized here:

   `pushf' pushes a new frame to the run-time environment.  This is
   used when entering a new environment, such as a function.

   `popf' pops a frame from the run-time environment.  After this
   happens, if no references are left to the popped frame, both the
   frame and the variables stored in the frame are eventually
   garbage-collected.

   `popvar' pops the value at the top of the main stack and creates a
   new variable in the run-time environment to hold that value.

   `pushvar BACK, OVER' retrieves the value of a variable from the
   run-time environment and pushes it in the main stack.  BACK is the
   number of frames to traverse and OVER is the order of the variable
   in its containing frame.  The BACK,OVER pairs (also known as
   lexical addresses) are produced by the compiler; see `pkl-env.h'
   for a description of the compile-time environment.

   This header file provides the prototypes for the functions used to
   implement the above-mentioned PVM instructions.  */

typedef struct pvm_env *pvm_env;  /* Struct defined in pvm-env.c */

/* Create a new run-time environment, containing an empty top-level
   frame, and return it.  */

pvm_env pvm_env_new (void);

/* Push a new empty frame to ENV and return the modified run-time
   environment.  */

/* XXX: allow to specify the number of variables in
   pvm_env_push_frame, with a special value for `variable length' for
   the toplevel */

pvm_env pvm_env_push_frame (pvm_env env);

/* Pop a frame from ENV and return the modified run-time environment.
   The popped frame will eventually be garbage-collected if there are
   no more references to it.  Trying to pop the top-level frame is an
   error.  */

pvm_env pvm_env_pop_frame (pvm_env env);

/* Create a new variable in the current frame of ENV, whose value is
   VAL.  */

void pvm_env_register (pvm_env env, pvm_val val);

/* Return the value for the variable occupying the position BACK, OVER
   in the the run-time environment ENV.  Return PVM_NULL if the
   variable is not found.  */

pvm_val pvm_env_lookup (pvm_env env, int back, int over);

/* Set the value of the variable occupying the position BACK, OVER in
   the run-time environment ENV to VAL.  */

void pvm_env_set_var (pvm_env env, int back, int over, pvm_val val);

/* Return 1 if the given run-time environment ENV contains only one
   frame.  Return 0 otherwise.  */

int pvm_env_toplevel_p (pvm_env env);

#endif /* !PVM_ENV_H */
