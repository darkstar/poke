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
              fprintf (stderr, "error: duplicated name element in struct\n");
              payload->errors++;
              /* Do not report more duplicates in this struct.  */
              break;
            }
        }
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_anal1 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_anal_bf_program),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT, pkl_anal1_df_struct),
  };

/* Every expression, array and struct node should be annotated with a
   type, and the type's completeness should have been determined.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal2_df_checktype)
{
  pkl_anal_payload payload
    = (pkl_anal_payload) PKL_PASS_PAYLOAD;
  pkl_ast_node type = PKL_AST_TYPE (PKL_PASS_NODE);

  if (type == NULL)
    {
      fprintf (stderr,
               "internal compiler error: node with no type\n");
      payload->errors++;
      PKL_PASS_DONE;
    }

  if (PKL_AST_TYPE_COMPLETE (type)
      == PKL_AST_TYPE_COMPLETE_UNKNOWN)
    {
      fprintf (stderr,
               "internal compiler error: type completeness is unknown\n");
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
      fprintf (stderr,
               "error: expected integer expression in offset\n");
      payload->errors++;
      PKL_PASS_DONE;
    }

  if (type == NULL)
    {
      fprintf (stderr,
               "internal compiler error: node with no type\n");
      payload->errors++;
      PKL_PASS_DONE;
    }

  if (PKL_AST_TYPE_COMPLETE (type)
      == PKL_AST_TYPE_COMPLETE_UNKNOWN)
    {
      fprintf (stderr,
               "internal compiler error: type completeness is unknown\n");
      payload->errors++;
      PKL_PASS_DONE;
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
