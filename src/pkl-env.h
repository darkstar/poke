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

/* Make a new frame for the variable list DECLS and stack it in the
   environment ENV.  */

pkl_env pkl_env_push_frame (pkl_env env, pkl_ast_node decls);

/* Pop a frame from environment ENV and dispose it, then return the
   resulting environment.  Return NULL if ENV only contains the
   top-level frame.  */

pkl_env pkl_env_pop_frame (pkl_env env);

/* Search in the environment ENV for a declaration for IDENTIFIER, and
   put the lexical address of the first match in BACK and OVER.

   BACK is the number of frames back the declaration is located.  It
   is 0-based.

   OVER indicates its position in the list of declarations in the
   resulting frame.  It is 0-based.

   Return the result declaration node if a declaration was found for
   the free variable IDENTIFIER.  NULL otherwise.  The main purpose of
   returning the declaration is for the client to have access to the
   associated type.  */

pkl_ast_node pkl_env_lookup (pkl_env env, pkl_ast_node identifier,
                             int *back, int *over);

#endif /* !PKL_ENV_H  */
