/* pkl-prep.c - Environment creation phase for the poke compiler.  */

/* Copyright (C) 2018 Jose E. Marchesi  */

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

#include "pkl.h"
#include "pkl-ast.h"
#include "pkl-pass.h"
#include "pkl-prep.h"

/* This file implements a compiler phase that, given a global
   compile-time environment:

   - Expands it with global declarations found right under the PROGRAM
     node.

   - When it finds a local declaration (type, variable or function) it
     pushes a new frame to the environment containing the declaration.
     When the scope of the local declaration ends, it pops the frame.

   - When it finds a named type (PKL_AST_TYPE with a name), it
     searches for its declaration in the current environment and
     replaces the named type with a complete PKL_AST_TYPE node.  If a
     declaration is not found for the named type a compile-time error
     is raised.

   - When it finds a named variable (PKL_AST_VAR with a name), it
     searches for its declaration in the current environment and turns
     the named variable into a lexical variable, with a lexical
     address.  If a declaration is not found for the named variable a
     compile-time error is raised.  The variable may be a function.

   After this phase every type and variable reference are resolved.

   The global compile-time environment is given in the `env' field of
   the payload.  It should not be NULL.  */
