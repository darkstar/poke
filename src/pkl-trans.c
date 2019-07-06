/* pkl-trans.c - Transformation phases for the poke compiler.  */

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

#include <stdio.h>
#include <xalloc.h>

#include "pkl.h"
#include "pkl-ast.h"
#include "pkl-pass.h"
#include "pkl-trans.h"

/* This file implements several transformation compiler phases which,
   generally speaking, are restartable.

   `transl' makes adjustments to the compile-time lexical environment,
            after parsing.  This phase should run on its own pass, and
            is not restartable.

   `trans1' finishes ARRAY, STRUCT and TYPE_STRUCT nodes by
            determining its number of elements and characteristics.
            It also finishes OFFSET nodes by replacing certain unit
            identifiers with factors and completes/annotates other
            structures.  It also finishes STRING nodes.

   `trans2' scans the AST and annotates nodes that are literals.
            Henceforth any other phase relying on this information
            should be executed after trans2.

   `trans3' handles nodes that can be replaced for something else at
            compilation-time: SIZEOF for complete types.  This phase
            is intended to be executed short before code generation.

   `trans4' is executed just before the code generation pass.

   See the handlers below for details.  */


#define PKL_TRANS_PAYLOAD ((pkl_trans_payload) PKL_PASS_PAYLOAD)

/* The following handler is used in all trans phases and initializes
   the phase payload.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans_pr_program)
{
  PKL_TRANS_PAYLOAD->errors = 0;
  PKL_TRANS_PAYLOAD->add_frames = -1;
}
PKL_PHASE_END_HANDLER

/* The array mappers introduce a lexical frame.  Unfortunately, it is
   not possible to add this frame in the bison parser due to syntactic
   ambiguities.  So, we need to reflect the extra lexical frame
   here, by adjusting lexical addresses accordingly.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_pr_map)
{
  PKL_TRANS_PAYLOAD->in_map += 1;
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_transl_ps_map)
{
  assert (PKL_TRANS_PAYLOAD->in_map > 0);
  PKL_TRANS_PAYLOAD->in_map -= 1;
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_transl_pr_type_array)
{
  if (PKL_TRANS_PAYLOAD->in_map == 0)
    PKL_PASS_BREAK;

  PKL_TRANS_PAYLOAD->add_frames += 1;
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_transl_ps_type_array)
{
  assert (PKL_TRANS_PAYLOAD->add_frames != -1);
  PKL_TRANS_PAYLOAD->add_frames -= 1;
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_transl_ps_var)
{
  if (PKL_TRANS_PAYLOAD->add_frames != -1)
    {
        pkl_ast_node var = PKL_PASS_NODE;
        PKL_AST_VAR_BACK (var) += PKL_TRANS_PAYLOAD->add_frames;
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_transl =
  {
   PKL_PHASE_PR_HANDLER (PKL_AST_MAP, pkl_transl_pr_map),
   PKL_PHASE_PS_HANDLER (PKL_AST_MAP, pkl_transl_ps_map),
   PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_trans_pr_program),
   PKL_PHASE_PS_HANDLER (PKL_AST_VAR, pkl_transl_ps_var),
   PKL_PHASE_PR_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_transl_pr_type_array),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_transl_ps_type_array),
  };



/* Compute and set the number of elements in a STRUCT node.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_struct)
{
  pkl_ast_node astruct = PKL_PASS_NODE;
  pkl_ast_node t;
  size_t nelem = 0;

  for (t = PKL_AST_STRUCT_FIELDS (astruct); t; t = PKL_AST_CHAIN (t))
    nelem++;

  PKL_AST_STRUCT_NELEM (astruct) = nelem;
}
PKL_PHASE_END_HANDLER

/* Compute and set the number of elements in a struct TYPE node.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_type_struct)
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

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_array)
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

/* At this point offsets can have either an identifier, an integer or
   a type expressing its unit.  This handler takes care of the first
   case, replacing the identifier with a suitable unit factor.  If the
   identifier is invalid, then an error is raised.
   
   Also, if the magnitude of the offset wasn't specified then it
   defaults to 1. */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_offset)
{
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
          PKL_TRANS_PAYLOAD->errors++;
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
   factor.  If the identifier is invalid, then an error is raised.
   XXX: to remove.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_type_offset)
{
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
          PKL_TRANS_PAYLOAD->errors++;
          PKL_PASS_ERROR;
        }

      PKL_AST_TYPE_O_UNIT (offset_type) = ASTREF (new_unit);
      pkl_ast_node_free (unit);
      PKL_PASS_RESTART = 1;
    }
}
PKL_PHASE_END_HANDLER

