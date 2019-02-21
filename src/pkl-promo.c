/* pkl-promo.c - Operand promotion phase for the poke compiler.  */

/* Copyright (C) 2019 Jose E. Marchesi */

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

/* Note the following macro evaluates the arguments twice!  */
#define MAX(A,B) ((A) > (B) ? (A) : (B))

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

/* Promote a given node A, which should be an offset, to an offset
   type featuring BASE_TYPE and UNIT.  Put the resulting node in A.
   Return 1 if the promotion was successful, 0 otherwise.  */

static int
promote_offset (pkl_ast ast,
                pkl_ast_node base_type, pkl_ast_node unit,
                pkl_ast_node *a,
                int *restart)
{
  pkl_ast_node type = PKL_AST_TYPE (*a);
  
  *restart = 0;
  if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_OFFSET)
    {
      if (!pkl_ast_type_equal (base_type,
                               PKL_AST_TYPE_O_BASE_TYPE (type)))
        {
          pkl_ast_node desired_type
            = pkl_ast_make_offset_type (ast, base_type, unit);
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

/* Division is defined on the following configurations of operands and
   result types:

      INTEGRAL / INTEGRAL -> INTEGRAL
      OFFSET   / OFFSET   -> INTEGRAL

   In the I / I -> I configuration, the types of the operands are
   promoted to match the type of the result, if needed.

   In the O / O -> I configuration, the magnitude types of the offset
   operands are promoted to match the type of the integral result, if
   needed.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_op_div)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (exp);
  size_t size = PKL_AST_TYPE_I_SIZE (type);
  int sign = PKL_AST_TYPE_I_SIGNED (type);
  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (exp, 0);
  pkl_ast_node op2 = PKL_AST_EXP_OPERAND (exp, 1);
  pkl_ast_node op1_type = PKL_AST_TYPE (op1);
  pkl_ast_node op2_type = PKL_AST_TYPE (op2);
        
  /* Note we discriminate on the first operand type in order to
     distinguish between configurations.  */
  switch (PKL_AST_TYPE_CODE (op1_type))
    {
    case PKL_TYPE_INTEGRAL:
      {
        int restart1, restart2;

        if (!promote_integral (PKL_PASS_AST, size, sign,
                               &PKL_AST_EXP_OPERAND (exp, 0), &restart1))
          goto error;

        if (!promote_integral (PKL_PASS_AST, size, sign,
                               &PKL_AST_EXP_OPERAND (exp, 1), &restart2))
          goto error;

        PKL_PASS_RESTART = restart1 || restart2;
        break;
      }
    case PKL_TYPE_OFFSET:
      {
        int restart1, restart2;
        
        if (!promote_offset (PKL_PASS_AST,
                             type, PKL_AST_TYPE_O_UNIT (op1_type),
                             &PKL_AST_EXP_OPERAND (exp, 0), &restart1))
          goto error;

        if (!promote_offset (PKL_PASS_AST,
                             type, PKL_AST_TYPE_O_UNIT (op2_type),
                             &PKL_AST_EXP_OPERAND (exp, 1), &restart2))
          goto error;

        PKL_PASS_RESTART = restart1 || restart2;
        break;
      }
    default:
      goto error;
    }

  PKL_PASS_DONE;

 error:
  pkl_ice (PKL_PASS_AST, PKL_AST_LOC (exp),
           "couldn't promote operands of expression #%" PRIu64,
           PKL_AST_UID (exp));
  PKL_PASS_ERROR;
}
PKL_PHASE_END_HANDLER

/* Addition, subtraction and modulus are defined on the following
   configurations of operand and result types:

      INTEGRAL x INTEGRAL -> INTEGRAL
      OFFSET   x OFFSET   -> OFFSET

   In the I x I -> I configuration, the types of the operands are
   promoted to match the type of the result, if needed.

   In the O x O -> O configuration, the magnitude types of the offset
   operands are promoted to match the type of the magnitude type of
   the result offset, if needed.

   Also, addition is used to concatenate strings:

      STRING x STRING -> STRING

   In this configuration no promotions are done.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_op_add_sub_mod)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (exp);
  size_t size = PKL_AST_TYPE_I_SIZE (type);
  int sign = PKL_AST_TYPE_I_SIGNED (type);
  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (exp, 0);
  pkl_ast_node op2 = PKL_AST_EXP_OPERAND (exp, 1);
  pkl_ast_node op1_type = PKL_AST_TYPE (op1);
  pkl_ast_node op2_type = PKL_AST_TYPE (op2);
        
  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      {
        int restart1, restart2;

        if (!promote_integral (PKL_PASS_AST, size, sign,
                               &PKL_AST_EXP_OPERAND (exp, 0), &restart1))
          goto error;

        if (!promote_integral (PKL_PASS_AST, size, sign,
                               &PKL_AST_EXP_OPERAND (exp, 1), &restart2))
          goto error;

        PKL_PASS_RESTART = restart1 || restart2;
        break;
      }
    case PKL_TYPE_OFFSET:
      {
        int restart1, restart2;
        pkl_ast_node exp_base_type = PKL_AST_TYPE_O_BASE_TYPE (type);
        
        if (!promote_offset (PKL_PASS_AST,
                             exp_base_type, PKL_AST_TYPE_O_UNIT (op1_type),
                             &PKL_AST_EXP_OPERAND (exp, 0), &restart1))
          goto error;

        if (!promote_offset (PKL_PASS_AST,
                             exp_base_type, PKL_AST_TYPE_O_UNIT (op2_type),
                             &PKL_AST_EXP_OPERAND (exp, 1), &restart2))
          goto error;

        PKL_PASS_RESTART = restart1 || restart2;
        break;
      }
    case PKL_TYPE_STRING:
      if (PKL_AST_EXP_CODE (exp) != PKL_AST_OP_ADD)
        goto error;
      break;
    default:
      goto error;
    }

  PKL_PASS_DONE;

 error:
  pkl_ice (PKL_PASS_AST, PKL_AST_LOC (exp),
           "couldn't promote operands of expression #%" PRIu64,
           PKL_AST_UID (exp));
  PKL_PASS_ERROR;
}
PKL_PHASE_END_HANDLER

/* Multiplication is defined on the following configurations of
   operand and result types:

      INTEGRAL x INTEGRAL -> INTEGRAL
      OFFSET   x INTEGRAL -> OFFSET
      INTEGRAL x OFFSET   -> OFFSET

   In the I x I -> I configuration, the types of the operands are
   promoted to match the type of the result, if needed.

   In the O x I -> O and I x O -> O configurations, both the type of
   the integral operand and the base type of the offset operand are
   promoted to match the base type of the offset result.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_op_mul)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node exp_type = PKL_AST_TYPE (exp);
  int exp_type_code = PKL_AST_TYPE_CODE (exp_type);
  int i;

  for (i = 0; i < 2; ++i)
    {
      int restart;

      pkl_ast_node op = PKL_AST_EXP_OPERAND (exp, i);
      pkl_ast_node op_type = PKL_AST_TYPE (op);

      if (PKL_AST_TYPE_CODE (op_type) == PKL_TYPE_INTEGRAL)
        {
          size_t size;
          int sign;

          if (exp_type_code == PKL_TYPE_INTEGRAL)
            {
              size = PKL_AST_TYPE_I_SIZE (exp_type);
              sign = PKL_AST_TYPE_I_SIGNED  (exp_type);
            }
          else
            {
              pkl_ast_node exp_base_type
                = PKL_AST_TYPE_O_BASE_TYPE (exp_type);

              size = PKL_AST_TYPE_I_SIZE (exp_base_type);
              sign = PKL_AST_TYPE_I_SIGNED (exp_base_type);
            }

          if (!promote_integral (PKL_PASS_AST, size, sign,
                                 &PKL_AST_EXP_OPERAND (exp, i), &restart))
            goto error;

          PKL_PASS_RESTART = restart;
        }
      else if (PKL_AST_TYPE_CODE (op_type) == PKL_TYPE_OFFSET)
        {
          pkl_ast_node exp_base_type = PKL_AST_TYPE_O_BASE_TYPE (exp_type);
        
          if (!promote_offset (PKL_PASS_AST,
                               exp_base_type, PKL_AST_TYPE_O_UNIT (op_type),
                               &PKL_AST_EXP_OPERAND (exp, i), &restart))
            goto error;

          PKL_PASS_RESTART = restart;
        }
      else
        assert (0);
    }

  PKL_PASS_DONE;

 error:
  pkl_ice (PKL_PASS_AST, PKL_AST_LOC (exp),
           "couldn't promote operands of expression #%" PRIu64,
           PKL_AST_UID (exp));
  PKL_PASS_ERROR;
}
PKL_PHASE_END_HANDLER

/* The relational operations are defined on the following
   configurations of operand and result types:

           INTEGRAL x INTEGRAL -> BOOL
           STRING   x STRING   -> BOOL
           OFFSET   x OFFSET   -> BOOL
          
   In the I x I -> I configuration, the types of the operands are
   promoted in a way both operands end having the same type, following
   the language's promotion rules.

   The same logic is applied to the magnitudes of the offset operands
   in the O x O -> O configuration.

   No operand promotion is performed in the the S x S -> S
   configuration.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_op_rela)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (exp, 0);
  pkl_ast_node op2 = PKL_AST_EXP_OPERAND (exp, 1);
  pkl_ast_node op1_type = PKL_AST_TYPE (op1);
  pkl_ast_node op2_type = PKL_AST_TYPE (op2);

  if (PKL_AST_TYPE_CODE (op1_type) != PKL_AST_TYPE_CODE (op2_type))
    goto error;
        
  switch (PKL_AST_TYPE_CODE (op1_type))
    {
    case PKL_TYPE_INTEGRAL:
      {
        int restart1, restart2;

        size_t size = MAX (PKL_AST_TYPE_I_SIZE (op1_type),
                           PKL_AST_TYPE_I_SIZE (op2_type));
        int sign = (PKL_AST_TYPE_I_SIGNED (op1_type)
                    && PKL_AST_TYPE_I_SIGNED (op2_type));


        if (!promote_integral (PKL_PASS_AST, size, sign,
                               &PKL_AST_EXP_OPERAND (exp, 0), &restart1))
          goto error;

        if (!promote_integral (PKL_PASS_AST, size, sign,
                               &PKL_AST_EXP_OPERAND (exp, 1), &restart2))
          goto error;

        PKL_PASS_RESTART = restart1 || restart2;
        break;
      }
    case PKL_TYPE_OFFSET:
      {
        int restart1, restart2;

        pkl_ast_node op1_base_type = PKL_AST_TYPE_O_BASE_TYPE (op1_type);
        pkl_ast_node op2_base_type = PKL_AST_TYPE_O_BASE_TYPE (op2_type);

        size_t size = MAX (PKL_AST_TYPE_I_SIZE (op1_base_type),
                           PKL_AST_TYPE_I_SIZE (op2_base_type));
        int sign = (PKL_AST_TYPE_I_SIGNED (op1_base_type)
                    && PKL_AST_TYPE_I_SIGNED (op2_base_type));

        pkl_ast_node to_type =
          pkl_ast_make_integral_type (PKL_PASS_AST, size, sign);
        PKL_AST_LOC (to_type) = PKL_AST_LOC (PKL_PASS_NODE);
        
        if (!promote_offset (PKL_PASS_AST,
                             to_type, PKL_AST_TYPE_O_UNIT (op1_type),
                             &PKL_AST_EXP_OPERAND (exp, 0), &restart1))
          goto error;

        if (!promote_offset (PKL_PASS_AST,
                             to_type, PKL_AST_TYPE_O_UNIT (op2_type),
                             &PKL_AST_EXP_OPERAND (exp, 1), &restart2))
          goto error;

        if (!restart1 && !restart2)
          {
            ASTREF (to_type);
            pkl_ast_node_free (to_type);
          }

        PKL_PASS_RESTART = restart1 || restart2;
        break;
        case PKL_TYPE_STRING:
          /* Nothing to do.  */
          break;
      }
    default:
      goto error;
    }

  PKL_PASS_DONE;

 error:
  pkl_ice (PKL_PASS_AST, PKL_AST_LOC (exp),
           "couldn't promote operands of expression #%" PRIu64,
           PKL_AST_UID (exp));
  PKL_PASS_ERROR;
}
PKL_PHASE_END_HANDLER

