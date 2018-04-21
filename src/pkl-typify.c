/* pkl-typify.c - Type annotation phases for the poke compiler.  */

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

/* This file contains the implementation of two compiler phases:

   `typify1' annotates expression nodes in the AST with their
   respective types, according to the rules documented in the handlers
   below.  It also performs type-checking.  It relies on the lexer and
   previous phases to set the types for INTEGER, CHAR, STRING and
   other entities, and propagates that information up the AST.

   `typify2' determines which types are "complete" and annotates the
   type nodes accordingly, for EXP nodes whose type-completeness has
   not been already determined in the lexer or indirectly, by
   propagating types, in typify1: namely, ARRAYs and STRUCTs.  A type
   if complete if its size in bits can be determined at compile-time,
   and that size is constant.  Note that not-complete types are legal
   poke entities, but certain operations are not allowed on them.
*/

#include <config.h>

#include <string.h>

#include "pkl.h"
#include "pkl-ast.h"
#include "pkl-pass.h"
#include "pkl-typify.h"

/* Note the following macro evaluates the arguments twice!  */
#define MAX(A,B) ((A) > (B) ? (A) : (B))

/* The following handler is used in both `typify1' and `typify2'.  It
   initializes the payload.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify_bf_program)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;

  payload->errors = 0;
}
PKL_PHASE_END_HANDLER

/* The type of a NOT is a boolean encoded as a 32-bit signed integer,
   and the type of its sole operand sould be suitable to be promoted
   to a boolean, i.e. it is an integral value.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_op_not)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node op = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);
  pkl_ast_node op_type = PKL_AST_TYPE (op);

  if (PKL_AST_TYPE_CODE (op_type) != PKL_TYPE_INTEGRAL)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (op),
                 "invalid operand to NOT");
      payload->errors++;
      PKL_PASS_ERROR;
    }
  else
    {
      pkl_ast_node exp_type
        = pkl_ast_make_integral_type (PKL_PASS_AST, 32, 1);

      PKL_AST_LOC (exp_type) = PKL_AST_LOC (PKL_PASS_NODE);
      PKL_AST_TYPE (PKL_PASS_NODE) = ASTREF (exp_type);
    }
}
PKL_PHASE_END_HANDLER

/* The type of the relational operations EQ, NE, LT, GT, LE and GE is
   a boolean encoded as a 32-bit signed integer type.  Their operands
   should be either both integral types, or strings, or offsets.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_op_rela)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);
  pkl_ast_node op1_type = PKL_AST_TYPE (op1);
  int op1_type_code = PKL_AST_TYPE_CODE (op1_type);

  pkl_ast_node op2 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 1);
  pkl_ast_node op2_type = PKL_AST_TYPE (op2);
  int op2_type_code = PKL_AST_TYPE_CODE (op2_type);

  if (op1_type_code == op2_type_code
      && (op1_type_code == PKL_TYPE_INTEGRAL
          || op1_type_code == PKL_TYPE_STRING
          || op1_type_code == PKL_TYPE_OFFSET))
    {
      pkl_ast_node exp_type
        = pkl_ast_make_integral_type (PKL_PASS_AST, 32, 1);

      PKL_AST_LOC (exp_type) = PKL_AST_LOC (PKL_PASS_NODE);
      PKL_AST_TYPE (PKL_PASS_NODE) = ASTREF (exp_type);
    }
  else
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (PKL_PASS_NODE),
                 "invalid operands to relational operator");
      payload->errors++;
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

/* The type of a binary operation EQ, NE, LT, GT, LE, GE, AND and OR
   is a boolean encoded as a 32-bit signed integer type.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_op_boolean)
{
  pkl_ast_node type
    = pkl_ast_make_integral_type (PKL_PASS_AST, 32, 1);
  PKL_AST_LOC (type) = PKL_AST_LOC (PKL_PASS_NODE);

  PKL_AST_TYPE (PKL_PASS_NODE) = ASTREF (type);
}
PKL_PHASE_END_HANDLER

/* The type of an unary operation NEG, POS, BNOT is the type of its
   single operand.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_first_operand)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (PKL_AST_EXP_OPERAND (exp, 0));
  
  PKL_AST_TYPE (exp) = ASTREF (type);
}
PKL_PHASE_END_HANDLER

/* The type of a CAST is the type of its target type.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_cast)
{
  pkl_ast_node cast = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_CAST_TYPE (cast);
  
  PKL_AST_TYPE (cast) = ASTREF (type);
  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

/* When applied to integral arguments, the type of a binary operation
   SL and SR is an integral type with the same characteristics than
   the type of the value being shifted, i.e. the first operand.

   When applied to integral arguments, the type of a binary operation
   ADD, SUB, MUL, DIV, MOD, IOR, XOR and BAND is an integral type with
   the following characteristics: if any of the operands is unsigned,
   the operation is unsigned.  The width of the operation is the width
   of the widest operand.

   When applied to strings, the type of ADD is a string.

   When applied to offsets, the type of ADD, SUB is an offset, whose
   magnitude's type is calculated following the same rules than for
   integrals.  The unit of the resulting offset is the common
   denominator of the units of the operands.

   When applied to offsets, the type of DIV is an integer, whose type
   is calculated following the same rules than for integrals.

   When applied to an offset and an integer, the type of MUL is an
   offset, whose magnitude's type is calculated following the same
   rules than for integrals.  The unit of the resulting offset is the
   same than the unit of the operand.  */