/* Calculate the number of arguments in funcalls.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_funcall)
{
  pkl_ast_node arg;
  int nargs = 0;

  for (arg = PKL_AST_FUNCALL_ARGS (PKL_PASS_NODE);
       arg;
       arg = PKL_AST_CHAIN (arg))
    nargs++;

  PKL_AST_FUNCALL_NARG (PKL_PASS_NODE) = nargs;
}
PKL_PHASE_END_HANDLER

/* Variables that refer to parameterless functions are transformed
   into funcalls to these functions, but only if the variables are not
   part of funcall themselves! :)  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_var)
{
  if (PKL_PASS_PARENT
      && PKL_AST_CODE (PKL_PASS_PARENT) != PKL_AST_FUNCALL)
    {
      pkl_ast_node var = PKL_PASS_NODE;
      pkl_ast_node decl = PKL_AST_VAR_DECL (var);
      pkl_ast_node initial = PKL_AST_DECL_INITIAL (decl);
      pkl_ast_node initial_type = PKL_AST_TYPE (initial);

      if (PKL_AST_TYPE_CODE (initial_type) == PKL_TYPE_FUNCTION
          && (PKL_AST_TYPE_F_NARG (initial_type) == 0
              || pkl_ast_func_all_optargs (initial_type)))
        {
          pkl_ast_node funcall = pkl_ast_make_funcall (PKL_PASS_AST,
                                                       var,
                                                       NULL /* args */);
          
          PKL_AST_LOC (funcall) = PKL_AST_LOC (var);
          PKL_PASS_NODE = funcall;
          PKL_PASS_RESTART = 1;
        }
    }
}
PKL_PHASE_END_HANDLER

/* Finish strings, by expanding \-sequences, and emit errors if an
   invalid \-sequence is found.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_string)
{
  pkl_ast_node string = PKL_PASS_NODE;
  char *string_pointer = PKL_AST_STRING_POINTER (string);
  char *new_string_pointer;
  char *p;
  size_t string_length, i;

  /* Please keep this code in sync with the string printer in
     pvm-val.c:pvm_print_val.  */

  /* First pass: calculate the size of the resulting string after
     \-expansion, and report errors in the contents of the string.  */
  for (p = string_pointer, string_length = 0; *p != '\0'; ++p)
    {
      if (p[0] == '\\')
        {
          switch (p[1])
            {
            case '\\':
            case 'n':
            case 't':
              string_length++;
              break;
            default:
              pkl_error (PKL_PASS_AST, PKL_AST_LOC (string),
                         "invalid \\%c sequence in string", p[1]);
              PKL_TRANS_PAYLOAD->errors++;
              PKL_PASS_ERROR;
            }
          p++;
        }
      else
        string_length++;
    }

  /* Second pass: compose the new string.  */
  new_string_pointer = xmalloc (string_length + 1);

  for (p = string_pointer, i = 0; *p != '\0'; ++p, ++i)
    {
      if (p[0] == '\\')
        {
          switch (p[1])
            {
            case '\\': new_string_pointer[i] = '\\'; break;
            case 'n':  new_string_pointer[i] = '\n'; break;
            case 't':  new_string_pointer[i] = '\t'; break;
            default:
              assert (0);
            }
          p++;
        }
      else
        new_string_pointer[i] = p[0];
    }
  new_string_pointer[i] = '\0';
  
  free (string_pointer);
  PKL_AST_STRING_POINTER (string) = new_string_pointer;
}
PKL_PHASE_END_HANDLER

