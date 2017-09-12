/* pcl-ast.c - Abstract Syntax Tree for PCL.  */

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
#include "xalloc.h"
#include "pcl-ast.h"

/* Return the endianness of the running system.  */

enum pcl_ast_endian
pcl_ast_default_endian (void)
{
  char buffer[4] = { 0x0, 0x0, 0x0, 0x1 };
  uint32_t canary = *((uint32_t *) buffer);

  return canary == 0x1 ? PCL_AST_MSB : PCL_AST_LSB;
}

/* Allocate and return a new AST node, with the given CODE.  The rest
   of the node is initialized to zero.  */
  
static pcl_ast
pcl_ast_make_node (enum pcl_ast_code code)
{
  pcl_ast ast;

  ast = xmalloc (sizeof (union pcl_ast_s));
  memset (ast, 0, sizeof (ast));
  PCL_AST_CODE (ast) = code;

  return ast;
}

/* Chain AST2 at the end of the tree node chain in AST1.  If AST1 is
   null then it returns AST2.  */

pcl_ast
pcl_ast_chainon (pcl_ast ast1, pcl_ast ast2)
{
  if (ast1)
    {
      pcl_ast tmp;

      for (tmp = ast1; PCL_AST_CHAIN (tmp); tmp = PCL_AST_CHAIN (tmp))
        if (tmp == ast2)
          abort ();

      PCL_AST_CHAIN (tmp) = ast2;
      return ast1;
    }

  return ast2;
}

/* The PCL_AST_IDENTIFIER nodes are unique by name, and are stored in
   a hash table to avoid replicating data unnecessarily.  */

#define HASH_TABLE_SIZE 1008
static pcl_ast ids_hash_table[HASH_TABLE_SIZE];

/* Return a PCL_AST_IDENTIFIER node whose name is the NULL-terminated
   string STR.  If an identifier with that name has previously been
   referred to, the name node is returned this time.  */

pcl_ast 
pcl_ast_get_identifier (const char *str)
{
  pcl_ast id;
  int hash;
  int i;
  size_t len;

  /* Compute the hash code for the identifier string.  */
  len = strlen (str);

  hash = len;
  for (i = 0; i < len; i++)
    hash = ((hash * 613) + (unsigned)(str[i]));

#define HASHBITS 30  
  hash &= (1 << HASHBITS) - 1;
  hash %= HASH_TABLE_SIZE;
#undef HASHBITS

  /* Search the hash table for the identifier.  */
  for (id = ids_hash_table[hash]; id != NULL; id = PCL_AST_CHAIN (id))
    if (PCL_AST_IDENTIFIER_LENGTH (id) == len
        && !strcmp (PCL_AST_IDENTIFIER_POINTER (id), str))
      return id;

  /* Create a new node for this identifier, and put it in the hash
     table.  */
  id = pcl_ast_make_node (PCL_AST_IDENTIFIER);
  PCL_AST_IDENTIFIER_LENGTH (id) = len;
  PCL_AST_IDENTIFIER_POINTER (id) = strdup (str);

  PCL_AST_CHAIN (id) = ids_hash_table[hash];
  ids_hash_table[hash] = id;

  return id;
}

/* Named types are stored in a hash table, and are referred from AST
   nodes.  */

static pcl_ast types_hash_table[HASH_TABLE_SIZE];

static int
type_hash (const char *name)
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

/* Register a named type, storing it in the types hash table, and
   return a pointer to it.  If a type with the given name has been
   already registered, return NULL.  */

pcl_ast
pcl_ast_register_type (const char *name, pcl_ast type)
{
  int hash;
  pcl_ast t;

  hash = type_hash (name);

  /* Search the hash table for the type.  */
  for (t = types_hash_table[hash]; t != NULL; t = PCL_AST_CHAIN (t))
    if (PCL_AST_TYPE_NAME (t)
        && !strcmp (PCL_AST_TYPE_NAME (t), name))
      return NULL;

  /* Put the passed type in the hash table.  */
  PCL_AST_CHAIN (type) = types_hash_table[hash];
  types_hash_table[hash] = type;

  return type;
}

