/* pkl-ast.c - Abstract Syntax Tree for Poke.  */

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
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "xalloc.h"
#include "pkl-ast.h"

/* Return the default endianness.  */

enum pkl_ast_endian
pkl_ast_default_endian (void)
{
  return PKL_AST_MSB;
}

/* Allocate and return a new AST node, with the given CODE.  The rest
   of the node is initialized to zero.  */
  
static pkl_ast_node
pkl_ast_make_node (pkl_ast ast,
                   enum pkl_ast_code code)
{
  pkl_ast_node node;

  node = xmalloc (sizeof (union pkl_ast_node));
  memset (node, 0, sizeof (union pkl_ast_node));
  PKL_AST_AST (node) = ast;
  PKL_AST_CODE (node) = code;
  PKL_AST_UID (node) = ast->uid++;

  return node;
}

/* Chain AST2 at the end of the tree node chain in AST1.  If AST1 is
   null then it returns AST2.  */

pkl_ast_node
pkl_ast_chainon (pkl_ast_node ast1, pkl_ast_node ast2)
{
  if (ast1)
    {
      pkl_ast_node tmp;

      for (tmp = ast1; PKL_AST_CHAIN (tmp); tmp = PKL_AST_CHAIN (tmp))
        if (tmp == ast2)
          abort ();

      PKL_AST_CHAIN (tmp) = ASTREF (ast2);
      return ast1;
    }

  return ast2;
}

/* Build and return an AST node for an integer constant.  */

pkl_ast_node 
pkl_ast_make_integer (pkl_ast ast,
                      uint64_t value)
{
  pkl_ast_node new = pkl_ast_make_node (ast, PKL_AST_INTEGER);

  PKL_AST_INTEGER_VALUE (new) = value;
  PKL_AST_LITERAL_P (new) = 1;
  
  return new;
}

/* Build and return an AST node for a string constant.  */

pkl_ast_node 
pkl_ast_make_string (pkl_ast ast,
                     const char *str)
{
  pkl_ast_node new = pkl_ast_make_node (ast, PKL_AST_STRING);

  assert (str);

  PKL_AST_STRING_POINTER (new) = xstrdup (str);
  PKL_AST_STRING_LENGTH (new) = strlen (str);
  PKL_AST_LITERAL_P (new) = 1;

  return new;
}

/* Build and return an AST node for an identifier.  */

pkl_ast_node
pkl_ast_make_identifier (pkl_ast ast,
                         const char *str)
{
  pkl_ast_node id = pkl_ast_make_node (ast, PKL_AST_IDENTIFIER);

  PKL_AST_IDENTIFIER_POINTER (id) = xstrdup (str);
  PKL_AST_IDENTIFIER_LENGTH (id) = strlen (str);

  return id;
}

/* Build and return an AST node for an enumerator.  */

pkl_ast_node 
pkl_ast_make_enumerator (pkl_ast ast,
                         pkl_ast_node identifier,
                         pkl_ast_node value)
{
  pkl_ast_node enumerator
    = pkl_ast_make_node (ast, PKL_AST_ENUMERATOR);

  assert (identifier != NULL);

  PKL_AST_ENUMERATOR_IDENTIFIER (enumerator) = ASTREF (identifier);
  PKL_AST_ENUMERATOR_VALUE (enumerator) = ASTREF (value);

  return enumerator;
}

/* Build and return an AST node for a conditional expression.  */

pkl_ast_node
pkl_ast_make_cond_exp (pkl_ast ast,
                       pkl_ast_node cond,
                       pkl_ast_node thenexp,
                       pkl_ast_node elseexp)
{
  pkl_ast_node cond_exp
    = pkl_ast_make_node (ast, PKL_AST_COND_EXP);

  assert (cond && thenexp && elseexp);

  PKL_AST_COND_EXP_COND (cond_exp) = ASTREF (cond);
  PKL_AST_COND_EXP_THENEXP (thenexp) = ASTREF (thenexp);
  PKL_AST_COND_EXP_ELSEEXP (elseexp) = ASTREF (elseexp);

  PKL_AST_LITERAL_P (cond_exp)
    = PKL_AST_LITERAL_P (thenexp) && PKL_AST_LITERAL_P (elseexp);
  
  return cond_exp;
}

/* Build and return an AST node for a binary expression.  */

pkl_ast_node
pkl_ast_make_binary_exp (pkl_ast ast,
                         enum pkl_ast_op code,
                         pkl_ast_node op1,
                         pkl_ast_node op2)
{
  pkl_ast_node exp = pkl_ast_make_node (ast, PKL_AST_EXP);

  assert (op1 && op2);

  PKL_AST_EXP_CODE (exp) = code;
  PKL_AST_EXP_ATTR (exp) = PKL_AST_ATTR_NONE;
  PKL_AST_EXP_NUMOPS (exp) = 2;
  PKL_AST_EXP_OPERAND (exp, 0) = ASTREF (op1);
  PKL_AST_EXP_OPERAND (exp, 1) = ASTREF (op2);

  PKL_AST_LITERAL_P (exp)
    = PKL_AST_LITERAL_P (op1) && PKL_AST_LITERAL_P (op2);

  return exp;
}

/* Return the written form of the given attribute code.  This returns
   NULL for PKL_AST_ATTR_NONE */

const char *
pkl_attr_name (enum pkl_ast_attr attr)
{
#define PKL_DEF_ATTR(SYM, STRING) STRING,
  static const char* attr_names[] =
    {
#include "pkl-attrs.def"
     NULL
    };
#undef PKL_DEF_ATTR

  return attr_names[attr];
}

/* Build and return an AST node for an unary expression.  */

pkl_ast_node
pkl_ast_make_unary_exp (pkl_ast ast,
                        enum pkl_ast_op code,
                        pkl_ast_node op)
{
  pkl_ast_node exp = pkl_ast_make_node (ast, PKL_AST_EXP);

  PKL_AST_EXP_CODE (exp) = code;
  PKL_AST_EXP_ATTR (exp) = PKL_AST_ATTR_NONE;
  PKL_AST_EXP_NUMOPS (exp) = 1;
  PKL_AST_EXP_OPERAND (exp, 0) = ASTREF (op);
  PKL_AST_LITERAL_P (exp) = PKL_AST_LITERAL_P (op);
  
  return exp;
}

/* Build and return an AST node for a function definition.  */

pkl_ast_node
pkl_ast_make_func (pkl_ast ast, pkl_ast_node ret_type,
                   pkl_ast_node args, pkl_ast_node body)
{
  pkl_ast_node func = pkl_ast_make_node (ast, PKL_AST_FUNC);

  assert (body);

  if (ret_type)
    PKL_AST_FUNC_RET_TYPE (func) = ASTREF (ret_type);
  if (args)
    PKL_AST_FUNC_ARGS (func) = ASTREF (args);
  PKL_AST_FUNC_BODY (func) = ASTREF (body);

  return func;
}

/* Build and return an AST node for a function definition formal
   argument.  */

pkl_ast_node
pkl_ast_make_func_arg (pkl_ast ast, pkl_ast_node type,
                       pkl_ast_node identifier)
{
  pkl_ast_node func_arg = pkl_ast_make_node (ast, PKL_AST_FUNC_ARG);

  assert (type && identifier);

  PKL_AST_FUNC_ARG_TYPE (func_arg) = ASTREF (type);
  PKL_AST_FUNC_ARG_IDENTIFIER (func_arg) = ASTREF (identifier);

  return func_arg;
}

/* Build and return an AST node for an array reference.  */

pkl_ast_node
pkl_ast_make_array_ref (pkl_ast ast,
                        pkl_ast_node array, pkl_ast_node index)
{
  pkl_ast_node aref = pkl_ast_make_node (ast, PKL_AST_ARRAY_REF);

  assert (array && index);

  PKL_AST_ARRAY_REF_ARRAY (aref) = ASTREF (array);
  PKL_AST_ARRAY_REF_INDEX (aref) = ASTREF (index);
  PKL_AST_LITERAL_P (aref) = 0;

  return aref;
}

/* Build and return an AST node for a struct reference.  */

pkl_ast_node
pkl_ast_make_struct_ref (pkl_ast ast,
                         pkl_ast_node sct, pkl_ast_node identifier)
{
  pkl_ast_node sref = pkl_ast_make_node (ast, PKL_AST_STRUCT_REF);

  assert (sct && identifier);

  PKL_AST_STRUCT_REF_STRUCT (sref) = ASTREF (sct);
  PKL_AST_STRUCT_REF_IDENTIFIER (sref) = ASTREF (identifier);
  
  return sref;
}

/* Build and return type AST nodes.  */

static pkl_ast_node
pkl_ast_make_type (pkl_ast ast)
{
  pkl_ast_node type = pkl_ast_make_node (ast, PKL_AST_TYPE);

  PKL_AST_TYPE_NAME (type) = NULL;
  PKL_AST_TYPE_COMPLETE (type)
    = PKL_AST_TYPE_COMPLETE_UNKNOWN;
  return type;
}

