/* pkl-ast.c - Abstract Syntax Tree for Poke.  */

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

/* Build and return an AST node for an enumerator.  */

pkl_ast_node 
pkl_ast_make_enumerator (pkl_ast_node identifier,
                         pkl_ast_node value)
{
  pkl_ast_node enumerator = pkl_ast_make_node (PKL_AST_ENUMERATOR);

  assert (identifier != NULL);

  PKL_AST_ENUMERATOR_IDENTIFIER (enumerator) = ASTREF (identifier);
  PKL_AST_ENUMERATOR_VALUE (enumerator) = ASTREF (value);

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

/* Build and return an AST node for a struct reference.  */

pkl_ast_node
pkl_ast_make_struct_ref (pkl_ast_node sct, pkl_ast_node identifier)
{
  pkl_ast_node sref = pkl_ast_make_node (PKL_AST_STRUCT_REF);

  assert (sct && identifier);

  PKL_AST_STRUCT_REF_STRUCT (sref) = ASTREF (sct);
  PKL_AST_STRUCT_REF_IDENTIFIER (sref) = ASTREF (identifier);
  
  return sref;
}

/* Build and return type AST nodes.  */

static pkl_ast_node
pkl_ast_make_type (void)
{
  pkl_ast_node type = pkl_ast_make_node (PKL_AST_TYPE);

  PKL_AST_TYPE_NAME (type) = NULL;
  return type;
}

pkl_ast_node
pkl_ast_make_integral_type (int signed_p, size_t size)
{
  pkl_ast_node type = pkl_ast_make_type ();

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_INTEGRAL;
  PKL_AST_TYPE_COMPLETE_P (type) = 1;
  PKL_AST_TYPE_I_SIGNED (type) = signed_p;
  PKL_AST_TYPE_I_SIZE (type) = size;
  return type;
}

pkl_ast_node
pkl_ast_make_array_type (pkl_ast_node nelem, pkl_ast_node etype)
{
  pkl_ast_node type = pkl_ast_make_type ();

  assert (etype);

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_ARRAY;
  PKL_AST_TYPE_A_NELEM (type) = ASTREF (nelem);
  PKL_AST_TYPE_A_ETYPE (type) = ASTREF (etype);

  PKL_AST_TYPE_COMPLETE_P (type)
    = (nelem != NULL
       && PKL_AST_CODE (nelem) == PKL_AST_INTEGER
       && PKL_AST_TYPE_COMPLETE_P (etype));
  
  return type;
}

pkl_ast_node
pkl_ast_make_string_type (void)
{
  pkl_ast_node type = pkl_ast_make_type ();

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_STRING;
  PKL_AST_TYPE_COMPLETE_P (type) = 0;
  return type;
}

pkl_ast_node
pkl_ast_make_offset_type (pkl_ast_node base_type, int unit)
{
  pkl_ast_node type = pkl_ast_make_type ();

  assert (base_type);

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_OFFSET;
  PKL_AST_TYPE_COMPLETE_P (type) = 1;
  PKL_AST_TYPE_O_UNIT (type) = unit;
  PKL_AST_TYPE_O_BASE_TYPE (type) = base_type;

  return type;
}

pkl_ast_node
pkl_ast_make_struct_type (size_t nelem, pkl_ast_node struct_type_elems)
{
  pkl_ast_node type = pkl_ast_make_type ();

  PKL_AST_TYPE_CODE (type) = PKL_TYPE_STRUCT;
  PKL_AST_TYPE_S_NELEM (type) = nelem;
  if (struct_type_elems)
    PKL_AST_TYPE_S_ELEMS (type) = ASTREF (struct_type_elems);

  /* XXX: determine here whether the struct type is complete or
     not.  For the time being, they are all complete.  */
  PKL_AST_TYPE_COMPLETE_P (type) = 1;
  
  return type;
}

pkl_ast_node
pkl_ast_make_struct_type_elem (pkl_ast_node name,
                               pkl_ast_node type)
{
  pkl_ast_node struct_type_elem
    = pkl_ast_make_node (PKL_AST_STRUCT_TYPE_ELEM);

  PKL_AST_STRUCT_TYPE_ELEM_NAME (struct_type_elem)
    = ASTREF (name);
  PKL_AST_STRUCT_TYPE_ELEM_TYPE (struct_type_elem)
    = ASTREF (type);

  return struct_type_elem;
}

/* Allocate and return a duplicated type AST node.  */

pkl_ast_node
pkl_ast_dup_type (pkl_ast_node type)
{
  pkl_ast_node t, new = pkl_ast_make_type ();
  
  PKL_AST_TYPE_CODE (new) = PKL_AST_TYPE_CODE (type);
  PKL_AST_TYPE_COMPLETE_P (new) = PKL_AST_TYPE_COMPLETE_P (type);
  
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
        PKL_AST_TYPE_A_NELEM (new) = PKL_AST_TYPE_A_NELEM (type);
        PKL_AST_TYPE_A_ETYPE (new) = ASTREF (etype);
      }
      break;
    case PKL_TYPE_STRUCT:
      PKL_AST_TYPE_S_NELEM (new) = PKL_AST_TYPE_S_NELEM (type);
      for (t = PKL_AST_TYPE_S_ELEMS (type); t; t = PKL_AST_CHAIN (t))
        {
          pkl_ast_node struct_type_elem_name
            = PKL_AST_STRUCT_TYPE_ELEM_NAME (t);
          pkl_ast_node struct_type_elem_type
            = PKL_AST_STRUCT_TYPE_ELEM_TYPE (t);
          pkl_ast_node new_struct_type_elem_name
            = (struct_type_elem_name
               ? pkl_ast_make_identifier (PKL_AST_IDENTIFIER_POINTER (struct_type_elem_name))
               : NULL);
          pkl_ast_node struct_type_elem
            = pkl_ast_make_struct_type_elem (new_struct_type_elem_name,
                                             pkl_ast_dup_type (struct_type_elem_type));

          PKL_AST_TYPE_S_ELEMS (new)
            = pkl_ast_chainon (PKL_AST_TYPE_S_ELEMS (new),
                               struct_type_elem);
          PKL_AST_TYPE_S_ELEMS (new) = ASTREF (PKL_AST_TYPE_S_ELEMS (new));
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
  if (PKL_AST_TYPE_CODE (a) != PKL_AST_TYPE_CODE (b)
      || PKL_AST_TYPE_COMPLETE_P (a) != PKL_AST_TYPE_COMPLETE_P (b))
    return 0;

  switch (PKL_AST_TYPE_CODE (a))
    {
    case PKL_TYPE_INTEGRAL:
      if (PKL_AST_TYPE_I_SIZE (a) != PKL_AST_TYPE_I_SIZE (b)
          || PKL_AST_TYPE_I_SIGNED (a) != PKL_AST_TYPE_I_SIGNED (b))
        return 0;
      break;
    case PKL_TYPE_ARRAY:
      {
        pkl_ast_node a_nelem = PKL_AST_TYPE_A_NELEM (a);
        pkl_ast_node b_nelem = PKL_AST_TYPE_A_NELEM (b);

        if ((PKL_AST_INTEGER_VALUE (a_nelem)
             != PKL_AST_INTEGER_VALUE (b_nelem))
            || !pkl_ast_type_equal (PKL_AST_TYPE_A_ETYPE (a),
                                    PKL_AST_TYPE_A_ETYPE (b)))
          return 0;
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
            if (strcmp (PKL_AST_IDENTIFIER_POINTER (PKL_AST_STRUCT_TYPE_ELEM_NAME (sa)),
                        PKL_AST_IDENTIFIER_POINTER (PKL_AST_STRUCT_TYPE_ELEM_NAME (sb)))
                || !pkl_ast_type_equal (PKL_AST_STRUCT_TYPE_ELEM_TYPE (sa),
                                        PKL_AST_STRUCT_TYPE_ELEM_TYPE (sb)))
              return 0;
          }
        break;
      }
    case PKL_TYPE_OFFSET:
      /* Fallthrough.  */
    case PKL_TYPE_STRING:
      /* Fallthrough.  */
    default:
      break;
    }

  return 1;
}

