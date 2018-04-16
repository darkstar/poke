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

#include "pkl.h"
#include "pkl-ast.h"
#include "pkl-pass.h"
#include "pkl-trans.h"


/* This file implements several transformation compiler phases which,
   generally speaking, are restartable.

   `trans1' finishes ARRAY, STRUCT and TYPE_STRUCT nodes by
            determining its number of elements and characteristics.
            It also finishes OFFSET nodes by replacing certain unit
            identifiers with factors.  It should be executed right
            after parsing.

   `trans2' scans the AST and annotates nodes that are literals.
            Henceforth any other phase relying on this information
            should be executed after trans2.

   `trans3' handles nodes that can be replaced for something else at
            compilation-time: SIZEOF for complete types.  This phase
            is intended to be executed short before code generation.

   See the handlers below for details.  */

/* The following handler is used in all trans phases and initializes
   the phase payload.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans_bf_program)
{
  pkl_trans_payload payload
    = (pkl_trans_payload) PKL_PASS_PAYLOAD;
  payload->errors = 0;
}
PKL_PHASE_END_HANDLER

/* Compute and set the number of elements in a STRUCT node.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_df_struct)
{
  pkl_ast_node astruct = PKL_PASS_NODE;
  pkl_ast_node t;
  size_t nelem = 0;

  for (t = PKL_AST_STRUCT_ELEMS (astruct); t; t = PKL_AST_CHAIN (t))
    nelem++;

  PKL_AST_STRUCT_NELEM (astruct) = nelem;
}
PKL_PHASE_END_HANDLER

/* Compute and set the number of elements in a struct TYPE node.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_df_type_struct)
{
  pkl_ast_node struct_type = PKL_PASS_NODE;
  pkl_ast_node t;
  size_t nelem = 0;

  for (t = PKL_AST_TYPE_S_ELEMS (struct_type); t;
       t = PKL_AST_CHAIN (t))
    nelem++;

  PKL_AST_TYPE_S_NELEM (struct_type) = nelem;
}
PKL_PHASE_END_HANDLER

/* Compute and set the indexes of all the elements of an ARRAY node
   and set the size of the array consequently.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_df_array)
{
  pkl_ast_node array = PKL_PASS_NODE;
  pkl_ast_node initializers
    = PKL_AST_ARRAY_INITIALIZERS (array);

  pkl_ast_node tmp;
  size_t index, nelem, ninitializer;

  nelem = 0;
  for (index = 0, tmp = initializers, ninitializer = 0;
       tmp;
       tmp = PKL_AST_CHAIN (tmp), ++ninitializer)
    {
      pkl_ast_node initializer_index_node
        = PKL_AST_ARRAY_INITIALIZER_INDEX (tmp);
      size_t initializer_index;
      size_t elems_appended, effective_index;

      /* Set the index of the initializer.  */
      if (initializer_index_node == NULL)
        {
          pkl_ast_node initializer_index_type
            = pkl_ast_make_integral_type (PKL_PASS_AST, 64, 0);
          PKL_AST_LOC (initializer_index_type)
            = PKL_AST_LOC (tmp);

          
          initializer_index_node
            = pkl_ast_make_integer (PKL_PASS_AST, index);
          PKL_AST_TYPE (initializer_index_node)
            = ASTREF (initializer_index_type);
          PKL_AST_LOC (initializer_index_node)
            = PKL_AST_LOC (tmp);
          
          PKL_AST_ARRAY_INITIALIZER_INDEX (tmp)
            = ASTREF (initializer_index_node);

          PKL_PASS_RESTART = 1;
          elems_appended = 1;
        }
      else
        {
          if (PKL_AST_CODE (initializer_index_node)
              != PKL_AST_INTEGER)
            {
              pkl_ice (PKL_PASS_AST, PKL_AST_NOLOC,
                       "array initialize index should be an integer node");
              PKL_PASS_ERROR;
            }

          initializer_index
            = PKL_AST_INTEGER_VALUE (initializer_index_node);

          if (initializer_index < index)
            elems_appended = 0;
          else
            elems_appended = initializer_index - index + 1;
          effective_index = initializer_index;

          PKL_AST_INTEGER_VALUE (initializer_index_node)
            = effective_index;
        }
          
      index += elems_appended;
      nelem += elems_appended;
    }

  PKL_AST_ARRAY_NELEM (array) = nelem;
  PKL_AST_ARRAY_NINITIALIZER (array) = ninitializer;
}
PKL_PHASE_END_HANDLER