pkl_ast_node
pkl_ast_make_named_type (pkl_ast ast, pkl_ast_node name)
{
  pkl_ast_node type = pkl_ast_make_type (ast);

  assert (name);

  PKL_AST_TYPE_NAME (type) = ASTREF (name);
  return type;
}

pkl_ast_node
pkl_ast_make_integral_type (pkl_ast ast, size_t size, int signed_p)
{
  pkl_ast_node type = pkl_ast_make_type (ast);

  assert (signed_p == 0 || signed_p == 1);

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_INTEGRAL;
  PKL_AST_TYPE_COMPLETE (type)
    = PKL_AST_TYPE_COMPLETE_YES;
  PKL_AST_TYPE_I_SIGNED (type) = signed_p;
  PKL_AST_TYPE_I_SIZE (type) = size;
  return type;
}

pkl_ast_node
pkl_ast_make_array_type (pkl_ast ast, pkl_ast_node etype)
{
  pkl_ast_node type = pkl_ast_make_type (ast);

  assert (etype);

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_ARRAY;
  PKL_AST_TYPE_A_NELEM (type) = NULL;
  PKL_AST_TYPE_A_ETYPE (type) = ASTREF (etype);

  return type;
}

pkl_ast_node
pkl_ast_make_string_type (pkl_ast ast)
{
  pkl_ast_node type = pkl_ast_make_type (ast);

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_STRING;
  PKL_AST_TYPE_COMPLETE (type)
    = PKL_AST_TYPE_COMPLETE_NO;
  return type;
}

pkl_ast_node
pkl_ast_make_void_type (pkl_ast ast)
{
  pkl_ast_node type = pkl_ast_make_type (ast);

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_VOID;
  PKL_AST_TYPE_COMPLETE (type)
    = PKL_AST_TYPE_COMPLETE_NO;
  return type;
}

pkl_ast_node
pkl_ast_make_offset_type (pkl_ast ast,
                          pkl_ast_node base_type,
                          pkl_ast_node unit)
{
  pkl_ast_node type = pkl_ast_make_type (ast);

  assert (base_type && unit);

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_OFFSET;
  PKL_AST_TYPE_COMPLETE (type)
    = PKL_AST_TYPE_COMPLETE_YES;
  PKL_AST_TYPE_O_UNIT (type) = ASTREF (unit);
  PKL_AST_TYPE_O_BASE_TYPE (type) = ASTREF (base_type);

  return type;
}

pkl_ast_node
pkl_ast_make_struct_type (pkl_ast ast,
                          size_t nelem, pkl_ast_node struct_type_elems)
{
  pkl_ast_node type = pkl_ast_make_type (ast);

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_STRUCT;
  PKL_AST_TYPE_S_NELEM (type) = nelem;
  if (struct_type_elems)
    PKL_AST_TYPE_S_ELEMS (type) = ASTREF (struct_type_elems);

  return type;
}

pkl_ast_node
pkl_ast_make_struct_elem_type (pkl_ast ast,
                               pkl_ast_node name,
                               pkl_ast_node type)
{
  pkl_ast_node struct_type_elem
    = pkl_ast_make_node (ast, PKL_AST_STRUCT_ELEM_TYPE);

  PKL_AST_STRUCT_ELEM_TYPE_NAME (struct_type_elem)
    = ASTREF (name);
  PKL_AST_STRUCT_ELEM_TYPE_TYPE (struct_type_elem)
    = ASTREF (type);

  return struct_type_elem;
}

pkl_ast_node
pkl_ast_make_function_type (pkl_ast ast, pkl_ast_node rtype,
                            size_t narg, pkl_ast_node args)
{
  pkl_ast_node type = pkl_ast_make_type (ast);

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_FUNCTION;
  PKL_AST_TYPE_F_RTYPE (type) = ASTREF (rtype);
  PKL_AST_TYPE_F_NARG (type) = narg;
  PKL_AST_TYPE_F_ARGS (type) = ASTREF (args);

  return type;
}

pkl_ast_node
pkl_ast_make_func_arg_type (pkl_ast ast, pkl_ast_node type)
{
  pkl_ast_node function_type_arg
    = pkl_ast_make_node (ast, PKL_AST_FUNC_ARG_TYPE);

  assert (type);

  PKL_AST_FUNC_ARG_TYPE_TYPE (function_type_arg)
    = ASTREF (type);

  return function_type_arg;
}

/* Allocate and return a duplicated type AST node.  */

pkl_ast_node
pkl_ast_dup_type (pkl_ast_node type)
{
  pkl_ast_node t, new = pkl_ast_make_type (PKL_AST_AST (type));
  
  PKL_AST_TYPE_CODE (new) = PKL_AST_TYPE_CODE (type);
  PKL_AST_TYPE_COMPLETE (new) = PKL_AST_TYPE_COMPLETE (type);
  
  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      PKL_AST_TYPE_I_SIZE (new) = PKL_AST_TYPE_I_SIZE (type);
      PKL_AST_TYPE_I_SIGNED (new) = PKL_AST_TYPE_I_SIGNED (type);
      break;
    case PKL_TYPE_ARRAY:
      {
        pkl_ast_node etype
          = pkl_ast_dup_type (PKL_AST_TYPE_A_ETYPE (type));
        PKL_AST_TYPE_A_NELEM (new) = ASTREF (PKL_AST_TYPE_A_NELEM (type));
        PKL_AST_TYPE_A_ETYPE (new) = ASTREF (etype);
      }
      break;
    case PKL_TYPE_STRUCT:
      PKL_AST_TYPE_S_NELEM (new) = PKL_AST_TYPE_S_NELEM (type);
      for (t = PKL_AST_TYPE_S_ELEMS (type); t; t = PKL_AST_CHAIN (t))
        {
          pkl_ast_node struct_type_elem_name
            = PKL_AST_STRUCT_ELEM_TYPE_NAME (t);
          pkl_ast_node struct_type_elem_type
            = PKL_AST_STRUCT_ELEM_TYPE_TYPE (t);
          pkl_ast_node new_struct_type_elem_name
            = (struct_type_elem_name
               ? pkl_ast_make_identifier (PKL_AST_AST (new),
                                          PKL_AST_IDENTIFIER_POINTER (struct_type_elem_name))
               : NULL);
          pkl_ast_node struct_type_elem
            = pkl_ast_make_struct_elem_type (PKL_AST_AST (new),
                                             new_struct_type_elem_name,
                                             pkl_ast_dup_type (struct_type_elem_type));

          PKL_AST_TYPE_S_ELEMS (new)
            = pkl_ast_chainon (PKL_AST_TYPE_S_ELEMS (new),
                               struct_type_elem);
          PKL_AST_TYPE_S_ELEMS (new) = ASTREF (PKL_AST_TYPE_S_ELEMS (new));
        }
      break;
    case PKL_TYPE_FUNCTION:
      PKL_AST_TYPE_F_RTYPE (new)
        = pkl_ast_dup_type (PKL_AST_TYPE_F_RTYPE (type));
      PKL_AST_TYPE_F_NARG (new) = PKL_AST_TYPE_F_NARG (type);
      for (t = PKL_AST_TYPE_F_ARGS (type); t; t = PKL_AST_CHAIN (t))
        {
          pkl_ast_node fun_type_arg_type
            = PKL_AST_FUNC_ARG_TYPE_TYPE (t);

          pkl_ast_node function_type_arg
            = pkl_ast_make_func_arg_type (PKL_AST_AST (new),
                                              fun_type_arg_type);

          PKL_AST_TYPE_F_ARGS (new)
            = pkl_ast_chainon (PKL_AST_TYPE_F_ARGS (new),
                               function_type_arg);
          PKL_AST_TYPE_F_ARGS (new) = ASTREF (PKL_AST_TYPE_F_ARGS (new));
        }
      break;
    case PKL_TYPE_OFFSET:
      /* Fallthrough.  */
    case PKL_TYPE_STRING:
      /* Fallthrough.  */
    default:
      break;
    }

  return new;
}

/* Return whether two given type AST nodes are equal, i.e. they denote
   the same type.  */

