/* pkl-ast.c - Abstract Syntax Tree for Poke.  */

/* Copyright (C) 2017 Jose E. Marchesi */

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
#include "xalloc.h"
#include "pkl-ast.h"

/* Return the endianness of the running system.  */

enum pkl_ast_endian
pkl_ast_default_endian (void)
{
  char buffer[4] = { 0x0, 0x0, 0x0, 0x1 };
  uint32_t canary = *((uint32_t *) buffer);

  return canary == 0x1 ? PKL_AST_MSB : PKL_AST_LSB;
}

/* Allocate and return a new AST node, with the given CODE.  The rest
   of the node is initialized to zero.  */
  
static pkl_ast_node
pkl_ast_make_node (enum pkl_ast_code code)
{
  pkl_ast_node ast;

  ast = xmalloc (sizeof (union pkl_ast_node));
  memset (ast, 0, sizeof (union pkl_ast_node));
  PKL_AST_CODE (ast) = code;

  return ast;
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

/* Build and return an AST node for the location counter.  */

pkl_ast_node
pkl_ast_make_loc (void)
{
  return pkl_ast_make_node (PKL_AST_LOC);
}

/* Build and return an AST node for an integer constant.  */

pkl_ast_node 
pkl_ast_make_integer (uint64_t value)
{
  pkl_ast_node new = pkl_ast_make_node (PKL_AST_INTEGER);

  PKL_AST_INTEGER_VALUE (new) = value;
  PKL_AST_LITERAL_P (new) = 1;
  
  return new;
}

/* Build and return an AST node for a string constant.  */

pkl_ast_node 
pkl_ast_make_string (const char *str)
{
  pkl_ast_node new = pkl_ast_make_node (PKL_AST_STRING);

  assert (str);
  
  PKL_AST_STRING_POINTER (new) = xstrdup (str);
  PKL_AST_STRING_LENGTH (new) = strlen (str);
  PKL_AST_LITERAL_P (new) = 1;

  return new;
}

/* Build and return an AST node for an identifier.  */

pkl_ast_node
pkl_ast_make_identifier (const char *str)
{
  pkl_ast_node id = pkl_ast_make_node (PKL_AST_IDENTIFIER);

  PKL_AST_IDENTIFIER_POINTER (id) = xstrdup (str);
  PKL_AST_IDENTIFIER_LENGTH (id) = strlen (str);

  return id;
}

/* Build and return an AST node for a doc string.  */

pkl_ast_node
pkl_ast_make_doc_string (const char *str, pkl_ast_node entity)
{
  pkl_ast_node doc_string = pkl_ast_make_node (PKL_AST_DOC_STRING);

  assert (str);

  PKL_AST_DOC_STRING_POINTER (doc_string) = xstrdup (str);
  PKL_AST_DOC_STRING_LENGTH (doc_string) = strlen (str);

  return doc_string;
}

/* Build and return an AST node for an enumerator.  */

pkl_ast_node 
pkl_ast_make_enumerator (pkl_ast_node identifier,
                         pkl_ast_node value,
                         pkl_ast_node docstr)
{
  pkl_ast_node enumerator = pkl_ast_make_node (PKL_AST_ENUMERATOR);

  assert (identifier != NULL);

  PKL_AST_ENUMERATOR_IDENTIFIER (enumerator) = ASTREF (identifier);
  PKL_AST_ENUMERATOR_VALUE (enumerator) = ASTREF (value);
  PKL_AST_ENUMERATOR_DOCSTR (enumerator) = ASTREF (docstr);

  return enumerator;
}

/* Build and return an AST node for a conditional expression.  */

pkl_ast_node
pkl_ast_make_cond_exp (pkl_ast_node cond,
                       pkl_ast_node thenexp,
                       pkl_ast_node elseexp)
{
  pkl_ast_node cond_exp = pkl_ast_make_node (PKL_AST_COND_EXP);

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
pkl_ast_make_binary_exp (enum pkl_ast_op code,
                         pkl_ast_node op1,
                         pkl_ast_node op2)
{
  pkl_ast_node exp = pkl_ast_make_node (PKL_AST_EXP);

  assert (op1 && op2);

  PKL_AST_EXP_CODE (exp) = code;
  PKL_AST_EXP_NUMOPS (exp) = 2;
  PKL_AST_EXP_OPERAND (exp, 0) = ASTREF (op1);
  PKL_AST_EXP_OPERAND (exp, 1) = ASTREF (op2);

  PKL_AST_LITERAL_P (exp)
    = PKL_AST_LITERAL_P (op1) && PKL_AST_LITERAL_P (op2);

  return exp;
}

/* Build and return an AST node for an unary expression.  */

pkl_ast_node
pkl_ast_make_unary_exp (enum pkl_ast_op code,
                        pkl_ast_node op)
{
  pkl_ast_node exp = pkl_ast_make_node (PKL_AST_EXP);

  PKL_AST_EXP_CODE (exp) = code;
  PKL_AST_EXP_NUMOPS (exp) = 1;
  PKL_AST_EXP_OPERAND (exp, 0) = ASTREF (op);
  PKL_AST_LITERAL_P (exp) = PKL_AST_LITERAL_P (op);
  
  return exp;
}

/* Build and return an AST node for an array reference.  */

pkl_ast_node
pkl_ast_make_array_ref (pkl_ast_node array, pkl_ast_node index)
{
  pkl_ast_node aref = pkl_ast_make_node (PKL_AST_ARRAY_REF);

  assert (array && index);

  PKL_AST_ARRAY_REF_ARRAY (aref) = ASTREF (array);
  PKL_AST_ARRAY_REF_INDEX (aref) = ASTREF (index);
  PKL_AST_LITERAL_P (aref) = 0;

  return aref;
}

/* Build and return an AST node for a tuple reference.  */

pkl_ast_node
pkl_ast_make_tuple_ref (pkl_ast_node tuple, pkl_ast_node identifier)
{
  pkl_ast_node tref = pkl_ast_make_node (PKL_AST_TUPLE_REF);

  assert (tuple && identifier);

  PKL_AST_TUPLE_REF_TUPLE (tref) = ASTREF (tuple);
  PKL_AST_TUPLE_REF_IDENTIFIER (tref) = ASTREF (identifier);
  
  return tref;
}

/* Build and return an AST node for a struct reference.  */

pkl_ast_node
pkl_ast_make_struct_ref (pkl_ast_node base, pkl_ast_node identifier)
{
  pkl_ast_node sref = pkl_ast_make_node (PKL_AST_STRUCT_REF);

  assert (base && identifier
          && PKL_AST_CODE (identifier) == PKL_AST_IDENTIFIER);

  PKL_AST_STRUCT_REF_BASE (sref) = ASTREF (base);
  PKL_AST_STRUCT_REF_IDENTIFIER (sref) = ASTREF (identifier);

  return sref;
}

/* Build and return type AST nodes.  */

static pkl_ast_node
pkl_ast_make_type (void)
{
  pkl_ast_node type = pkl_ast_make_node (PKL_AST_TYPE);

  PKL_AST_TYPE_NAME (type) = NULL;
  PKL_AST_TYPE_TYPEOF (type) = 0;

  return type;
}

pkl_ast_node
pkl_ast_make_integral_type (int signed_p, size_t size)
{
  pkl_ast_node type = pkl_ast_make_type ();

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_INTEGRAL;
  PKL_AST_TYPE_I_SIGNED (type) = signed_p;
  PKL_AST_TYPE_I_SIZE (type) = size;
  return type;
}

pkl_ast_node
pkl_ast_make_array_type (pkl_ast_node etype)
{
  pkl_ast_node type = pkl_ast_make_type ();

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_ARRAY;
  PKL_AST_TYPE_A_ETYPE (type) = ASTREF (etype);
  return type;
}

pkl_ast_node
pkl_ast_make_string_type (void)
{
  pkl_ast_node type = pkl_ast_make_type ();

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_STRING;
  return type;
}

pkl_ast_node
pkl_ast_make_tuple_type (size_t nelem,
                         pkl_ast_node enames,
                         pkl_ast_node etypes)
{
  pkl_ast_node type = pkl_ast_make_type ();

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_TUPLE;
  PKL_AST_TYPE_T_NELEM (type) = nelem;
  PKL_AST_TYPE_T_ENAMES (type) = ASTREF (enames);
  PKL_AST_TYPE_T_ETYPES (type) = ASTREF (etypes);
  
  return type;
}

pkl_ast_node
pkl_ast_make_metatype (pkl_ast_node type)
{
  pkl_ast_node metatype;

  assert (PKL_AST_CODE (type) == PKL_AST_TYPE
          && PKL_AST_TYPE_TYPEOF (type) == 0);

  metatype = pkl_ast_dup_type (type);
  PKL_AST_TYPE_TYPEOF (metatype) = 1;

  return metatype;
}

/* Allocate and return a duplicated type AST node.  */

pkl_ast_node
pkl_ast_dup_type (pkl_ast_node type)
{
  pkl_ast_node t, new = pkl_ast_make_type ();
  
  PKL_AST_TYPE_CODE (new) = PKL_AST_TYPE_CODE (type);
  PKL_AST_TYPE_TYPEOF (new) = PKL_AST_TYPE_TYPEOF (type);
  
  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      PKL_AST_TYPE_I_SIZE (new) = PKL_AST_TYPE_I_SIZE (type);
      PKL_AST_TYPE_I_SIGNED (new) = PKL_AST_TYPE_I_SIGNED (type);
      break;
    case PKL_TYPE_ARRAY:
      PKL_AST_TYPE_A_ETYPE (new)
        = ASTREF (pkl_ast_dup_type (PKL_AST_TYPE_A_ETYPE (type)));
      break;
    case PKL_TYPE_TUPLE:
      PKL_AST_TYPE_T_NELEM (new) = PKL_AST_TYPE_T_NELEM (type);
      for (t = PKL_AST_TYPE_T_ENAMES (type); t; t = PKL_AST_CHAIN (t))
        {
          pkl_ast_node ename
            = pkl_ast_make_identifier (PKL_AST_IDENTIFIER_POINTER (t));
          PKL_AST_TYPE_T_ENAMES (new)
            = pkl_ast_chainon (PKL_AST_TYPE_T_ENAMES (new), ename);
        }
      for (t = PKL_AST_TYPE_T_ETYPES (type); t; t = PKL_AST_CHAIN (t))
        {
          pkl_ast_node etype = pkl_ast_dup_type (t);
          PKL_AST_TYPE_T_ETYPES (new)
            = pkl_ast_chainon (PKL_AST_TYPE_T_ETYPES (new), etype);
        }
      break;
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
  pkl_ast_node ta, tb;
  
  if (PKL_AST_TYPE_CODE (a) != PKL_AST_TYPE_CODE (b))
    return 0;

  switch (PKL_AST_TYPE_CODE (a))
    {
    case PKL_TYPE_INTEGRAL:
      if (PKL_AST_TYPE_I_SIZE (a) != PKL_AST_TYPE_I_SIZE (b)
          || PKL_AST_TYPE_I_SIGNED (a) != PKL_AST_TYPE_I_SIGNED (a))
        return 0;
      break;
    case PKL_TYPE_ARRAY:
      if (!pkl_ast_type_equal (PKL_AST_TYPE_A_ETYPE (a),
                               PKL_AST_TYPE_A_ETYPE (a)))
        return 0;
      break;
    case PKL_TYPE_TUPLE:
      if (PKL_AST_TYPE_T_NELEM (a) != PKL_AST_TYPE_T_NELEM (b))
        return 0;
      for (ta = PKL_AST_TYPE_T_ENAMES (a), tb = PKL_AST_TYPE_T_ENAMES (b);
           ta && tb;
           ta = PKL_AST_CHAIN (ta), tb = PKL_AST_CHAIN (tb))
        {
          if (strcmp (PKL_AST_IDENTIFIER_POINTER (ta),
                      PKL_AST_IDENTIFIER_POINTER (tb)) != 0)
            return 0;
        }
      for (ta = PKL_AST_TYPE_T_ETYPES (a), tb = PKL_AST_TYPE_T_ETYPES (b);
           ta && tb;
           ta = PKL_AST_CHAIN (ta), tb = PKL_AST_CHAIN (tb))
        {
          if (!pkl_ast_type_equal (ta, tb))
            return 0;
        }
      break;
    case PKL_TYPE_STRING:
      /* Fallthrough.  */
    default:
      break;
    }

  return 1;
}

/* Build and return an AST node for a struct.  */

pkl_ast_node
pkl_ast_make_struct (pkl_ast_node tag, pkl_ast_node docstr,
                     pkl_ast_node mem)
{
  pkl_ast_node strct = pkl_ast_make_node (PKL_AST_STRUCT);

  assert (tag);

  PKL_AST_STRUCT_TAG (strct) = ASTREF (tag);
  PKL_AST_STRUCT_DOCSTR (strct) = ASTREF (docstr);
  PKL_AST_STRUCT_MEM (strct) = ASTREF (mem);

  return strct;
}

/* Build and return an AST node for a memory layout.  */

pkl_ast_node
pkl_ast_make_mem (enum pkl_ast_endian endian,
                  pkl_ast_node components)
{
  pkl_ast_node mem = pkl_ast_make_node (PKL_AST_MEM);

  PKL_AST_MEM_ENDIAN (mem) = endian;
  PKL_AST_MEM_COMPONENTS (mem) = ASTREF (components);

  return mem;
}

/* Build and return an AST node for an enum.  */

pkl_ast_node
pkl_ast_make_enum (pkl_ast_node tag, pkl_ast_node values, pkl_ast_node docstr)
{
  pkl_ast_node enumeration = pkl_ast_make_node (PKL_AST_ENUM);

  assert (tag && values);

  PKL_AST_ENUM_TAG (enumeration) = ASTREF (tag);
  PKL_AST_ENUM_VALUES (enumeration) = ASTREF (values);
  PKL_AST_ENUM_DOCSTR (enumeration) = ASTREF (docstr);

  return enumeration;
}

/* Build and return an AST node for a struct field.  */

pkl_ast_node
pkl_ast_make_field (pkl_ast_node name, pkl_ast_node type, pkl_ast_node docstr,
                    enum pkl_ast_endian endian, pkl_ast_node num_ents,
                    pkl_ast_node size)
{
  pkl_ast_node field = pkl_ast_make_node (PKL_AST_FIELD);

  assert (name);

  PKL_AST_FIELD_NAME (field) = ASTREF (name);
  PKL_AST_FIELD_TYPE (field) = ASTREF (type);
  PKL_AST_FIELD_DOCSTR (field) = ASTREF (docstr);
  PKL_AST_FIELD_ENDIAN (field) = endian;
  PKL_AST_FIELD_NUM_ENTS (field) = ASTREF (num_ents);
  PKL_AST_FIELD_SIZE (field) = ASTREF (size);

  return field;
}

/* Build and return an AST node for a struct conditional.  */

pkl_ast_node
pkl_ast_make_cond (pkl_ast_node exp, pkl_ast_node thenpart, pkl_ast_node elsepart)
{
  pkl_ast_node cond = pkl_ast_make_node (PKL_AST_COND);

  assert (exp);

  PKL_AST_COND_EXP (cond) = ASTREF (exp);
  PKL_AST_COND_THENPART (cond) = ASTREF (thenpart);
  PKL_AST_COND_ELSEPART (cond) = ASTREF (elsepart);

  return cond;
}

/* Build and return an AST node for a struct loop.  */

pkl_ast_node
pkl_ast_make_loop (pkl_ast_node pre, pkl_ast_node cond, pkl_ast_node post,
                   pkl_ast_node body)
{
  pkl_ast_node loop = pkl_ast_make_node (PKL_AST_LOOP);

  PKL_AST_LOOP_PRE (loop) = ASTREF (pre);
  PKL_AST_LOOP_COND (loop) = ASTREF (cond);
  PKL_AST_LOOP_POST (loop) = ASTREF (post);
  PKL_AST_LOOP_BODY (loop) = ASTREF (body);

  return loop;
}

/* Build and return an AST node for a cast.  */

pkl_ast_node
pkl_ast_make_cast (pkl_ast_node type, pkl_ast_node exp)
{
  pkl_ast_node cast = pkl_ast_make_node (PKL_AST_CAST);

  PKL_AST_TYPE (cast) = ASTREF (type);
  PKL_AST_CAST_EXP (cast) = ASTREF (exp);

  return cast;
}

/* Build and return an AST node for an array.  */

pkl_ast_node
pkl_ast_make_array (size_t nelem, pkl_ast_node elems)
{
  pkl_ast_node array = pkl_ast_make_node (PKL_AST_ARRAY);

  PKL_AST_ARRAY_NELEM (array) = nelem;
  PKL_AST_ARRAY_ELEMS (array) = ASTREF (elems);

  return array;
}

/* Build and return an AST node for an array element.  */

pkl_ast_node
pkl_ast_make_array_elem (size_t index, pkl_ast_node exp)
{
  pkl_ast_node elem = pkl_ast_make_node (PKL_AST_ARRAY_ELEM);

  PKL_AST_ARRAY_ELEM_INDEX (elem) = index;
  PKL_AST_ARRAY_ELEM_EXP (elem) = ASTREF (exp);

  return elem;
}

/* Build and return an AST node for a tuple.  */

pkl_ast_node
pkl_ast_make_tuple (size_t nelem,
                    pkl_ast_node elems)
{
  pkl_ast_node tuple = pkl_ast_make_node (PKL_AST_TUPLE);

  PKL_AST_TUPLE_NELEM (tuple) = nelem;
  PKL_AST_TUPLE_ELEMS (tuple) = ASTREF (elems);

  return tuple;
}

/* Build and return an AST node for a tuple element.  */

pkl_ast_node
pkl_ast_make_tuple_elem (pkl_ast_node name,
                         pkl_ast_node exp)
{
  pkl_ast_node elem = pkl_ast_make_node (PKL_AST_TUPLE_ELEM);

  if (name != NULL)
    PKL_AST_TUPLE_ELEM_NAME (elem) = ASTREF (name);
  PKL_AST_TUPLE_ELEM_EXP (elem) = ASTREF (exp);

  return elem;
}

/* Build and return an AST node for an assert.  */

pkl_ast_node
pkl_ast_make_assertion (pkl_ast_node exp)
{
  pkl_ast_node assertion = pkl_ast_make_node (PKL_AST_ASSERTION);

  PKL_AST_ASSERTION_EXP (assertion) = ASTREF (exp);
  return assertion;
}

/* Build and return an AST node for a PKL program.  */

pkl_ast_node
pkl_ast_make_program (pkl_ast_node elems)
{
  pkl_ast_node program = pkl_ast_make_node (PKL_AST_PROGRAM);

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
      pkl_ast_node_free (PKL_AST_ENUM_DOCSTR (ast));
      
      for (t = PKL_AST_ENUM_VALUES (ast); t; t = n)
        {
          n = PKL_AST_CHAIN (t);
          pkl_ast_node_free (t);
        }

      break;
      
    case PKL_AST_ENUMERATOR:

      pkl_ast_node_free (PKL_AST_ENUMERATOR_IDENTIFIER (ast));
      pkl_ast_node_free (PKL_AST_ENUMERATOR_VALUE (ast));
      pkl_ast_node_free (PKL_AST_ENUMERATOR_DOCSTR (ast));
      break;
      
    case PKL_AST_STRUCT:

      pkl_ast_node_free (PKL_AST_STRUCT_TAG (ast));
      pkl_ast_node_free (PKL_AST_STRUCT_DOCSTR (ast));
      pkl_ast_node_free (PKL_AST_STRUCT_MEM (ast));
      break;
      
    case PKL_AST_MEM:

      for (t = PKL_AST_MEM_COMPONENTS (ast); t; t = n)
        {
          n = PKL_AST_CHAIN (t);
          pkl_ast_node_free (t);
        }

      break;
      
    case PKL_AST_FIELD:

      pkl_ast_node_free (PKL_AST_FIELD_NAME (ast));
      pkl_ast_node_free (PKL_AST_FIELD_TYPE (ast));
      pkl_ast_node_free (PKL_AST_FIELD_DOCSTR (ast));
      pkl_ast_node_free (PKL_AST_FIELD_NUM_ENTS (ast));
      pkl_ast_node_free (PKL_AST_FIELD_SIZE (ast));
      break;
      
    case PKL_AST_COND:

      pkl_ast_node_free (PKL_AST_COND_EXP (ast));
      pkl_ast_node_free (PKL_AST_COND_THENPART (ast));
      pkl_ast_node_free (PKL_AST_COND_ELSEPART (ast));
      break;
      
    case PKL_AST_LOOP:

      pkl_ast_node_free (PKL_AST_LOOP_PRE (ast));
      pkl_ast_node_free (PKL_AST_LOOP_COND (ast));
      pkl_ast_node_free (PKL_AST_LOOP_POST (ast));
      pkl_ast_node_free (PKL_AST_LOOP_BODY (ast));
      break;
      
    case PKL_AST_ASSERTION:

      pkl_ast_node_free (PKL_AST_ASSERTION_EXP (ast));
      break;
      
    case PKL_AST_TYPE:

      free (PKL_AST_TYPE_NAME (ast));
      switch (PKL_AST_TYPE_CODE (ast))
        {
        case PKL_TYPE_ARRAY:
          pkl_ast_node_free (PKL_AST_TYPE_A_ETYPE (ast));
          break;
        case PKL_TYPE_TUPLE:
          for (t = PKL_AST_TYPE_T_ENAMES (ast); t; t = n)
            {
              n = PKL_AST_CHAIN (t);
              pkl_ast_node_free (t);
            }
          for (t = PKL_AST_TYPE_T_ETYPES (ast); t; t = n)
            {
              n = PKL_AST_CHAIN (t);
              pkl_ast_node_free (t);
            }
          break;
        case PKL_TYPE_INTEGRAL:
        case PKL_TYPE_STRING:
        default:
          break;
        }
      
      break;
      
    case PKL_AST_ARRAY_REF:

      pkl_ast_node_free (PKL_AST_ARRAY_REF_ARRAY (ast));
      pkl_ast_node_free (PKL_AST_ARRAY_REF_INDEX (ast));
      break;
      
    case PKL_AST_STRUCT_REF:

      pkl_ast_node_free (PKL_AST_STRUCT_REF_BASE (ast));
      pkl_ast_node_free (PKL_AST_STRUCT_REF_IDENTIFIER (ast));
      break;

    case PKL_AST_CAST:

      pkl_ast_node_free (PKL_AST_CAST_EXP (ast));
      break;
      
    case PKL_AST_STRING:

      free (PKL_AST_STRING_POINTER (ast));
      break;
      
    case PKL_AST_IDENTIFIER:

      free (PKL_AST_IDENTIFIER_POINTER (ast));
      break;
      
    case PKL_AST_DOC_STRING:

      free (PKL_AST_DOC_STRING_POINTER (ast));
      break;

    case PKL_AST_TUPLE_REF:

      pkl_ast_node_free (PKL_AST_TUPLE_REF_TUPLE (ast));
      pkl_ast_node_free (PKL_AST_TUPLE_REF_IDENTIFIER (ast));
      break;
      
    case PKL_AST_TUPLE_ELEM:

      pkl_ast_node_free (PKL_AST_TUPLE_ELEM_NAME (ast));
      pkl_ast_node_free (PKL_AST_TUPLE_ELEM_EXP (ast));
      break;

    case PKL_AST_TUPLE:

      for (t = PKL_AST_TUPLE_ELEMS (ast); t; t = n)
        {
          n = PKL_AST_CHAIN (t);
          pkl_ast_node_free (t);
        }
      break;
      
    case PKL_AST_ARRAY_ELEM:

      pkl_ast_node_free (PKL_AST_ARRAY_ELEM_EXP (ast));
      break;

    case PKL_AST_ARRAY:

      for (t = PKL_AST_ARRAY_ELEMS (ast); t; t = n)
        {
          n = PKL_AST_CHAIN (t);
          pkl_ast_node_free (t);
        }
      
      pkl_ast_node_free (PKL_AST_TYPE (ast));
      break;

    case PKL_AST_INTEGER:
      /* Fallthrough.  */
    case PKL_AST_LOC:
      break;
      
    default:
      assert (0);
    }

  free (ast);
}

/* Allocate and initialize a new AST and return it.  The hash tables
   are zeroed.  */

pkl_ast
pkl_ast_init (void)
{
  static struct
    {
      int code;
      char *id;
      size_t size;
      int signed_p;
    } *type, stditypes[] =
        {
#define PKL_DEF_TYPE(CODE,ID,SIZE,SIGNED) {CODE, ID, SIZE, SIGNED},
# include "pkl-types.def"
#undef PKL_DEF_TYPE
          { PKL_TYPE_NOTYPE, NULL, 0 }
        };
  struct pkl_ast *ast;
  size_t nentries;

  /* Allocate a new AST and initialize it to 0.  */
  
  ast = xmalloc (sizeof (struct pkl_ast));
  memset (ast, 0, sizeof (struct pkl_ast));

  /* Create and register standard types in the types hash and also in
     the stdtypes array for easy access by type code.  */

  nentries
    = (sizeof (stditypes) / sizeof (stditypes[0]));
  ast->stdtypes = xmalloc (nentries * sizeof (pkl_ast_node *));

  /* Integral types.  */
  for (type = stditypes; type->code != PKL_TYPE_NOTYPE; type++)
    {
      pkl_ast_node t
        = pkl_ast_make_integral_type (type->signed_p, type->size);
      pkl_ast_register (ast, type->id, t);
      ast->stdtypes[type->code] = ASTREF (t);
    }
  ast->stdtypes[nentries - 1] = NULL;
  
  /* String type.  */
  ast->stringtype = ASTREF (pkl_ast_make_string_type ());
  pkl_ast_register (ast, "string", ast->stringtype);

  return ast;
}

/* Free all the memory allocated to store the nodes and the hash
   tables of an AST.  */

static void
free_hash_table (pkl_hash *hash_table)
{
  size_t i;
  pkl_ast_node t, n;

  for (i = 0; i < HASH_TABLE_SIZE; ++i)
    if ((*hash_table)[i])
      for (t = (*hash_table)[i]; t; t = n)
        {
          n = PKL_AST_CHAIN2 (t);
          pkl_ast_node_free (t);
        }
}

void
pkl_ast_free (pkl_ast ast)
{
  size_t i;

  if (ast == NULL)
    return;
  
  pkl_ast_node_free (ast->ast);

  free_hash_table (&ast->ids_hash_table);
  free_hash_table (&ast->types_hash_table);
  free_hash_table (&ast->enums_hash_table);
  free_hash_table (&ast->structs_hash_table);

  for (i = 0; ast->stdtypes[i] != NULL; i++)
    pkl_ast_node_free (ast->stdtypes[i]);
  pkl_ast_node_free (ast->stringtype);
  
  free (ast);
}


/* Hash a string.  This is used by the functions below.  */

static int
hash_string (const char *name)
{
  size_t len;
  int hash;
  int i;

  len = strlen (name);
  hash = len;
  for (i = 0; i < len; i++)
    hash = ((hash * 613) + (unsigned)(name[i]));

#define HASHBITS 30
  hash &= (1 << HASHBITS) - 1;
  hash %= HASH_TABLE_SIZE;
#undef HASHBITS

  return hash;
}

/* Return a PKL_AST_IDENTIFIER node whose name is the NULL-terminated
   string STR.  If an identifier with that name has previously been
   referred to, the name node is returned this time.  */

pkl_ast_node
pkl_ast_get_identifier (struct pkl_ast *ast,
                        const char *str)
{
  pkl_ast_node id;
  int hash;
  size_t len;

  /* Compute the hash code for the identifier string.  */
  len = strlen (str);

  hash = hash_string (str);

  /* Search the hash table for the identifier.  */
  for (id = ast->ids_hash_table[hash];
       id != NULL;
       id = PKL_AST_CHAIN2 (id))
    if (PKL_AST_IDENTIFIER_LENGTH (id) == len
        && !strcmp (PKL_AST_IDENTIFIER_POINTER (id), str))
      return id;

  /* Create a new node for this identifier, and put it in the hash
     table.  */
  id = pkl_ast_make_identifier (str);
  PKL_AST_CHAIN2 (id) = ast->ids_hash_table[hash];
  ast->ids_hash_table[hash] = ASTREF (id);

  return id;

}

/* Return the standard type string.  */

pkl_ast_node
pkl_ast_get_string_type (pkl_ast ast)
{
  return ast->stringtype;
}

/* Return an integral type with the given attribute SIZE and SIGNED_P.
   If the type exists in the stdtypes array, return it.  Otherwise
   create a new one.  */

pkl_ast_node
pkl_ast_get_integral_type (pkl_ast ast, size_t size, int signed_p)
{
  size_t i;

  i = 0;
  while (ast->stdtypes[i] != NULL)
    {
      pkl_ast_node stdtype = ast->stdtypes[i];

      if (PKL_AST_TYPE_I_SIZE (stdtype) == size
          && PKL_AST_TYPE_I_SIGNED (stdtype) == signed_p)
        return stdtype;

      ++i;
    }

  return pkl_ast_make_integral_type (size, signed_p);
}

/* Register an AST node under the given NAME in the corresponding hash
   table maintained by the AST, and return a pointer to it.  */

pkl_ast_node
pkl_ast_register (struct pkl_ast *ast,
                  const char *name,
                  pkl_ast_node ast_node)
{
  enum pkl_ast_code code;
  pkl_hash *hash_table;
  int hash;
  pkl_ast_node t;

  code = PKL_AST_CODE (ast_node);
  assert (code == PKL_AST_TYPE || code == PKL_AST_ENUM
          || code == PKL_AST_STRUCT);

  if (code == PKL_AST_ENUM)
    hash_table = &ast->enums_hash_table;
  else if (code == PKL_AST_STRUCT)
    hash_table = &ast->structs_hash_table;
  else
    hash_table = &ast->types_hash_table;

  hash = hash_string (name);

  for (t = (*hash_table)[hash]; t != NULL; t = PKL_AST_CHAIN (t))
    if ((code == PKL_AST_TYPE
         && (PKL_AST_TYPE_NAME (t)
             && !strcmp (PKL_AST_TYPE_NAME (t), name)))
        || (code == PKL_AST_ENUM
            && PKL_AST_ENUM_TAG (t)
            && PKL_AST_IDENTIFIER_POINTER (PKL_AST_ENUM_TAG (t))
            && !strcmp (PKL_AST_IDENTIFIER_POINTER (PKL_AST_ENUM_TAG (t)), name))
        || (code == PKL_AST_STRUCT
            && PKL_AST_STRUCT_TAG (t)
            && PKL_AST_IDENTIFIER_POINTER (PKL_AST_STRUCT_TAG (t))
            && !strcmp (PKL_AST_IDENTIFIER_POINTER (PKL_AST_STRUCT_TAG (t)), name)))
      return NULL;

  if (code == PKL_AST_TYPE)
    /* Put the passed type in the hash table.  */
    PKL_AST_TYPE_NAME (ast_node) = xstrdup (name);

  PKL_AST_CHAIN2 (ast_node) = (*hash_table)[hash];
  (*hash_table)[hash] = ASTREF (ast_node);

  return ast_node;
}

/* Return the AST node registered under the name NAME, of type CODE
   has not been registered, return NULL.  */

pkl_ast_node
pkl_ast_get_registered (pkl_ast ast,
                        const char *name,
                        enum pkl_ast_code code)
{
  int hash;
  pkl_ast_node t;
  pkl_hash *hash_table;

  assert (code == PKL_AST_TYPE || code == PKL_AST_ENUM
          || code == PKL_AST_STRUCT);

  if (code == PKL_AST_ENUM)
    hash_table = &ast->enums_hash_table;
  else if (code == PKL_AST_STRUCT)
    hash_table = &ast->structs_hash_table;
  else
    hash_table = &ast->types_hash_table;
    
  hash = hash_string (name);

  /* Search the hash table for the type.  */
  for (t = (*hash_table)[hash]; t != NULL; t = PKL_AST_CHAIN2 (t))
    if ((code == PKL_AST_TYPE
         && (PKL_AST_TYPE_NAME (t)
             && !strcmp (PKL_AST_TYPE_NAME (t), name)))
        || (code == PKL_AST_ENUM
            && PKL_AST_ENUM_TAG (t)
            && PKL_AST_IDENTIFIER_POINTER (PKL_AST_ENUM_TAG (t))
            && !strcmp (PKL_AST_IDENTIFIER_POINTER (PKL_AST_ENUM_TAG (t)), name))
        || (code == PKL_AST_STRUCT
            && PKL_AST_STRUCT_TAG (t)
            && PKL_AST_IDENTIFIER_POINTER (PKL_AST_STRUCT_TAG (t))
            && !strcmp (PKL_AST_IDENTIFIER_POINTER (PKL_AST_STRUCT_TAG (t)), name)))
      return t;

  return NULL;
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
          printf ("|");                         \
        else                                    \
          printf (" ");                         \
      printf (__VA_ARGS__);                     \
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
  
  switch (PKL_AST_CODE (ast))
    {
    case PKL_AST_PROGRAM:
      IPRINTF ("PROGRAM::\n");

      PRINT_AST_SUBAST_CHAIN (PROGRAM_ELEMS);
      break;

    case PKL_AST_IDENTIFIER:
      IPRINTF ("IDENTIFIER::\n");

      PRINT_AST_IMM (length, IDENTIFIER_LENGTH, "%lu");
      PRINT_AST_IMM (pointer, IDENTIFIER_POINTER, "%p");
      PRINT_AST_OPT_IMM (*pointer, IDENTIFIER_POINTER, "'%s'");
      break;

    case PKL_AST_INTEGER:
      IPRINTF ("INTEGER::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_IMM (value, INTEGER_VALUE, "%lu");
      break;

    case PKL_AST_STRING:
      IPRINTF ("STRING::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_IMM (length, STRING_LENGTH, "%lu");
      PRINT_AST_IMM (pointer, STRING_POINTER, "%p");
      PRINT_AST_OPT_IMM (*pointer, STRING_POINTER, "'%s'");
      break;

    case PKL_AST_DOC_STRING:
      IPRINTF ("DOCSTR::\n");

      PRINT_AST_IMM (length, DOC_STRING_LENGTH, "%lu");
      PRINT_AST_IMM (pointer, DOC_STRING_POINTER, "%p");
      PRINT_AST_OPT_IMM (*pointer, DOC_STRING_POINTER, "'%s'");
      break;

    case PKL_AST_EXP:
      {

#define PKL_DEF_OP(SYM, STRING) STRING,
        static char *pkl_ast_op_name[] =
          {
#include "pkl-ops.def"
          };
#undef PKL_DEF_OP

        IPRINTF ("EXPRESSION::\n");
        IPRINTF ("opcode: %s\n",
                 pkl_ast_op_name[PKL_AST_EXP_CODE (ast)]);
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

      PRINT_AST_SUBAST (condition, COND_EXP_COND);
      PRINT_AST_OPT_SUBAST (thenexp, COND_EXP_THENEXP);
      PRINT_AST_OPT_SUBAST (elseexp, COND_EXP_ELSEEXP);
      break;

    case PKL_AST_TUPLE_ELEM:
      IPRINTF ("TUPLE_ELEM::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (name, TUPLE_ELEM_NAME);
      PRINT_AST_SUBAST (exp, TUPLE_ELEM_EXP);
      break;

    case PKL_AST_TUPLE:
      IPRINTF ("TUPLE::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_IMM (nelem, TUPLE_NELEM, "%lu");
      IPRINTF ("elems:\n");
      PRINT_AST_SUBAST_CHAIN (TUPLE_ELEMS);
      break;
      
    case PKL_AST_ARRAY_ELEM:
      IPRINTF ("ARRAY_ELEM::\n");

      PRINT_AST_IMM (index, ARRAY_ELEM_INDEX, "%lu");
      PRINT_AST_SUBAST (exp, ARRAY_ELEM_EXP);
      break;

    case PKL_AST_ARRAY:
      IPRINTF ("ARRAY::\n");

      PRINT_AST_IMM (nelem, ARRAY_NELEM, "%lu");
      PRINT_AST_SUBAST (type, TYPE);
      IPRINTF ("elems:\n");
      PRINT_AST_SUBAST_CHAIN (ARRAY_ELEMS);
      break;

    case PKL_AST_ENUMERATOR:
      IPRINTF ("ENUMERATOR::\n");

      PRINT_AST_SUBAST (identifier, ENUMERATOR_IDENTIFIER);
      PRINT_AST_SUBAST (value, ENUMERATOR_VALUE);
      PRINT_AST_OPT_SUBAST (docstr, ENUMERATOR_DOCSTR);
      break;

    case PKL_AST_ENUM:
      IPRINTF ("ENUM::\n");

      PRINT_AST_SUBAST (tag, ENUM_TAG);
      PRINT_AST_OPT_SUBAST (docstr, ENUM_DOCSTR);
      IPRINTF ("values:\n");
      PRINT_AST_SUBAST_CHAIN (ENUM_VALUES);
      break;

    case PKL_AST_STRUCT:
      IPRINTF ("STRUCT::\n");

      PRINT_AST_SUBAST (tag, STRUCT_TAG);
      PRINT_AST_OPT_SUBAST (docstr, STRUCT_DOCSTR);
      PRINT_AST_SUBAST (mem, STRUCT_MEM);

      break;

    case PKL_AST_MEM:
      IPRINTF ("MEM::\n");
      
      IPRINTF ("endian:\n");
      IPRINTF ("  %s\n",
               PKL_AST_MEM_ENDIAN (ast)
               == PKL_AST_MSB ? "msb" : "lsb");
      IPRINTF ("components:\n");
      PRINT_AST_SUBAST_CHAIN (MEM_COMPONENTS);

      break;
      
    case PKL_AST_FIELD:
      IPRINTF ("FIELD::\n");

      IPRINTF ("endian:\n");
      IPRINTF ("  %s\n",
               PKL_AST_FIELD_ENDIAN (ast) == PKL_AST_MSB
               ? "msb" : "lsb");
      PRINT_AST_SUBAST (name, FIELD_NAME);
      PRINT_AST_SUBAST (type, FIELD_TYPE);
      PRINT_AST_OPT_SUBAST (num_ents, FIELD_NUM_ENTS);
      PRINT_AST_OPT_SUBAST (size, FIELD_SIZE);
      PRINT_AST_OPT_SUBAST (docstr, FIELD_DOCSTR);
      
      break;

    case PKL_AST_COND:
      IPRINTF ("COND::\n");

      PRINT_AST_SUBAST (exp, COND_EXP);
      PRINT_AST_SUBAST (thenpart, COND_THENPART);
      PRINT_AST_OPT_SUBAST (elsepart, COND_ELSEPART);

      break;

    case PKL_AST_LOOP:
      IPRINTF ("LOOP::\n");

      PRINT_AST_SUBAST (pre, LOOP_PRE);
      PRINT_AST_SUBAST (cond, LOOP_COND);
      PRINT_AST_SUBAST (post, LOOP_POST);
      PRINT_AST_SUBAST (body, LOOP_BODY);

      break;

    case PKL_AST_TYPE:
      IPRINTF ("TYPE::\n");

      PRINT_AST_SUBAST (metatype, TYPE);

      IPRINTF ("code:\n");
      switch (PKL_AST_TYPE_CODE (ast))
        {
        case PKL_TYPE_INTEGRAL: IPRINTF ("  integral\n"); break;
        case PKL_TYPE_STRING: IPRINTF ("  string\n"); break;
        case PKL_TYPE_ARRAY: IPRINTF ("  array\n"); break;
        case PKL_TYPE_TUPLE: IPRINTF ("  tuple\n"); break;
        default:
          IPRINTF (" unknown (%d)\n", PKL_AST_TYPE_CODE (ast));
          break;
        }
      PRINT_AST_IMM (code, TYPE_CODE, "%d");
      switch (PKL_AST_TYPE_CODE (ast))
        {
        case PKL_TYPE_INTEGRAL:
          PRINT_AST_IMM (signed_p, TYPE_I_SIGNED, "%d");
          PRINT_AST_IMM (size, TYPE_I_SIZE, "%lu");
          break;
        case PKL_TYPE_ARRAY:
          PRINT_AST_SUBAST (etype, TYPE_A_ETYPE);
          break;
        case PKL_TYPE_TUPLE:
          PRINT_AST_IMM (nelem, TYPE_T_NELEM, "%lu");
          PRINT_AST_SUBAST_CHAIN (TYPE_T_ENAMES);
          PRINT_AST_SUBAST_CHAIN (TYPE_T_ETYPES);
          break;
        case PKL_TYPE_STRING:
          /* Fallthrough.  */
        default:
          break;
        }
      break;

    case PKL_AST_ASSERTION:
      IPRINTF ("ASSERTION::\n");

      PRINT_AST_SUBAST (exp, ASSERTION_EXP);
      break;

    case PKL_AST_LOC:
      IPRINTF ("LOC::\n");
      break;

    case PKL_AST_STRUCT_REF:
      IPRINTF ("STRUCT_REF::\n");

      PRINT_AST_SUBAST (base, STRUCT_REF_BASE);
      PRINT_AST_SUBAST (identifier, STRUCT_REF_IDENTIFIER);
      break;

    case PKL_AST_ARRAY_REF:
      IPRINTF ("ARRAY_REF::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (array, ARRAY_REF_ARRAY);
      PRINT_AST_SUBAST (index, ARRAY_REF_INDEX);
      break;

    case PKL_AST_TUPLE_REF:
      IPRINTF ("TUPLE_REF::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (tuple, TUPLE_REF_TUPLE);
      PRINT_AST_SUBAST (identifier, TUPLE_REF_IDENTIFIER);
      break;

    case PKL_AST_CAST:
      IPRINTF ("CAST::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (exp, CAST_EXP);
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
