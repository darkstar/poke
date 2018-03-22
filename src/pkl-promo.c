/* pkl-promo.c - Operand promotion phase for the poke compiler.  */

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

/* This file defines a compiler phase that promotes the operands of
   expressions in order to satisfy the compiler backend
   restrictions.  */

#include <config.h>

#include "pkl-ast.h"
#include "pkl-pass.h"

/* Promote a given node AST to an integral type of width SIZE and sign
   SIGN, if possible.  Put the resulting node in A.  Return 1 if the
   promotion was successful, 0 otherwise.  */

static int
promote_to_integral (size_t size, int sign,
                     pkl_ast ast, pkl_ast_node *a)
{
  pkl_ast_node type = PKL_AST_TYPE (*a);
  
  if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_INTEGRAL)
    {
      if (PKL_AST_TYPE_I_SIZE (type) != size
          || PKL_AST_TYPE_I_SIGNED (type) != sign)
        {
          pkl_ast_node desired_type
            = pkl_ast_get_integral_type (ast, size, sign);
          *a = pkl_ast_make_unary_exp (PKL_AST_OP_CAST, *a);
          PKL_AST_TYPE (*a) = ASTREF (desired_type);
          ASTREF (*a);
        }

      return 1;
    }
  
  return 0;
}

/* Promote a given node AST to a bool type, if possible.  Put the
   resulting node in A.  Return 1 if the promotion was successful, 0
   otherwise.  */

static int
promote_to_bool (pkl_ast ast, pkl_ast_node *a)
{
  return promote_to_integral (32, 1, ast, a);
}

/* Promote a given node AST to an unsigned long type, if possible.
   Put the resulting node in A.  Return 1 if the promotion was
   successful, 0 otherwise.  */

static int
promote_to_ulong (pkl_ast ast, pkl_ast_node *a)
{
  return promote_to_integral (64, 0, ast, a);
}

/* Promote the arguments to a binary operand to satisfy the language
   restrictions.  Put the resulting nodes in A and B.  Return 1 if the
   promotions were successful, 0 otherwise.  */

static int
promote_operands_binary (pkl_ast ast,
                         pkl_ast_node exp,
                         int allow_strings,
                         int allow_arrays,
                         int allow_structs)
{
  pkl_ast_node a = PKL_AST_EXP_OPERAND (exp, 0);
  pkl_ast_node b = PKL_AST_EXP_OPERAND (exp, 1);
  pkl_ast_node *to_promote_a = NULL;
  pkl_ast_node *to_promote_b = NULL;
  pkl_ast_node ta = PKL_AST_TYPE (a);
  pkl_ast_node tb = PKL_AST_TYPE (b);
  size_t size_a;
  size_t size_b;
  int sign_a;
  int sign_b;

  /* Both arguments should be either integrals, strings, arrays or
     structs.  */

  if (PKL_AST_TYPE_CODE (ta) != PKL_AST_TYPE_CODE (tb))
    goto error;

  if ((!allow_strings && PKL_AST_TYPE_CODE (ta) == PKL_TYPE_STRING)
      || (!allow_arrays && PKL_AST_TYPE_CODE (ta) == PKL_TYPE_ARRAY)
      || (!allow_structs && PKL_AST_TYPE_CODE (ta) == PKL_TYPE_STRUCT))
    goto error;

  if (!(PKL_AST_TYPE_CODE (ta) == PKL_TYPE_INTEGRAL))
    /* No need to promote non-integral types.  */
    return 1;

  /* Handle promotion of integral operands.  The rules are:

     - If one operand is narrower than the other, it is promoted to
       have the same width.  

     - If one operand is unsigned and the other signed, the signed
       operand is promoted to unsigned.  */

  size_a = PKL_AST_TYPE_I_SIZE (ta);
  size_b = PKL_AST_TYPE_I_SIZE (tb);
  sign_a = PKL_AST_TYPE_I_SIGNED (ta);
  sign_b = PKL_AST_TYPE_I_SIGNED (tb);

  if (size_a > size_b)
    {
      size_b = size_a;
      to_promote_b = &b;
    }
  else if (size_a < size_b)
    {
      size_a = size_b;
      to_promote_a = &a;
    }

  if (sign_a == 0 && sign_b == 1)
    {
      sign_b = 0;
      to_promote_b = &b;
    }
  else if (sign_a == 1 && sign_b == 0)
    {
      sign_a = 0;
      to_promote_a = &b;
    }

  if (to_promote_a != NULL)
    {
      pkl_ast_node t
        = pkl_ast_get_integral_type (ast, size_a, sign_a);
      a = pkl_ast_make_unary_exp (PKL_AST_OP_CAST, a);
      PKL_AST_TYPE (a) = ASTREF (t);
      PKL_AST_EXP_OPERAND (exp, 0) = ASTREF (a);
    }

  if (to_promote_b != NULL)
    {
      pkl_ast_node t
        = pkl_ast_get_integral_type (ast, size_b, sign_b);
      b = pkl_ast_make_unary_exp (PKL_AST_OP_CAST, b);
      PKL_AST_TYPE (b) = ASTREF (t);
      PKL_AST_EXP_OPERAND (exp, 1) = ASTREF (b);
    }

  return 1;

 error:
  fprintf (stderr, "error: invalid operands to expression\n");
  return 0;
}

