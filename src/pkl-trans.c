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
   and set the size of the array consequently.  Also, reverse the list
   of initializers so they are handled in the right order in
   depth-first.  */

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
      size_t initializer_index = PKL_AST_ARRAY_INITIALIZER_INDEX (tmp);
      size_t elems_appended, effective_index;
      
      /* Set the index of the initializer.  */
      if (initializer_index == PKL_AST_ARRAY_NOINDEX)
        {
          effective_index = index;
          elems_appended = 1;
        }
      else
        {
          if (initializer_index < index)
            elems_appended = 0;
          else
            elems_appended = initializer_index - index + 1;
          effective_index = initializer_index;
        }
      
      PKL_AST_ARRAY_INITIALIZER_INDEX (tmp) = effective_index;
      index += elems_appended;
      nelem += elems_appended;
    }

  PKL_AST_ARRAY_NELEM (array) = nelem;
  PKL_AST_ARRAY_NINITIALIZER (array) = ninitializer;
  initializers = pkl_ast_reverse (initializers);
  PKL_AST_ARRAY_INITIALIZERS (array)
    = ASTREF (initializers);
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_trans1 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_trans_bf_program),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY, pkl_trans1_df_array),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT, pkl_trans1_df_struct),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_trans1_df_type_struct),
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
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (op),
                 "sizeof only works on complete types");
      payload->errors++;
      PKL_PASS_ERROR;
    }

  {    
    /* Calculate the size of the complete type in bytes and put it in
       an integer node.  */
    pkl_ast_node magnitude_type
      = pkl_ast_get_integral_type (PKL_PASS_AST, 64, 0);
    pkl_ast_node magnitude
      = pkl_ast_make_integer (PKL_PASS_AST,
                              pkl_ast_sizeof_type (op));
    PKL_AST_TYPE (magnitude) = ASTREF (magnitude_type);
  
    /* Build an offset with that magnitude, and unit bits.  */
    offset = pkl_ast_make_offset (PKL_PASS_AST, magnitude,
                                  PKL_AST_OFFSET_UNIT_BITS);
    offset_type = pkl_ast_make_offset_type (PKL_PASS_AST,
                                            magnitude_type,
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