#define TYPIFY_BIN(OP)                                                  \
  PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_##OP)                         \
  {                                                                     \
    pkl_typify_payload payload                                          \
      = (pkl_typify_payload) PKL_PASS_PAYLOAD;                          \
                                                                        \
    pkl_ast_node exp = PKL_PASS_NODE;                                   \
    pkl_ast_node op1 = PKL_AST_EXP_OPERAND (exp, 0);                    \
    pkl_ast_node op2 = PKL_AST_EXP_OPERAND (exp, 1);                    \
    pkl_ast_node t1 = PKL_AST_TYPE (op1);                               \
    pkl_ast_node t2 = PKL_AST_TYPE (op2);                               \
                                                                        \
    pkl_ast_node type;                                                  \
                                                                        \
    if (PKL_AST_TYPE_CODE (t1) != PKL_AST_TYPE_CODE (t2))               \
      goto error;                                                       \
                                                                        \
    switch (PKL_AST_TYPE_CODE (t1))                                     \
      {                                                                 \
      CASE_STR                                                          \
      CASE_OFFSET                                                       \
      CASE_INTEGRAL                                                     \
      default:                                                          \
        goto error;                                                     \
        break;                                                          \
      }                                                                 \
                                                                        \
    PKL_AST_LOC (type) = PKL_AST_LOC (exp);                             \
    PKL_AST_TYPE (exp) = ASTREF (type);                                 \
    PKL_PASS_DONE;                                                      \
                                                                        \
  error:                                                                \
    pkl_error (PKL_PASS_AST, PKL_AST_LOC (exp),                         \
               "invalid operands in expression");                       \
    payload->errors++;                                                  \
    PKL_PASS_ERROR;                                                     \
  }                                                                     \
  PKL_PHASE_END_HANDLER

/* The following operations only accept integers.  */

#define CASE_STR
#define CASE_OFFSET

#define CASE_INTEGRAL                                                   \
  case PKL_TYPE_INTEGRAL:                                               \
  {                                                                     \
    int signed_p = PKL_AST_TYPE_I_SIGNED (t1);                          \
    int size = PKL_AST_TYPE_I_SIZE (t1);                                \
                                                                        \
    type = pkl_ast_make_integral_type (PKL_PASS_AST, size, signed_p);   \
    break;                                                              \
  }

TYPIFY_BIN (sl);
TYPIFY_BIN (sr);

#undef CASE_INTEGRAL
#define CASE_INTEGRAL                                                   \
  case PKL_TYPE_INTEGRAL:                                               \
  {                                                                     \
    int signed_p = (PKL_AST_TYPE_I_SIGNED (t1)                          \
                    && PKL_AST_TYPE_I_SIGNED (t2));                     \
    int size = MAX (PKL_AST_TYPE_I_SIZE (t1),                           \
                    PKL_AST_TYPE_I_SIZE (t2));                          \
                                                                        \
    type = pkl_ast_make_integral_type (PKL_PASS_AST, size, signed_p);   \
    break;                                                              \
  }