/* The bit shift operations are defined on the following
   configurations of operand and result types:

           INTEGRAL x INTEGRAL(32,0) -> INTEGRAL

   In this configuration, the type of the first operand is promoted to
   match the type of the result.  The type of the second operand is
   promoted to an unsigned 32-bit integral type.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_op_bshift)
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
          || !promote_integral (PKL_PASS_AST, 32, 0,
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

/* The rest of the binary operations are defined on the following
   configurations of operand and result types:

       INTEGRAL OP INTEGRAL -> INTEGRAL.

   In the I OP I -> I configuration, the types of the operands are
   promoted to match the type of the result, if needed.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_op_binary)
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

/* All the unary operations are defined on the following
   configurations of operand and result types:

                    INTEGRAL -> INTEGRAL

   In the I -> I configuration, the type of the operand is promoted to
   match the type of the result, if needed.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_op_unary)
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
          pkl_ice (PKL_PASS_AST, PKL_AST_LOC (node),
                   "couldn't promote operands of expression #%" PRIu64,
                   PKL_AST_UID (node));
          PKL_PASS_ERROR;
        }
    }

  PKL_PASS_RESTART = restart;
}
PKL_PHASE_END_HANDLER

/* Handler for promoting indexes in indexers to unsigned 64 bit
   values.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_indexer)
{
  int restart;
  pkl_ast_node node = PKL_PASS_NODE;

  if (!promote_integral (PKL_PASS_AST, 64, 0,
                         &PKL_AST_INDEXER_INDEX (node), &restart))
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (node),
               "couldn't promote indexer subscript");
      PKL_PASS_ERROR;
    }

  PKL_PASS_RESTART = restart;
}
PKL_PHASE_END_HANDLER

/* Handler for promoting indexes in trimmers to unsigned 64 bit
   values.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_trimmer)
{
  int restart;
  pkl_ast_node trimmer = PKL_PASS_NODE;
  pkl_ast_node from = PKL_AST_TRIMMER_FROM (trimmer);
  pkl_ast_node to = PKL_AST_TRIMMER_TO (trimmer);

  if (!promote_integral (PKL_PASS_AST, 64, 0,
                         &PKL_AST_TRIMMER_FROM (trimmer), &restart))
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (from),
               "couldn't promote trimmer index");
      PKL_PASS_ERROR;
    }

  if (!promote_integral (PKL_PASS_AST, 64, 0,
                         &PKL_AST_TRIMMER_TO (trimmer), &restart))
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (to),
               "couldn't promote trimmer index");
      PKL_PASS_ERROR;
    }

  PKL_PASS_RESTART = restart;
}
PKL_PHASE_END_HANDLER

/* Handler for promoting the array size in array type literals to 64
   unsigned bit values, or to offset<uint<64>,*> if they are
   offsets.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_type_array)
{
  int restart;
  pkl_ast_node array_type = PKL_PASS_NODE;
  pkl_ast_node nelem = PKL_AST_TYPE_A_NELEM (array_type);
  pkl_ast_node nelem_type;
  
  if (nelem == NULL)
    /* This array type hasn't a number of elements.  Be done.  */
    PKL_PASS_DONE;

  nelem_type = PKL_AST_TYPE (nelem);

  switch (PKL_AST_TYPE_CODE (nelem_type))
    {
    case PKL_TYPE_INTEGRAL:
      if (!promote_integral (PKL_PASS_AST, 64, 0,
                             &PKL_AST_TYPE_A_NELEM (array_type), &restart))
        {
          pkl_ice (PKL_PASS_AST, PKL_AST_LOC (nelem),
                   "couldn't promote array type size expression");
          PKL_PASS_ERROR;
        }
      break;
    case PKL_TYPE_OFFSET:
      {
        pkl_ast_node to_magnitude_type
          = pkl_ast_make_integral_type (PKL_PASS_AST, 64, 0);
        PKL_AST_LOC (to_magnitude_type) = PKL_AST_LOC (nelem);
        
        if (!promote_offset (PKL_PASS_AST,
                             to_magnitude_type, PKL_AST_TYPE_O_UNIT (nelem_type),
                             &PKL_AST_TYPE_A_NELEM (array_type), &restart))
          {
            pkl_ice (PKL_PASS_AST, PKL_AST_LOC (nelem),
                     "couldn't promote array type size expression");
            PKL_PASS_ERROR;
          }
        break;
      }
    default:
      assert (0); /* This can't happen.  */
    }

  PKL_PASS_RESTART = restart;
}
PKL_PHASE_END_HANDLER

