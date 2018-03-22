/* pkl-pass.h - Support for compiler passes.  */

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

#ifndef PKL_PASS_H
#define PKL_PASS_H

#include <config.h>

#include <assert.h>
#include <stdarg.h>
#include "pkl-ast.h"

/* A `pass' is a complete run over a given AST.  A `phase' is an
   analysis or a transformation performed over a subset of the nodes
   in the AST.  One pass may integrate several phases.

   Implementing a phase involves defining a struct pkl_phase variable
   and filling it up.  Unlike Gaul, a pkl_hase contains four tables:

   - A table indexed by node codes, which must be values in the
     `pkl_ast_code' enumeration defined in pkl-ast.h.  For example,
     PKL_AST_ARRAY.

   - A table indexed by operator codes, which must be values in the
     `pkl_ast_op' enumeration defined in pkl-ast.h.  This enumeration
     is in turn generated from the operators defined in pkl-ops.def.
     For example, PKL_AST_OP_ADD.

   - A table indexed by type codes, which must be values in the
     `pkl_ast_type_code' enumeration defined in pkl-ast.h. For
     example, PKL_TYPE_STRING.

   - A table indexed by integral type codes, which must be values in
     the `pkl_ast_integral_type_code' enumeration defined in
     pkl-ast.h.  This enumeration is in turn generated from the types
     define din pkl-types.def.  For example, PKL_TYPE_UINT32.

   These tables map codes to node handlers.  Each node handler must
   follow the function prototype declared below.

   Note that it is possible to define a handler for PKL_AST_EXP nodes,
   and also handlers for specific PKL_AST_OP_* operations; the more
   general handler will be executed first, followed by the more
   particular handlers.  The same applies to handlers for specific
   integral types and PKL_TYPE_INTEGRAL, and for handlers for specific
   type codesa nd PKL_AST_TYPE.

   If there is not a handler registered for a given node class, then
   DEFAULT_HANDLER is invoked if it is defined.  If the default
   handler is NULL then no action is performed on the node other than
   traversing it.  */

typedef pkl_ast_node (*pkl_phase_handler_fn) (pkl_ast_node ast,
                                              void *data);

struct pkl_phase
{
  pkl_phase_handler_fn default_handler;

  pkl_phase_handler_fn code_handlers[PKL_AST_LAST];
  pkl_phase_handler_fn op_handlers[PKL_AST_OP_LAST];
  pkl_phase_handler_fn type_handlers[PKL_TYPE_NOTYPE];
  pkl_phase_handler_fn op_integral_types[PKL_TYPE_LAST_INTEGRAL];
};

typedef struct pkl_phase *pkl_phase;

/* Traverse AST in a depth-first fashion, applying the provided phases
   (or transformations) in sequence to each AST node.  USER is a
   pointer that will be passed to the node handlers defined as a list
   of PHASEs, terminated by a NULL.  Return the traversed AST.
   
   XXX: error handling.  */

pkl_ast pkl_do_pass (pkl_ast ast, void *data, /* pkl_phase phase */ ...);

#endif /* PKL_PASS_H  */