TYPIFY_BIN (ior);
TYPIFY_BIN (xor);
TYPIFY_BIN (band);

/* DIV, MOD and SUB accept integral and offset operands.  */

#undef CASE_OFFSET
#define CASE_OFFSET                                                     \
  case PKL_TYPE_OFFSET:                                                 \
  {                                                                     \
    pkl_ast_node base_type_1 = PKL_AST_TYPE_O_BASE_TYPE (t1);           \
    pkl_ast_node base_type_2 = PKL_AST_TYPE_O_BASE_TYPE (t2);           \
                                                                        \
    if (PKL_AST_EXP_CODE (exp) == PKL_AST_OP_DIV)                       \
      {                                                                 \
        size_t base_type_1_size = PKL_AST_TYPE_I_SIZE (base_type_1);    \
        size_t base_type_2_size = PKL_AST_TYPE_I_SIZE (base_type_2);    \
        int base_type_1_signed = PKL_AST_TYPE_I_SIGNED (base_type_1);   \
        int base_type_2_signed = PKL_AST_TYPE_I_SIGNED (base_type_2);   \
                                                                        \
        int signed_p = (base_type_1_signed && base_type_2_signed);      \
        int size = MAX (base_type_1_size, base_type_2_size);            \
                                                                        \
        type = pkl_ast_make_integral_type (PKL_PASS_AST,                \
                                           size, signed_p);             \
      }                                                                 \
    else if (PKL_AST_EXP_CODE (exp) == PKL_AST_OP_MOD)                  \
      {                                                                 \
        type = pkl_ast_make_offset_type (PKL_PASS_AST,                  \
                                         base_type_1,                   \
                                         PKL_AST_TYPE_O_UNIT (t2));     \
      }                                                                 \
    else                                                                \
      assert (0);                                                       \
    break;                                                              \
  }

TYPIFY_BIN (div);
TYPIFY_BIN (mod);

/* SUB accepts integrals and offsets.  */

#undef CASE_OFFSET
#define CASE_OFFSET                                                     \
  case PKL_TYPE_OFFSET:                                                 \
  {                                                                     \
    pkl_ast_node base_type_1 = PKL_AST_TYPE_O_BASE_TYPE (t1);           \
    pkl_ast_node base_type_2 = PKL_AST_TYPE_O_BASE_TYPE (t2);           \
                                                                        \
    /* Promotion rules work like in integral operations.  */            \
    int signed_p = (PKL_AST_TYPE_I_SIGNED (base_type_1)                 \
                    && PKL_AST_TYPE_I_SIGNED (base_type_2));            \
    int size                                                            \
      = MAX (PKL_AST_TYPE_I_SIZE (base_type_1),                         \
             PKL_AST_TYPE_I_SIZE (base_type_2));                        \
                                                                        \
    pkl_ast_node base_type                                              \
      = pkl_ast_make_integral_type (PKL_PASS_AST, size, signed_p);      \
    PKL_AST_LOC (base_type) = PKL_AST_LOC (exp);                        \
                                                                        \
    /* Use bits for now.  */                                            \
    pkl_ast_node unit_type                                              \
      = pkl_ast_make_integral_type (PKL_PASS_AST, 64, 0);               \
    PKL_AST_LOC (unit_type) = PKL_AST_LOC (exp);                        \
                                                                        \
    pkl_ast_node unit                                                   \
      = pkl_ast_make_integer (PKL_PASS_AST, 1);                         \
    PKL_AST_LOC (unit) = PKL_AST_LOC (exp);                             \
                                                                        \
    PKL_AST_TYPE (unit) = ASTREF (unit_type);                           \
                                                                        \
    type = pkl_ast_make_offset_type (PKL_PASS_AST,                      \
                                     base_type,                         \
                                     unit);                             \
    break;                                                              \
  }

TYPIFY_BIN (sub);

