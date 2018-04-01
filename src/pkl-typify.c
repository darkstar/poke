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
   Note that this phase should be run after constant-folding.
*/

#include <config.h>

#include <string.h>

#include "pkl.h"
#include "pkl-ast.h"
#include "pkl-pass.h"
#include "pkl-typify.h"

/* The following handler is used in both `typify1' and `typify2'.  It
   initializes the payload.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify_bf_program)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;

  payload->errors = 0;
}
PKL_PHASE_END_HANDLER

/* The type of an unary operation NOT or a binary operation EQ, NE,
   LT, GT, LE, GE, AND and OR is a boolean encoded as a 32-bit signed
   integer type.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_op_boolean)
{
  pkl_ast_node type
    = pkl_ast_make_integral_type (PKL_PASS_AST, 32, 1);

  PKL_AST_TYPE (PKL_PASS_NODE)
    = ASTREF (type);
  PKL_AST_LOC (type) = PKL_AST_LOC (PKL_PASS_NODE);
}
PKL_PHASE_END_HANDLER

/* The type of an unary operation NEG, POS, BNOT is the type of its
   single operand.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_first_operand)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node type
    = PKL_AST_TYPE (PKL_AST_EXP_OPERAND (exp, 0));
  
  PKL_AST_TYPE (exp) = ASTREF (type);
}
PKL_PHASE_END_HANDLER

/* The type of a CAST is the type of its target type.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_cast)
{
  pkl_ast_node cast = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_CAST_TYPE (cast);
  
  PKL_AST_TYPE (cast) = ASTREF (type);
}
PKL_PHASE_END_HANDLER

/* When applied to integral arguments, the type of a binary operation
   ADD, SUB, MUL, DIV, MOD, SL, SR, IOR, XOR and BAND is an integral
   type with the following characteristics: if any of the operands is
   unsigned, the operation is unsigned.  The width of the operation is
   the width of the widest operand.

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
      case PKL_TYPE_INTEGRAL:                                           \
        {                                                               \
          int signed_p = (PKL_AST_TYPE_I_SIGNED (t1)                    \
                          && PKL_AST_TYPE_I_SIGNED (t2));               \
          int size                                                      \
            = (PKL_AST_TYPE_I_SIZE (t1) > PKL_AST_TYPE_I_SIZE (t2)      \
               ? PKL_AST_TYPE_I_SIZE (t1) : PKL_AST_TYPE_I_SIZE (t2));  \
                                                                        \
          type = pkl_ast_make_integral_type (PKL_PASS_AST, size, signed_p); \
          PKL_AST_LOC (type) = PKL_AST_LOC (exp);                       \
          break;                                                        \
        }                                                               \
      default:                                                          \
        goto error;                                                     \
        break;                                                          \
      }                                                                 \
                                                                        \
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

/* ADD and SUB accept integral, string and offset operands.  */

#define CASE_STR                                                        \
    case PKL_TYPE_STRING:                                               \
      type = pkl_ast_make_string_type (PKL_PASS_AST);                   \
      PKL_AST_LOC (exp) = PKL_AST_LOC (PKL_PASS_NODE);                  \
      break;

#define CASE_OFFSET                             \
  case PKL_TYPE_OFFSET:                         \
  {                                             \
    type = NULL;                                \
    break;                                      \
  }

TYPIFY_BIN (add);
TYPIFY_BIN (sub);

/* The following operations only accept integers.  */

#undef CASE_STR
#define CASE_STR

#undef CASE_OFFSET
#define CASE_OFFSET

TYPIFY_BIN (mul);
TYPIFY_BIN (div);
TYPIFY_BIN (mod);
TYPIFY_BIN (sl);
TYPIFY_BIN (sr);
TYPIFY_BIN (ior);
TYPIFY_BIN (xor);
TYPIFY_BIN (band);

#undef TYPIFY_BIN

/* The type of a SIZEOF operation is an offset type with an unsigned
   64-bit magnitude and units bits.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_op_sizeof)
{
  pkl_ast_node itype
    = pkl_ast_make_integral_type (PKL_PASS_AST,
                                 64, 0);
  pkl_ast_node type
    = pkl_ast_make_offset_type (PKL_PASS_AST,
                                itype, PKL_AST_OFFSET_UNIT_BITS);

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
}
PKL_PHASE_END_HANDLER

/* The type of an ARRAY is determined from the number and the type of
   its initializers.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_array)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;

  /* Check that the types of all the array elements are the same, and
     derive the type of the array from the first of them.  */

  pkl_ast_node array = PKL_PASS_NODE;
  pkl_ast_node initializers = PKL_AST_ARRAY_INITIALIZERS (array);
  
  pkl_ast_node tmp, type = NULL, array_nelem, array_nelem_type;

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
        =  PKL_AST_TYPE (t);

      struct_elem_types = pkl_ast_chainon (struct_elem_types,
                                           ASTREF (struct_elem_type));
    }

  /* Build the type of the struct.  */
  type = pkl_ast_make_struct_type (PKL_PASS_AST,
                                   PKL_AST_STRUCT_NELEM (node),
                                   struct_elem_types);
  PKL_AST_LOC (type) = PKL_AST_LOC (node);
  PKL_AST_TYPE (node) = ASTREF (type);
}
PKL_PHASE_END_HANDLER