/* Determine the attribute code of attribute expressions, emitting an
   error if the given attribute name is not defined.  Finally, turn
   the binary expression into an unary expression.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_op_attr)
{
  pkl_ast_node exp = PKL_PASS_NODE;

  pkl_ast_node identifier = PKL_AST_EXP_OPERAND (exp, 1);
  const char *identifier_name = PKL_AST_IDENTIFIER_POINTER (identifier);
  enum pkl_ast_attr attr = PKL_AST_ATTR_NONE;

  if (PKL_AST_EXP_ATTR (exp) != PKL_AST_ATTR_NONE)
    PKL_PASS_DONE;

  for (attr = 0; pkl_attr_name (attr); ++attr)
    {
      if (strcmp (pkl_attr_name (attr), identifier_name) == 0)
        break;
    }

  if (attr == PKL_AST_ATTR_NONE)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (identifier),
                 "invalid attribute '%s", identifier_name);
      PKL_TRANS_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }

  PKL_AST_EXP_ATTR (exp) = attr;

  /* Turn the binary expression into an unary expression.  */
  PKL_AST_EXP_NUMOPS (exp) = 1;
  pkl_ast_node_free (PKL_AST_EXP_OPERAND (exp, 1));
}
PKL_PHASE_END_HANDLER

/* Set the function's first optional argument.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_func)
{
  pkl_ast_node func = PKL_PASS_NODE;
  pkl_ast_node func_args = PKL_AST_FUNC_ARGS (func);
  pkl_ast_node fa;

  /* Find the first optional formal argument, if any, and set
     first_opt_arg accordingly.  */
  for (fa = func_args; fa; fa = PKL_AST_CHAIN (fa))
    {
      if (PKL_AST_FUNC_ARG_INITIAL (fa))
        {
          PKL_AST_FUNC_FIRST_OPT_ARG (func) = ASTREF (fa);
          break;
        }
    }
}
PKL_PHASE_END_HANDLER

/* Function types from function type literals don't have the number of
   elements set.  Do it here.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_type_function)
{
  pkl_ast_node function_type = PKL_PASS_NODE;
  pkl_ast_node function_type_args = PKL_AST_TYPE_F_ARGS (function_type);

  pkl_ast_node arg;
  size_t nargs = 0;

  /* Count the number of formal arguments taken by functions of this
     type.  */
  for (arg = function_type_args;  arg; arg = PKL_AST_CHAIN (arg))
    nargs++;
  PKL_AST_TYPE_F_NARG (function_type) = nargs;

  /* Find the first optional formal argument, if any, and set
     first_op_arg accordingly.  */
  for (arg = function_type_args; arg; arg = PKL_AST_CHAIN (arg))
    {
      if (PKL_AST_FUNC_TYPE_ARG_OPTIONAL (arg))
        {
          PKL_AST_TYPE_F_FIRST_OPT_ARG (function_type)
            = ASTREF (arg);
          break;
        }
    }

  /* Determine whether the function type gets a vararg.  */
  for (arg = PKL_AST_TYPE_F_ARGS (function_type);
       arg;
       arg = PKL_AST_CHAIN (arg))
    {
      if (PKL_AST_FUNC_TYPE_ARG_VARARG (arg))
        {
          PKL_AST_TYPE_F_VARARG (function_type) = 1;
          break;
        }
    }
}
PKL_PHASE_END_HANDLER

/* Complete trimmers lacking some of their indexes.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_trimmer)
{
  pkl_ast_node trimmer = PKL_PASS_NODE;
  pkl_ast_node entity = PKL_AST_TRIMMER_ENTITY (trimmer);
  pkl_ast_node from = PKL_AST_TRIMMER_FROM (trimmer);
  pkl_ast_node to = PKL_AST_TRIMMER_TO (trimmer);

  /* If the FROM index of a trimmer isn't specified, it defaults to
     0UL.  */
  if (!from)
    {
      pkl_ast_node idx_type
        = pkl_ast_make_integral_type (PKL_PASS_AST, 64, 0);

      from = pkl_ast_make_integer (PKL_PASS_AST, 0);
      PKL_AST_TYPE (from) = ASTREF (idx_type);
      PKL_AST_LOC (idx_type) = PKL_AST_LOC (trimmer);
      PKL_AST_LOC (from) = PKL_AST_LOC (trimmer);

      PKL_AST_TRIMMER_FROM (trimmer) = ASTREF (from);
    }

  /* If the TO index of a trimmer isn't specified, it defaults to an
     expression that evaluates to the size of the container, minus
     one.  */
  if (!to)
    {
      pkl_ast_node idx_type
        = pkl_ast_make_integral_type (PKL_PASS_AST, 64, 0);
      pkl_ast_node length_op = pkl_ast_make_unary_exp (PKL_PASS_AST,
                                                       PKL_AST_OP_ATTR,
                                                       entity);
      pkl_ast_node one = pkl_ast_make_integer (PKL_PASS_AST, 1);
      pkl_ast_node sub_op = pkl_ast_make_binary_exp (PKL_PASS_AST,
                                                     PKL_AST_OP_SUB,
                                                     length_op, one);

      PKL_AST_EXP_ATTR (length_op) = PKL_AST_ATTR_LENGTH;
      PKL_AST_TYPE (one) = ASTREF (idx_type);

      PKL_AST_LOC (length_op) = PKL_AST_LOC (trimmer);
      PKL_AST_LOC (idx_type) = PKL_AST_LOC (trimmer);
      PKL_AST_LOC (one) = PKL_AST_LOC (trimmer);
      PKL_AST_LOC (sub_op) = PKL_AST_LOC (trimmer);

      PKL_AST_TRIMMER_TO (trimmer) = ASTREF (sub_op);
      PKL_PASS_RESTART = 1;
    }
}
PKL_PHASE_END_HANDLER