int
pkl_ast_type_equal (pkl_ast_node a, pkl_ast_node b)
{
  if (PKL_AST_TYPE_CODE (a) != PKL_AST_TYPE_CODE (b))
    return 0;

  switch (PKL_AST_TYPE_CODE (a))
    {
    case PKL_TYPE_INTEGRAL:
      return (PKL_AST_TYPE_I_SIZE (a) == PKL_AST_TYPE_I_SIZE (b)
              && PKL_AST_TYPE_I_SIGNED (a) == PKL_AST_TYPE_I_SIGNED (b));
      break;
    case PKL_TYPE_ARRAY:
      {
        return pkl_ast_type_equal (PKL_AST_TYPE_A_ETYPE (a),
                                   PKL_AST_TYPE_A_ETYPE (b));
        break;
      }
    case PKL_TYPE_STRUCT:
      {
        pkl_ast_node sa, sb;
  
        if (PKL_AST_TYPE_S_NELEM (a) != PKL_AST_TYPE_S_NELEM (b))
          return 0;
        for (sa = PKL_AST_TYPE_S_ELEMS (a), sb = PKL_AST_TYPE_S_ELEMS (b);
             sa && sb;
             sa = PKL_AST_CHAIN (sa), sb = PKL_AST_CHAIN (sb))
          {
            if (strcmp (PKL_AST_IDENTIFIER_POINTER (PKL_AST_STRUCT_ELEM_TYPE_NAME (sa)),
                        PKL_AST_IDENTIFIER_POINTER (PKL_AST_STRUCT_ELEM_TYPE_NAME (sb)))
                || !pkl_ast_type_equal (PKL_AST_STRUCT_ELEM_TYPE_TYPE (sa),
                                        PKL_AST_STRUCT_ELEM_TYPE_TYPE (sb)))
              return 0;
          }
        break;
      }
    case PKL_TYPE_FUNCTION:
      {
        pkl_ast_node fa, fb;

        if (PKL_AST_TYPE_F_NARG (a) != PKL_AST_TYPE_F_NARG (b))
          return 0;
        for (fa = PKL_AST_TYPE_F_ARGS (a), fb = PKL_AST_TYPE_F_ARGS (b);
             fa && fb;
             fa = PKL_AST_CHAIN (fa), fb = PKL_AST_CHAIN (fb))
          {
            if (!pkl_ast_type_equal (PKL_AST_FUNC_ARG_TYPE_TYPE (fa),
                                     PKL_AST_FUNC_ARG_TYPE_TYPE (fb)))
              return 0;
          }
        break;
      }
    case PKL_TYPE_OFFSET:
      return pkl_ast_type_equal (PKL_AST_TYPE_O_BASE_TYPE (a),
                                 PKL_AST_TYPE_O_BASE_TYPE (b));
      break;
    case PKL_TYPE_STRING:
      /* Fallthrough.  */
    default:
      break;
    }

  return 1;
}

/* Build and return an expression that computes the size of TYPE in
   bits, as an unsigned 64-bit value.  */

pkl_ast_node
pkl_ast_sizeof_type (pkl_ast ast, pkl_ast_node type)
{
  pkl_ast_node res;
  pkl_ast_node res_type
    = pkl_ast_make_integral_type (ast, 64, 0);
  PKL_AST_LOC (res_type) = PKL_AST_LOC (type);

  /* This function should only be called on complete types.  */
  assert (PKL_AST_TYPE_COMPLETE (type)
          == PKL_AST_TYPE_COMPLETE_YES);

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      {
        res = pkl_ast_make_integer (ast, PKL_AST_TYPE_I_SIZE (type));
        PKL_AST_LOC (res) = PKL_AST_LOC (type);
        PKL_AST_TYPE (res) = ASTREF (res_type);
        break;
      }
    case PKL_TYPE_ARRAY:
      {
        pkl_ast_node sizeof_etype
          = pkl_ast_sizeof_type (ast,
                                 PKL_AST_TYPE_A_ETYPE (type));
        res= pkl_ast_make_binary_exp (ast, PKL_AST_OP_MUL,
                                      PKL_AST_TYPE_A_NELEM (type),
                                      sizeof_etype);
        PKL_AST_LOC (res) = PKL_AST_LOC (type);
        PKL_AST_TYPE (res) = ASTREF (res_type);
        break;
      }
    case PKL_TYPE_STRUCT:
      {
        pkl_ast_node t;

        res = pkl_ast_make_integer (ast, 0);
        PKL_AST_TYPE (res) = ASTREF (res_type);
        PKL_AST_LOC (res) = PKL_AST_LOC (type);

        for (t = PKL_AST_TYPE_S_ELEMS (type); t; t = PKL_AST_CHAIN (t))
          {
            pkl_ast_node elem_type = PKL_AST_STRUCT_ELEM_TYPE_TYPE (t);

            res = pkl_ast_make_binary_exp (ast, PKL_AST_OP_ADD,
                                           res,
                                           pkl_ast_sizeof_type (ast, elem_type));
            PKL_AST_TYPE (res) = ASTREF (res_type);
            PKL_AST_LOC (res) = PKL_AST_LOC (type);
          }

        break;
      }
    case PKL_TYPE_FUNCTION:
      {
        /* By convention functions have sizeof 0#b  */
        res = pkl_ast_make_integer (ast, 0);
        PKL_AST_TYPE (res) = ASTREF (res_type);
        PKL_AST_LOC (res) = PKL_AST_LOC (type);
        break;
      }
    case PKL_TYPE_OFFSET:
      return pkl_ast_sizeof_type (ast, PKL_AST_TYPE_O_BASE_TYPE (type));
      break;
    case PKL_TYPE_STRING:
    default:
      assert (0);
      break;
    }

  return res;
}

/* Return PKL_AST_TYPE_COMPLETE_YES if the given TYPE is a complete
   type.  Return PKL_AST_TYPE_COMPLETE_NO otherwise.  This function
   assumes that the children of TYPE have correct completeness
   annotations.  */

int
pkl_ast_type_is_complete (pkl_ast_node type)
{
  int complete = PKL_AST_TYPE_COMPLETE_UNKNOWN;
  
  switch (PKL_AST_TYPE_CODE (type))
    {
      /* Integral, offset and struct types are always complete.  */
    case PKL_TYPE_INTEGRAL:
    case PKL_TYPE_OFFSET:
    case PKL_TYPE_STRUCT:
      complete = PKL_AST_TYPE_COMPLETE_YES;
      break;
      /* String types are never complete.  */
    case PKL_TYPE_STRING:
      complete = PKL_AST_TYPE_COMPLETE_NO;
      break;
      /* Array types are complete if the number of elements in the
         array are specified and it is a literal expression.  */
    case PKL_TYPE_ARRAY:
      {
        pkl_ast_node nelem = PKL_AST_TYPE_A_NELEM (type);

        if (nelem && PKL_AST_LITERAL_P (nelem))
          complete = PKL_AST_TYPE_COMPLETE_YES;
        else
          complete = PKL_AST_TYPE_COMPLETE_NO;
      }
    default:
      break;
    }

  assert (complete != PKL_AST_TYPE_COMPLETE_UNKNOWN);
  return complete;
}

/* Print a textual description of TYPE to the file OUT.  If TYPE is a
   named type then it's given name is preferred if USE_GIVEN_NAME is
   1.  */

void
pkl_print_type (FILE *out, pkl_ast_node type, int use_given_name)
{
  assert (PKL_AST_CODE (type) == PKL_AST_TYPE);

  /* Use the type's given name, if requested and this specific type
     instance is named.  */
  if (use_given_name
      && PKL_AST_TYPE_NAME (type))
    {
      fprintf (out,
               PKL_AST_IDENTIFIER_POINTER (PKL_AST_TYPE_NAME (type)));
      return;
    }

  /* Otherwise, print a description of the type, as terse as possible
     but complete.  The descriptions should follow the same
     style/syntax/conventions used in both the language specification
     and the PVM.  */
  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      if (!PKL_AST_TYPE_I_SIGNED (type))
        fputc ('u', out);
      fprintf (out, "int<%zd>", PKL_AST_TYPE_I_SIZE (type));
      break;
    case PKL_TYPE_VOID:
      fprintf (out, "void");
      break;
    case PKL_TYPE_STRING:
      fprintf (out, "string");
      break;
    case PKL_TYPE_ARRAY:
      pkl_print_type (out, PKL_AST_TYPE_A_ETYPE (type),
                      use_given_name);
      fputs ("[]", out);
      break;
    case PKL_TYPE_STRUCT:
      {
        pkl_ast_node t;

        fputs ("struct {", out);

        for (t = PKL_AST_TYPE_S_ELEMS (type); t;
             t = PKL_AST_CHAIN (t))
          {
            pkl_ast_node ename
              = PKL_AST_STRUCT_ELEM_TYPE_NAME (t);
            pkl_ast_node etype
              = PKL_AST_STRUCT_ELEM_TYPE_TYPE (t);

            pkl_print_type (out, etype, use_given_name);
            if (ename)
              fprintf (out, " %s",
                       PKL_AST_IDENTIFIER_POINTER (ename));
            fputc (';', out);
          }
        fputs ("}", out);
        break;
      }
    case PKL_TYPE_FUNCTION:
      {
        pkl_ast_node t;

        if (PKL_AST_TYPE_F_NARG (type) > 0)
          {
            fputc ('(', out);
            
            for (t = PKL_AST_TYPE_F_ARGS (type); t;
                 t = PKL_AST_CHAIN (t))
              {
                pkl_ast_node atype
                  = PKL_AST_FUNC_ARG_TYPE_TYPE (t);
                
                if (t != PKL_AST_TYPE_F_ARGS (type))
                  fputc (',', out);
                pkl_print_type (out, atype, use_given_name);
              }
            
            fputc (')', out);
          }

        pkl_print_type (out,
                        PKL_AST_TYPE_F_RTYPE (type),
                        use_given_name);
        fputc (':', out);
        break;
      }
    case PKL_TYPE_OFFSET:
      {
        pkl_ast_node unit = PKL_AST_TYPE_O_UNIT (type);

        fputs ("offset<", out);
        pkl_print_type (out, PKL_AST_TYPE_O_BASE_TYPE (type),
                        use_given_name);
        fputc (',', out);

        if (PKL_AST_CODE (unit) == PKL_AST_TYPE)
          pkl_print_type (out, unit, use_given_name);
        else if (PKL_AST_CODE (unit) == PKL_AST_IDENTIFIER)
          fputs (PKL_AST_IDENTIFIER_POINTER (unit), out);
        else if (PKL_AST_CODE (unit) == PKL_AST_INTEGER)
          fprintf (out, "%" PRIu64, PKL_AST_INTEGER_VALUE (unit));
        else
          assert (0);
        
        fputc ('>', out);
        break;
      }
    case PKL_TYPE_NOTYPE:
    default:
      assert (0);
      break;
    }
}