/* Indexes in array initializers should be unsigned long.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_array_initializer)
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

/* Exception numbers in `raise' statements should be ints.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_raise_stmt)
{
  pkl_ast_node raise_stmt = PKL_PASS_NODE;
  pkl_ast_node exp = PKL_AST_RAISE_STMT_EXP (raise_stmt);

  /* Note that the exception number is optional.  */
  if (exp != NULL)
    {
      int restart;

      if (!promote_integral (PKL_PASS_AST, 32, 1,
                             &PKL_AST_RAISE_STMT_EXP (raise_stmt),
                             &restart))
        {
          pkl_ice (PKL_PASS_AST, PKL_AST_LOC (exp),
                   "couldn't promote exception number to int<32>");
          PKL_PASS_ERROR;
        }

      PKL_PASS_RESTART = restart;
    }
}
PKL_PHASE_END_HANDLER

/* Exception numbers in try-catch-if statements should be ints.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_try_catch_stmt)
{
  pkl_ast_node try_catch_stmt = PKL_PASS_NODE;
  pkl_ast_node exp = PKL_AST_TRY_CATCH_STMT_EXP (try_catch_stmt);

  if (exp)
    {
      int restart;

      if (!promote_integral (PKL_PASS_AST, 32, 1,
                             &PKL_AST_TRY_CATCH_STMT_EXP (try_catch_stmt),
                             &restart))
        {
          pkl_ice (PKL_PASS_AST, PKL_AST_LOC (exp),
                   "couldn't promote exception number to int<32>");
          PKL_PASS_ERROR;
        }

      PKL_PASS_RESTART = restart;
    }
}
PKL_PHASE_END_HANDLER

/* In assignments, the r-value should be promoted to the type of the
   l-value, if that is suitable.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_ass_stmt)
{
  pkl_ast_node ass_stmt = PKL_PASS_NODE;
  pkl_ast_node lvalue = PKL_AST_ASS_STMT_LVALUE (ass_stmt);
  pkl_ast_node exp = PKL_AST_ASS_STMT_EXP (ass_stmt);
  pkl_ast_node lvalue_type = PKL_AST_TYPE (lvalue);
  pkl_ast_node exp_type = PKL_AST_TYPE (exp);
  int restart = 0;

  /* At this point it is assured that exp_type is promoteable to
     lvalue_type, or typify1 wouldn't have allowed this node to
     pass.  */

  if (!pkl_ast_type_equal (lvalue_type, exp_type))
    {
      switch (PKL_AST_TYPE_CODE (lvalue_type))
        {
        case PKL_TYPE_ANY:
        case PKL_TYPE_ARRAY: /* any[] */
          break;
        case PKL_TYPE_INTEGRAL:
          if (!promote_integral (PKL_PASS_AST,
                                 PKL_AST_TYPE_I_SIZE (lvalue_type),
                                 PKL_AST_TYPE_I_SIGNED (lvalue_type),
                                 &PKL_AST_ASS_STMT_EXP (ass_stmt),
                                 &restart))
            {
              pkl_ice (PKL_PASS_AST, PKL_AST_LOC (exp),
                       "couldn't promote r-value in assignment");
              PKL_PASS_ERROR;
            }
          break;
        case PKL_TYPE_OFFSET:
          if (!promote_offset (PKL_PASS_AST,
                               PKL_AST_TYPE_O_BASE_TYPE (lvalue_type),
                               PKL_AST_TYPE_O_UNIT (lvalue_type),
                               &PKL_AST_ASS_STMT_EXP (ass_stmt),
                               &restart))
            {
              pkl_ice (PKL_PASS_AST, PKL_AST_LOC (exp),
                       "couldn't promote r-value in assignment");
              PKL_PASS_ERROR;
            }
          break;
        default:
          pkl_ice (PKL_PASS_AST, PKL_AST_LOC (ass_stmt),
                   "non-promoteable r-value in assignment statement at promo time");
          PKL_PASS_ERROR;
          break;
        }

      PKL_PASS_RESTART = restart;
    }
}
PKL_PHASE_END_HANDLER