/* ADD accepts integral, string and offset operands.  */

#undef CASE_STR
#define CASE_STR                                                        \
    case PKL_TYPE_STRING:                                               \
      type = pkl_ast_make_string_type (PKL_PASS_AST);                   \
      break;

TYPIFY_BIN (add);

/* MUL accepts integral, string and offset operands.  We can't use
   TYPIFY_BIN here because it relies on a different logic to determine
   the result type.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_mul)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;
    
  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (exp, 0);
  pkl_ast_node op2 = PKL_AST_EXP_OPERAND (exp, 1);
  pkl_ast_node t1 = PKL_AST_TYPE (op1);
  pkl_ast_node t2 = PKL_AST_TYPE (op2);
  int t1_code = PKL_AST_TYPE_CODE (t1);
  int t2_code = PKL_AST_TYPE_CODE (t2);

    
  pkl_ast_node type;

  if (t1_code == PKL_TYPE_OFFSET || t2_code == PKL_TYPE_OFFSET)
    {
      pkl_ast_node offset_type;
      pkl_ast_node int_type;
      pkl_ast_node offset_base_type;
      int signed_p;
      size_t size;

      /* One operand must be an offset, the other an integral */
      if (t1_code == PKL_TYPE_INTEGRAL && t2_code == PKL_TYPE_OFFSET)
        {
          offset_type = t2;
          int_type = t1;
        }
      else if (t1_code == PKL_TYPE_OFFSET && t2_code == PKL_TYPE_INTEGRAL)
        {
          offset_type = t1;
          int_type = t2;
        }
      else
        goto error;

      offset_base_type = PKL_AST_TYPE_O_BASE_TYPE (offset_type);

      /* Promotion rules work like in integral operations.  */
      signed_p = (PKL_AST_TYPE_I_SIGNED (offset_base_type)
                  && PKL_AST_TYPE_I_SIGNED (int_type));
      size = MAX (PKL_AST_TYPE_I_SIZE (offset_base_type),
                  PKL_AST_TYPE_I_SIZE (int_type));

      pkl_ast_node res_base_type
        = pkl_ast_make_integral_type (PKL_PASS_AST, size, signed_p);
      PKL_AST_LOC (res_base_type) = PKL_AST_LOC (exp);

      /* The unit of the result is the unit of the offset operand */
      type = pkl_ast_make_offset_type (PKL_PASS_AST,
                                       res_base_type,
                                       PKL_AST_TYPE_O_UNIT (offset_type));
    }
  else
    {
      if (PKL_AST_TYPE_CODE (t1) != PKL_AST_TYPE_CODE (t2))
        goto error;

      switch (PKL_AST_TYPE_CODE (t1))
        {
          CASE_STR
            CASE_INTEGRAL        
        default:
          goto error;
          break;
        }
    }

  PKL_AST_LOC (type) = PKL_AST_LOC (exp);
  PKL_AST_TYPE (exp) = ASTREF (type);
  PKL_PASS_DONE;

 error:
  pkl_error (PKL_PASS_AST, PKL_AST_LOC (exp),
             "invalid operands in expression");
  payload->errors++;
  PKL_PASS_ERROR;
}
PKL_PHASE_END_HANDLER

#undef CASE_INTEGRAL
#undef CASE_STR
#undef CAST_OFFSET
#undef TYPIFY_BIN