size_t
pkl_ast_sizeof_type (pkl_ast_node type)
{
  /* This function should only be called on complete types.  */
  assert (PKL_AST_TYPE_COMPLETE_P (type));

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      return PKL_AST_TYPE_I_SIZE (type);
      break;
    case PKL_TYPE_ARRAY:
      {
        pkl_ast_node etype = PKL_AST_TYPE_A_ETYPE (type);
        pkl_ast_node nelem = PKL_AST_TYPE_A_NELEM (type);

        /* If the array type is complete the node holding the number
           of elements should be an integer node.  */
        if (PKL_AST_CODE (nelem) != PKL_AST_INTEGER)
          assert (0);

        return PKL_AST_INTEGER_VALUE (nelem)
          * pkl_ast_sizeof_type (etype);
        break;
      }
    case PKL_TYPE_STRUCT:
      {
        pkl_ast_node t;
        size_t size = 0;

        for (t = PKL_AST_TYPE_S_ELEMS (type); t; t = PKL_AST_CHAIN (t))
          {
            pkl_ast_node elem_type = PKL_AST_STRUCT_TYPE_ELEM_TYPE (t);
            size += pkl_ast_sizeof_type (elem_type);
          }

        return size;
        break;
      }
    case PKL_TYPE_OFFSET:
      return pkl_ast_sizeof_type (PKL_AST_TYPE_O_BASE_TYPE (type));
      break;
    case PKL_TYPE_STRING:
    default:
      break;
    }

  assert (0);
  return 0;
}