/* In function calls, the actual arguments should be promoted to the
   type of the formal arguments, if that is suitable.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_funcall)
{
  pkl_ast_node funcall = PKL_PASS_NODE;
  pkl_ast_node function = PKL_AST_FUNCALL_FUNCTION (funcall);
  pkl_ast_node function_type = PKL_AST_TYPE (function);

  pkl_ast_node fa, aa;

  for (fa = PKL_AST_TYPE_F_ARGS (function_type),
       aa = PKL_AST_FUNCALL_ARGS (funcall);
       fa && aa;
       fa = PKL_AST_CHAIN (fa), aa = PKL_AST_CHAIN (aa))
    {
      pkl_ast_node fa_type = PKL_AST_FUNC_ARG_TYPE (fa);
      pkl_ast_node aa_exp = PKL_AST_FUNCALL_ARG_EXP (aa);
      pkl_ast_node aa_type = PKL_AST_TYPE (aa_exp);

      /* Do not promote varargs.  */
      if (PKL_AST_FUNC_TYPE_ARG_VARARG (fa))
        continue;

      /* At this point it is assured that the types of the actual
         argument and the formal argument are promoteable, or typify
         wouldn't have allowed it to pass.  If both types are equal,
         we have got nothing to do.  */
      if (!pkl_ast_type_equal (fa_type, aa_type))
        {
          int restart;
          
          switch (PKL_AST_TYPE_CODE (fa_type))
            {
            case PKL_TYPE_ANY:
            case PKL_TYPE_ARRAY: /* any[] */
              /* Do nothing.  Promoting to ANY doesn't require
                 altering the AST in any way.  */
              break;
            case PKL_TYPE_INTEGRAL:
              if (!promote_integral (PKL_PASS_AST,
                                     PKL_AST_TYPE_I_SIZE (fa_type),
                                     PKL_AST_TYPE_I_SIGNED (fa_type),
                                     &PKL_AST_FUNCALL_ARG_EXP (aa),
                                     &restart))
                {
                  pkl_ice (PKL_PASS_AST, PKL_AST_LOC (aa),
                           "couldn't promote funcall argument");
                  PKL_PASS_ERROR;
                }
              break;
            case PKL_TYPE_OFFSET:
              if (!promote_offset (PKL_PASS_AST,
                                   PKL_AST_TYPE_O_BASE_TYPE (fa_type),
                                   PKL_AST_TYPE_O_UNIT (fa_type),
                                   &PKL_AST_FUNCALL_ARG_EXP (aa),
                                   &restart))
                {
                  pkl_ice (PKL_PASS_AST, PKL_AST_LOC (aa),
                           "couldn't promote funcall argument");
                  PKL_PASS_ERROR;
                }
              break;
            default:
              pkl_ice (PKL_PASS_AST, PKL_AST_LOC (funcall),
                       "funcall contains non-promoteable arguments at promo time");
              PKL_PASS_ERROR;
              break;
            }

          PKL_PASS_RESTART = PKL_PASS_RESTART || restart;
        }
    }
}
PKL_PHASE_END_HANDLER