/* Like pkl_print_type, but return the string describing the type in a
   string.  It is up to the caller to free the string memory.  */

char *
pkl_type_str (pkl_ast_node type, int use_given_name)
{
  char *str;
  size_t str_size;
  FILE *buffer = open_memstream (&str, &str_size);

  pkl_print_type (buffer, type, use_given_name);
  fclose (buffer);
  return str;
}

/* Build and return an AST node for an enum.  */

pkl_ast_node
pkl_ast_make_enum (pkl_ast ast,
                   pkl_ast_node tag, pkl_ast_node values)
{
  pkl_ast_node enumeration
    = pkl_ast_make_node (ast, PKL_AST_ENUM);

  assert (tag && values);

  PKL_AST_ENUM_TAG (enumeration) = ASTREF (tag);
  PKL_AST_ENUM_VALUES (enumeration) = ASTREF (values);

  return enumeration;
}

/* Build and return an AST node for an array.  */

pkl_ast_node
pkl_ast_make_array (pkl_ast ast,
                    size_t nelem, size_t ninitializer,
                    pkl_ast_node initializers)
{
  pkl_ast_node array
    = pkl_ast_make_node (ast, PKL_AST_ARRAY);

  PKL_AST_ARRAY_NELEM (array) = nelem;
  PKL_AST_ARRAY_NINITIALIZER (array) = ninitializer;
  PKL_AST_ARRAY_INITIALIZERS (array) = ASTREF (initializers);

  return array;
}

/* Build and return an AST node for an array element.  */

pkl_ast_node
pkl_ast_make_array_initializer (pkl_ast ast,
                                pkl_ast_node index, pkl_ast_node exp)
{
  pkl_ast_node initializer
    = pkl_ast_make_node (ast, PKL_AST_ARRAY_INITIALIZER);

  PKL_AST_ARRAY_INITIALIZER_INDEX (initializer) = ASTREF (index);
  PKL_AST_ARRAY_INITIALIZER_EXP (initializer) = ASTREF (exp);

  return initializer;
}

/* Build and return an AST node for a struct.  */

pkl_ast_node
pkl_ast_make_struct (pkl_ast ast,
                     size_t nelem,
                     pkl_ast_node elems)
{
  pkl_ast_node sct = pkl_ast_make_node (ast, PKL_AST_STRUCT);

  PKL_AST_STRUCT_NELEM (sct) = nelem;
  PKL_AST_STRUCT_ELEMS (sct) = ASTREF (elems);

  return sct;
}

/* Build and return an AST node for a struct element.  */

pkl_ast_node
pkl_ast_make_struct_elem (pkl_ast ast,
                          pkl_ast_node name,
                          pkl_ast_node exp)
{
  pkl_ast_node elem = pkl_ast_make_node (ast, PKL_AST_STRUCT_ELEM);

  if (name != NULL)
    PKL_AST_STRUCT_ELEM_NAME (elem) = ASTREF (name);
  PKL_AST_STRUCT_ELEM_EXP (elem) = ASTREF (exp);

  return elem;
}

/* Build and return an AST node for a declaration.  */

pkl_ast_node
pkl_ast_make_decl (pkl_ast ast, int kind, pkl_ast_node name,
                   pkl_ast_node initial, const char *source)
{
  pkl_ast_node decl = pkl_ast_make_node (ast, PKL_AST_DECL);

  assert (name);

  PKL_AST_DECL_KIND (decl) = kind;
  PKL_AST_DECL_NAME (decl) = ASTREF (name);
  PKL_AST_DECL_INITIAL (decl) = ASTREF (initial);
  if (source)
    PKL_AST_DECL_SOURCE (decl) = xstrdup (source);

  return decl;
}

/* Build and return an AST node for an offset construct.  */

pkl_ast_node
pkl_ast_make_offset (pkl_ast ast,
                     pkl_ast_node magnitude, pkl_ast_node unit)
{
  pkl_ast_node offset = pkl_ast_make_node (ast, PKL_AST_OFFSET);

  assert (unit);
  
  if (magnitude != NULL)
    PKL_AST_OFFSET_MAGNITUDE (offset) = ASTREF (magnitude);
  PKL_AST_OFFSET_UNIT (offset) = ASTREF (unit);

  return offset;
}

/* Get an identifier with the name of a predefined offset unit (like
   b, B, Kb, etc) and return an integral expression evaluating to its
   number of bits.  If the identifier doesn't identify a predefined
   offset unit, return NULL.  */

pkl_ast_node
pkl_ast_id_to_offset_unit (pkl_ast ast, pkl_ast_node id)
{
  pkl_ast_node unit, unit_type;
  size_t factor = 0;
  const char *id_pointer = PKL_AST_IDENTIFIER_POINTER (id);

  if (strcmp (id_pointer, "b") == 0)
    factor = PKL_AST_OFFSET_UNIT_BITS;
  else if (strcmp (id_pointer, "N") == 0)
    factor = PKL_AST_OFFSET_UNIT_NIBBLES;
  else if (strcmp (id_pointer, "B") == 0)
    factor = PKL_AST_OFFSET_UNIT_BYTES;
  else if (strcmp (id_pointer, "Kb") == 0)
    factor = PKL_AST_OFFSET_UNIT_KILOBITS;
  else if (strcmp (id_pointer, "KB") == 0)
    factor =  PKL_AST_OFFSET_UNIT_KILOBYTES;
  else if (strcmp (id_pointer, "Mb") == 0)
    factor = PKL_AST_OFFSET_UNIT_MEGABITS;
  else if (strcmp (id_pointer, "MB") == 0)
    factor = PKL_AST_OFFSET_UNIT_MEGABYTES;
  else if (strcmp (id_pointer, "Gb") == 0)
    factor = PKL_AST_OFFSET_UNIT_GIGABITS;
  else
    /* Invalid offset unit.  */
    return NULL;

  unit_type = pkl_ast_make_integral_type (ast, 64, 0);
  PKL_AST_LOC (unit_type) = PKL_AST_LOC (id);

  unit = pkl_ast_make_integer (ast, factor);
  PKL_AST_LOC (unit) = PKL_AST_LOC (id);
  PKL_AST_TYPE (unit) = ASTREF (unit_type);

  return unit;
}

/* Build and return an AST node for a cast.  */

pkl_ast_node
pkl_ast_make_cast (pkl_ast ast,
                   pkl_ast_node type, pkl_ast_node exp)
{
  pkl_ast_node cast = pkl_ast_make_node (ast, PKL_AST_CAST);

  assert (type && exp);

  PKL_AST_CAST_TYPE (cast) = ASTREF (type);
  PKL_AST_CAST_EXP (cast) = ASTREF (exp);

  return cast;
}

/* Build and return an AST node for a map.  */

pkl_ast_node
pkl_ast_make_map (pkl_ast ast,
                  pkl_ast_node type, pkl_ast_node offset)
{
  pkl_ast_node map = pkl_ast_make_node (ast, PKL_AST_MAP);

  assert (type && offset);

  PKL_AST_MAP_TYPE (map) = ASTREF (type);
  PKL_AST_MAP_OFFSET (map) = ASTREF (offset);

  return map;
}

/* Build and return an AST node for a struct constructor.  */

pkl_ast_node
pkl_ast_make_scons (pkl_ast ast,
                    pkl_ast_node type, pkl_ast_node value,
                    int constructor_back, int constructor_over)
{
  pkl_ast_node scons = pkl_ast_make_node (ast, PKL_AST_SCONS);

  assert (type && value);

  PKL_AST_SCONS_TYPE (scons) = ASTREF (type);
  PKL_AST_SCONS_VALUE (scons) = ASTREF (value);
  PKL_AST_SCONS_CONSTRUCTOR_BACK (scons) = constructor_back;
  PKL_AST_SCONS_CONSTRUCTOR_OVER (scons) = constructor_over;

  return scons;
}

/* Build and return an AST node for a function call.  */

pkl_ast_node
pkl_ast_make_funcall (pkl_ast ast,
                      pkl_ast_node function, pkl_ast_node args)
{
  pkl_ast_node funcall = pkl_ast_make_node (ast,
                                            PKL_AST_FUNCALL);

  assert (function);

  PKL_AST_FUNCALL_FUNCTION (funcall) = ASTREF (function);
  if (args)
    PKL_AST_FUNCALL_ARGS (funcall) = ASTREF (args);
  return funcall;
}

/* Build and return an AST node for an actual argument of a function
   call.  */