/* Decode format strings in `printf' instructions.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_print_stmt)
{
  pkl_ast_node print_stmt = PKL_PASS_NODE;
  pkl_ast_node args = PKL_AST_PRINT_STMT_ARGS (print_stmt);
  pkl_ast_node print_fmt = PKL_AST_PRINT_STMT_FMT (print_stmt);
  char *fmt, *p;
  pkl_ast_node t, arg;
  int ntag, nargs = 0;
  pkl_ast_node types = NULL;

  /* Calculate the number of arguments.  */
  for (t = args; t; t = PKL_AST_CHAIN (t))
    nargs++;
  PKL_AST_PRINT_STMT_NARGS (print_stmt) = nargs;

  /* If this is a `print', then we are done.  */
  if (!print_fmt)
    PKL_PASS_DONE;

  fmt = PKL_AST_STRING_POINTER (print_fmt);
  p = fmt;

  /* Process the prefix string, if any.  */
  if (*p != '%')
    {
      char *prefix = xmalloc (strlen (fmt) + 1);
      size_t j = 0;

      while (*p != '%' && *p != '\0')
            {
              prefix[j] = *p;
              p++;
              j++;
            }
      prefix[j] = '\0';
      PKL_AST_PRINT_STMT_PREFIX (print_stmt) = prefix;
    }

  /* Process the format string.  */
  for (types = NULL, ntag = 0, arg = args;
       *p != '\0' && args;
       ntag++, arg = PKL_AST_CHAIN (arg))
    {
      pkl_ast_node atype;

      assert (*p == '%');
      if (ntag >= nargs)
        {
          pkl_error (PKL_PASS_AST, PKL_AST_LOC (print_stmt),
                     "not enough arguments in printf");
          PKL_TRANS_PAYLOAD->errors++;
          PKL_PASS_ERROR;
        }

      switch (p[1])
        {
        case 's':
          p += 2;
          PKL_AST_PRINT_STMT_ARG_BASE (arg) = 10; /* Arbitrary.  */
          atype = pkl_ast_make_string_type (PKL_PASS_AST);
          PKL_AST_LOC (atype) = PKL_AST_LOC (print_fmt);
          types = pkl_ast_chainon (types, atype);
          break;
        case 'i':
        case 'u':
          {
            unsigned int bits;
            
            if (p[2] >= '0' && p[2] <= '9')
              {
                int base_idx;
                
                if (p[3] >= '0' && p[3] <= '9')
                  {
                    bits = (p[2] - '0') * 10 + (p[3] - '0');
                    base_idx = 4;
                  }
                else
                  {
                    bits = p[2] - '0';
                    base_idx = 3;
                  }
                
                if (bits > 64)
                  goto invalid_tag;
                
                switch (p[base_idx])
                  {
                  case 'b': PKL_AST_PRINT_STMT_ARG_BASE (arg) = 2; break;
                  case 'o': PKL_AST_PRINT_STMT_ARG_BASE (arg) = 8; break;
                  case 'd': PKL_AST_PRINT_STMT_ARG_BASE (arg) = 10; break;
                  case 'x': PKL_AST_PRINT_STMT_ARG_BASE (arg) = 16; break;
                  default:
                    goto invalid_tag;
                  }
                
                atype = pkl_ast_make_integral_type (PKL_PASS_AST,
                                                    bits, p[1] == 'i');
                PKL_AST_LOC (atype) = PKL_AST_LOC (print_fmt);
                types = pkl_ast_chainon (types, atype);
                
                if (base_idx == 4)
                  p += 5;
                else
                  p += 4;
              }
            else
              goto invalid_tag;
            break;
          }
        default:
          goto invalid_tag;
        }        

      /* Add the optional suffix to the argument.  */
      if (*p != '%')
        {
          /* This argument has a prefix.  */
          size_t j;
          char *suffix = xmalloc (strlen (fmt) + 1);

          j = 0;
          while (*p != '%' && *p != '\0')
            {
              suffix[j] = *p;
              p++;
              j++;
            }
          suffix[j] = '\0';
          PKL_AST_PRINT_STMT_ARG_SUFFIX (arg) = suffix;
        }
    }

  if (nargs > ntag)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (print_stmt),
                 "too many arguments in printf");
      PKL_TRANS_PAYLOAD->errors++;
      PKL_PASS_ERROR;
    }

  PKL_AST_PRINT_STMT_TYPES (print_stmt) = ASTREF (types);

  PKL_PASS_RESTART=1;
  PKL_PASS_DONE;

 invalid_tag:
  pkl_error (PKL_PASS_AST, PKL_AST_LOC (print_fmt),
             "invalid %%- tag in format string");
  PKL_TRANS_PAYLOAD->errors++;
  PKL_PASS_ERROR;
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_trans1 =
  {
   PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_trans_pr_program),
   PKL_PHASE_PS_HANDLER (PKL_AST_ARRAY, pkl_trans1_ps_array),
   PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT, pkl_trans1_ps_struct),
   PKL_PHASE_PS_HANDLER (PKL_AST_OFFSET, pkl_trans1_ps_offset),
   PKL_PHASE_PS_HANDLER (PKL_AST_FUNCALL, pkl_trans1_ps_funcall),
   PKL_PHASE_PS_HANDLER (PKL_AST_STRING, pkl_trans1_ps_string),
   PKL_PHASE_PS_HANDLER (PKL_AST_VAR, pkl_trans1_ps_var),
   PKL_PHASE_PS_HANDLER (PKL_AST_FUNC, pkl_trans1_ps_func),
   PKL_PHASE_PS_HANDLER (PKL_AST_TRIMMER, pkl_trans1_ps_trimmer),
   PKL_PHASE_PS_HANDLER (PKL_AST_PRINT_STMT, pkl_trans1_ps_print_stmt),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_ATTR, pkl_trans1_ps_op_attr),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_trans1_ps_type_struct),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_OFFSET, pkl_trans1_ps_type_offset),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_FUNCTION, pkl_trans1_ps_type_function),
  };