/* The value returned from a function can be promoted in certain
   circumstances.  Do it!  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_return_stmt)
{
  pkl_ast_node return_stmt = PKL_PASS_NODE;
  pkl_ast_node exp = PKL_AST_RETURN_STMT_EXP (return_stmt);
  pkl_ast_node function = PKL_AST_RETURN_STMT_FUNCTION (return_stmt);

  pkl_ast_node returned_type;
  pkl_ast_node expected_type;

  if (exp == NULL)
    PKL_PASS_DONE;

  returned_type = PKL_AST_TYPE (exp);
  expected_type = PKL_AST_FUNC_RET_TYPE (function);

  /* At this point it is assured that the expected type and returned
     type are promoteable, or typify wouldn't have allowed it to pass.
     If both types are equal, then we have got nothing to to.  */

  if (PKL_AST_TYPE_CODE (expected_type) != PKL_TYPE_VOID
      && !pkl_ast_type_equal (returned_type, expected_type))
    {
      int restart = 0;
      
      switch (PKL_AST_TYPE_CODE (expected_type))
        {
        case PKL_TYPE_ANY:
        case PKL_TYPE_ARRAY: /* any[] */
          /* Nothing to do.  */
          break;
        case PKL_TYPE_INTEGRAL:
          if (!promote_integral (PKL_PASS_AST,
                                 PKL_AST_TYPE_I_SIZE (expected_type),
                                 PKL_AST_TYPE_I_SIGNED (expected_type),
                                 &PKL_AST_RETURN_STMT_EXP (return_stmt),
                                 &restart))
                {
                  pkl_ice (PKL_PASS_AST, PKL_AST_LOC (exp),
                           "couldn't promote return expression");
                  PKL_PASS_ERROR;
                }
              break;
        case PKL_TYPE_OFFSET:
              if (!promote_offset (PKL_PASS_AST,
                                   PKL_AST_TYPE_O_BASE_TYPE (expected_type),
                                   PKL_AST_TYPE_O_UNIT (expected_type),
                                   &PKL_AST_RETURN_STMT_EXP (return_stmt),
                                   &restart))
                {
                  pkl_ice (PKL_PASS_AST, PKL_AST_LOC (exp),
                           "couldn't promote return expression");
                  PKL_PASS_ERROR;
                }
              break;
        default:
          pkl_ice (PKL_PASS_AST, PKL_AST_LOC (return_stmt),
                   "return statement non-promoteable arguments at promo time");
          PKL_PASS_ERROR;
          break;
        }

      PKL_PASS_RESTART = restart;
    }
}
PKL_PHASE_END_HANDLER