pkl_ast_node
pkl_ast_make_funcall_arg (pkl_ast ast, pkl_ast_node exp)
{
  pkl_ast_node funcall_arg = pkl_ast_make_node (ast,
                                                PKL_AST_FUNCALL_ARG);

  assert (exp);

  PKL_AST_FUNCALL_ARG_EXP (funcall_arg) = ASTREF (exp);
  return funcall_arg;
}

/* Build and return an AST node for a variable reference.  */

pkl_ast_node
pkl_ast_make_var (pkl_ast ast, pkl_ast_node name,
                  pkl_ast_node decl, int back, int over)
{
  pkl_ast_node var = pkl_ast_make_node (ast, PKL_AST_VAR);

  assert (name && decl);

  PKL_AST_VAR_NAME (var) = ASTREF (name);
  PKL_AST_VAR_DECL (var) = ASTREF (decl);
  PKL_AST_VAR_BACK (var) = back;
  PKL_AST_VAR_OVER (var) = over;

  return var;
}

/* Build and return an AST node for a compound statement.  */

pkl_ast_node
pkl_ast_make_comp_stmt (pkl_ast ast, pkl_ast_node stmts)
{
  pkl_ast_node comp_stmt = pkl_ast_make_node (ast,
                                              PKL_AST_COMP_STMT);

  if (stmts)
    PKL_AST_COMP_STMT_STMTS (comp_stmt) = ASTREF (stmts);
  PKL_AST_COMP_STMT_BUILTIN (comp_stmt) = PKL_AST_BUILTIN_NONE;
  return comp_stmt;
}

/* Build and return an AST node for a compiler builtin.  */

pkl_ast_node
pkl_ast_make_builtin (pkl_ast ast, int builtin)
{
  pkl_ast_node comp_stmt = pkl_ast_make_node (ast,
                                              PKL_AST_COMP_STMT);

  PKL_AST_COMP_STMT_BUILTIN (comp_stmt) = builtin;
  return comp_stmt;
}

/* Build and return an AST node for an assignment statement.  */

pkl_ast_node
pkl_ast_make_ass_stmt (pkl_ast ast, pkl_ast_node lvalue,
                       pkl_ast_node exp)
{
  pkl_ast_node ass_stmt = pkl_ast_make_node (ast,
                                             PKL_AST_ASS_STMT);

  assert (lvalue && exp);

  PKL_AST_ASS_STMT_LVALUE (ass_stmt) = ASTREF (lvalue);
  PKL_AST_ASS_STMT_EXP (ass_stmt) = ASTREF (exp);

  return ass_stmt;
}

/* Build and return an AST node for a conditional statement.  */

pkl_ast_node
pkl_ast_make_if_stmt (pkl_ast ast, pkl_ast_node exp,
                      pkl_ast_node then_stmt, pkl_ast_node else_stmt)
{
  pkl_ast_node if_stmt = pkl_ast_make_node (ast, PKL_AST_IF_STMT);

  assert (exp && then_stmt);

  PKL_AST_IF_STMT_EXP (if_stmt) = ASTREF (exp);
  PKL_AST_IF_STMT_THEN_STMT (if_stmt) = ASTREF (then_stmt);
  if (else_stmt)
    PKL_AST_IF_STMT_ELSE_STMT (if_stmt) = ASTREF (else_stmt);

  return if_stmt;
}

/* Build and return an AST node for a loop statement.  */

pkl_ast_node
pkl_ast_make_loop_stmt (pkl_ast ast, pkl_ast_node condition,
                        pkl_ast_node body)
{
  pkl_ast_node loop_stmt
    = pkl_ast_make_node (ast, PKL_AST_LOOP_STMT);

  assert (condition && body);

  PKL_AST_LOOP_STMT_CONDITION (loop_stmt) = ASTREF (condition);
  PKL_AST_LOOP_STMT_BODY (loop_stmt) = ASTREF (body);

  return loop_stmt;
}

/* Build and return an AST node for a return statement.  */

pkl_ast_node
pkl_ast_make_return_stmt (pkl_ast ast, pkl_ast_node exp)
{
  pkl_ast_node return_stmt = pkl_ast_make_node (ast,
                                                PKL_AST_RETURN_STMT);

  PKL_AST_RETURN_STMT_EXP (return_stmt) = ASTREF (exp);
  return return_stmt;
}

/* Build and return an AST node for a "null statement".  */

pkl_ast_node
pkl_ast_make_null_stmt (pkl_ast ast)
{
  pkl_ast_node null_stmt = pkl_ast_make_node (ast,
                                              PKL_AST_NULL_STMT);
  return null_stmt;
}

/* Build and return an AST node for an "expression statement".  */

pkl_ast_node
pkl_ast_make_exp_stmt (pkl_ast ast, pkl_ast_node exp)
{
  pkl_ast_node exp_stmt = pkl_ast_make_node (ast,
                                             PKL_AST_EXP_STMT);

  assert (exp);

  PKL_AST_EXP_STMT_EXP (exp_stmt) = ASTREF (exp);
  return exp_stmt;
}

/* Build and return an AST node for a try-catch statement.  */

pkl_ast_node
pkl_ast_make_try_catch_stmt (pkl_ast ast, pkl_ast_node code,
                             pkl_ast_node handler, pkl_ast_node arg,
                             pkl_ast_node exp)
{
  pkl_ast_node try_catch_stmt = pkl_ast_make_node (ast,
                                                   PKL_AST_TRY_CATCH_STMT);

  assert (code && handler);
  assert (!arg || !exp);

  PKL_AST_TRY_CATCH_STMT_CODE (try_catch_stmt) = ASTREF (code);
  PKL_AST_TRY_CATCH_STMT_HANDLER (try_catch_stmt) = ASTREF (handler);
  if (arg)
    PKL_AST_TRY_CATCH_STMT_ARG (try_catch_stmt) = ASTREF (arg);
  if (exp)
    PKL_AST_TRY_CATCH_STMT_EXP (try_catch_stmt) = ASTREF (exp);

  return try_catch_stmt;
}

/* Build and return an AST node for a `raise' statement.  */

pkl_ast_node
pkl_ast_make_raise_stmt (pkl_ast ast, pkl_ast_node exp)
{
  pkl_ast_node raise_stmt = pkl_ast_make_node (ast,
                                               PKL_AST_RAISE_STMT);

  if (exp)
    PKL_AST_RAISE_STMT_EXP (raise_stmt) = ASTREF (exp);
  return raise_stmt;
}

/* Build and return an AST node for a PKL program.  */

pkl_ast_node
pkl_ast_make_program (pkl_ast ast, pkl_ast_node elems)
{
  pkl_ast_node program
    = pkl_ast_make_node (ast, PKL_AST_PROGRAM);

  PKL_AST_PROGRAM_ELEMS (program) = ASTREF (elems);
  return program;
}

/* Free all allocated resources used by AST.  Note that nodes marked
   as "registered", as well as their children, are not disposed.  */

