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

/* This file implements a compiler phase that promotes the operands of
   expressions following language rules.  This phase expects that
   every expression operand is anotated with its proper type.  */

#include <config.h>

#include "pkl.h"
#include "pkl-ast.h"
#include "pkl-pass.h"

/* Promote a given node A to an integral type of width SIZE and sign
   SIGN, if possible.  Put the resulting node in A.  Return 1 if the
   promotion was successful, 0 otherwise.  */

static int
promote_integral (pkl_ast ast,
                  size_t size, int sign,
                  pkl_ast_node *a,
                  int *restart)
{
  pkl_ast_node type = PKL_AST_TYPE (*a);
  
  *restart = 0;
  if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_INTEGRAL)
    {
      if (PKL_AST_TYPE_I_SIZE (type) != size
          || PKL_AST_TYPE_I_SIGNED (type) != sign)
        {
          pkl_ast_node desired_type
            = pkl_ast_make_integral_type (ast, size, sign);
          pkl_ast_loc loc = PKL_AST_LOC (*a);
          
          *a = pkl_ast_make_cast (ast, desired_type, *a);
          PKL_AST_TYPE (*a) = ASTREF (desired_type);
          PKL_AST_LOC (*a) = loc;
          PKL_AST_LOC (desired_type) = loc;
          ASTREF (*a);
          *restart = 1;
        }

      return 1;
    }
  
  return 0;
}

/* Handler for binary operations.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_binary)
{
  int restart1, restart2;

  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (exp);

  if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_INTEGRAL)
    {
      size_t size = PKL_AST_TYPE_I_SIZE (type);
      int sign = PKL_AST_TYPE_I_SIGNED (type);
      
      if (!promote_integral (PKL_PASS_AST, size, sign,
                             &PKL_AST_EXP_OPERAND (exp, 0), &restart1)
          || !promote_integral (PKL_PASS_AST, size, sign,
                                &PKL_AST_EXP_OPERAND (exp, 1), &restart2))
        {
          pkl_ice (PKL_PASS_AST, PKL_AST_LOC (exp),
                   "couldn't promote operands of expression #%" PRIu64,
                   PKL_AST_UID (exp));
          PKL_PASS_ERROR;
        }

      PKL_PASS_RESTART = restart1 || restart2;
    }
}
PKL_PHASE_END_HANDLER

/* Handler for unary operations.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_unary)
{
  int restart;

  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (node);

  if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_INTEGRAL)
    {
      size_t size = PKL_AST_TYPE_I_SIZE (type);
      int sign = PKL_AST_TYPE_I_SIGNED (type);

      if (!promote_integral (PKL_PASS_AST, size, sign,
                             &PKL_AST_EXP_OPERAND (node, 0), &restart))
        {
          pkl_error (PKL_PASS_AST, PKL_AST_LOC (node),
                     "operator requires a boolean argument");
          PKL_PASS_ERROR;
        }
    }

  PKL_PASS_RESTART = restart;
}
PKL_PHASE_END_HANDLER

/* Handler for promoting indexes in array references to unsigned 64
   bit values.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_array_ref)
{
  int restart;
  pkl_ast_node node = PKL_PASS_NODE;

  if (!promote_integral (PKL_PASS_AST, 64, 0,
                         &PKL_AST_ARRAY_REF_INDEX (node), &restart))
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (node),
               "couldn't promote array subscript");
      PKL_PASS_ERROR;
    }

  PKL_PASS_RESTART = restart;
}
PKL_PHASE_END_HANDLER

/* Handler for promoting the array size in array type literals to 64
   unsigned bit values.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_type_array)
{
  int restart;
  pkl_ast_node node = PKL_PASS_NODE;

  if (PKL_AST_TYPE_A_NELEM (node) == NULL)
    /* This array type hasn't a number of elements.  Be done.  */
    PKL_PASS_DONE;

  if (!promote_integral (PKL_PASS_AST, 64, 0,
                         &PKL_AST_TYPE_A_NELEM (node), &restart))
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (node),
               "couldn't promote array type size expression");
      PKL_PASS_ERROR;
    }

  PKL_PASS_RESTART = restart;
}
PKL_PHASE_END_HANDLER

/* Indexes in array initializers should be unsigned long.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_array_initializer)
{
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node index = PKL_AST_ARRAY_INITIALIZER_INDEX (node);

  /* Note that the index is optional.  */
  if (index != NULL)
    {
      /* We can't use casts here, as array index initializers should
         be INTEGER nodes, not expressions.  */

      pkl_ast_node index_type = PKL_AST_TYPE (index);

      if (PKL_AST_TYPE_CODE (index_type) != PKL_TYPE_INTEGRAL
          || PKL_AST_TYPE_I_SIZE (index_type) != 64
          || PKL_AST_TYPE_I_SIGNED (index_type) != 0)
        {
          pkl_ast_node_free (index_type);

          index_type = pkl_ast_make_integral_type (PKL_PASS_AST,
                                                   64, 0);
          PKL_AST_TYPE (index) = ASTREF (index_type);
          PKL_AST_LOC (index_type) = PKL_AST_LOC (node);
          PKL_PASS_RESTART = 1;
        }
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_promo =
  {
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY_REF, pkl_promo_array_ref),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY_INITIALIZER, pkl_promo_array_initializer),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_ADD, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_EQ, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NE, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LT, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GT, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LE, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GE, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SUB, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_MUL, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_DIV, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_MOD, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SL, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SR, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_IOR, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_XOR, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_BAND, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_AND, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_OR, pkl_promo_binary),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NOT, pkl_promo_unary),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_promo_type_array),
  };