/* Promote arguments to printf.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_print_stmt)
{
  pkl_ast_node print_stmt = PKL_PASS_NODE;
  pkl_ast_node print_stmt_types = PKL_AST_PRINT_STMT_TYPES (print_stmt);
  pkl_ast_node print_stmt_args = PKL_AST_PRINT_STMT_ARGS (print_stmt);

  pkl_ast_node type, arg;

  for (arg = print_stmt_args, type = print_stmt_types;
       arg && type;
       arg = PKL_AST_CHAIN (arg), type = PKL_AST_CHAIN (type))
    {
      pkl_ast_node arg_exp = PKL_AST_PRINT_STMT_ARG_EXP (arg);
      pkl_ast_node exp_type = PKL_AST_TYPE (arg_exp);

      if (PKL_AST_TYPE_CODE (exp_type) == PKL_TYPE_INTEGRAL)
        {
          int restart = 0;

          if (!promote_integral (PKL_PASS_AST,
                                 PKL_AST_TYPE_I_SIZE (type),
                                 PKL_AST_TYPE_I_SIGNED (type),
                                 &PKL_AST_PRINT_STMT_ARG_EXP (arg),
                                 &restart))
            {
              pkl_ice (PKL_PASS_AST, PKL_AST_LOC (arg),
                       "couldn't promote printf argument initializer");
                  PKL_PASS_ERROR;
            }

          PKL_PASS_RESTART = PKL_PASS_RESTART || restart;
        }
    }
}
PKL_PHASE_END_HANDLER

/* Promote function argument initializers, if needed.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_func_arg)
{
  pkl_ast_node func_arg = PKL_PASS_NODE;
  pkl_ast_node initial = PKL_AST_FUNC_ARG_INITIAL (func_arg);

  if (initial)
    {
      pkl_ast_node arg_type = PKL_AST_FUNC_ARG_TYPE (func_arg);
      pkl_ast_node initial_type = PKL_AST_TYPE (initial);

      if (!pkl_ast_type_equal (arg_type, initial_type))
        {
          int restart = 0;

          switch (PKL_AST_TYPE_CODE (arg_type))
            {
            case PKL_TYPE_ANY:
            case PKL_TYPE_ARRAY: /* any[] */
              /* Nothing to do.  */
              break;
            case PKL_TYPE_INTEGRAL:
              if (!promote_integral (PKL_PASS_AST,
                                     PKL_AST_TYPE_I_SIZE (arg_type),
                                     PKL_AST_TYPE_I_SIGNED (arg_type),
                                     &PKL_AST_FUNC_ARG_INITIAL (func_arg),
                                     &restart))
                {
                  pkl_ice (PKL_PASS_AST, PKL_AST_LOC (initial),
                           "couldn't promote argument initializer");
                  PKL_PASS_ERROR;
                }
              break;
            case PKL_TYPE_OFFSET:
              if (!promote_offset (PKL_PASS_AST,
                                   PKL_AST_TYPE_O_BASE_TYPE (arg_type),
                                   PKL_AST_TYPE_O_UNIT (arg_type),
                                   &PKL_AST_FUNC_ARG_INITIAL (func_arg),
                                   &restart))
                {
                  pkl_ice (PKL_PASS_AST, PKL_AST_LOC (initial),
                           "couldn't promote argument initializer");
                  PKL_PASS_ERROR;
                }
              break;
            default:
              pkl_ice (PKL_PASS_AST, PKL_AST_LOC (initial),
                       "non-promoteable function argument initializer at promo time");
              PKL_PASS_ERROR;
            }

          PKL_PASS_RESTART = restart;
        }
    }
}
PKL_PHASE_END_HANDLER