void
pkl_ast_node_free (pkl_ast_node ast)
{
  pkl_ast_node t, n;
  size_t i;
  
  if (ast == NULL)
    return;

  assert (PKL_AST_REFCOUNT (ast) > 0);

  if (PKL_AST_REFCOUNT (ast) > 1)
    {
      PKL_AST_REFCOUNT (ast) -= 1;
      return;
    }

  switch (PKL_AST_CODE (ast))
    {
    case PKL_AST_PROGRAM:

      for (t = PKL_AST_PROGRAM_ELEMS (ast); t; t = n)
        {
          n = PKL_AST_CHAIN (t);
          pkl_ast_node_free (t);
        }
      
      break;

    case PKL_AST_EXP:

      for (i = 0; i < PKL_AST_EXP_NUMOPS (ast); i++)
        pkl_ast_node_free (PKL_AST_EXP_OPERAND (ast, i));

      break;
      
    case PKL_AST_COND_EXP:

      pkl_ast_node_free (PKL_AST_COND_EXP_COND (ast));
      pkl_ast_node_free (PKL_AST_COND_EXP_THENEXP (ast));
      pkl_ast_node_free (PKL_AST_COND_EXP_ELSEEXP (ast));
      break;
      
    case PKL_AST_ENUM:

      pkl_ast_node_free (PKL_AST_ENUM_TAG (ast));
      
      for (t = PKL_AST_ENUM_VALUES (ast); t; t = n)
        {
          n = PKL_AST_CHAIN (t);
          pkl_ast_node_free (t);
        }

      break;
      
    case PKL_AST_ENUMERATOR:

      pkl_ast_node_free (PKL_AST_ENUMERATOR_IDENTIFIER (ast));
      pkl_ast_node_free (PKL_AST_ENUMERATOR_VALUE (ast));
      break;
      
    case PKL_AST_TYPE:

      free (PKL_AST_TYPE_NAME (ast));
      switch (PKL_AST_TYPE_CODE (ast))
        {
        case PKL_TYPE_ARRAY:
          pkl_ast_node_free (PKL_AST_TYPE_A_ETYPE (ast));
          break;
        case PKL_TYPE_STRUCT:
          for (t = PKL_AST_TYPE_S_ELEMS (ast); t; t = n)
            {
              n = PKL_AST_CHAIN (t);
              pkl_ast_node_free (t);
            }
          break;
        case PKL_TYPE_FUNCTION:
          for (t = PKL_AST_TYPE_F_ARGS (ast); t; t = n)
            {
              n = PKL_AST_CHAIN (t);
              pkl_ast_node_free (t);
            }
          break;
        case PKL_TYPE_INTEGRAL:
        case PKL_TYPE_STRING:
        case PKL_TYPE_OFFSET:
        default:
          break;
        }
      
      break;

    case PKL_AST_STRUCT_ELEM_TYPE:

      pkl_ast_node_free (PKL_AST_STRUCT_ELEM_TYPE_NAME (ast));
      pkl_ast_node_free (PKL_AST_STRUCT_ELEM_TYPE_TYPE (ast));
      break;

    case PKL_AST_FUNC_ARG_TYPE:

      pkl_ast_node_free (PKL_AST_FUNC_ARG_TYPE_TYPE (ast));
      break;
      
    case PKL_AST_ARRAY_REF:

      pkl_ast_node_free (PKL_AST_ARRAY_REF_ARRAY (ast));
      pkl_ast_node_free (PKL_AST_ARRAY_REF_INDEX (ast));
      break;

    case PKL_AST_FUNC:

      pkl_ast_node_free (PKL_AST_FUNC_RET_TYPE (ast));
      pkl_ast_node_free (PKL_AST_FUNC_BODY (ast));
      for (t = PKL_AST_FUNC_ARGS (ast); t; t = n)
            {
              n = PKL_AST_CHAIN (t);
              pkl_ast_node_free (t);
            }
      break;

    case PKL_AST_FUNC_ARG:

      pkl_ast_node_free (PKL_AST_FUNC_ARG_TYPE (ast));
      pkl_ast_node_free (PKL_AST_FUNC_ARG_IDENTIFIER (ast));
      break;
      
    case PKL_AST_STRING:

      free (PKL_AST_STRING_POINTER (ast));
      break;
      
    case PKL_AST_IDENTIFIER:

      free (PKL_AST_IDENTIFIER_POINTER (ast));
      break;
      
    case PKL_AST_STRUCT_REF:

      pkl_ast_node_free (PKL_AST_STRUCT_REF_STRUCT (ast));
      pkl_ast_node_free (PKL_AST_STRUCT_REF_IDENTIFIER (ast));
      break;
      
    case PKL_AST_STRUCT_ELEM:

      pkl_ast_node_free (PKL_AST_STRUCT_ELEM_NAME (ast));
      pkl_ast_node_free (PKL_AST_STRUCT_ELEM_EXP (ast));
      break;

    case PKL_AST_STRUCT:

      for (t = PKL_AST_STRUCT_ELEMS (ast); t; t = n)
        {
          n = PKL_AST_CHAIN (t);
          pkl_ast_node_free (t);
        }
      break;
      
    case PKL_AST_ARRAY_INITIALIZER:

      pkl_ast_node_free (PKL_AST_ARRAY_INITIALIZER_INDEX (ast));
      pkl_ast_node_free (PKL_AST_ARRAY_INITIALIZER_EXP (ast));
      break;

    case PKL_AST_ARRAY:

      for (t = PKL_AST_ARRAY_INITIALIZERS (ast); t; t = n)
        {
          n = PKL_AST_CHAIN (t);
          pkl_ast_node_free (t);
        }
      
      break;

    case PKL_AST_DECL:

      free (PKL_AST_DECL_SOURCE (ast));
      pkl_ast_node_free (PKL_AST_DECL_NAME (ast));
      pkl_ast_node_free (PKL_AST_DECL_INITIAL (ast));
      break;

    case PKL_AST_OFFSET:

      pkl_ast_node_free (PKL_AST_OFFSET_MAGNITUDE (ast));
      pkl_ast_node_free (PKL_AST_OFFSET_UNIT (ast));
      break;

    case PKL_AST_CAST:

      pkl_ast_node_free (PKL_AST_CAST_TYPE (ast));
      pkl_ast_node_free (PKL_AST_CAST_EXP (ast));
      break;

    case PKL_AST_MAP:

      pkl_ast_node_free (PKL_AST_MAP_TYPE (ast));
      pkl_ast_node_free (PKL_AST_MAP_OFFSET (ast));
      break;

    case PKL_AST_SCONS:

      pkl_ast_node_free (PKL_AST_SCONS_TYPE (ast));
      pkl_ast_node_free (PKL_AST_SCONS_VALUE (ast));
      break;

    case PKL_AST_FUNCALL:

      pkl_ast_node_free (PKL_AST_FUNCALL_FUNCTION (ast));
      for (t = PKL_AST_FUNCALL_ARGS (ast); t; t = n)
        {
          n = PKL_AST_CHAIN (t);
          pkl_ast_node_free (t);
        }
      
      break;

    case PKL_AST_FUNCALL_ARG:

      pkl_ast_node_free (PKL_AST_FUNCALL_ARG_EXP (ast));
      break;

    case PKL_AST_VAR:

      pkl_ast_node_free (PKL_AST_VAR_NAME (ast));
      pkl_ast_node_free (PKL_AST_VAR_DECL (ast));
      break;

    case PKL_AST_COMP_STMT:

      for (t = PKL_AST_COMP_STMT_STMTS (ast); t; t = n)
        {
          n = PKL_AST_CHAIN (t);
          pkl_ast_node_free (t);
        }

      break;

    case PKL_AST_ASS_STMT:

      pkl_ast_node_free (PKL_AST_ASS_STMT_LVALUE (ast));
      pkl_ast_node_free (PKL_AST_ASS_STMT_EXP (ast));
      break;

    case PKL_AST_IF_STMT:

      pkl_ast_node_free (PKL_AST_IF_STMT_EXP (ast));
      pkl_ast_node_free (PKL_AST_IF_STMT_THEN_STMT (ast));
      pkl_ast_node_free (PKL_AST_IF_STMT_ELSE_STMT (ast));
      break;

    case PKL_AST_LOOP_STMT:

      pkl_ast_node_free (PKL_AST_LOOP_STMT_CONDITION (ast));
      pkl_ast_node_free (PKL_AST_LOOP_STMT_BODY (ast));
      break;

    case PKL_AST_RETURN_STMT:

      pkl_ast_node_free (PKL_AST_RETURN_STMT_EXP (ast));
      break;

    case PKL_AST_EXP_STMT:

      pkl_ast_node_free (PKL_AST_EXP_STMT_EXP (ast));
      break;

    case PKL_AST_TRY_CATCH_STMT:

      pkl_ast_node_free (PKL_AST_TRY_CATCH_STMT_CODE (ast));
      pkl_ast_node_free (PKL_AST_TRY_CATCH_STMT_HANDLER (ast));
      pkl_ast_node_free (PKL_AST_TRY_CATCH_STMT_ARG (ast));
      pkl_ast_node_free (PKL_AST_TRY_CATCH_STMT_EXP (ast));
      break;

    case PKL_AST_RAISE_STMT:

      pkl_ast_node_free (PKL_AST_RAISE_STMT_EXP (ast));
      break;

    case PKL_AST_NULL_STMT:
      break;

    case PKL_AST_INTEGER:
      /* Fallthrough.  */
      break;
      
    default:
      assert (0);
    }

  pkl_ast_node_free (PKL_AST_TYPE (ast));
  free (ast);
}

/* Allocate and initialize a new AST and return it.  */

pkl_ast
pkl_ast_init (void)
{
  struct pkl_ast *ast;

  /* Allocate a new AST and initialize it to 0.  */
  ast = xmalloc (sizeof (struct pkl_ast));
  memset (ast, 0, sizeof (struct pkl_ast));

  return ast;
}

/* Free all the memory allocated to store the nodes of an AST.  */

void
pkl_ast_free (pkl_ast ast)
{
#if 0
  size_t i;
#endif

  if (ast == NULL)
    return;
  
  pkl_ast_node_free (ast->ast);
  free (ast->buffer);
  free (ast->filename);
  free (ast);
}

pkl_ast_node
pkl_ast_reverse (pkl_ast_node ast)
{
  pkl_ast_node prev = NULL, decl, next;

  for (decl = ast; decl != NULL; decl = next)
    {
      next = PKL_AST_CHAIN (decl);
      if (next)
        PKL_AST_REFCOUNT (next) -= 1;
      PKL_AST_CHAIN (decl) = ASTREF (prev);
      prev = decl;
    }

  return prev;
}

/* Annotate FUNCTIONs return statements with the function and their
   nest level within the function.  */