/* Handler for binary operations whose operands should be both
   integral values of the same size and signedness.  */

PKL_PHASE_HANDLER (pkl_promo_binary_int)
{
  if (!promote_operands_binary (PKL_PASS_AST,
                                PKL_PASS_NODE,
                                0 /* allow_strings */,
                                0 /* allow_arrays */,
                                0 /* allow_structs */))
    PKL_PASS_ERROR;

  return PKL_PASS_NODE;
}

/* Handler for binary operations whose operands should be both either
   integral values of the same szie and signedness, or string
   values.  */

PKL_PHASE_HANDLER (pkl_promo_binary_int_str)
{
  if (!promote_operands_binary (PKL_PASS_AST,
                                PKL_PASS_NODE,
                                1 /* allow_strings */,
                                0 /* allow_arrays */,
                                0 /* allow_structs */))
    PKL_PASS_ERROR;

  return PKL_PASS_NODE;
}

/* Handler for binary operations whose operands should be both boolean
   values.  */

PKL_PHASE_HANDLER (pkl_promo_binary_bool)
{
  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);
  pkl_ast_node op2 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 1);
  
  if (!promote_to_bool (PKL_PASS_AST, &op1)
      || !promote_to_bool (PKL_PASS_AST, &op2))
    {
      fprintf (stderr, "error: operator requires boolean arguments\n");
      PKL_PASS_ERROR;
    }

  PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0) = op1;
  PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 1) = op2;
  return PKL_PASS_NODE;
}

/* Handler for unary operations whose operand should be a boolean
   value.  */

PKL_PHASE_HANDLER (pkl_promo_unary_bool)
{
  pkl_ast_node op = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);

  if (!promote_to_bool (PKL_PASS_AST, &op))
    {
      fprintf (stderr, "error: operator requires a boolean argument\n");
      PKL_PASS_ERROR;
    }

  PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0) = op;
  return PKL_PASS_NODE;
}


/* Handler for promoting indexes in array references to unsigned 64
   bit values.  */

PKL_PHASE_HANDLER (pkl_promo_array_ref)
{
  pkl_ast_node index = PKL_AST_ARRAY_REF_INDEX (PKL_PASS_NODE);

  if (!promote_to_ulong (PKL_PASS_AST, &index))
    {
      fprintf (stderr, "error: an array subscript should be an integral value\n");
      PKL_PASS_ERROR;
    }

  PKL_AST_ARRAY_REF_INDEX (PKL_PASS_NODE) = index;
  return PKL_PASS_NODE;
}

/* Handler for promoting the array size in array type literals to 64
   unsigned bit values.  */

PKL_PHASE_HANDLER (pkl_promo_type_array)
{
  pkl_ast_node nelem = PKL_AST_TYPE_A_NELEM (PKL_PASS_NODE);

  if (!promote_to_ulong (PKL_PASS_AST, &nelem))
    {
      fprintf (stderr, "error: an array size should be an integral value\n");
      PKL_PASS_ERROR;
    }

  PKL_AST_TYPE_A_NELEM (PKL_PASS_NODE) = nelem;
  return PKL_PASS_NODE;
}

struct pkl_phase pkl_phase_promo =
  { .op_df_handlers[PKL_AST_OP_ADD] = pkl_promo_binary_int_str,
    .op_df_handlers[PKL_AST_OP_EQ] = pkl_promo_binary_int_str,
    .op_df_handlers[PKL_AST_OP_NE] = pkl_promo_binary_int_str,
    .op_df_handlers[PKL_AST_OP_LT] = pkl_promo_binary_int_str,
    .op_df_handlers[PKL_AST_OP_GT] = pkl_promo_binary_int_str,
    .op_df_handlers[PKL_AST_OP_LE] = pkl_promo_binary_int_str,
    .op_df_handlers[PKL_AST_OP_GE] = pkl_promo_binary_int_str,
    .op_df_handlers[PKL_AST_OP_SUB] = pkl_promo_binary_int,
    .op_df_handlers[PKL_AST_OP_MUL] = pkl_promo_binary_int,
    .op_df_handlers[PKL_AST_OP_DIV] = pkl_promo_binary_int,
    .op_df_handlers[PKL_AST_OP_MOD] = pkl_promo_binary_int,
    .op_df_handlers[PKL_AST_OP_SL] = pkl_promo_binary_int,
    .op_df_handlers[PKL_AST_OP_SR] = pkl_promo_binary_int,
    .op_df_handlers[PKL_AST_OP_IOR] = pkl_promo_binary_int,
    .op_df_handlers[PKL_AST_OP_XOR] = pkl_promo_binary_int,
    .op_df_handlers[PKL_AST_OP_BAND] = pkl_promo_binary_int,
    .op_df_handlers[PKL_AST_OP_AND] = pkl_promo_binary_bool,
    .op_df_handlers[PKL_AST_OP_OR] = pkl_promo_binary_bool,
    .op_df_handlers[PKL_AST_OP_NOT] = pkl_promo_unary_bool,
    .op_df_handlers[PKL_AST_ARRAY_REF] = pkl_promo_array_ref,
    .type_df_handlers[PKL_TYPE_ARRAY] = pkl_promo_type_array,
  };