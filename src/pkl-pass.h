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
#include <setjmp.h>
#include "pkl-ast.h"

/* A `pass' is a complete run over a given AST.  A `phase' is an
   analysis or a transformation performed over a subset of the nodes
   in the AST.  One pass may integrate several phases.

   Implementing a phase involves defining a struct pkl_phase variable
   and filling it up.  A pkl_hase struct contains:

   - CODE_DF_HANDLERS is a table indexed by node codes, which must be
     values in the `pkl_ast_code' enumeration defined in pkl-ast.h.
     For example, PKL_AST_ARRAY.  It maps codes to
     pkl_phase_handler_fn functions.

   - OP_DF_HANDLERS is a table indexed by operator codes, which must
     be values in the `pkl_ast_op' enumeration defined in pkl-ast.h.
     This enumeration is in turn generated from the operators defined
     in pkl-ops.def.  For example, PKL_AST_OP_ADD.  It maps codes to
     pkl_phase_handler_fn functions.

   - TYPE_DF_HANDLERS a table is indexed by type codes, which must be
     values in the `pkl_ast_type_code' enumeration defined in
     pkl-ast.h. For example, PKL_TYPE_STRING.  It maps codes to
     pkl_phase_handler_fn functions.

   - DEFAULT_DF_HANDLER is invoked for every node.  It can be NULL,
     meaning no default handler is invoked at all.  It maps codes to
     pkl_phase_handler_fn functions.

   The handlers defined in the tables above are invoked while
   traversing the AST in depth-first order.  Additional tables exist
   to define handlers that are executed while traversing the AST in
   breadth-first order:

   - CODE_BF_HANDLERS
   - OP_BF_HANDLERS
   - TYPE_BF_HANDLERS
   - DEFAULT_BF_HANDLERS

   A given phase can define handlers of both types: DF and BF.

   Note that if a given node class falls in several categories as
   implemented in the handlers tables, the more general handler will
   be executed first, followed by the more particular handlers.  For
   example, for a PKL_AST_TYPE node with cype code PKL_TYPE_ARRAY, the
   handler in `type_handlers' will be invoked first, followed by the
   handler in `code_handlers'.

   If the default handler is NULL and no other handler is executed,
   then no action is performed on a node other than traversing it.  */

typedef pkl_ast_node (*pkl_phase_handler_fn) (jmp_buf toplevel,
                                              pkl_ast_node ast,
                                              void *data);

struct pkl_phase
{
  pkl_phase_handler_fn default_df_handler;
  pkl_phase_handler_fn code_df_handlers[PKL_AST_LAST];
  pkl_phase_handler_fn op_df_handlers[PKL_AST_OP_LAST];
  pkl_phase_handler_fn type_df_handlers[PKL_TYPE_NOTYPE];

  pkl_phase_handler_fn default_bf_handler;
  pkl_phase_handler_fn code_bf_handlers[PKL_AST_LAST];
  pkl_phase_handler_fn op_bf_handlers[PKL_AST_OP_LAST];
  pkl_phase_handler_fn type_bf_handlers[PKL_TYPE_NOTYPE];
};

typedef struct pkl_phase *pkl_phase;

/* The following macros are to be used in node handlers.
   
   PKL_PASS_EXIT can be used in order to interrupt the execution of
   the compiler pass.

   PKL_PASS_ERROR can be used in order to interrupt the execution of
   the compiler pass, making `pkl_do_pass' to return an error code.  */

#define PKL_PASS_EXIT do { longjmp (toplevel, 1); } while (0)
#define PKL_PASS_ERROR do { longjmp (toplevel, 2); } while (0)

/* Traverse AST in a depth-first fashion, applying the provided phases
   (or transformations) in sequence to each AST node.  USER is a
   pointer that will be passed to the node handlers defined in the
   array PHASES, which should be NULL terminated.

   Return 0 if some error occurred during the pass execution.  Return
   1 otherwise.  */

int pkl_do_pass (pkl_ast ast, void *data, struct pkl_phase *phases[]);

#endif /* PKL_PASS_H  */