static void
pkl_ast_finish_returns_1 (pkl_ast_node function, pkl_ast_node stmt,
                          int *nframes)
{
  /* STMT can be a statement or a declaration.  */

  switch (PKL_AST_CODE (stmt))
    {
    case PKL_AST_RETURN_STMT:
      PKL_AST_RETURN_STMT_FUNCTION (stmt) = function; /* Note no ASTREF.  */
      PKL_AST_RETURN_STMT_NFRAMES (stmt) = *nframes;
      break;
    case PKL_AST_COMP_STMT:
      {
        pkl_ast_node t;

        *nframes += 1;
        for (t = PKL_AST_COMP_STMT_STMTS (stmt); t;
             t = PKL_AST_CHAIN (t))
          pkl_ast_finish_returns_1 (function, t, nframes);

        /* Pop the frame of the compount itself.  */
        *nframes -= 1;
        break;
      }
    case PKL_AST_IF_STMT:
      pkl_ast_finish_returns_1 (function,
                                PKL_AST_IF_STMT_THEN_STMT (stmt),
                                nframes);
      if (PKL_AST_IF_STMT_ELSE_STMT (stmt))
        pkl_ast_finish_returns_1 (function,
                                  PKL_AST_IF_STMT_ELSE_STMT (stmt),
                                  nframes);
      break;
    case PKL_AST_LOOP_STMT:
      pkl_ast_finish_returns_1 (function,
                                PKL_AST_LOOP_STMT_BODY (stmt),
                                nframes);
      break;
    case PKL_AST_TRY_CATCH_STMT:
      pkl_ast_finish_returns_1 (function,
                                PKL_AST_TRY_CATCH_STMT_CODE (stmt),
                                nframes);
      pkl_ast_finish_returns_1 (function,
                                PKL_AST_TRY_CATCH_STMT_HANDLER (stmt),
                                nframes);
      break;
    case PKL_AST_DECL:
    case PKL_AST_EXP_STMT:
    case PKL_AST_ASS_STMT:
    case PKL_AST_RAISE_STMT:
    case PKL_AST_NULL_STMT:
      break;
    default:
      assert (0);
      break;
    }
}

void
pkl_ast_finish_returns (pkl_ast_node function)
{
  int nframes = 0;
  pkl_ast_finish_returns_1 (function, PKL_AST_FUNC_BODY (function),
                            &nframes);
}

#ifdef PKL_DEBUG

/* The following macros are commodities to be used to keep the
   `pkl_ast_print' function as readable and easy to update as
   possible.  Do not use them anywhere else.  */

#define IPRINTF(...)                            \
  do                                            \
    {                                           \
      int i;                                    \
      for (i = 0; i < indent; i++)              \
        if (indent >= 2 && i % 2 == 0)          \
          fprintf (fd, "|");                    \
        else                                    \
          fprintf (fd, " ");                    \
      fprintf (fd, __VA_ARGS__);                \
    } while (0)

