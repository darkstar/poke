/* pkl-anal.c - Analysis phase for the poke compiler.  */

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

   `pkl_phase_anal1' is run immediately after parsing.
   `pkl_phase_anal2' is run after constant folding.

   See the handlers below for detailed information about what these
   passes check for.  */


PKL_PHASE_BEGIN_HANDLER (pkl_anal_bf_program)
{
  pkl_anal_payload payload
    = (pkl_anal_payload) PKL_PASS_PAYLOAD;
    
  /* No errors initially.  */
  payload->errors = 0;
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_anal1_struct)
{
  pkl_anal_payload payload
    = (pkl_anal_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node elems = PKL_AST_STRUCT_ELEMS (node);
  pkl_ast_node t;

  /* Make sure that the names in the structure elements are unique in
     the structure.  */
  for (t = elems; t; t = PKL_AST_CHAIN (t))
    {     
      pkl_ast_node ename = PKL_AST_STRUCT_ELEM_NAME (t);
      pkl_ast_node u;

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
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT, pkl_anal1_struct),
  };

struct pkl_phase pkl_phase_anal2 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_anal_bf_program),
  };
