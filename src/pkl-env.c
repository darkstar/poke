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
pkl_env_push_frame (pkl_env env, pkl_ast_node decls)
{
  pkl_env new = pkl_env_new ();

  PKL_ENV_DECLS (env) = ASTREF (decls);
  PKL_ENV_UP (env) = env;
  return new;
}

pkl_env
pkl_env_pop_frame (pkl_env env)
{
  pkl_ast_node decl, next;
  pkl_env up = PKL_ENV_UP (env);
  pkl_ast_node decls = PKL_ENV_DECLS (env);

  for (decl = decls; decl; decl = next)
    {
      next = PKL_AST_CHAIN2 (decl);
      pkl_ast_node_free (decl);
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
      pkl_ast_node decls = PKL_ENV_DECLS (env);
      pkl_ast_node decl, next;
      int num_decl = 0;
 
      for (decl = decls; decl; decl = next)
        {
          pkl_ast_node decl_name = PKL_AST_DECL_NAME (decl);

          next = PKL_AST_CHAIN2 (decl);

          if (strcmp (PKL_AST_IDENTIFIER_POINTER (identifier),
                      PKL_AST_IDENTIFIER_POINTER (decl_name)) == 0)
            {
              *back = num_frame;
              *over = num_decl;
              return decl;
            }

          num_decl++;
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