/* The base type of an offset in a map should be promoted to uint<64>.
   This is expected by the code generator.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_map)
{
  pkl_ast_node map = PKL_PASS_NODE;
  pkl_ast_node map_offset = PKL_AST_MAP_OFFSET (map);
  pkl_ast_node map_offset_type = PKL_AST_TYPE (map_offset);

  pkl_ast_node to_magnitude_type;
  int restart;

  to_magnitude_type = pkl_ast_make_integral_type (PKL_PASS_AST,
                                                  64, 0);
  PKL_AST_LOC (to_magnitude_type) = PKL_AST_LOC (map_offset);

  if (!promote_offset (PKL_PASS_AST,
                       to_magnitude_type,
                       PKL_AST_TYPE_O_UNIT (map_offset_type),
                       &PKL_AST_MAP_OFFSET (map),
                       &restart))
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (map_offset),
               "couldn't promote offset of map #%" PRIu64,
               PKL_AST_UID (map));
      PKL_PASS_ERROR;
    }

  PKL_PASS_RESTART = restart;
}
PKL_PHASE_END_HANDLER

/* Element constraints in struct types are promoteable to
   booleans.  */

PKL_PHASE_BEGIN_HANDLER (pkl_promo_ps_struct_elem_type)
{
  pkl_ast_node elem = PKL_PASS_NODE;
  pkl_ast_node elem_constraint = PKL_AST_STRUCT_ELEM_TYPE_CONSTRAINT (elem);
  pkl_ast_node elem_label = PKL_AST_STRUCT_ELEM_TYPE_LABEL (elem);

  if (elem_constraint)
    {
      pkl_ast_node constraint_type = PKL_AST_TYPE (elem_constraint);
      int restart = 0;
        
      switch (PKL_AST_TYPE_CODE (constraint_type))
        {
        case PKL_TYPE_INTEGRAL:
          if (!promote_integral (PKL_PASS_AST, 32, 1,
                                 &PKL_AST_STRUCT_ELEM_TYPE_CONSTRAINT (elem),
                                 &restart))
            {
              pkl_ice (PKL_PASS_AST, PKL_AST_LOC (elem_constraint),
                       "couldn't promote struct element constraint");
              PKL_PASS_ERROR;
            }
          break;
        default:
          pkl_ice (PKL_PASS_AST, PKL_AST_LOC (elem_constraint),
                   "non-promoteable struct element constraint at promo time");
          PKL_PASS_ERROR;
          break;
        }        

      PKL_PASS_RESTART = restart;
    }

  if (elem_label)
    {
      pkl_ast_node label_type = PKL_AST_TYPE (elem_label);
      int restart = 0;
        
      switch (PKL_AST_TYPE_CODE (label_type))
        {
        case PKL_TYPE_OFFSET:
          {
            pkl_ast_node to_magnitude_type
              = pkl_ast_make_integral_type (PKL_PASS_AST,
                                            64, 0);
            PKL_AST_LOC (to_magnitude_type) = PKL_AST_LOC (elem_label);
            
            if (!promote_offset (PKL_PASS_AST,
                                 to_magnitude_type,
                                 PKL_AST_TYPE_O_UNIT (label_type),
                                 &PKL_AST_STRUCT_ELEM_TYPE_LABEL (elem),
                                 &restart))
              {
                pkl_ice (PKL_PASS_AST, PKL_AST_LOC (elem_label),
                         "couldn't promote struct element label");
                PKL_PASS_ERROR;
              }
            break;
          }
        default:
          pkl_ice (PKL_PASS_AST, PKL_AST_LOC (elem_label),
                   "non-promoteable struct element label at promo time");
          PKL_PASS_ERROR;
          break;
        }        

      PKL_PASS_RESTART = PKL_PASS_RESTART || restart;
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_promo =
  {
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_EQ, pkl_promo_ps_op_rela),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_NE, pkl_promo_ps_op_rela),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_LT, pkl_promo_ps_op_rela),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_GT, pkl_promo_ps_op_rela),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_LE, pkl_promo_ps_op_rela),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_GE, pkl_promo_ps_op_rela),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_SL, pkl_promo_ps_op_bshift),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_SR, pkl_promo_ps_op_bshift),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_IOR, pkl_promo_ps_op_binary),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_XOR, pkl_promo_ps_op_binary),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_BAND, pkl_promo_ps_op_binary),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_AND, pkl_promo_ps_op_binary),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_OR, pkl_promo_ps_op_binary),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_NOT, pkl_promo_ps_op_unary),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_ADD, pkl_promo_ps_op_add_sub_mod),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_SUB, pkl_promo_ps_op_add_sub_mod),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_MOD, pkl_promo_ps_op_add_sub_mod),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_MUL, pkl_promo_ps_op_mul),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_DIV, pkl_promo_ps_op_div),
   PKL_PHASE_PS_HANDLER (PKL_AST_FUNC_ARG, pkl_promo_ps_func_arg),
   PKL_PHASE_PS_HANDLER (PKL_AST_MAP, pkl_promo_ps_map),
   PKL_PHASE_PS_HANDLER (PKL_AST_INDEXER, pkl_promo_ps_indexer),
   PKL_PHASE_PS_HANDLER (PKL_AST_TRIMMER, pkl_promo_ps_trimmer),
   PKL_PHASE_PS_HANDLER (PKL_AST_ARRAY_INITIALIZER, pkl_promo_ps_array_initializer),
   PKL_PHASE_PS_HANDLER (PKL_AST_RAISE_STMT, pkl_promo_ps_raise_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_TRY_CATCH_STMT, pkl_promo_ps_try_catch_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_FUNCALL, pkl_promo_ps_funcall),
   PKL_PHASE_PS_HANDLER (PKL_AST_ASS_STMT, pkl_promo_ps_ass_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_RETURN_STMT, pkl_promo_ps_return_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_PRINT_STMT, pkl_promo_ps_print_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT_ELEM_TYPE, pkl_promo_ps_struct_elem_type),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_promo_ps_type_array),
  };
