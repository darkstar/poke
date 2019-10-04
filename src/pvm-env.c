/* pvm-env.c - Run-time environment for Poke.  */

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

#include <assert.h>
#include <string.h>

#include "pvm-val.h"
#include "pvm-env.h"
#include "pvm-alloc.h"

/* The variables in each frame are organized in an array that can be
   efficiently accessed using OVER.

   UP is a link to the immediately enclosing frame.  This is NULL for
   the top-level frame.  */

#define MAX_VARS 1024

struct pvm_env
{
  int num_vars;
  pvm_val vars[MAX_VARS];

  struct pvm_env *up;
};


/* The following functions are documentd in pvm-env.h */

pvm_env
pvm_env_new ()
{
  int i;
  pvm_env env = pvm_alloc (sizeof (struct pvm_env));

  memset (env, 0, sizeof (struct pvm_env));

  for (i = 0; i < MAX_VARS; ++i)
    env->vars[i] = PVM_NULL;

  return env;
}

pvm_env
pvm_env_push_frame (pvm_env env)
{
  pvm_env frame = pvm_env_new ();

  frame->up = env;
  return frame;
}

pvm_env
pvm_env_pop_frame (pvm_env env)
{
  assert (env->up != NULL);
  return env->up;
}

void
pvm_env_register (pvm_env env, pvm_val val)
{
  assert (env->num_vars < MAX_VARS);
  env->vars[env->num_vars++] = val;
}

/* Note the function attribute to assure the functions are compiled
   with tail-recursion optimization.  */

pvm_val __attribute__((optimize ("optimize-sibling-calls")))
pvm_env_lookup (pvm_env env, int back, int over)
{
  if (back == 0)
    return env->vars[over];
  else
    return pvm_env_lookup (env->up, back - 1, over);
}

void __attribute__((optimize ("optimize-sibling-calls")))
pvm_env_set_var (pvm_env env, int back, int over, pvm_val val)
{
  if (back == 0)
    env->vars[over] = val;
  else
    pvm_env_set_var (env->up, back - 1, over, val);
}

int
pvm_env_toplevel_p (pvm_env env)
{
  return (env->up == NULL);
}