/* At this point offsets can have either an identifier or a type name
   expressing its unit.  This handler takes care of the first case,
   replacing the identifier with a suitable unit factor.  If the
   identifier is invalid, then an error is raised.
   
   Also, if the magnitude of the offset wasn't specified then it
   defaults to 1. */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_df_offset)
{
  pkl_trans_payload payload
    = (pkl_trans_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node offset = PKL_PASS_NODE;
  pkl_ast_node unit = PKL_AST_OFFSET_UNIT (offset);

  if (PKL_AST_OFFSET_MAGNITUDE (offset) == NULL)
    {
      pkl_ast_node magnitude_type
        = pkl_ast_make_integral_type (PKL_PASS_AST, 32, 1);
      pkl_ast_node magnitude
        = pkl_ast_make_integer (PKL_PASS_AST, 1);

      PKL_AST_LOC (magnitude_type) = PKL_AST_LOC (offset);
      PKL_AST_LOC (magnitude) = PKL_AST_LOC (offset);
      PKL_AST_TYPE (magnitude) = ASTREF (magnitude_type);

      PKL_AST_OFFSET_MAGNITUDE (offset) = ASTREF (magnitude);
      PKL_PASS_RESTART = 1;
    }

  if (PKL_AST_CODE (unit) == PKL_AST_IDENTIFIER)
    {
      pkl_ast_node new_unit
        = pkl_ast_id_to_offset_unit (PKL_PASS_AST, unit);

      if (!new_unit)
        {
          pkl_error (PKL_PASS_AST, PKL_AST_LOC (unit),
                     "expected `b', `N', `B', `Kb', `KB', `Mb', 'MB' or `Gb'");
          payload->errors++;
          PKL_PASS_ERROR;
        }

      PKL_AST_OFFSET_UNIT (offset) = ASTREF (new_unit);
      pkl_ast_node_free (unit);
      PKL_PASS_RESTART = 1;
    }
}
PKL_PHASE_END_HANDLER

/* At this point offset types can have an identifier expressing its
   units.  This handler replaces the identifier with a suitable unit
   factor.  If the identifier is invalid, then an error is raised.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_df_type_offset)
{
  pkl_trans_payload payload
    = (pkl_trans_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node offset_type = PKL_PASS_NODE;
  pkl_ast_node unit = PKL_AST_TYPE_O_UNIT (offset_type);

  if (PKL_AST_CODE (unit) == PKL_AST_IDENTIFIER)
    {
      pkl_ast_node new_unit
        = pkl_ast_id_to_offset_unit (PKL_PASS_AST, unit);

      if (!new_unit)
        {
          pkl_error (PKL_PASS_AST, PKL_AST_LOC (unit),
                     "expected `b', `B', `Kb', `KB', `Mb', 'MB' or `Gb'");
          payload->errors++;
          PKL_PASS_ERROR;
        }

      PKL_AST_TYPE_O_UNIT (offset_type) = ASTREF (new_unit);
      pkl_ast_node_free (unit);
      PKL_PASS_RESTART = 1;
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_trans1 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_trans_bf_program),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY, pkl_trans1_df_array),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT, pkl_trans1_df_struct),
   PKL_PHASE_DF_HANDLER (PKL_AST_OFFSET, pkl_trans1_df_offset),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_trans1_df_type_struct),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_OFFSET, pkl_trans1_df_type_offset),
  };



/* Annotate expression nodes to reflect whether they are literals.
   Entities created by the lexer (INTEGER, STRING, etc) already have
   this attribute set if needed.

   Expressions having literals for operators are constant.
   Expressions having only constant operators are constant.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_df_exp)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  int o, literal_p = 1;
 
  for (o = 0; o < PKL_AST_EXP_NUMOPS (exp); ++o)
    {
      pkl_ast_node op = PKL_AST_EXP_OPERAND (exp, o);

      literal_p &= PKL_AST_LITERAL_P (op);
      if (!literal_p)
        break;
    }
}
PKL_PHASE_END_HANDLER

/* An offset is a literal if its magnitude is also a literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_df_offset)
{
  pkl_ast_node magnitude
    = PKL_AST_OFFSET_MAGNITUDE (PKL_PASS_NODE);

  PKL_AST_LITERAL_P (PKL_PASS_NODE) = PKL_AST_LITERAL_P (magnitude);
}
PKL_PHASE_END_HANDLER

/* An array is a literal if all its initializers are literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_df_array)
{
  int literal_p = 1;
  pkl_ast_node t, array = PKL_PASS_NODE;

  for (t = PKL_AST_ARRAY_INITIALIZERS (array); t;
       t = PKL_AST_CHAIN (t))
    {
      pkl_ast_node array_initializer_exp
        = PKL_AST_ARRAY_INITIALIZER_EXP (t);
      
      literal_p &= PKL_AST_LITERAL_P (array_initializer_exp);
      if (!literal_p)
        break;
    }

  PKL_AST_LITERAL_P (array) = literal_p;
}
PKL_PHASE_END_HANDLER

/* An array ref is a literal if the referred array element is also a
   literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_df_array_ref)
{
  pkl_ast_node array = PKL_AST_ARRAY_REF_ARRAY (PKL_PASS_NODE);
  PKL_AST_LITERAL_P (PKL_PASS_NODE)  = PKL_AST_LITERAL_P (array);
}
PKL_PHASE_END_HANDLER

/* A struct is a literal if all its element values are literals.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_df_struct)
{
  pkl_ast_node t;
  int literal_p = 1;
  
  for (t = PKL_AST_STRUCT_ELEMS (PKL_PASS_NODE); t;
       t = PKL_AST_CHAIN (t))
    {
      pkl_ast_node struct_elem_exp = PKL_AST_STRUCT_ELEM_EXP (t);

      literal_p &= PKL_AST_LITERAL_P (struct_elem_exp);
      if (!literal_p)
        break;
    }

  PKL_AST_LITERAL_P (PKL_PASS_NODE) = literal_p;
}
PKL_PHASE_END_HANDLER

/* A struct ref is a literal if the value of the referred element is
   also a literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_df_struct_ref)
{
  pkl_ast_node stct = PKL_AST_STRUCT_REF_STRUCT (PKL_PASS_NODE);
  PKL_AST_LITERAL_P (PKL_PASS_NODE) = PKL_AST_LITERAL_P (stct);
}
PKL_PHASE_END_HANDLER

/* A cast is considered a literal if the value of the referred element
   is also a literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_df_cast)
{
  PKL_AST_LITERAL_P (PKL_PASS_NODE)
    = PKL_AST_LITERAL_P (PKL_AST_CAST_EXP (PKL_PASS_NODE));
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_trans2 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_trans_bf_program),
   PKL_PHASE_DF_HANDLER (PKL_AST_EXP, pkl_trans2_df_exp),
   PKL_PHASE_DF_HANDLER (PKL_AST_OFFSET, pkl_trans2_df_offset),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY, pkl_trans2_df_array),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY_REF, pkl_trans2_df_array_ref),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT, pkl_trans2_df_struct),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT_REF, pkl_trans2_df_struct_ref),
   PKL_PHASE_DF_HANDLER (PKL_AST_CAST, pkl_trans2_df_cast),
  };



/* SIZEOF nodes whose operand is a complete type should be replaced
   with an offset.  The type should be complete.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans3_df_op_sizeof)
{
  pkl_trans_payload payload
    = (pkl_trans_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node op = PKL_AST_EXP_OPERAND (node, 0);
  pkl_ast_node offset, offset_type, unit, unit_type;

  if (PKL_AST_CODE (op) != PKL_AST_TYPE)
    /* This is a TYPEOF (VALUE).  Nothing to do.  */
    PKL_PASS_DONE;

  if (PKL_AST_TYPE_COMPLETE (op)
      != PKL_AST_TYPE_COMPLETE_YES)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (op),
                 "sizeof only works on complete types");
      payload->errors++;
      PKL_PASS_ERROR;
    }

  {    
    /* Calculate the size of the complete type in bytes and put it in
       an integer node.  */
    pkl_ast_node magnitude
      = pkl_ast_sizeof_type (PKL_PASS_AST, op);
    PKL_AST_LOC (magnitude) = PKL_AST_LOC (node);
    PKL_AST_LOC (PKL_AST_TYPE (magnitude)) = PKL_AST_LOC (node);
  
    /* Build an offset with that magnitude, and unit bits.  */
    unit_type = pkl_ast_make_integral_type (PKL_PASS_AST, 64, 0);
    PKL_AST_LOC (unit_type) = PKL_AST_LOC (node);

    unit = pkl_ast_make_integer (PKL_PASS_AST, PKL_AST_OFFSET_UNIT_BITS);
    PKL_AST_LOC (unit) = PKL_AST_LOC (node);
    PKL_AST_TYPE (unit) = ASTREF (unit_type);
    
    offset = pkl_ast_make_offset (PKL_PASS_AST, magnitude, unit);

    PKL_AST_LOC (offset) = PKL_AST_LOC (node);
    offset_type = pkl_ast_make_offset_type (PKL_PASS_AST,
                                            PKL_AST_TYPE (magnitude),
                                            unit);
    PKL_AST_LOC (offset_type) = PKL_AST_LOC (node);
    PKL_AST_TYPE (offset) = ASTREF (offset_type);
  }

  pkl_ast_node_free (PKL_PASS_NODE);
  PKL_PASS_NODE = offset;
  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