#define PRINT_AST_IMM(NAME,MACRO,FMT)                    \
  do                                                     \
    {                                                    \
      IPRINTF (#NAME ":\n");                             \
      IPRINTF ("  " FMT "\n", PKL_AST_##MACRO (ast));    \
    }                                                    \
  while (0)

#define PRINT_AST_SUBAST(NAME,MACRO)                            \
  do                                                            \
    {                                                           \
      IPRINTF (#NAME ":\n");                                    \
      pkl_ast_print_1 (fd, PKL_AST_##MACRO (ast), indent + 2);  \
    }                                                           \
  while (0)

#define PRINT_AST_OPT_IMM(NAME,MACRO,FMT)       \
  if (PKL_AST_##MACRO (ast))                    \
    {                                           \
      PRINT_AST_IMM (NAME, MACRO, FMT);         \
    }

#define PRINT_AST_OPT_SUBAST(NAME,MACRO)        \
  if (PKL_AST_##MACRO (ast))                    \
    {                                           \
      PRINT_AST_SUBAST (NAME, MACRO);           \
    }

#define PRINT_AST_SUBAST_CHAIN(MACRO)           \
  for (child = PKL_AST_##MACRO (ast);           \
       child;                                   \
       child = PKL_AST_CHAIN (child))           \
    {                                           \
      pkl_ast_print_1 (fd, child, indent + 2);  \
    }

/* Auxiliary function used by `pkl_ast_print', defined below.  */

static void
pkl_ast_print_1 (FILE *fd, pkl_ast_node ast, int indent)
{
  pkl_ast_node child;
  size_t i;
 
  if (ast == NULL)
    {
      IPRINTF ("NULL::\n");
      return;
    }

#define PRINT_COMMON_FIELDS                                     \
  do                                                            \
    {                                                           \
      IPRINTF ("uid: %" PRIu64 "\n", PKL_AST_UID (ast));        \
      IPRINTF ("refcount: %d\n", PKL_AST_REFCOUNT (ast));\
      IPRINTF ("location: %d,%d-%d,%d\n",                       \
               PKL_AST_LOC (ast).first_line,                    \
               PKL_AST_LOC (ast).first_column,                  \
               PKL_AST_LOC (ast).last_line,                     \
               PKL_AST_LOC (ast).last_column);                  \
    }                                                           \
  while (0)
  
  switch (PKL_AST_CODE (ast))
    {
    case PKL_AST_PROGRAM:
      IPRINTF ("PROGRAM::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST_CHAIN (PROGRAM_ELEMS);
      break;

    case PKL_AST_IDENTIFIER:
      IPRINTF ("IDENTIFIER::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_IMM (length, IDENTIFIER_LENGTH, "%zu");
      PRINT_AST_IMM (pointer, IDENTIFIER_POINTER, "%p");
      PRINT_AST_OPT_IMM (*pointer, IDENTIFIER_POINTER, "'%s'");
      break;

    case PKL_AST_INTEGER:
      IPRINTF ("INTEGER::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_IMM (value, INTEGER_VALUE, "%" PRIu64);
      break;

    case PKL_AST_STRING:
      IPRINTF ("STRING::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_IMM (length, STRING_LENGTH, "%zu");
      PRINT_AST_IMM (pointer, STRING_POINTER, "%p");
      PRINT_AST_OPT_IMM (*pointer, STRING_POINTER, "'%s'");
      break;

    case PKL_AST_EXP:
      {

#define PKL_DEF_OP(SYM, STRING) STRING,
        static const char *pkl_ast_op_name[] =
          {
#include "pkl-ops.def"
          };
#undef PKL_DEF_OP

        IPRINTF ("EXPRESSION::\n");

        PRINT_COMMON_FIELDS;
        IPRINTF ("opcode: %s\n",
                 pkl_ast_op_name[PKL_AST_EXP_CODE (ast)]);
        if (PKL_AST_EXP_ATTR (ast) != PKL_AST_ATTR_NONE)
          IPRINTF ("attr: %s\n", pkl_attr_name (PKL_AST_EXP_ATTR (ast)));
        IPRINTF ("literal_p: %d\n", PKL_AST_LITERAL_P (ast));
        PRINT_AST_SUBAST (type, TYPE);
        PRINT_AST_IMM (numops, EXP_NUMOPS, "%d");
        IPRINTF ("operands:\n");
        for (i = 0; i < PKL_AST_EXP_NUMOPS (ast); i++)
          pkl_ast_print_1 (fd, PKL_AST_EXP_OPERAND (ast, i),
                         indent + 2);
        break;
      }

    case PKL_AST_COND_EXP:
      IPRINTF ("COND_EXPRESSION::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (condition, COND_EXP_COND);
      PRINT_AST_OPT_SUBAST (thenexp, COND_EXP_THENEXP);
      PRINT_AST_OPT_SUBAST (elseexp, COND_EXP_ELSEEXP);
      break;

    case PKL_AST_STRUCT_ELEM:
      IPRINTF ("STRUCT_ELEM::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (name, STRUCT_ELEM_NAME);
      PRINT_AST_SUBAST (exp, STRUCT_ELEM_EXP);
      break;

    case PKL_AST_STRUCT:
      IPRINTF ("STRUCT::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_IMM (nelem, STRUCT_NELEM, "%zu");
      IPRINTF ("elems:\n");
      PRINT_AST_SUBAST_CHAIN (STRUCT_ELEMS);
      break;
      
    case PKL_AST_ARRAY_INITIALIZER:
      IPRINTF ("ARRAY_INITIALIZER::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (index, ARRAY_INITIALIZER_INDEX);
      PRINT_AST_SUBAST (exp, ARRAY_INITIALIZER_EXP);
      break;

    case PKL_AST_ARRAY:
      IPRINTF ("ARRAY::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_IMM (nelem, ARRAY_NELEM, "%zu");
      PRINT_AST_IMM (ninitializer, ARRAY_NINITIALIZER, "%zu");
      PRINT_AST_SUBAST (type, TYPE);
      IPRINTF ("initializers:\n");
      PRINT_AST_SUBAST_CHAIN (ARRAY_INITIALIZERS);
      break;

    case PKL_AST_ENUMERATOR:
      IPRINTF ("ENUMERATOR::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (identifier, ENUMERATOR_IDENTIFIER);
      PRINT_AST_SUBAST (value, ENUMERATOR_VALUE);
      break;

    case PKL_AST_ENUM:
      IPRINTF ("ENUM::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (tag, ENUM_TAG);
      IPRINTF ("values:\n");
      PRINT_AST_SUBAST_CHAIN (ENUM_VALUES);
      break;

    case PKL_AST_TYPE:
      IPRINTF ("TYPE::\n");

      PRINT_COMMON_FIELDS;
      if (PKL_AST_TYPE_NAME (ast))
        PRINT_AST_SUBAST (name, TYPE_NAME);
      else
        {
          IPRINTF ("code:\n");
          switch (PKL_AST_TYPE_CODE (ast))
            {
            case PKL_TYPE_INTEGRAL: IPRINTF ("  integral\n"); break;
            case PKL_TYPE_STRING: IPRINTF ("  string\n"); break;
            case PKL_TYPE_ARRAY: IPRINTF ("  array\n"); break;
            case PKL_TYPE_STRUCT: IPRINTF ("  struct\n"); break;
            case PKL_TYPE_FUNCTION: IPRINTF ("  function\n"); break;
            case PKL_TYPE_OFFSET: IPRINTF ("  offset\n"); break;
            default:
              IPRINTF (" unknown (%d)\n", PKL_AST_TYPE_CODE (ast));
              break;
            }
          PRINT_AST_IMM (complete, TYPE_COMPLETE, "%d");
          switch (PKL_AST_TYPE_CODE (ast))
            {
            case PKL_TYPE_INTEGRAL:
              PRINT_AST_IMM (signed_p, TYPE_I_SIGNED, "%d");
              PRINT_AST_IMM (size, TYPE_I_SIZE, "%zu");
              break;
            case PKL_TYPE_ARRAY:
              PRINT_AST_SUBAST (nelem, TYPE_A_NELEM);
              PRINT_AST_SUBAST (etype, TYPE_A_ETYPE);
              break;
            case PKL_TYPE_STRUCT:
              PRINT_AST_IMM (nelem, TYPE_S_NELEM, "%zu");
              IPRINTF ("elems:\n");
              PRINT_AST_SUBAST_CHAIN (TYPE_S_ELEMS);
              break;
            case PKL_TYPE_FUNCTION:
              PRINT_AST_IMM (narg, TYPE_F_NARG, "%zu");
              IPRINTF ("args:\n");
              PRINT_AST_SUBAST_CHAIN (TYPE_F_ARGS);
              break;
            case PKL_TYPE_OFFSET:
              PRINT_AST_SUBAST (base_type, TYPE_O_BASE_TYPE);
              PRINT_AST_SUBAST (unit, TYPE_O_UNIT);
              break;
            case PKL_TYPE_STRING:
            default:
              break;
            }
        }
      break;

    case PKL_AST_STRUCT_ELEM_TYPE:
      IPRINTF ("STRUCT_ELEM_TYPE::\n");

      PRINT_COMMON_FIELDS;      
      PRINT_AST_SUBAST (name, STRUCT_ELEM_TYPE_NAME);
      PRINT_AST_SUBAST (type, STRUCT_ELEM_TYPE_TYPE);
      break;

    case PKL_AST_FUNC_ARG_TYPE:
      IPRINTF ("FUNC_ARG_TYPE::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, FUNC_ARG_TYPE_TYPE);
      break;
      
    case PKL_AST_ARRAY_REF:
      IPRINTF ("ARRAY_REF::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (array, ARRAY_REF_ARRAY);
      PRINT_AST_SUBAST (index, ARRAY_REF_INDEX);
      break;

    case PKL_AST_FUNC:
      IPRINTF ("FUNC::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (ret_type, FUNC_RET_TYPE);
      PRINT_AST_SUBAST_CHAIN (FUNC_ARGS);
      PRINT_AST_SUBAST (body, FUNC_BODY);
      break;

    case PKL_AST_FUNC_ARG:
      IPRINTF ("FUNC_ARG::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, FUNC_ARG_TYPE);
      PRINT_AST_SUBAST (identifier, FUNC_ARG_IDENTIFIER);
      break;
      
    case PKL_AST_STRUCT_REF:
      IPRINTF ("STRUCT_REF::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (struct, STRUCT_REF_STRUCT);
      PRINT_AST_SUBAST (identifier, STRUCT_REF_IDENTIFIER);
      break;

    case PKL_AST_DECL:
      IPRINTF ("DECL::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_IMM (kind, DECL_KIND, "%d");
      if (PKL_AST_DECL_SOURCE (ast))
        PRINT_AST_IMM (source, DECL_SOURCE, "'%s'");
      PRINT_AST_SUBAST (name, DECL_NAME);
      PRINT_AST_SUBAST (initial, DECL_INITIAL);
      break;

    case PKL_AST_OFFSET:
      IPRINTF ("OFFSET::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (magnitude, OFFSET_MAGNITUDE);
      PRINT_AST_SUBAST (unit, OFFSET_UNIT);
      break;

    case PKL_AST_CAST:
      IPRINTF ("CAST::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (cast_type, CAST_TYPE);
      PRINT_AST_SUBAST (exp, CAST_EXP);
      break;

    case PKL_AST_MAP:
      IPRINTF ("MAP::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (map_type, MAP_TYPE);
      PRINT_AST_SUBAST (offset, MAP_OFFSET);
      PRINT_AST_IMM (mapper_back, MAP_MAPPER_BACK, "%d");
      PRINT_AST_IMM (mapper_over, MAP_MAPPER_OVER, "%d");
      break;

    case PKL_AST_SCONS:
      IPRINTF ("SCONS::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (scons_type, SCONS_TYPE);
      PRINT_AST_SUBAST (scons_value, SCONS_VALUE);
      PRINT_AST_IMM (constructor_back, SCONS_CONSTRUCTOR_BACK, "%d");
      PRINT_AST_IMM (constructor_over, SCONS_CONSTRUCTOR_OVER, "%d");
      break;

    case PKL_AST_FUNCALL:
      IPRINTF ("FUNCALL::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (function, FUNCALL_FUNCTION);
      IPRINTF ("args:\n");
      PRINT_AST_SUBAST_CHAIN (FUNCALL_ARGS);
      break;

    case PKL_AST_FUNCALL_ARG:
      IPRINTF ("FUNCALL_ARG::\n");

      PRINT_AST_SUBAST (exp, FUNCALL_ARG_EXP);
      break;

    case PKL_AST_VAR:
      IPRINTF ("VAR::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_IMM (back, VAR_BACK, "%d");
      PRINT_AST_IMM (over, VAR_OVER, "%d");
      break;
      
    case PKL_AST_COMP_STMT:
      IPRINTF ("COMP_STMT::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_IMM (builtin, COMP_STMT_BUILTIN, "%d");
      IPRINTF ("stmts:\n");
      PRINT_AST_SUBAST_CHAIN (COMP_STMT_STMTS);
      break;

    case PKL_AST_ASS_STMT:
      IPRINTF ("ASS_STMT::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (lvalue, ASS_STMT_LVALUE);
      PRINT_AST_SUBAST (exp, ASS_STMT_EXP);
      break;

    case PKL_AST_IF_STMT:
      IPRINTF ("IF_STMT::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (exp, IF_STMT_EXP);
      PRINT_AST_SUBAST (then_stmt, IF_STMT_THEN_STMT);
      PRINT_AST_SUBAST (else_stmt, IF_STMT_ELSE_STMT);
      break;

    case PKL_AST_LOOP_STMT:
      IPRINTF ("LOOP_STMT::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (condition, LOOP_STMT_CONDITION);
      PRINT_AST_SUBAST (body, LOOP_STMT_BODY);
      break;

    case PKL_AST_RETURN_STMT:
      IPRINTF ("RETURN_STMT::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (exp, RETURN_STMT_EXP);
      break;

    case PKL_AST_EXP_STMT:
      IPRINTF ("EXP_STMT::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (exp_stmt, EXP_STMT_EXP);
      break;

    case PKL_AST_TRY_CATCH_STMT:
      IPRINTF ("TRY_CATCH_STMT::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (try_catch_stmt_code, TRY_CATCH_STMT_CODE);
      PRINT_AST_SUBAST (try_catch_stmt_handler, TRY_CATCH_STMT_HANDLER);
      PRINT_AST_SUBAST (try_catch_stmt_arg, TRY_CATCH_STMT_ARG);
      PRINT_AST_SUBAST (try_catch_stmt_exp, TRY_CATCH_STMT_EXP);
      break;

    case PKL_AST_RAISE_STMT:
      IPRINTF ("RAISE_STMT::\n");

      PRINT_COMMON_FIELDS;
      PRINT_AST_SUBAST (raise_stmt_exp, RAISE_STMT_EXP);
      break;

    case PKL_AST_NULL_STMT:
      IPRINTF ("NULL_STMT::\n");
      
      PRINT_COMMON_FIELDS;
      break;
      
    default:
      IPRINTF ("UNKNOWN:: code=%d\n", PKL_AST_CODE (ast));
      break;
    }
}

/* Dump a printable representation of AST to the file descriptor FD.
   This function is intended to be useful to debug the PKL
   compiler.  */

void
pkl_ast_print (FILE *fd, pkl_ast_node ast)
{
  pkl_ast_print_1 (fd, ast, 0);
}

#undef IPRINTF
#undef PRINT_AST_IMM
#undef PRINT_AST_SUBAST
#undef PRINT_AST_OPT_FIELD
#undef PRINT_AST_OPT_SUBAST

#endif /* PKL_DEBUG */