/* The following handlers annotate expression nodes to reflect whether
   they are literals.  Entities created by the lexer (INTEGER, STRING,
   etc) already have this attribute set if needed. */

/*  Expressions having only literal operands are literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_exp)
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

  PKL_AST_LITERAL_P (exp) = literal_p;
}
PKL_PHASE_END_HANDLER

/* An offset is a literal if its magnitude is also a literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_offset)
{
  pkl_ast_node magnitude
    = PKL_AST_OFFSET_MAGNITUDE (PKL_PASS_NODE);

  PKL_AST_LITERAL_P (PKL_PASS_NODE) = PKL_AST_LITERAL_P (magnitude);
}
PKL_PHASE_END_HANDLER

/* An array is a literal if all its initializers are literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_array)
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

/* An indexer is a literal if the referred entity element is also a
   literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_indexer)
{
  pkl_ast_node entity = PKL_AST_INDEXER_ENTITY (PKL_PASS_NODE);
  PKL_AST_LITERAL_P (PKL_PASS_NODE) = PKL_AST_LITERAL_P (entity);
}
PKL_PHASE_END_HANDLER

/* A trim is a literal if the trimmed entity is also a literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_trimmer)
{
  pkl_ast_node entity = PKL_AST_TRIMMER_ENTITY (PKL_PASS_NODE);
  PKL_AST_LITERAL_P (PKL_PASS_NODE) = PKL_AST_LITERAL_P (entity);
}
PKL_PHASE_END_HANDLER

/* A struct is a literal if all its element values are literals.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_struct)
{
  pkl_ast_node t;
  int literal_p = 1;
  
  for (t = PKL_AST_STRUCT_FIELDS (PKL_PASS_NODE); t;
       t = PKL_AST_CHAIN (t))
    {
      pkl_ast_node struct_field_exp = PKL_AST_STRUCT_FIELD_EXP (t);

      literal_p &= PKL_AST_LITERAL_P (struct_field_exp);
      if (!literal_p)
        break;
    }

  PKL_AST_LITERAL_P (PKL_PASS_NODE) = literal_p;
}
PKL_PHASE_END_HANDLER

/* A struct ref is a literal if the value of the referred element is
   also a literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_struct_ref)
{
  pkl_ast_node stct = PKL_AST_STRUCT_REF_STRUCT (PKL_PASS_NODE);
  PKL_AST_LITERAL_P (PKL_PASS_NODE) = PKL_AST_LITERAL_P (stct);
}
PKL_PHASE_END_HANDLER

/* A cast is considered a literal if the value of the referred element
   is also a literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_cast)
{
  PKL_AST_LITERAL_P (PKL_PASS_NODE)
    = PKL_AST_LITERAL_P (PKL_AST_CAST_EXP (PKL_PASS_NODE));
}
PKL_PHASE_END_HANDLER

/* In offset types having another type as their unit, replace it with
   it's size in bits.  Emit a diagnostic if the type is not
   complete.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_offset_type)
{
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
      PKL_TRANS_PAYLOAD->errors++;
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

struct pkl_phase pkl_phase_trans2 =
  {
   PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_trans_pr_program),
   PKL_PHASE_PS_HANDLER (PKL_AST_EXP, pkl_trans2_ps_exp),
   PKL_PHASE_PS_HANDLER (PKL_AST_OFFSET, pkl_trans2_ps_offset),
   PKL_PHASE_PS_HANDLER (PKL_AST_ARRAY, pkl_trans2_ps_array),
   PKL_PHASE_PS_HANDLER (PKL_AST_INDEXER, pkl_trans2_ps_indexer),
   PKL_PHASE_PS_HANDLER (PKL_AST_TRIMMER, pkl_trans2_ps_trimmer),
   PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT, pkl_trans2_ps_struct),
   PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT_REF, pkl_trans2_ps_struct_ref),
   PKL_PHASE_PS_HANDLER (PKL_AST_CAST, pkl_trans2_ps_cast),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_OFFSET, pkl_trans2_ps_offset_type),
  };



/* SIZEOF nodes whose operand is a complete type should be replaced
   with an offset.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans3_ps_op_sizeof)
{
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node op = PKL_AST_EXP_OPERAND (node, 0);
  pkl_ast_node offset, offset_type, unit, unit_type;

  if (PKL_AST_TYPE_COMPLETE (op)
      != PKL_AST_TYPE_COMPLETE_YES)
    {
      pkl_error (PKL_PASS_AST, PKL_AST_LOC (op),
                 "invalid operand to sizeof");
      PKL_TRANS_PAYLOAD->errors++;
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

struct pkl_phase pkl_phase_trans3 =
  {
   PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_trans_pr_program),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_SIZEOF, pkl_trans3_ps_op_sizeof),
  };



/* Reverse the list of initializers in array literals.

   This is needed because at code generation time, the mka instruction
   processes initializers from top to bottom of the stack.  Since
   several initializers can refer to the same array element, they
   should be processed in the right order.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans4_ps_array)
{
  pkl_ast_node array = PKL_PASS_NODE;
  pkl_ast_node initializers = PKL_AST_ARRAY_INITIALIZERS (array);

  initializers = pkl_ast_reverse (initializers);
  PKL_AST_ARRAY_INITIALIZERS (array) = ASTREF (initializers);
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_trans4 =
  {
   PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_trans_pr_program),
   PKL_PHASE_PS_HANDLER (PKL_AST_ARRAY, pkl_trans4_ps_array),
  };
