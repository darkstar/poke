/* pkl-anal.c - Analysis phases for the poke compiler.  */

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

#include <config.h>
#include <string.h>

#include "pkl.h"
#include "pkl-ast.h"
#include "pkl-pass.h"
#include "pkl-anal.h"

/* This file implements several analysis compiler phases, which can
   raise errors and/or warnings, and update annotations in nodes, but
   won't alter the structure of the AST.  These phases are
   restartable.

   `anal1' is run immediately after trans1.
   `anal2' is run after constant folding.

   `analf' is run in the backend pass, right before gen.  Its main
   purpose is to determine that every node that is traversed
   optionally in do_pass but that is required by the code generator
   exists.  This avoids the codegen to generate invalid code silently.

   See the handlers below for detailed information about what these
   phases check for.  */

#define PKL_ANAL_PAYLOAD ((pkl_anal_payload) PKL_PASS_PAYLOAD)

/* The following handler is used in all anal phases, and initializes
   the phase payload.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal_pr_program)
{
  /* No errors initially.  */
  PKL_ANAL_PAYLOAD->errors = 0;
}
PKL_PHASE_END_HANDLER

/* In struct literals, make sure that the names of its elements are
   unique in the structure.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal1_ps_struct)
{
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
              PKL_ANAL_PAYLOAD->errors++;
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

PKL_PHASE_BEGIN_HANDLER (pkl_anal1_ps_type_struct)
{
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
              PKL_ANAL_PAYLOAD->errors++;
              PKL_PASS_ERROR;
            }
        }
    }
}
PKL_PHASE_END_HANDLER

/* Builtin compound statements can't contain statements
   themselves.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal1_ps_comp_stmt)
{
  pkl_ast_node comp_stmt = PKL_PASS_NODE;

  if (PKL_AST_COMP_STMT_BUILTIN (comp_stmt) != PKL_AST_BUILTIN_NONE
      && PKL_AST_COMP_STMT_STMTS (comp_stmt) != NULL)
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (comp_stmt),
               "builtin comp-stmt contains statements");
      PKL_ANAL_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

/* Every node in the AST should have a valid location after parsing.
   This handler is used in both anal1 and anal2.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal_ps_default)
{
  if (!PKL_AST_LOC_VALID (PKL_AST_LOC (PKL_PASS_NODE)))
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_NOLOC,
               "node #%" PRIu64 " with code %d has no location",
               PKL_AST_UID (PKL_PASS_NODE), PKL_AST_CODE (PKL_PASS_NODE));
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

/* The arguments to a funcall should be either all named, or none
   named.  Also, it is not allowed to specify the same argument
   twice.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal1_ps_funcall)
{
  pkl_ast_node funcall = PKL_PASS_NODE;
  pkl_ast_node funcall_arg;
  int some_named = 0;
  int some_unnamed = 0;

  /* Check that all arguments are either named or unnamed.  */
  for (funcall_arg = PKL_AST_FUNCALL_ARGS (funcall);
       funcall_arg;
       funcall_arg = PKL_AST_CHAIN (funcall_arg))
    {
      if (PKL_AST_FUNCALL_ARG_NAME (funcall_arg))
        some_named = 1;
      else
        some_unnamed = 1;
    }

  if (some_named && some_unnamed)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (funcall),
                 "mixed named and not-named arguments not allowed in funcall");
      PKL_ANAL_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }

  /* If arguments are named, check that there are not arguments named
     twice.  */
  if (some_named)
    { 
      for (funcall_arg = PKL_AST_FUNCALL_ARGS (funcall);
           funcall_arg;
           funcall_arg = PKL_AST_CHAIN (funcall_arg))
        {
          pkl_ast_node aa;
          for (aa = PKL_AST_CHAIN (funcall_arg); aa; aa = PKL_AST_CHAIN (aa))
            {
              pkl_ast_node identifier1 = PKL_AST_FUNCALL_ARG_NAME (funcall_arg);
              pkl_ast_node identifier2 = PKL_AST_FUNCALL_ARG_NAME (aa);

              if (strcmp (PKL_AST_IDENTIFIER_POINTER (identifier1),
                          PKL_AST_IDENTIFIER_POINTER (identifier2)) == 0)
                {
                  pkl_error (PKL_PASS_AST, PKL_AST_LOC (aa),
                             "duplicated argument in funcall");
                  PKL_ANAL_PAYLOAD->errors++;
                  PKL_PASS_ERROR;
                }
            }
        }
    }
}
PKL_PHASE_END_HANDLER

