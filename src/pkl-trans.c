/* pkl-trans.c - Transformation phases for the poke compiler.  */

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
#include <stdio.h>

#include "pkl-ast.h"
#include "pkl-pass.h"
#include "pkl-trans.h"


/* This file implements several transformation compiler phases which,
   generally speaking, are not restartable.

   `trans1' is run immediately after parsing.
   `trans2' is run before anal2.

  See the handlers below for detailed information about the specific
  transformations these phases perform.  */

/* The following handler is used in both trans1 and tran2 and
   initializes the phase payload.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans_bf_program)
{
  pkl_trans_payload payload
    = (pkl_trans_payload) PKL_PASS_PAYLOAD;
  payload->errors = 0;
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_trans1 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_trans_bf_program),
  };

/* SIZEOF nodes whose operand is a complete type should be replaced
   with an offset.  The type should be complete.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_df_op_sizeof)
{
  pkl_trans_payload payload
    = (pkl_trans_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node op = PKL_AST_EXP_OPERAND (node, 0);
  pkl_ast_node offset, offset_type;

  if (PKL_AST_CODE (op) != PKL_AST_TYPE)
    /* This is a TYPEOF (VALUE).  Nothing to do.  */
    PKL_PASS_DONE;

  if (PKL_AST_TYPE_COMPLETE (op)
      != PKL_AST_TYPE_COMPLETE_YES)
    {
      fputs ("error: sizeof only works on complete types\n",
             stderr);
      payload->errors++;
      PKL_PASS_DONE;
    }

  {    
    /* Calculate the size of the complete type in bytes and put it in
       an integer node.  */
    pkl_ast_node magnitude_type
      = pkl_ast_get_integral_type (PKL_PASS_AST, 64, 0);
    pkl_ast_node magnitude
      = pkl_ast_make_integer (pkl_ast_sizeof_type (op));
    PKL_AST_TYPE (magnitude) = ASTREF (magnitude_type);
  
    /* Build an offset with that magnitude, and unit bits.  */
    offset = pkl_ast_make_offset (magnitude,
                                  PKL_AST_OFFSET_UNIT_BITS);
    offset_type = pkl_ast_make_offset_type (magnitude_type,
                                            PKL_AST_OFFSET_UNIT_BITS);
    PKL_AST_TYPE (offset) = ASTREF (offset_type);
  }

  PKL_PASS_NODE = offset;
  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_trans2 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_trans_bf_program),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SIZEOF, pkl_trans2_df_op_sizeof),
  };
