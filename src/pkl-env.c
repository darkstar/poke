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

#define HASH_TABLE_SIZE 1008
typedef pkl_ast_node pkl_hash[HASH_TABLE_SIZE];

struct pkl_env
{
  pkl_hash types_table;
  pkl_hash vars_table;

  int num_types;
  int num_vars;
  
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
free_hash_table (pkl_hash hash_table)
{
  size_t i;
  pkl_ast_node t, n;

  for (i = 0; i < HASH_TABLE_SIZE; ++i)
    if (hash_table[i])
      for (t = hash_table[i]; t; t = n)
        {
          n = PKL_AST_CHAIN2 (t);
          pkl_ast_node_free (t);
        }
}

pkl_ast_node
get_registered (pkl_hash hash_table, const char *name)
{
  pkl_ast_node t;
  int hash;

  hash = hash_string (name);
  for (t = hash_table[hash]; t != NULL; t = PKL_AST_CHAIN2 (t))
    {
      pkl_ast_node t_name = PKL_AST_DECL_NAME (t);
      
      if (strcmp (PKL_AST_IDENTIFIER_POINTER (t_name),
                  name) == 0)
        return t;
    }

  return NULL;
}

static int
register_decl (pkl_hash hash_table,
               const char *name,
               pkl_ast_node decl)
{
  int hash;
  
  if (get_registered (hash_table, name) != NULL)
    /* Already registered.  */
    return 0;

  /* Add the declaration to the hash table.  */
  hash = hash_string (name);
  PKL_AST_CHAIN2 (decl) = hash_table[hash];
  hash_table[hash] = ASTREF (decl);

  return 1;
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
      pkl_env_free (env->up);

      free_hash_table (env->types_table);
      free_hash_table (env->vars_table);
      free (env);
    }
}

pkl_env
pkl_env_push_frame (pkl_env env)
{
  pkl_env frame = pkl_env_new ();

  frame->up = env;
  return frame;
}

pkl_env
pkl_env_pop_frame (pkl_env env)
{
  pkl_env up = NULL;

  if (env)
    up = env->up;

  pkl_env_free (env);
  return up;
}

int
pkl_env_register_type (pkl_env env,
                       const char *name,
                       pkl_ast_node decl)
{
  if (register_decl (env->types_table, name, decl))
    {
      PKL_AST_DECL_ORDER (decl) = env->num_types++;
      return 1;
    }

  return 0;
}

int
pkl_env_register_var (pkl_env env,
                      const char *name,
                      pkl_ast_node decl)
{
  if (register_decl (env->vars_table, name, decl))
    {
      PKL_AST_DECL_ORDER (decl) = env->num_vars++;
      return 1;
    }

  return 0;
}

static pkl_ast_node
pkl_env_lookup_var_1 (pkl_env env, const char *name,
                      int *back, int *over, int num_frame)
{
  if (env == NULL)
    return NULL;
  else
    {
      pkl_ast_node decl = get_registered (env->vars_table, name);
      if (decl)
        {
          *back = num_frame;
          *over = PKL_AST_DECL_ORDER (decl);
          return decl;
        }
    }

  return pkl_env_lookup_var_1 (env->up, name, back, over,
                               num_frame + 1);
}

pkl_ast_node
pkl_env_lookup_var (pkl_env env, const char *name,
                    int *back, int *over)
{
  return pkl_env_lookup_var_1 (env, name, back, over, 0);
}

pkl_ast_node
pkl_env_lookup_type (pkl_env env, const char *name)
{
  if (env == NULL)
    return NULL;
  else
    {
      pkl_ast_node decl = get_registered (env->types_table, name);
      if (decl)
        return PKL_AST_DECL_INITIAL (decl);
    }

  return pkl_env_lookup_type (env->up, name);
}

int
pkl_env_toplevel_p (pkl_env env)
{
  assert (env);
  return env->up == NULL;
}