/* The type of a STRUCT_ELEM in a struct initializer is the type of
   it's expression.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_struct_elem)
{
  pkl_ast_node struct_elem = PKL_PASS_NODE;
  pkl_ast_node struct_elem_name
    = PKL_AST_STRUCT_ELEM_NAME (struct_elem);
  pkl_ast_node struct_elem_exp
    = PKL_AST_STRUCT_ELEM_EXP (struct_elem);
  pkl_ast_node struct_elem_exp_type
    = PKL_AST_TYPE (struct_elem_exp);
  
  pkl_ast_node type
    = pkl_ast_make_struct_elem_type (PKL_PASS_AST,
                                     struct_elem_name,
                                     struct_elem_exp_type);

  PKL_AST_LOC (type) = PKL_AST_LOC (struct_elem);
  PKL_AST_TYPE (struct_elem) = ASTREF (type);
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
      
      if (strcmp (PKL_AST_IDENTIFIER_POINTER (struct_elem_type_name),
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
}
PKL_PHASE_END_HANDLER

/* The array sizes in array type literals should be integer
   expressions.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify1_df_type_array)
{
  pkl_typify_payload payload
    = (pkl_typify_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node nelem = PKL_AST_TYPE_A_NELEM (PKL_PASS_NODE);
  pkl_ast_node nelem_type = PKL_AST_TYPE (nelem);

  if (PKL_AST_TYPE_CODE (nelem_type) != PKL_TYPE_INTEGRAL)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (nelem),
                 "an array type size should be an integral value");
      payload->errors++;
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_typify1 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_typify_bf_program),

   PKL_PHASE_DF_HANDLER (PKL_AST_CAST, pkl_typify1_df_cast),
   PKL_PHASE_DF_HANDLER (PKL_AST_OFFSET, pkl_typify1_df_offset),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY, pkl_typify1_df_array),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY_REF, pkl_typify1_df_array_ref),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT, pkl_typify1_df_struct),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT_ELEM, pkl_typify1_df_struct_elem),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT_REF, pkl_typify1_df_struct_ref),

   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SIZEOF, pkl_typify1_df_op_sizeof),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NOT, pkl_typify1_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_EQ, pkl_typify1_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NE, pkl_typify1_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LT, pkl_typify1_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GT, pkl_typify1_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LE, pkl_typify1_df_op_boolean),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GE, pkl_typify1_df_op_boolean),
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



/* An array type is considered complete if the number of elements
   contained in the array is known, and it is constant.  */

PKL_PHASE_BEGIN_HANDLER (pkl_typify2_df_type_array)
{
  pkl_ast_node type = PKL_PASS_NODE;
  pkl_ast_node nelem = PKL_AST_TYPE_A_NELEM (type);

  int complete = PKL_AST_TYPE_COMPLETE_NO;

  if (PKL_AST_LITERAL_P (nelem))
    complete = PKL_AST_TYPE_COMPLETE_YES;

  PKL_AST_TYPE_COMPLETE (type) = complete;
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_typify2_df_type_struct)
{
  PKL_AST_TYPE_COMPLETE (PKL_PASS_NODE) = PKL_AST_TYPE_COMPLETE_YES;
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

  /* XXX: the logic in this switch is duplicated from the other rules.
     Abstract it in a single function.  */
  switch (PKL_AST_TYPE_CODE (op))
    {
    case PKL_TYPE_INTEGRAL:
    case PKL_TYPE_OFFSET:
    case PKL_TYPE_STRUCT: /* XXX: this will change.  */
      PKL_AST_TYPE_COMPLETE (op)
        = PKL_AST_TYPE_COMPLETE_YES;
      break;
    case PKL_TYPE_STRING:
      PKL_AST_TYPE_COMPLETE (op)
        = PKL_AST_TYPE_COMPLETE_NO;
      break;
    case PKL_TYPE_ARRAY:
      {
        pkl_ast_node nelem
          = PKL_AST_TYPE_A_NELEM (op);

        int complete = PKL_AST_TYPE_COMPLETE_NO;

        if (PKL_AST_LITERAL_P (nelem))
          complete = PKL_AST_TYPE_COMPLETE_YES;

        PKL_AST_TYPE_COMPLETE (op) = complete;
        break;
      }
    default:
      break;
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_typify2 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_typify_bf_program),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_typify2_df_type_array),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_typify2_df_type_struct),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SIZEOF, pkl_typify2_df_op_sizeof),
  };