/* The type of a SIZEOF operation is an offset type with an unsigned
   64-bit magnitude and units bits.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_op_sizeof)
{
  pkl_ast_node itype
    = pkl_ast_make_integral_type (PKL_PASS_AST,
                                  64, 0);
  pkl_ast_node unit
    = pkl_ast_make_integer (PKL_PASS_AST, PKL_AST_OFFSET_UNIT_BITS);
    
  pkl_ast_node type
    = pkl_ast_make_offset_type (PKL_PASS_AST, itype, unit);

  PKL_AST_TYPE (unit) = ASTREF (itype);
  PKL_AST_LOC (unit) = PKL_AST_LOC (PKL_PASS_NODE);
  PKL_AST_LOC (itype) = PKL_AST_LOC (PKL_PASS_NODE);
  PKL_AST_LOC (type) = PKL_AST_LOC (PKL_PASS_NODE);
  PKL_AST_TYPE (PKL_PASS_NODE) = ASTREF (type);
}
PKL_PHASE_END_HANDLER

/* The type of an offset is an offset type featuring the type of its
   magnitude, and its unit.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_offset)
{
  pkl_ast_node offset = PKL_PASS_NODE;
  pkl_ast_node magnitude_type
    = PKL_AST_TYPE (PKL_AST_OFFSET_MAGNITUDE (offset));
  pkl_ast_node type
    = pkl_ast_make_offset_type (PKL_PASS_AST,
                                magnitude_type,
                                PKL_AST_OFFSET_UNIT (offset));

  PKL_AST_LOC (type) = PKL_AST_LOC (offset);
  PKL_AST_TYPE (offset) = ASTREF (type);
  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

/* The type of an ARRAY is determined from the number and the type of
   its initializers.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_array)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node array = PKL_PASS_NODE;
  pkl_ast_node initializers = PKL_AST_ARRAY_INITIALIZERS (array);
  
  pkl_ast_node tmp, type = NULL, array_nelem, array_nelem_type;

  /* Check that the types of all the array elements are the same, and
     derive the type of the array from the first of them.  */
  for (tmp = initializers; tmp; tmp = PKL_AST_CHAIN (tmp))
    {
      pkl_ast_node initializer = PKL_AST_ARRAY_INITIALIZER_EXP (tmp);

      if (type == NULL)
        type = PKL_AST_TYPE (initializer);
      else if (!pkl_ast_type_equal (PKL_AST_TYPE (initializer), type))
        {
          pkl_error (PKL_PASS_AST, PKL_AST_LOC (array),
                     "array initializers should be of the same type");
          payload->errors++;
          PKL_PASS_ERROR;
        }        
    }

  /* Build the type of the array. */
  array_nelem = pkl_ast_make_integer (PKL_PASS_AST,
                                      PKL_AST_ARRAY_NELEM (array));
  PKL_AST_LOC (array_nelem) = PKL_AST_LOC (PKL_PASS_NODE);

  array_nelem_type = pkl_ast_make_integral_type (PKL_PASS_AST,
                                                 64, 0);
  PKL_AST_LOC (array_nelem_type) = PKL_AST_LOC (PKL_PASS_NODE);
  
  PKL_AST_TYPE (array_nelem) = ASTREF (array_nelem_type);

  type = pkl_ast_make_array_type (PKL_PASS_AST,
                                  array_nelem, type);
  PKL_AST_LOC (type) = PKL_AST_LOC (PKL_PASS_NODE);
  PKL_AST_TYPE (array) = ASTREF (type);

  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

/* The type of an ARRAY_REF is the type of the elements of the array
   it references.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_array_ref)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node array_ref = PKL_PASS_NODE;
  pkl_ast_node array_ref_index
    = PKL_AST_ARRAY_REF_INDEX (array_ref);
  pkl_ast_node array_ref_array
    = PKL_AST_ARRAY_REF_ARRAY (array_ref);

  pkl_ast_node type;

  if (PKL_AST_TYPE_CODE (PKL_AST_TYPE (array_ref_array))
      != PKL_TYPE_ARRAY)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (array_ref_array),
                 "operator to [] must be an array");
      payload->errors++;
      PKL_PASS_ERROR;
    }

  if (PKL_AST_TYPE_CODE (PKL_AST_TYPE (array_ref_index))
      != PKL_TYPE_INTEGRAL)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (array_ref_index),
                 "array index should be an integer");
      PKL_PASS_ERROR;
    }

  type
    = PKL_AST_TYPE_A_ETYPE (PKL_AST_TYPE (array_ref_array));
  PKL_AST_TYPE (array_ref) = ASTREF (type);

  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

/* The type of a STRUCT is determined from the types of its
   elements.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_struct)
{
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type;
  pkl_ast_node t, struct_elem_types = NULL;


  /* Build a chain with the types of the struct elements.  */
  for (t = PKL_AST_STRUCT_ELEMS (node); t; t = PKL_AST_CHAIN (t))
    {
      pkl_ast_node struct_elem_type
        =  pkl_ast_make_struct_elem_type (PKL_PASS_AST,
                                          PKL_AST_STRUCT_ELEM_NAME (t),
                                          PKL_AST_TYPE (t));
      PKL_AST_LOC (struct_elem_type) = PKL_AST_LOC (t);

      struct_elem_types = pkl_ast_chainon (struct_elem_types,
                                           ASTREF (struct_elem_type));
    }

  /* Build the type of the struct.  */
  type = pkl_ast_make_struct_type (PKL_PASS_AST,
                                   PKL_AST_STRUCT_NELEM (node),
                                   struct_elem_types);
  PKL_AST_LOC (type) = PKL_AST_LOC (node);
  PKL_AST_TYPE (node) = ASTREF (type);
  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

