/* pkl-env.c - Compile-time lexical environments for Poke.  */

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

#include <stdlib.h>
#include <xalloc.h>
#include <string.h>

#include "pkl-env.h"

pkl_env
pkl_env_new ()
{
  pkl_env env = xmalloc (sizeof (struct pkl_env));

  memset (env, 0, sizeof (struct pkl_env));
  return env;
}

pkl_env
pkl_env_push_frame (pkl_env env, pkl_ast_node vars)
{
  pkl_env new = pkl_env_new ();

  PKL_ENV_VARS (env) = ASTREF (vars);
  PKL_ENV_UP (env) = env;
  return new;
}

pkl_env
pkl_env_pop_frame (pkl_env env)
{
  pkl_ast_node vars, bind, next;
  pkl_env up = PKL_ENV_UP (env);

  vars = PKL_ENV_VARS (env);
  for (bind = vars; bind; bind = next)
    {
      next = PKL_AST_CHAIN2 (bind);
      pkl_ast_node_free (bind);
    }
  
  free (env);
  return up;
}

static pkl_ast_node
pkl_env_lookup_1 (pkl_env env, pkl_ast_node identifier,
                  int *back, int *over, int num_frame)
{
  if (env == NULL)
    return NULL;
  else
    {
      pkl_ast_node vars = PKL_ENV_VARS (env);
      pkl_ast_node bind, next;
      int num_var = 0;
 
      for (bind = vars; bind; bind = next)
        {
          next = PKL_AST_CHAIN2 (bind);

          if (strcmp (PKL_AST_IDENTIFIER_POINTER (identifier),
                      PKL_AST_IDENTIFIER_POINTER (bind)) == 0)
            {
              *back = num_frame;
              *over = num_var;
              return bind;
            }

          num_var++;
        }
    }      

  return pkl_env_lookup_1 (PKL_ENV_UP (env), identifier,
                           back, over, num_frame + 1);
}

pkl_ast_node
pkl_env_lookup (pkl_env env, pkl_ast_node identifier,
                int *back, int *over)
{
  return pkl_env_lookup_1 (env, identifier, back, over, 0);
}