/* In OFFSET nodes whose units are types, these should be replaced
   with an expression that calculates their size.  This only works
   with complete types.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans3_df_offset)
{
  pkl_trans_payload payload
    = (pkl_trans_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node offset = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_OFFSET_UNIT (offset);
  pkl_ast_node unit;

  if (PKL_AST_CODE (type) != PKL_AST_TYPE)
    /* The unit of this offset is not a type.  Nothing to do.  */
    PKL_PASS_DONE;

  if (PKL_AST_TYPE_COMPLETE (type) != PKL_AST_TYPE_COMPLETE_YES)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (type),
                 "offsets only work on complete types");
      payload->errors++;
      PKL_PASS_ERROR;
    }

  /* Calculate the size of the complete type in bytes and put it in
     an integer node.  */
  unit = pkl_ast_sizeof_type (PKL_PASS_AST, type);
  PKL_AST_LOC (unit) = PKL_AST_LOC (type);
  PKL_AST_LOC (PKL_AST_TYPE (unit)) = PKL_AST_LOC (type);

  /* Replace the unit type with this expression.  */
  PKL_AST_OFFSET_UNIT (offset) = ASTREF (unit);
  pkl_ast_node_free (type);

  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

/* Ditto for offset types.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans3_df_offset_type)
{
  pkl_trans_payload payload
    = (pkl_trans_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node type = PKL_PASS_NODE;
  pkl_ast_node unit_type = PKL_AST_TYPE_O_UNIT (type);
  pkl_ast_node unit;

  if (PKL_AST_CODE (unit_type) != PKL_AST_TYPE)
    /* The unit of this offset is not a type.  Nothing to do.  */
    PKL_PASS_DONE;

  if (PKL_AST_TYPE_COMPLETE (unit_type) != PKL_AST_TYPE_COMPLETE_YES)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (unit_type),
                 "offset types only work on complete types");
      payload->errors++;
      PKL_PASS_ERROR;
    }

  /* Calculate the size of the complete type in bytes and put it in
     an integer node.  */
  unit = pkl_ast_sizeof_type (PKL_PASS_AST, unit_type);
  PKL_AST_LOC (unit) = PKL_AST_LOC (unit_type);
  PKL_AST_LOC (PKL_AST_TYPE (unit)) = PKL_AST_LOC (unit_type);

  /* Replace the unit type with this expression.  */
  PKL_AST_TYPE_O_UNIT (type) = ASTREF (unit);
  pkl_ast_node_free (unit_type);

  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_trans3 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_trans_bf_program),
   PKL_PHASE_DF_HANDLER (PKL_AST_OFFSET, pkl_trans3_df_offset),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_OFFSET, pkl_trans3_df_offset_type),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SIZEOF, pkl_trans3_df_op_sizeof),
  };