/* The type of a FUNC is determined from the types of its
   arguments, and its return type.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_func)
{
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type;
  pkl_ast_node t, func_arg_types = NULL;
  size_t nargs = 0;

  /* Build a chain with the types of the function arguments.  */
  for (t = PKL_AST_FUNC_ARGS (node); t; t = PKL_AST_CHAIN (t))
    {
      pkl_ast_node func_arg_type
        = pkl_ast_make_func_arg_type (PKL_PASS_AST,
                                          PKL_AST_FUNC_ARG_TYPE (t));
      PKL_AST_LOC (func_arg_type) = PKL_AST_LOC (t);

      func_arg_types = pkl_ast_chainon (func_arg_types,
                                            ASTREF (func_arg_type));
      nargs++;
    }

  /* Build the type of the function.  */
  type = pkl_ast_make_function_type (PKL_PASS_AST,
                                     PKL_AST_FUNC_RET_TYPE (node),
                                     nargs, func_arg_types);
  PKL_AST_LOC (type) = PKL_AST_LOC (node);
  PKL_AST_TYPE (node) = ASTREF (type);
  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

/* The expression to which a FUNCALL is applied should be a function,
   and the types of the formal parameters should match the types of
   the actual arguments in the funcall.  Also, set the type of the
   funcall, which is the type returned by the function.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_funcall)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node funcall = PKL_PASS_NODE;
  pkl_ast_node funcall_function
    = PKL_AST_FUNCALL_FUNCTION (funcall);
  pkl_ast_node funcall_function_type
    = PKL_AST_TYPE (funcall_function);

  if (PKL_AST_TYPE_CODE (funcall_function_type)
      != PKL_TYPE_FUNCTION)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (funcall_function),
                 "variable is not a function");
      payload->errors++;
      PKL_PASS_ERROR;
    }

  /* XXX: check the types of the function and the funcall.  */
  if (PKL_AST_FUNCALL_NARG (funcall) <
      PKL_AST_TYPE_F_NARG (funcall_function_type))
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (funcall_function),
                 "too few argument passed to function");
      payload->errors++;
      PKL_PASS_ERROR;
    }
  else
    {
      pkl_ast_node fa, aa;
      int narg = 0;

      for (fa = PKL_AST_TYPE_F_ARGS  (funcall_function_type),
           aa = PKL_AST_FUNCALL_ARGS (funcall);
           fa && aa;
           fa = PKL_AST_CHAIN (fa), aa = PKL_AST_CHAIN (aa))
        {
          pkl_ast_node fa_type = PKL_AST_FUNC_ARG_TYPE (fa);
          pkl_ast_node aa_exp = PKL_AST_FUNCALL_ARG_EXP (aa);
          pkl_ast_node aa_type = PKL_AST_TYPE (aa_exp);

          assert (aa_type);

          if (!pkl_ast_type_equal (fa_type, aa_type))
            {
              char *passed_type = pkl_type_str (aa_type, 1);
              char *expected_type = pkl_type_str (fa_type, 1);

              pkl_error (PKL_PASS_AST, PKL_AST_LOC (aa),
                         "passing function argument %d of the wrong type.  Expected %s, got %s",
                         narg, expected_type, passed_type);
              free (expected_type);
              free (passed_type);
              payload->errors++;
              PKL_PASS_ERROR;
            }

          narg++;
        }
    }

  /* Set the type of the funcall itself.  */
  PKL_AST_TYPE (funcall)
    = ASTREF (PKL_AST_TYPE_F_RTYPE (funcall_function_type));

  /* If the called function is a void function, i.e. it doesn't return
     any type, the parent of this funcall shouldn't expect a
     value.  */
  {
    int parent_code = PKL_AST_CODE (PKL_PASS_PARENT);

    if (PKL_AST_TYPE (funcall) == NULL
        && (parent_code == PKL_AST_EXP
            || parent_code == PKL_AST_COND_EXP
            || parent_code == PKL_AST_ARRAY_INITIALIZER
            || parent_code == PKL_AST_ARRAY_REF
            || parent_code == PKL_AST_STRUCT_ELEM
            || parent_code == PKL_AST_OFFSET
            || parent_code == PKL_AST_CAST
            || parent_code == PKL_AST_MAP
            || parent_code == PKL_AST_FUNCALL
            || parent_code == PKL_AST_FUNCALL_ARG
            || parent_code == PKL_AST_DECL))
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (funcall_function),
                 "function doesn't return a value");
      payload->errors++;
      PKL_PASS_ERROR;
    }
  }
}
PKL_PHASE_END_HANDLER

