/* pkl-env.h - Compile-time environments for Poke.  */

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
   "frames", each containing a list of variables.

   The purpose of building this data structure is to aid in the
   determination of lexical addresses in variable references and
   assignments.  Lexical addresses are known at-compile time, and
   avoid the need of performing expensive lookups at run-time.

   The compile-time environment effectively mimics the corresponding
   run-time environments that will happen at run-time when a given
   lambda is created.

   For details on this technique, see the Wizard Book (SICP) section
   3.2, "The Environment model of Evaluation".  */

/* Each frame contains a list of variables, which in effect are
   PKL_AST_IDENTIFIER nodes.  (XXX: we also need types!.)

   There are no values bound to these variables, as values are not
   generally available at compile-time.

   VARS is a pointer to the first of such identifiers, or NULL if the
   frame is empty.  The identifier nodes are chained through CHAIN2.
   Note that a given variable can only be linked by one frame.

   UP is a link to the immediately enclosing frame.  This is NULL for
   the top-level frame.  */

#define PKL_FRAME_VARS(F) ((F)->vars)
#define PKL_FRAME_UP(F) ((F)->up)

struct pkl_frame
{
  union pkl_ast_node *vars;
  struct pkl_env *up;
};

typedef struct pkl_env *pkl_env;

#endif /* !PKL_ENV_H  */
