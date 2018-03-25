/* pkl-anal.c - Analysis phases for the poke compiler.  */

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
#include <string.h>

#include "pkl.h"
#include "pkl-ast.h"
#include "pkl-pass.h"
#include "pkl-anal.h"

/* This file implements several analysis compiler phases, which can
   raise errors and/or warnings but won't alter the structure of the
   AST.  These phases are restartable.

   `anal1' is run immediately after trans1.
   `anal2' is run after constant folding and before gen.

   See the handlers below for detailed information about what these
   phases check for.  */

/* The following handler is used in both anal1 and anal2, and
   initializes the phase payload.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal_bf_program)
{
  pkl_anal_payload payload
    = (pkl_anal_payload) PKL_PASS_PAYLOAD;
    
  /* No errors initially.  */
  payload->errors = 0;
}
PKL_PHASE_END_HANDLER

/* In struct literals, make sure that the names of it's elements are
   unique in the structure.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal1_df_struct)
{
  pkl_anal_payload payload
    = (pkl_anal_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node elems = PKL_AST_STRUCT_ELEMS (node);
  pkl_ast_node t;

  for (t = elems; t; t = PKL_AST_CHAIN (t))
    {     
      pkl_ast_node ename = PKL_AST_STRUCT_ELEM_NAME (t);
      pkl_ast_node u;

      if (ename == NULL)
        continue;

      for (u = elems; u != t; u = PKL_AST_CHAIN (u))
        {
          pkl_ast_node uname = PKL_AST_STRUCT_ELEM_NAME (u);

          if (uname == NULL)
            continue;

          if (strcmp (PKL_AST_IDENTIFIER_POINTER (ename),
                      PKL_AST_IDENTIFIER_POINTER (uname)) == 0)
            {
              pkl_error (PKL_PASS_AST, PKL_AST_LOC (uname),
                         "duplicated name element in struct");
              payload->errors++;
              PKL_PASS_ERROR;
              /* Do not report more duplicates in this struct.  */
              break;
            }
        }
    }
}
PKL_PHASE_END_HANDLER

/* In struct TYPE nodes, check that no duplicated named element are
   declared in the type.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal1_df_type_struct)
{
  pkl_anal_payload payload
    = (pkl_anal_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node struct_type = PKL_PASS_NODE;
  pkl_ast_node struct_type_elems
    = PKL_AST_TYPE_S_ELEMS (struct_type);
  pkl_ast_node t;

  for (t = struct_type_elems; t; t = PKL_AST_CHAIN (t))
    {
      pkl_ast_node u;
      for (u = struct_type_elems; u != t; u = PKL_AST_CHAIN (u))
        {
          pkl_ast_node tname = PKL_AST_STRUCT_ELEM_TYPE_NAME (u);
          pkl_ast_node uname = PKL_AST_STRUCT_ELEM_TYPE_NAME (t);
          
          if (uname
              && tname
              && strcmp (PKL_AST_IDENTIFIER_POINTER (uname),
                         PKL_AST_IDENTIFIER_POINTER (tname)) == 0)
            {
              pkl_error (PKL_PASS_AST, PKL_AST_LOC (u),
                         "duplicated element name in struct type spec");
              payload->errors++;
              PKL_PASS_ERROR;
            }
        }
    }
}
PKL_PHASE_END_HANDLER

/* Every node in the AST should have a valid location after
   parsing.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal1_df_default)
{
  if (!PKL_AST_LOC_VALID (PKL_AST_LOC (PKL_PASS_NODE)))
    {
      pkl_ice (PKL_AST_NOLOC, "node has no location");
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_anal1 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_anal_bf_program),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT, pkl_anal1_df_struct),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_anal1_df_type_struct),
   PKL_PHASE_DF_DEFAULT_HANDLER (pkl_anal1_df_default),
  };



/* Every expression, array and struct node should be annotated with a
   type, and the type's completeness should have been determined.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal2_df_checktype)
{
  pkl_anal_payload payload
    = (pkl_anal_payload) PKL_PASS_PAYLOAD;
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (node);

  if (type == NULL)
    {
      pkl_ice (PKL_AST_LOC (node), "node with no type");
      payload->errors++;
      PKL_PASS_DONE;
    }

  if (PKL_AST_TYPE_COMPLETE (type)
      == PKL_AST_TYPE_COMPLETE_UNKNOWN)
    {
      pkl_ice (PKL_AST_LOC (type), "type completeness is unknown");
      payload->errors++;
      PKL_PASS_DONE;
    }
}
PKL_PHASE_END_HANDLER

/* The magnitude in offset literals should be an integral expression.
   Also, it must have a type and its completeness should be known.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal2_df_offset)
{
  pkl_anal_payload payload
    = (pkl_anal_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node magnitude = PKL_AST_OFFSET_MAGNITUDE (node);
  pkl_ast_node magnitude_type = PKL_AST_TYPE (magnitude);
  pkl_ast_node type = PKL_AST_TYPE (node);

  if (PKL_AST_TYPE_CODE (magnitude_type)
      != PKL_TYPE_INTEGRAL)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (magnitude_type),
                 "expected integer expression in offset");
      payload->errors++;
      PKL_PASS_ERROR;
    }

  if (type == NULL)
    {
      pkl_ice (PKL_AST_LOC (node), "node with no type");
      payload->errors++;
      PKL_PASS_ERROR;
    }

  if (PKL_AST_TYPE_COMPLETE (type)
      == PKL_AST_TYPE_COMPLETE_UNKNOWN)
    {
      pkl_ice (PKL_AST_LOC (type),
               "type completeness is unknown");
      payload->errors++;
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_anal2 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_anal_bf_program),
   PKL_PHASE_DF_HANDLER (PKL_AST_EXP, pkl_anal2_df_checktype),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY, pkl_anal2_df_checktype),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT, pkl_anal2_df_checktype),
   PKL_PHASE_DF_HANDLER (PKL_AST_OFFSET, pkl_anal2_df_offset),
  };
