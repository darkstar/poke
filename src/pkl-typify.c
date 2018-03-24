/* pkl-typify.c - Type annotation phase for the poke compiler.  */

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

/* Each expression node in the AST should be characterized by a type.
   This file contains the implementation of a compiler phase that
   annotates these nodes with their respective types, according to the
   rules documented below.

   The types for INTEGER, CHAR and STRING nodes are set by the lexer.
   See pkl-lex.l.

   The type of an unary operation NOT or a binary operation EQ, NE,
   LT, GT, LE, GE, AND and OR is a boolean encoded as a 32-bit signed
   integer type.

   The type of an unary operation NEG, POS, BNOT is the type of its
   single operand.

   The type of a CAST is set by the parser. XXX: this is ugly.

   The type of a binary operation ADD, SUB, MUL, DIV, MOD, SL, SR,
   IOR, XOR, BAND, AND and OR is an integer type with the following
   characteristics:
   - If any of the operands is unsigned, the operation is unsigned.
   - The width of the operation is the width of the widest operand.

   The type of a SIZEOF operation is an offset type with an unsigned
   64-bit magnitude and units bits.

   The type of an offset is an offset type featuring the type of its
   magnitude, and its unit.

   The type of an ARRAY is determined from the number and the type of
   its initializers.

   The type of an ARRAY_REF is the type of the elements of the array
   it references.

   The type of a STRUCT is determined from the types of its elements.

   The type of a STRUCT_REF is the type of the referred element in the
   struct.
*/

#include <config.h>

#include "pkl-ast.h"
#include "pkl-pass.h"

PKL_PHASE_BEGIN_HANDLER (pkl_typify_df_op_boolean)
{
  pkl_ast_node type
    = pkl_ast_get_integral_type (PKL_PASS_AST, 32, 1);

  PKL_AST_TYPE (PKL_PASS_NODE)
    = ASTREF (type);
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_typify_df_first_operand)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node type
    = PKL_AST_TYPE (PKL_AST_EXP_OPERAND (exp, 0));
  
  PKL_AST_TYPE (exp) = ASTREF (type);
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_typify =
  {
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NOT, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_EQ, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NE, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LT, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GT, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LE, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GE, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_AND, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_OR, pkl_typify_df_op_boolean),

   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NEG, pkl_typify_df_first_operand),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_POS, pkl_typify_df_first_operand),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_BNOT, pkl_typify_df_first_operand),
  };