/* Build and return an AST node for an enum.  */

pkl_ast_node
pkl_ast_make_enum (pkl_ast_node tag, pkl_ast_node values)
{
  pkl_ast_node enumeration = pkl_ast_make_node (PKL_AST_ENUM);

  assert (tag && values);

  PKL_AST_ENUM_TAG (enumeration) = ASTREF (tag);
  PKL_AST_ENUM_VALUES (enumeration) = ASTREF (values);

  return enumeration;
}

/* Build and return an AST node for an array.  */

pkl_ast_node
pkl_ast_make_array (size_t nelem, size_t ninitializer,
                    pkl_ast_node initializers)
{
  pkl_ast_node array = pkl_ast_make_node (PKL_AST_ARRAY);

  PKL_AST_ARRAY_NELEM (array) = nelem;
  PKL_AST_ARRAY_NINITIALIZER (array) = ninitializer;
  PKL_AST_ARRAY_INITIALIZERS (array) = ASTREF (initializers);

  return array;
}

/* Build and return an AST node for an array element.  */

pkl_ast_node
pkl_ast_make_array_initializer (size_t index, pkl_ast_node exp)
{
  pkl_ast_node initializer
    = pkl_ast_make_node (PKL_AST_ARRAY_INITIALIZER);

  PKL_AST_ARRAY_INITIALIZER_INDEX (initializer) = index;
  PKL_AST_ARRAY_INITIALIZER_EXP (initializer) = ASTREF (exp);

  return initializer;
}

/* Build and return an AST node for a struct.  */

pkl_ast_node
pkl_ast_make_struct (size_t nelem,
                     pkl_ast_node elems)
{
  pkl_ast_node sct = pkl_ast_make_node (PKL_AST_STRUCT);

  PKL_AST_STRUCT_NELEM (sct) = nelem;
  PKL_AST_STRUCT_ELEMS (sct) = ASTREF (elems);

  return sct;
}

/* Build and return an AST node for a struct element.  */

pkl_ast_node
pkl_ast_make_struct_elem (pkl_ast_node name,
                          pkl_ast_node exp)
{
  pkl_ast_node elem = pkl_ast_make_node (PKL_AST_STRUCT_ELEM);

  if (name != NULL)
    PKL_AST_STRUCT_ELEM_NAME (elem) = ASTREF (name);
  PKL_AST_STRUCT_ELEM_EXP (elem) = ASTREF (exp);

  return elem;
}