/* The type of a STRUCT_ELEM in a struct initializer is the type of
   it's expression.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_struct_elem)
{
  pkl_ast_node struct_elem = PKL_PASS_NODE;
  pkl_ast_node struct_elem_exp
    = PKL_AST_STRUCT_ELEM_EXP (struct_elem);
  pkl_ast_node struct_elem_exp_type
    = PKL_AST_TYPE (struct_elem_exp);
  
  PKL_AST_TYPE (struct_elem) = ASTREF (struct_elem_exp_type);
  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

/* The type of a STRUCT_REF is the type of the referred element in the
   struct.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_struct_ref)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node struct_ref = PKL_PASS_NODE;
  pkl_ast_node astruct =
    PKL_AST_STRUCT_REF_STRUCT (struct_ref);
  pkl_ast_node field_name =
    PKL_AST_STRUCT_REF_IDENTIFIER (struct_ref);
  pkl_ast_node struct_type = PKL_AST_TYPE (astruct);
  pkl_ast_node t, type = NULL;

  if (PKL_AST_TYPE_CODE (struct_type) != PKL_TYPE_STRUCT)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (astruct),
                 "expected struct");
      payload->errors++;
      PKL_PASS_ERROR;
    }

  /* Search for the referred field type.  */
  for (t = PKL_AST_TYPE_S_ELEMS (struct_type); t;
       t = PKL_AST_CHAIN (t))
    {
      pkl_ast_node struct_elem_type_name
        = PKL_AST_STRUCT_ELEM_TYPE_NAME (t);
      
      if (struct_elem_type_name
          && strcmp (PKL_AST_IDENTIFIER_POINTER (struct_elem_type_name),
                     PKL_AST_IDENTIFIER_POINTER (field_name)) == 0)
        {
          type = PKL_AST_STRUCT_ELEM_TYPE_TYPE (t);
          break;
        }
    }

  if (type == NULL)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (field_name),
                 "referred field doesn't exist in struct");
      payload->errors++;
      PKL_PASS_ERROR;
    }

  PKL_AST_TYPE (struct_ref) = ASTREF (type);
  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

/* The array sizes in array type literals, if present, should be
   integer expressions.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_type_array)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node nelem = PKL_AST_TYPE_A_NELEM (PKL_PASS_NODE);
  pkl_ast_node nelem_type;

  if (nelem == NULL)
    /* This array type hasn't a number of elements.  Be done.  */
    PKL_PASS_DONE;

  nelem_type = PKL_AST_TYPE (nelem);
  if (PKL_AST_TYPE_CODE (nelem_type) != PKL_TYPE_INTEGRAL)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (nelem),
                 "an array type size should be an integral value");
      payload->errors++;
      PKL_PASS_ERROR;
    }

  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

