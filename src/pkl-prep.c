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

   - When it finds a type name in a PKL_AST_IDENTIFIER node, it
     searches for its declaration in the current environment and
     replaces the identifier with a PKL_AST_TYPE node.  If a
     declaration is not found for the type a compile-time error is
     raised.

   - When it finds a variable name in a PKL_AST_IDENTIFIER node, it
     searches for its declaration in the current environment and
     replaces the identifier with a PKL_AST_VAR_REF node containing
     its lexical address.  If a declaration is not found for the
     variable a compile-time error is raised.

   - When it finds a function name in a PKL_AST_IDENTIFIER node as
     part of a funcall, it searches for its declaration in the current
     environment and replaces the identifier with a PKL_AST_FUN_REF
     node containing its lexical address.  If a declaration is not
     found for the function a compile-time error is raised.

   After this phase every type, variable and function reference are
   resolved.

   The global compile-time environment is given in the `env' field of
   the payload.  It should not be NULL.  */