/* Build and return an AST node for an offset construct.  */

pkl_ast_node
pkl_ast_make_offset (pkl_ast_node magnitude, int unit)
{
  pkl_ast_node elem = pkl_ast_make_node (PKL_AST_OFFSET);

  assert (unit == PKL_AST_OFFSET_UNIT_BITS
          || unit == PKL_AST_OFFSET_UNIT_BYTES);
  
  if (magnitude != NULL)
    PKL_AST_OFFSET_MAGNITUDE (elem) = ASTREF (magnitude);
  PKL_AST_OFFSET_UNIT (elem) = unit;

  return elem;
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
        case PKL_TYPE_INTEGRAL:
        case PKL_TYPE_STRING:
        case PKL_TYPE_OFFSET:
        default:
          break;
        }
      
      break;

    case PKL_AST_STRUCT_TYPE_ELEM:

      pkl_ast_node_free (PKL_AST_STRUCT_TYPE_ELEM_NAME (ast));
      pkl_ast_node_free (PKL_AST_STRUCT_TYPE_ELEM_TYPE (ast));
      break;
      
    case PKL_AST_ARRAY_REF:

      pkl_ast_node_free (PKL_AST_ARRAY_REF_ARRAY (ast));
      pkl_ast_node_free (PKL_AST_ARRAY_REF_INDEX (ast));
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

      pkl_ast_node_free (PKL_AST_ARRAY_INITIALIZER_EXP (ast));
      break;

    case PKL_AST_ARRAY:

      for (t = PKL_AST_ARRAY_INITIALIZERS (ast); t; t = n)
        {
          n = PKL_AST_CHAIN (t);
          pkl_ast_node_free (t);
        }
      
      break;

    case PKL_AST_OFFSET:

      pkl_ast_node_free (PKL_AST_OFFSET_MAGNITUDE (ast));
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
  ast->stringtype = pkl_ast_make_string_type ();
  ast->stringtype = ASTREF (ast->stringtype);
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

  for (i = 0; ast->stdtypes[i] != NULL; i++)
    pkl_ast_node_free (ast->stdtypes[i]);
  free (ast->stdtypes);
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
  assert (code == PKL_AST_TYPE || code == PKL_AST_ENUM);

  if (code == PKL_AST_ENUM)
    hash_table = &ast->enums_hash_table;
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
            && !strcmp (PKL_AST_IDENTIFIER_POINTER (PKL_AST_ENUM_TAG (t)), name)))
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

  assert (code == PKL_AST_TYPE || code == PKL_AST_ENUM);

  if (code == PKL_AST_ENUM)
    hash_table = &ast->enums_hash_table;
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
            && !strcmp (PKL_AST_IDENTIFIER_POINTER (PKL_AST_ENUM_TAG (t)), name)))
      return t;

  return NULL;
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

      PRINT_AST_IMM (length, IDENTIFIER_LENGTH, "%zu");
      PRINT_AST_IMM (pointer, IDENTIFIER_POINTER, "%p");
      PRINT_AST_OPT_IMM (*pointer, IDENTIFIER_POINTER, "'%s'");
      break;

    case PKL_AST_INTEGER:
      IPRINTF ("INTEGER::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_IMM (value, INTEGER_VALUE, "%" PRIu64);
      break;

    case PKL_AST_STRING:
      IPRINTF ("STRING::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_IMM (length, STRING_LENGTH, "%zu");
      PRINT_AST_IMM (pointer, STRING_POINTER, "%p");
      PRINT_AST_OPT_IMM (*pointer, STRING_POINTER, "'%s'");
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

    case PKL_AST_STRUCT_ELEM:
      IPRINTF ("STRUCT_ELEM::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (name, STRUCT_ELEM_NAME);
      PRINT_AST_SUBAST (exp, STRUCT_ELEM_EXP);
      break;

    case PKL_AST_STRUCT:
      IPRINTF ("STRUCT::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_IMM (nelem, STRUCT_NELEM, "%zu");
      IPRINTF ("elems:\n");
      PRINT_AST_SUBAST_CHAIN (STRUCT_ELEMS);
      break;
      
    case PKL_AST_ARRAY_INITIALIZER:
      IPRINTF ("ARRAY_INITIALIZER::\n");

      PRINT_AST_IMM (index, ARRAY_INITIALIZER_INDEX, "%zu");
      PRINT_AST_SUBAST (exp, ARRAY_INITIALIZER_EXP);
      break;

    case PKL_AST_ARRAY:
      IPRINTF ("ARRAY::\n");

      PRINT_AST_IMM (nelem, ARRAY_NELEM, "%zu");
      PRINT_AST_IMM (ninitializer, ARRAY_NINITIALIZER, "%zu");
      PRINT_AST_SUBAST (type, TYPE);
      IPRINTF ("initializers:\n");
      PRINT_AST_SUBAST_CHAIN (ARRAY_INITIALIZERS);
      break;

    case PKL_AST_ENUMERATOR:
      IPRINTF ("ENUMERATOR::\n");

      PRINT_AST_SUBAST (identifier, ENUMERATOR_IDENTIFIER);
      PRINT_AST_SUBAST (value, ENUMERATOR_VALUE);
      break;

    case PKL_AST_ENUM:
      IPRINTF ("ENUM::\n");

      PRINT_AST_SUBAST (tag, ENUM_TAG);
      IPRINTF ("values:\n");
      PRINT_AST_SUBAST_CHAIN (ENUM_VALUES);
      break;

    case PKL_AST_TYPE:
      IPRINTF ("TYPE::\n");

      IPRINTF ("code:\n");
      switch (PKL_AST_TYPE_CODE (ast))
        {
        case PKL_TYPE_INTEGRAL: IPRINTF ("  integral\n"); break;
        case PKL_TYPE_STRING: IPRINTF ("  string\n"); break;
        case PKL_TYPE_ARRAY: IPRINTF ("  array\n"); break;
        case PKL_TYPE_STRUCT: IPRINTF ("  struct\n"); break;
        case PKL_TYPE_OFFSET: IPRINTF ("  offset\n"); break;
        default:
          IPRINTF (" unknown (%d)\n", PKL_AST_TYPE_CODE (ast));
          break;
        }
      PRINT_AST_IMM (code, TYPE_CODE, "%d");
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
          PRINT_AST_SUBAST_CHAIN (TYPE_S_ELEMS);
          break;
        case PKL_TYPE_OFFSET:
          PRINT_AST_SUBAST (base_type, TYPE_O_BASE_TYPE);
          PRINT_AST_IMM (unit, TYPE_O_UNIT, "%d");
          break;
        case PKL_TYPE_STRING:
        default:
          break;
        }
      break;

    case PKL_AST_STRUCT_TYPE_ELEM:
      IPRINTF ("STRUCT_TYPE_ELEM::\n");
      
      PRINT_AST_SUBAST (name, STRUCT_TYPE_ELEM_NAME);
      PRINT_AST_SUBAST (type, STRUCT_TYPE_ELEM_TYPE);
      break;
      
    case PKL_AST_ARRAY_REF:
      IPRINTF ("ARRAY_REF::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (array, ARRAY_REF_ARRAY);
      PRINT_AST_SUBAST (index, ARRAY_REF_INDEX);
      break;

    case PKL_AST_STRUCT_REF:
      IPRINTF ("STRUCT_REF::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (struct, STRUCT_REF_STRUCT);
      PRINT_AST_SUBAST (identifier, STRUCT_REF_IDENTIFIER);
      break;

    case PKL_AST_OFFSET:
      IPRINTF ("OFFSET::\n");

      PRINT_AST_SUBAST (type, TYPE);
      PRINT_AST_SUBAST (magnitude, OFFSET_MAGNITUDE);
      PRINT_AST_IMM (unit, OFFSET_UNIT, "%d");
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
