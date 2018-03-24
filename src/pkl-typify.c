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

   The type of a CAST is the type of its target type.

   The type of a binary operation ADD, SUB, MUL, DIV, MOD, SL, SR,
   IOR, XOR and BAND is an integer type with the following
   characteristics: - If any of the operands is unsigned, the
   operation is unsigned.  - The width of the operation is the width
   of the widest operand.

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

PKL_PHASE_BEGIN_HANDLER (pkl_typify_df_cast)
{
  pkl_ast_node cast = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_CAST_TYPE (cast);
  
  PKL_AST_TYPE (cast) = ASTREF (type);
}
PKL_PHASE_END_HANDLER

#define TYPIFY_BIN(op)                                                  \
  PKL_PHASE_BEGIN_HANDLER (pkl_typify_df_##op)                          \
  {                                                                     \
  pkl_ast_node exp = PKL_PASS_NODE;                                     \
  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (exp, 0);                      \
  pkl_ast_node op2 = PKL_AST_EXP_OPERAND (exp, 1);                      \
  pkl_ast_node t1 = PKL_AST_TYPE (op1);                                 \
  pkl_ast_node t2 = PKL_AST_TYPE (op2);                                 \
                                                                        \
  pkl_ast_node type;                                                    \
                                                                        \
  if (PKL_AST_TYPE_CODE (t1) != PKL_AST_TYPE_CODE (t2))                 \
    goto error;                                                         \
                                                                        \
  switch (PKL_AST_TYPE_CODE (t1))                                       \
    {                                                                   \
    CASE_STR                                                            \
    case PKL_TYPE_INTEGRAL:                                             \
      {                                                                 \
        int signed_p = (PKL_AST_TYPE_I_SIGNED (t1)                      \
                        && PKL_AST_TYPE_I_SIGNED (t2));                 \
        int size                                                        \
          = (PKL_AST_TYPE_I_SIZE (t1) > PKL_AST_TYPE_I_SIZE (t2)        \
             ? PKL_AST_TYPE_I_SIZE (t1) : PKL_AST_TYPE_I_SIZE (t2));    \
                                                                        \
        type = pkl_ast_get_integral_type (PKL_PASS_AST, size, signed_p); \
        break;                                                          \
      }                                                                 \
    default:                                                            \
      goto error;                                                       \
      break;                                                            \
    }                                                                   \
                                                                        \
  PKL_AST_TYPE (exp) = ASTREF (type);                                   \
  PKL_PASS_DONE;                                                        \
                                                                        \
  error:                                                                \
  fprintf (stderr, "error: invalid operands to expression\n");          \
  PKL_PASS_ERROR;                                                       \
  }                                                                     \
  PKL_PHASE_END_HANDLER

/* ADD and SUB accept both integral and string operands.  */

#define CASE_STR                                                        \
    case PKL_TYPE_STRING:                                               \
      type = pkl_ast_get_string_type (PKL_PASS_AST);                    \
      break;

TYPIFY_BIN (add);
TYPIFY_BIN (sub);

/* But the following operations only accept integers, so define
   CASE_STR to the empty string.  */

#undef CASE_STR
#define CASE_STR

TYPIFY_BIN (mul);
TYPIFY_BIN (div);
TYPIFY_BIN (mod);
TYPIFY_BIN (sl);
TYPIFY_BIN (sr);
TYPIFY_BIN (ior);
TYPIFY_BIN (xor);
TYPIFY_BIN (band);

#undef TYPIFY_BIN

PKL_PHASE_BEGIN_HANDLER (pkl_typify_df_op_sizeof)
{
  pkl_ast_node itype
    = pkl_ast_get_integral_type (PKL_PASS_AST,
                                 64, 0);
  pkl_ast_node type
    = pkl_ast_make_offset_type (itype,
                                PKL_AST_OFFSET_UNIT_BITS);

  PKL_AST_TYPE (PKL_PASS_NODE) = ASTREF (type);
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_typify_df_offset)
{
  pkl_ast_node offset = PKL_PASS_NODE;
  pkl_ast_node magnitude_type
    = PKL_AST_TYPE (PKL_AST_OFFSET_MAGNITUDE (offset));
  pkl_ast_node type
    = pkl_ast_make_offset_type (magnitude_type,
                                PKL_AST_OFFSET_UNIT (offset));

  PKL_AST_TYPE (offset) = ASTREF (type);
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_typify =
  {
   PKL_PHASE_DF_HANDLER (PKL_AST_CAST, pkl_typify_df_cast),
   PKL_PHASE_DF_HANDLER (PKL_AST_OFFSET, pkl_typify_df_offset),

   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SIZEOF, pkl_typify_df_op_sizeof),

   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NOT, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_EQ, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NE, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LT, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GT, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LE, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GE, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_AND, pkl_typify_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_OR, pkl_typify_df_op_boolean),

   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_ADD, pkl_typify_df_add),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SUB, pkl_typify_df_sub),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_MUL, pkl_typify_df_mul),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_DIV, pkl_typify_df_div),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_MOD, pkl_typify_df_mod),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SL, pkl_typify_df_sl),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SR, pkl_typify_df_sr),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_IOR, pkl_typify_df_ior),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_XOR, pkl_typify_df_xor),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_BAND, pkl_typify_df_band),

   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NEG, pkl_typify_df_first_operand),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_POS, pkl_typify_df_first_operand),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_BNOT, pkl_typify_df_first_operand),
  };