/* Check that all optional formal arguments in a function specifier
   are at the end of the arguments list, and other checks.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal1_ps_func)
{
  pkl_ast_node func = PKL_PASS_NODE;
  pkl_ast_node fa;

  for (fa = PKL_AST_FUNC_FIRST_OPT_ARG (func);
       fa;
       fa = PKL_AST_CHAIN (fa))
    {
      /* All optional formal arguments in a function specifier should
         be at the end of the arguments list.  */
      if (!PKL_AST_FUNC_ARG_INITIAL (fa))
        {
          pkl_error (PKL_PASS_AST, PKL_AST_LOC (fa),
                     "non-optional argument after optional arguments");
          PKL_ANAL_PAYLOAD->errors++;
          PKL_PASS_ERROR;
        }

      /* If there is a vararg argument, it should be at the end of the
         list of arguments.  Also, it should be unique.  */
      if (PKL_AST_FUNC_ARG_VARARG (fa) == 1
          && PKL_AST_CHAIN (fa) != NULL)
        {
          pkl_error (PKL_PASS_AST, PKL_AST_LOC (fa),
                     "vararg argument should be the last argument");
          PKL_ANAL_PAYLOAD->errors++;
          PKL_PASS_ERROR;
        }
    }
}
PKL_PHASE_END_HANDLER

/* In function type specifier arguments, only one vararg argument can
   exist, and it should be the last argument in the type.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal1_ps_type_function)
{
  pkl_ast_node func_type = PKL_PASS_NODE;
  pkl_ast_node arg;

  for (arg = PKL_AST_TYPE_F_ARGS (func_type);
       arg;
       arg = PKL_AST_CHAIN (arg))
    {
      if (PKL_AST_FUNC_TYPE_ARG_VARARG (arg)
          && PKL_AST_CHAIN (arg) != NULL)
        {
          pkl_error (PKL_PASS_AST, PKL_AST_LOC (arg),
                     "vararg argument should be the last argument");
          PKL_ANAL_PAYLOAD->errors++;
          PKL_PASS_ERROR;
        }
    }
}
PKL_PHASE_END_HANDLER

/* Make sure every BREAK statement has an associated entity.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal1_ps_break_stmt)
{
  pkl_ast_node break_stmt = PKL_PASS_NODE;

  if (!PKL_AST_BREAK_STMT_ENTITY (break_stmt))
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (break_stmt),
                 "`break' statement without containing statement");
      PKL_ANAL_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_anal1 =
  {
   PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_anal_pr_program),
   PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT, pkl_anal1_ps_struct),
   PKL_PHASE_PS_HANDLER (PKL_AST_COMP_STMT, pkl_anal1_ps_comp_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_BREAK_STMT, pkl_anal1_ps_break_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_FUNCALL, pkl_anal1_ps_funcall),
   PKL_PHASE_PS_HANDLER (PKL_AST_FUNC, pkl_anal1_ps_func),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_anal1_ps_type_struct),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_FUNCTION, pkl_anal1_ps_type_function),
   PKL_PHASE_PS_DEFAULT_HANDLER (pkl_anal_ps_default),
  };



/* Every expression, array and struct node should be annotated with a
   type, and the type's completeness should have been determined.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal2_ps_checktype)
{
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (node);

  if (type == NULL)
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (node),
               "node #%" PRIu64 " has no type",
               PKL_AST_UID (node));
      PKL_ANAL_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }

  if (PKL_AST_TYPE_COMPLETE (type)
      == PKL_AST_TYPE_COMPLETE_UNKNOWN)
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (type),
               "type completeness is unknown in node #%" PRIu64,
               PKL_AST_UID (node));
      PKL_ANAL_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

/* The magnitude in offset literals should be an integral expression.
   Also, it must have a type and its completeness should be known.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal2_ps_offset)
{
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node magnitude = PKL_AST_OFFSET_MAGNITUDE (node);
  pkl_ast_node magnitude_type = PKL_AST_TYPE (magnitude);
  pkl_ast_node type = PKL_AST_TYPE (node);

  if (PKL_AST_TYPE_CODE (magnitude_type)
      != PKL_TYPE_INTEGRAL)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (magnitude_type),
                 "expected integer expression in offset");
      PKL_ANAL_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }

  if (type == NULL)
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (node),
               "node #% " PRIu64 " has no type",
               PKL_AST_UID (node));
      PKL_ANAL_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }

  if (PKL_AST_TYPE_COMPLETE (type)
      == PKL_AST_TYPE_COMPLETE_UNKNOWN)
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (type),
               "type completeness is unknown in node #%" PRIu64,
               PKL_AST_UID (node));
      PKL_ANAL_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

/* A return statement returning a value is not allowed in a void
   function.  Also, an expressionless return statement is invalid in a
   non-void function.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal2_ps_return_stmt)
{
  pkl_ast_node return_stmt = PKL_PASS_NODE;
  pkl_ast_node exp = PKL_AST_RETURN_STMT_EXP (return_stmt);
  pkl_ast_node function = PKL_AST_RETURN_STMT_FUNCTION (return_stmt);

  if (exp
      && PKL_AST_TYPE_CODE (PKL_AST_FUNC_RET_TYPE (function)) == PKL_TYPE_VOID)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (exp),
                 "returning a value in a void function");
      PKL_ANAL_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }
  else if (!exp
           && PKL_AST_TYPE_CODE (PKL_AST_FUNC_RET_TYPE (function)) != PKL_TYPE_VOID)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (return_stmt),
                 "the function expects a return value");
      PKL_ANAL_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

/* A funcall to a void function is only allowed in an "expression
   statement.  */

