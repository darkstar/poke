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

   The handlers defined in the tables above are invoked while
   traversing the AST in depth-first order.  Additional tables exist
   to define handlers that are executed while traversing the AST in
   breadth-first order:

   - CODE_BF_HANDLERS
   - OP_BF_HANDLERS
   - TYPE_BF_HANDLERS

   A given phase can define handlers of both types: DF and BF.

   There are three additional handlers that the user can install:

   - DEFAULT_BF_HANDLER is invoked for every node in the AST, after the
     more particular ones, in breath-first order.

   - DEFAULT_DF_HANDLER is invoked for every node in the SAT, after
     the more particular ones, in depth-first order.

   - ELSE_HANDLER, if not NULL, is invoked for every node for which no
     other handler (BF or DF) has been invoked.

   Note that if a given node class falls in several categories as
   implemented in the handlers tables, the more general handler will
   be executed first, followed by the more particular handlers.  For
   example, for a PKL_AST_TYPE node with type code PKL_TYPE_ARRAY, the
   handler in `type_handlers' will be invoked first, followed by the
   handler in `code_handlers'.

   If the `else' handler is NULL and no other handler is executed,
   then no action is performed on a node other than traversing it.  */

typedef pkl_ast_node (*pkl_phase_handler_fn) (jmp_buf toplevel,
                                              pkl_ast ast,
                                              pkl_ast_node node,
                                              void *payload,
                                              int *restart,
                                              size_t child_pos);

struct pkl_phase
{
  pkl_phase_handler_fn else_handler;

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

/* The following macros should be used in order to register handlers
   in a `struct pkl_phase'.  This allows changing the structure layout
   without impacting the phase definitions.  */

#define PKL_PHASE_ELSE_HANDLER(handler)      \
  .else_handler = handler

#define PKL_PHASE_BF_DEFAULT_HANDLER(handler)   \
  .default_bf_handler = handler
#define PKL_PHASE_DF_DEFAULT_HANDLER(handler)   \
  .default_df_handler = handler

#define PKL_PHASE_BF_HANDLER(code, handler)     \
  .code_bf_handlers[code] = handler
#define PKL_PHASE_DF_HANDLER(code, handler)     \
  .code_df_handlers[code] = handler

#define PKL_PHASE_BF_OP_HANDLER(code,handler)   \
  .op_bf_handlers[code] = handler
#define PKL_PHASE_DF_OP_HANDLER(code,handler)   \
  .op_df_handlers[code] = handler

#define PKL_PHASE_BF_TYPE_HANDLER(code,handler) \
  .type_bf_handlers[code] = handler
#define PKL_PHASE_DF_TYPE_HANDLER(code,handler) \
  .type_df_handlers[code] = handler

/* The following macros are to be used in node handlers.

   PKL_PASS_PAYLOAD expands to an l-value holding the data pointer
   passed to `pkl_do_pass'.

   PKL_PASS_AST expands to an l-value holding the pkl_ast value
   corresponding to the AST being processed.

   PKL_PASS_NODE expands to an l-value holding the pkl_ast_node for
   the node being processed.

   PKL_PASS_CHILD_POS expands to an l-value holding the position (zero
   based) of the node being processed in the parent's node chain.  If
   the node is not part of a chain, this holds 0.

   PKL_PASS_DONE finishes the execution of the node handler.  This is
   equivalent to reaching the end of the handler body.

   PKL_PASS_RESTART expands to an l-value that should be set to 1 if
   the handler modifies its subtree structure in any way, either
   creating new nodes or removing existing nodes.  This makes the pass
   machinery to do the right thing (hopefully.)  By default its value
   is 0.  This macro should _not_ be used as an r-value.
   
   PKL_PASS_EXIT can be used in order to interrupt the execution of
   the compiler pass.

   PKL_PASS_ERROR can be used in order to interrupt the execution of
   the compiler pass, making `pkl_do_pass' to return an error code.
   Please make sure to any node you create unless they are linked to
   the AST.  Otherwise you will leak memory.  */

#define PKL_PASS_PAYLOAD _payload
#define PKL_PASS_AST _ast
#define PKL_PASS_NODE _node
#define PKL_PASS_RESTART (*_restart)
#define PKL_PASS_CHILD_POS _child_pos
#define PKL_PASS_DONE do { goto _exit; } while (0)

#define PKL_PASS_EXIT do { longjmp (_toplevel, 1); } while (0)
#define PKL_PASS_ERROR do { longjmp (_toplevel, 2); } while (0)

/* The following macros should be used in order to define phase
   handlers.  */

#define PKL_PHASE_BEGIN_HANDLER(name)                                   \
  static pkl_ast_node name (jmp_buf _toplevel, pkl_ast _ast,            \
                            pkl_ast_node _node, void *_payload,         \
                            int *_restart, size_t _child_pos)           \
  {                                                                     \
     PKL_PASS_RESTART = 0;

#define PKL_PHASE_END_HANDLER                        \
                                                     \
   goto _exit; /* To avoid compiler warning */       \
_exit:                                               \
   return PKL_PASS_NODE;                             \
  }

/* Traverse AST in a depth-first fashion, applying the provided phases
   (or transformations) in sequence to each AST node.
   
   PHASES is a NULL-terminated array of pointers to node handlers.

   PAYLOADS is an array of pointers to payloads, which will be passed
   to the node handlers occupying the same position in the PHASES
   array.

   Running several phases in parallel in the same pass is good for
   performance.  However, there is an important consideration: if a
   phase requires to process each AST nodes just once, no restarting
   phases must precede it in a pass.

   Return 0 if some error occurred during the pass execution.  Return
   1 otherwise.  */

int pkl_do_pass (pkl_ast ast,
                 struct pkl_phase *phases[], void *payloads[]);

#endif /* PKL_PASS_H  */
