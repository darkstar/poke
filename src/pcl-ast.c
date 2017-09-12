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
#include "xalloc.h"
#include "pcl-ast.h"

/* Allocate and return a new AST node, with the given CODE.  The rest
   of the node is initialized to zero.  */
  
static pcl_ast
pcl_ast_make_node (enum pcl_ast_code code)
{
  pcl_ast ast;

  ast = xmalloc (sizeof (union pcl_ast));
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
   referred to, the ame node is returned this time.  */

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
  hash %= MAX_HASH_TABLE;

  /* Search the hash table for the identifier.  */
  for (id = ids_hash_table[hash]; id != NULL; idp = PCL_AST_CHAIN (id))
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

  PCL_AST_STRING_POINTER (new) = str;
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

  assert (condition && thenexp && elseexp);

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
  PCL_AST_EXP_OPERAND (exp)[0] = op1;
  PCL_AST_EXP_OPERAND (exp)[1] = op2;

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

  assert (op == PCL_AST_OP_INC
          || op == PCL_AST_OP_DEC
          || op == PCL_AST_OP_SIZEOF
          || op == PCL_AST_OP_ADDRESS
          || op == PCL_AST_OP_POS
          || op == PCL_AST_OP_NEG
          || op == PCL_AST_OP_BNOT
          || op == PCL_AST_OP_NOT);

  PCL_AST_EXP_CODE (exp) = code;
  PCL_ST_EXP_NUMOPS (exp) = 1;
  PCL_AST_EXP_OPERAND (exp)[0] = op;
  PCL_AST_LITERAL_P (exp) = PCL_AST_LITERAL_p (op);
  
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
  PCL_AST_LITERAL_P (sref) = 0;

  return sref;
}
