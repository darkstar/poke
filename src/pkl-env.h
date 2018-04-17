/* pkl-env.h - Compile-time lexical environments for Poke.  */

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

#ifndef PKL_ENV_H
#define PKL_ENV_H

#include <config.h>
#include "pkl-ast.h"

/* The poke compiler maintains a data structure called the
   compile-time environment.  This structure keeps track of which
   variables will be at which position in which frames in the run-time
   environment when a particular variable-access operation is
   executed.  Conceptually, the compile-time environment is a list of
   "frames", each containing a list of declarations of variables,
   types and functions.

   The purpose of building this data structure is twofold:
   - When the parser finds a name, its meaning (particularly its type)
     can be found by searching the environment from the current frame
     out to the global one.

   - To aid in the determination of lexical addresses in variable
     references and assignments.  Lexical addresses are known
     at-compile time, and avoid the need of performing expensive
     lookups at run-time.

     The compile-time environment effectively mimics the corresponding
     run-time environments that will happen at run-time when a given
     lambda is created.

     For more details on this technique, see the Wizard Book (SICP)
     section 3.2, "The Environment model of Evaluation".  */

/* The environment uses several hash tables to speed access to
   declared entities.  */

#define HASH_TABLE_SIZE 1008
typedef pkl_ast_node pkl_hash[HASH_TABLE_SIZE];

/* An environment consists on a stack of frames, each frame containing
   a set of declarations, which in effect are PKL_AST_DECL nodes.

   There are no values bound to the entities being declared, as values
   are not generally available at compile-time.  However, the type
   information is always available at compile-time.  */

typedef struct pkl_env *pkl_env;  /* Struct defined in pkl-env.c */

/* Get an empty environment.  */

pkl_env pkl_env_new (void);

/* Destroy ENV, freeing all resources.  */

void pkl_env_free (pkl_env env);

/* Push a new frame to ENV and return the modified environment.  The
   new frame is empty.  */

pkl_env pkl_env_push_frame (pkl_env env);

/* Pop a frame from ENV and return the modified environment.  The
   contents of the popped frame are disposed.  */

pkl_env pkl_env_pop_frame (pkl_env env);

/* Register the type declaration DECL in the current frame under NAME.
   Return 1 if the type was properly registered.  Return 0 if there is
   already a type with the given name in the current frame.  */

int pkl_env_register_type (pkl_env env,
                           const char *name,
                           pkl_ast_node decl);

/* Register the variable or function declaration DECL in the current
   frame under NAME.  Return 1 if the variable or function was
   properly registered.  Return 0 if there is already a variable or
   function with the given name in the current frame.  */

int pkl_env_register_var (pkl_env env,
                          const char *name,
                          pkl_ast_node decl);

/* Search in the environment ENV for a declaration for the variable or
   function NAME, and put the lexical address of the first match in
   BACK and OVER.  Return the declaration node.

   BACK is the number of frames back the declaration is located.  It
   is 0-based.

   OVER indicates its position in the list of declarations in the
   resulting frame.  It is 0-based.  */

pkl_ast_node pkl_env_lookup_var (pkl_env env, const char *name,
                                 int *back, int *over);

/* Search in the environment ENV for a declaration for the type NAME,
   and return the type (not the declaration.)  Return a PKL_AST_TYPE
   in case the type is found.  Return NULL ortherwise.  */

pkl_ast_node pkl_env_lookup_type (pkl_env env, const char *name);

#endif /* !PKL_ENV_H  */