/* The type of a map is the type of the mapped value.  The expression
   in a map should be an offset.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_map)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node map = PKL_PASS_NODE;
  pkl_ast_node map_type = PKL_AST_MAP_TYPE (map);
  pkl_ast_node map_offset = PKL_AST_MAP_OFFSET (map);
  pkl_ast_node map_offset_type = PKL_AST_TYPE (map_offset);

  if (PKL_AST_TYPE_CODE (map_offset_type) != PKL_TYPE_OFFSET)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (map_offset),
                 "expected offset");
      payload->errors++;
      PKL_PASS_ERROR;
    }

  PKL_AST_TYPE (map) = ASTREF (map_type);
}
PKL_PHASE_END_HANDLER

/* The type of a variable reference is the type of its initializer.
   Note that due to the scope rules of the language it is guaranteed
   the type of the initializer has been already calculated.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_var)
{
  pkl_ast_node var = PKL_PASS_NODE;
  pkl_ast_node initial = PKL_AST_VAR_INITIAL (var);

  assert (PKL_AST_TYPE (initial) != NULL);
  PKL_AST_TYPE (var) = ASTREF (PKL_AST_TYPE (initial));
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_typify1 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_typify_bf_program),

   PKL_PHASE_DF_HANDLER (PKL_AST_VAR, pkl_typify1_df_var),
   PKL_PHASE_DF_HANDLER (PKL_AST_CAST, pkl_typify1_df_cast),
   PKL_PHASE_DF_HANDLER (PKL_AST_MAP, pkl_typify1_df_map),
   PKL_PHASE_DF_HANDLER (PKL_AST_OFFSET, pkl_typify1_df_offset),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY, pkl_typify1_df_array),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY_REF, pkl_typify1_df_array_ref),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT, pkl_typify1_df_struct),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT_ELEM, pkl_typify1_df_struct_elem),
   PKL_PHASE_DF_HANDLER (PKL_AST_FUNC, pkl_typify1_df_func),
   PKL_PHASE_DF_HANDLER (PKL_AST_FUNCALL, pkl_typify1_df_funcall),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT_REF, pkl_typify1_df_struct_ref),

   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SIZEOF, pkl_typify1_df_op_sizeof),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NOT, pkl_typify1_df_op_not),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_EQ, pkl_typify1_df_op_rela),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NE, pkl_typify1_df_op_rela),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LT, pkl_typify1_df_op_rela),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GT, pkl_typify1_df_op_rela),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LE, pkl_typify1_df_op_rela),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GE, pkl_typify1_df_op_rela),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_AND, pkl_typify1_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_OR, pkl_typify1_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_ADD, pkl_typify1_df_add),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SUB, pkl_typify1_df_sub),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_MUL, pkl_typify1_df_mul),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_DIV, pkl_typify1_df_div),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_MOD, pkl_typify1_df_mod),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SL, pkl_typify1_df_sl),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SR, pkl_typify1_df_sr),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_IOR, pkl_typify1_df_ior),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_XOR, pkl_typify1_df_xor),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_BAND, pkl_typify1_df_band),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NEG, pkl_typify1_df_first_operand),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_POS, pkl_typify1_df_first_operand),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_BNOT, pkl_typify1_df_first_operand),

   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_typify1_df_type_array),
  };



/* Determine the completeness of a type node.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify2_df_type)
{
  pkl_ast_node type = PKL_PASS_NODE;
  PKL_AST_TYPE_COMPLETE (type) = pkl_ast_type_is_complete (type);
}
PKL_PHASE_END_HANDLER

/* Determine the completeness of the type associated with a SIZEOF
   (TYPE).  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify2_df_op_sizeof)
{
  pkl_ast_node op
    = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);

  if (PKL_AST_CODE (op) != PKL_AST_TYPE)
    /* This is a SIZEOF (VALUE).  Nothing to do.  */
    PKL_PASS_DONE;

  PKL_AST_TYPE_COMPLETE (op) = pkl_ast_type_is_complete (op);
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_typify2 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_typify_bf_program),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_typify2_df_type),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_typify2_df_type),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SIZEOF, pkl_typify2_df_op_sizeof),
  };