PKL_PHASE_BEGIN_HANDLER (pkl_anal2_ps_funcall)
{
  pkl_ast_node funcall = PKL_PASS_NODE;
  pkl_ast_node funcall_function = PKL_AST_FUNCALL_FUNCTION (funcall);
  pkl_ast_node function_type = PKL_AST_TYPE (funcall_function);

  if (PKL_AST_TYPE_F_RTYPE (function_type) == NULL
      && PKL_PASS_PARENT
      && PKL_AST_CODE (PKL_PASS_PARENT) != PKL_AST_EXP_STMT)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (funcall_function),
                 "call to void function in expression");
      PKL_ANAL_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_anal2 =
  {
   PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_anal_pr_program),
   PKL_PHASE_PS_HANDLER (PKL_AST_EXP, pkl_anal2_ps_checktype),
   PKL_PHASE_PS_HANDLER (PKL_AST_ARRAY, pkl_anal2_ps_checktype),
   PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT, pkl_anal2_ps_checktype),
   PKL_PHASE_PS_HANDLER (PKL_AST_OFFSET, pkl_anal2_ps_offset),
   PKL_PHASE_PS_HANDLER (PKL_AST_RETURN_STMT, pkl_anal2_ps_return_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_FUNCALL, pkl_anal2_ps_funcall),
   PKL_PHASE_PS_DEFAULT_HANDLER (pkl_anal_ps_default),
  };



/* Make sure that every array initializer features an index at this
   point.  */

PKL_PHASE_BEGIN_HANDLER (pkl_analf_ps_array_initializer)
{
  if (!PKL_AST_ARRAY_INITIALIZER_INDEX (PKL_PASS_NODE))
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_NOLOC,
               "array initializer node #%" PRIu64 " has no index",
               PKL_AST_UID (PKL_PASS_NODE));
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

/* Make sure that the left-hand of an assignment expression is of the
   right kind.  */

PKL_PHASE_BEGIN_HANDLER (pkl_analf_ps_ass_stmt)
{
  pkl_ast_node ass_stmt = PKL_PASS_NODE;
  pkl_ast_node ass_stmt_lvalue = PKL_AST_ASS_STMT_LVALUE (ass_stmt);

  if (!pkl_ast_lvalue_p (ass_stmt_lvalue))
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (ass_stmt_lvalue),
                 "invalid l-value in assignment");
      PKL_ANAL_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_analf =
  {
   PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_anal_pr_program),
   PKL_PHASE_PS_HANDLER (PKL_AST_OFFSET, pkl_analf_ps_array_initializer),
   PKL_PHASE_PS_HANDLER (PKL_AST_ASS_STMT, pkl_analf_ps_ass_stmt),
  };
