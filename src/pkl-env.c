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

/* The declarations are organized in a set of hash tables:
   TYPES_TABLE contains type declarations.
   VARS_TABLE contains both variable and function declarations.

   Note that the reason we are storing variables and functions in a
   single table is because they share the same namespace.

   The declaration nodes are chained in the hash tables through
   CHAIN2.

   UP is a link to the immediately enclosing frame.  This is NULL for
   the top-level frame.  */

#define PKL_ENV_UP(F) ((F)->up)

struct pkl_env
{
  pkl_hash types_table;
  pkl_hash vars_table;
  
  struct pkl_env *up;
};

/* The hash tables above are handled using the following
   functions.  */

static int
hash_string (const char *name)
{
  size_t len;
  int hash;
  int i;

  len = strlen (name);
  hash = len;
  for (i = 0; i < len; i++)
    hash = ((hash * 613) + (unsigned)(name[i]));

#define HASHBITS 30
  hash &= (1 << HASHBITS) - 1;
  hash %= HASH_TABLE_SIZE;
#undef HASHBITS

  return hash;
}

static void
free_hash_table (pkl_hash *hash_table)
{
  size_t i;
  pkl_ast_node t, n;

  for (i = 0; i < HASH_TABLE_SIZE; ++i)
    if ((*hash_table)[i])
      for (t = (*hash_table)[i]; t; t = n)
        {
          n = PKL_AST_CHAIN2 (t);
          pkl_ast_node_free (t);
        }
}

/* The following functions are documented in pkl-env.h.  */

pkl_env
pkl_env_new ()
{
  pkl_env env = xmalloc (sizeof (struct pkl_env));

  memset (env, 0, sizeof (struct pkl_env));
  return env;
}

void
pkl_env_free (pkl_env env)
{
  if (env)
    {
      pkl_env_free (PKL_ENV_UP (env));

      free_hash_table (&env->types_table);
      free_hash_table (&env->vars_table);
      free (env);
    }
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