/* Return the type associated with the given NAME.  If the type name
   has not been registered, return NULL.  */

pcl_ast
pcl_ast_get_type (const char *name)
{
  int hash;
  pcl_ast t;
  
  hash = type_hash (name);

  /* Search the hash table for the type.  */
  for (t = types_hash_table[hash]; t != NULL; t = PCL_AST_CHAIN (t))
    if (PCL_AST_TYPE_NAME (t)
        && !strcmp (PCL_AST_TYPE_NAME (t), name))
      return t;

  return NULL;
}

/* Build and return an AST node for an integer constant.  */

pcl_ast 
pcl_ast_make_integer (uint64_t value)
{
  pcl_ast new = pcl_ast_make_node (PCL_AST_INTEGER);

  PCL_AST_INTEGER_VALUE (new) = value;
  PCL_AST_LITERAL_P (new) = 1;
  return new;
}

/* Build and return an AST node for a string constant.  */

pcl_ast 
pcl_ast_make_string (const char *str)
{
  pcl_ast new = pcl_ast_make_node (PCL_AST_STRING);

  PCL_AST_STRING_POINTER (new) = strdup (str);
  PCL_AST_STRING_LENGTH (new) = strlen (str);
  return new;
}

/* Build and return an AST node for an enumerator.  */

pcl_ast 
pcl_ast_make_enumerator (pcl_ast identifier,
                         pcl_ast value,
                         pcl_ast docstr)
{
  pcl_ast enumerator = pcl_ast_make_node (PCL_AST_ENUMERATOR);

  assert (identifier != NULL);
  
  PCL_AST_ENUMERATOR_IDENTIFIER (enumerator) = identifier;
  PCL_AST_ENUMERATOR_VALUE (enumerator) = value;
  PCL_AST_ENUMERATOR_DOCSTR (enumerator) = docstr;

  return enumerator;
}

/* Build and return an AST node for a conditional expression.  */

pcl_ast
pcl_ast_make_cond_exp (pcl_ast cond,
                       pcl_ast thenexp,
                       pcl_ast elseexp)
{
  pcl_ast cond_exp = pcl_ast_make_node (PCL_AST_COND_EXP);

  assert (cond && thenexp && elseexp);

  PCL_AST_COND_EXP_COND (cond_exp) = cond;
  PCL_AST_COND_EXP_THENEXP (thenexp) = thenexp;
  PCL_AST_COND_EXP_ELSEEXP (elseexp) = elseexp;

  PCL_AST_LITERAL_P (cond_exp)
    = PCL_AST_LITERAL_P (thenexp) && PCL_AST_LITERAL_P (elseexp);
  
  return cond_exp;
}

/* Build and return an AST node for a binary expression.  */

pcl_ast
pcl_ast_make_binary_exp (enum pcl_ast_op code,
                         pcl_ast op1,
                         pcl_ast op2)
{
  pcl_ast exp = pcl_ast_make_node (PCL_AST_EXP);

  assert (op1 && op2);

  PCL_AST_EXP_CODE (exp) = code;
  PCL_AST_EXP_NUMOPS (exp) = 2;
  PCL_AST_EXP_OPERAND (exp, 0) = op1;
  PCL_AST_EXP_OPERAND (exp, 1) = op2;

  PCL_AST_LITERAL_P (exp)
    = PCL_AST_LITERAL_P (op1) && PCL_AST_LITERAL_P (op2);

  return exp;
}

/* Build and return an AST node for an unary expression.  */

pcl_ast
pcl_ast_make_unary_exp (enum pcl_ast_op code,
                        pcl_ast op)
{
  pcl_ast exp = pcl_ast_make_node (PCL_AST_EXP);

  PCL_AST_EXP_CODE (exp) = code;
  PCL_AST_EXP_NUMOPS (exp) = 1;
  PCL_AST_EXP_OPERAND (exp, 0) = op;
  PCL_AST_LITERAL_P (exp) = PCL_AST_LITERAL_P (op);
  
  return exp;
}

/* Build and return an AST node for an array reference.  */

pcl_ast
pcl_ast_make_array_ref (pcl_ast base, pcl_ast index)
{
  pcl_ast aref = pcl_ast_make_node (PCL_AST_ARRAY_REF);

  assert (base && index);

  PCL_AST_ARRAY_REF_BASE (aref) = base;
  PCL_AST_ARRAY_REF_INDEX (aref) = index;
  PCL_AST_LITERAL_P (aref) = 0;

  return aref;
}

/* Build and return an AST node for a struct reference.  */

pcl_ast
pcl_ast_make_struct_ref (pcl_ast base, pcl_ast identifier)
{
  pcl_ast sref = pcl_ast_make_node (PCL_AST_STRUCT_REF);

  assert (base && identifier
          && PCL_AST_CODE (identifier) == PCL_AST_IDENTIFIER);

  PCL_AST_STRUCT_REF_BASE (sref) = base;
  PCL_AST_STRUCT_REF_IDENTIFIER (sref) = identifier;

  return sref;
}

/* Build and return an AST node for a type.  */

pcl_ast
pcl_ast_make_type (int signed_p, pcl_ast width,
                   pcl_ast enumeration, pcl_ast strct)
{
  pcl_ast type = pcl_ast_make_node (PCL_AST_TYPE);

  assert (!!width + !!enumeration + !!strct == 1);

  PCL_AST_TYPE_SIGNED_P (type) = signed_p;
  PCL_AST_TYPE_WIDTH (type) = width;
  PCL_AST_TYPE_ENUMERATION (type) = enumeration;
  PCL_AST_TYPE_STRUCT (type) = strct;

  return type;
}

/* Build and return an AST node for a struct.  */

pcl_ast
pcl_ast_make_struct (pcl_ast tag, pcl_ast fields, pcl_ast docstr,
                     enum pcl_ast_endian endian)
{
  pcl_ast strct = pcl_ast_make_node (PCL_AST_STRUCT);

  assert (tag && fields);

  PCL_AST_STRUCT_TAG (strct) = tag;
  PCL_AST_STRUCT_FIELDS (strct) = fields;
  PCL_AST_STRUCT_DOCSTR (strct) = docstr;
  PCL_AST_STRUCT_ENDIAN (strct) = endian;

  return strct;
}

/* Build and return an AST node for an enum.  */

pcl_ast
pcl_ast_make_enum (pcl_ast tag, pcl_ast values, pcl_ast docstr)
{
  pcl_ast enumeration = pcl_ast_make_node (PCL_AST_ENUM);

  assert (tag && values);

  PCL_AST_ENUM_TAG (enumeration) = tag;
  PCL_AST_ENUM_VALUES (enumeration) = values;
  PCL_AST_ENUM_DOCSTR (enumeration) = docstr;

  return enumeration;
}

/* Build and return an AST node for a struct field.  */

pcl_ast
pcl_ast_make_field (pcl_ast name, pcl_ast type, pcl_ast docstr,
                    enum pcl_ast_endian endian, pcl_ast size_exp)
{
  pcl_ast field = pcl_ast_make_node (PCL_AST_FIELD);

  assert (name);

  PCL_AST_FIELD_NAME (field) = name;
  PCL_AST_FIELD_TYPE (field) = type;
  PCL_AST_FIELD_DOCSTR (field) = docstr;
  PCL_AST_FIELD_ENDIAN (field) = endian;
  PCL_AST_FIELD_SIZE_EXP (field) = size_exp;

  return field;
}

/* Build and return an AST node for a PCL program.  */

pcl_ast
pcl_ast_make_program (void)
{
  pcl_ast program = pcl_ast_make_node (PCL_AST_PROGRAM);

  return program;
}
