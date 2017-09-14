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

#ifdef PCL_DEBUG
# include <stdio.h>
#endif

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

/* Return a PCL_AST_IDENTIFIER node whose name is the NULL-terminated
   string STR.  If an identifier with that name has previously been
   referred to, the name node is returned this time.  */

pcl_ast 
pcl_ast_get_identifier (const char *str)
{
  pcl_ast id;
  int hash;
  size_t len;

  /* Compute the hash code for the identifier string.  */
  len = strlen (str);

  hash = hash_string (str);

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

/* Register a named type, storing it in the types hash table, and
   return a pointer to it.  If a type with the given name has been
   already registered, return NULL.  */

pcl_ast
pcl_ast_register_type (const char *name, pcl_ast type)
{
  int hash;
  pcl_ast t;

  hash = hash_string (name);

  /* Search the hash table for the type.  */
  for (t = types_hash_table[hash]; t != NULL; t = PCL_AST_CHAIN (t))
    if (PCL_AST_TYPE_NAME (t)
        && !strcmp (PCL_AST_TYPE_NAME (t), name))
      return NULL;

  /* Put the passed type in the hash table.  */
  PCL_AST_TYPE_NAME (type) = xstrdup (name);
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
  
  hash = hash_string (name);

  /* Search the hash table for the type.  */
  for (t = types_hash_table[hash]; t != NULL; t = PCL_AST_CHAIN (t))
    if (PCL_AST_TYPE_NAME (t)
        && !strcmp (PCL_AST_TYPE_NAME (t), name))
      return t;

  return NULL;
}

/* Enumerations are stored in a hash table, and are referred from AST
   nodes.  */

static pcl_ast enums_hash_table[HASH_TABLE_SIZE];

/* Register an enumeration, storing it in the enums hash table, and
   return a pointer to it.  If an enumeration with the given tag has
   been already registered, return NULL.  */

pcl_ast
pcl_ast_register_enum (const char *tag, pcl_ast enumeration)
{
  int hash;
  pcl_ast e;

  hash = hash_string (tag);

  /* Search the hash table for the enumeration.  */
  for (e = enums_hash_table[hash]; e != NULL; e = PCL_AST_CHAIN (e))
    if (PCL_AST_ENUM_TAG (e)
        && PCL_AST_IDENTIFIER_POINTER (PCL_AST_ENUM_TAG (e))
        && !strcmp (PCL_AST_IDENTIFIER_POINTER (PCL_AST_ENUM_TAG (e)), tag))
      return NULL;

  /* Put the passed enumeration in the hash table.  */
  PCL_AST_CHAIN (enumeration) = enums_hash_table[hash];
  enums_hash_table[hash] = enumeration;

  return enumeration;
}

/* Return the enumeration associated with the given TAG.  If the
   enumeration tag has not been registered, return NULL.  */

pcl_ast
pcl_ast_get_enum (const char *tag)
{
  int hash;
  pcl_ast e;

  hash = hash_string (tag);

  /* Search the hash table for the enumeration.  */
  for (e = enums_hash_table[hash]; e != NULL; e = PCL_AST_CHAIN (e))
    if (PCL_AST_ENUM_TAG (e)
        && PCL_AST_IDENTIFIER_POINTER (PCL_AST_ENUM_TAG (e))
        && !strcmp (PCL_AST_IDENTIFIER_POINTER (PCL_AST_ENUM_TAG (e)), tag))
      return e;

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

  assert (str);
  
  PCL_AST_STRING_POINTER (new) = xstrdup (str);
  PCL_AST_STRING_LENGTH (new) = strlen (str);
  PCL_AST_LITERAL_P (new) = 1;

  return new;
}

/* Build and return an AST node for a doc string.  */

pcl_ast
pcl_ast_make_doc_string (const char *str, pcl_ast entity)
{
  pcl_ast doc_string = pcl_ast_make_node (PCL_AST_DOC_STRING);

  assert (str);

  PCL_AST_DOC_STRING_POINTER (doc_string) = xstrdup (str);
  PCL_AST_DOC_STRING_LENGTH (doc_string) = strlen (str);
  PCL_AST_DOC_STRING_ENTITY (doc_string) = entity;

  return doc_string;
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

  assert (tag);

  PCL_AST_STRUCT_TAG (strct) = tag;
  PCL_AST_STRUCT_FIELDS (strct) = fields;
  PCL_AST_STRUCT_DOCSTR (strct) = docstr;
  PCL_AST_STRUCT_ENDIAN (strct) = endian;

  return strct;
}

/* Build and return an AST node for an enum.  */

pcl_ast
pcl_ast_make_enum (pcl_ast type, pcl_ast tag, pcl_ast values,
                   pcl_ast docstr)
{
  pcl_ast enumeration = pcl_ast_make_node (PCL_AST_ENUM);

  assert (type && tag && values);

  PCL_AST_ENUM_TYPE (enumeration) = type;
  PCL_AST_ENUM_TAG (enumeration) = tag;
  PCL_AST_ENUM_VALUES (enumeration) = values;
  PCL_AST_ENUM_DOCSTR (enumeration) = docstr;

  return enumeration;
}

/* Build and return an AST node for a struct field.  */

pcl_ast
pcl_ast_make_field (pcl_ast name, pcl_ast type, pcl_ast docstr,
                    enum pcl_ast_endian endian, pcl_ast num_ents)
{
  pcl_ast field = pcl_ast_make_node (PCL_AST_FIELD);

  assert (name);

  PCL_AST_FIELD_NAME (field) = name;
  PCL_AST_FIELD_TYPE (field) = type;
  PCL_AST_FIELD_DOCSTR (field) = docstr;
  PCL_AST_FIELD_ENDIAN (field) = endian;
  PCL_AST_FIELD_NUM_ENTS (field) = num_ents;

  return field;
}

/* Build and return an AST node for a PCL program.  */

pcl_ast
pcl_ast_make_program (void)
{
  pcl_ast program = pcl_ast_make_node (PCL_AST_PROGRAM);

  return program;
}

#ifdef PCL_DEBUG

/* The following macros are commodities to be used to keep the
   `pcl_ast_print' function as readable and easy to update as
   possible.  Do not use them anywhere else.  */

#define IPRINTF(...)                            \
  do                                            \
    {                                           \
      int i;                                    \
      for (i = 0; i < indent; i++)              \
        printf (" ");                           \
      printf (__VA_ARGS__);                     \
    } while (0)

#define PRINT_AST_IMM(NAME,MACRO,FMT)                    \
  do                                                     \
    {                                                    \
      IPRINTF (#NAME ":\n");                             \
      IPRINTF ("  " FMT "\n", PCL_AST_##MACRO (ast));    \
    }                                                    \
  while (0)

#define PRINT_AST_SUBAST(NAME,MACRO)                            \
  do                                                            \
    {                                                           \
      IPRINTF (#NAME ":\n");                                    \
      pcl_ast_print_1 (fd, PCL_AST_##MACRO (ast), indent + 2);  \
    }                                                           \
  while (0)

#define PRINT_AST_OPT_IMM(NAME,MACRO,FMT)       \
  if (PCL_AST_##MACRO (ast))                    \
    {                                           \
      PRINT_AST_IMM (NAME, MACRO, FMT);         \
    }

#define PRINT_AST_OPT_SUBAST(NAME,MACRO)        \
  if (PCL_AST_##MACRO (ast))                    \
    {                                           \
      PRINT_AST_SUBAST (NAME, MACRO);           \
    }

#define PRINT_AST_SUBAST_CHAIN(MACRO)           \
  for (child = PCL_AST_##MACRO (ast);           \
       child;                                   \
       child = PCL_AST_CHAIN (child))           \
    {                                           \
      pcl_ast_print_1 (fd, child, indent + 2);  \
    }

/* Auxiliary function used by `pcl_ast_print', defined below.  */

static void
pcl_ast_print_1 (FILE *fd, pcl_ast ast, int indent)
{
  pcl_ast child;
  int i;
 
  if (ast == NULL)
    {
      IPRINTF ("NULL::\n");
      return;
    }
  
  switch (PCL_AST_CODE (ast))
    {
    case PCL_AST_PROGRAM:
      IPRINTF ("PROGRAM::\n");

      PRINT_AST_SUBAST_CHAIN (PROGRAM_DECLARATIONS);
      break;

    case PCL_AST_IDENTIFIER:
      IPRINTF ("IDENTIFIER::\n");

      PRINT_AST_IMM (length, IDENTIFIER_LENGTH, "%d");
      PRINT_AST_IMM (pointer, IDENTIFIER_POINTER, "0x%lx");
      PRINT_AST_OPT_IMM (*pointer, IDENTIFIER_POINTER, "'%s'");
      break;

    case PCL_AST_INTEGER:
      IPRINTF ("INTEGER::\n");

      PRINT_AST_IMM (value, INTEGER_VALUE, "%lu");
      break;

    case PCL_AST_STRING:
      IPRINTF ("STRING::\n");

      PRINT_AST_IMM (length, STRING_LENGTH, "%lu");
      PRINT_AST_IMM (pointer, STRING_POINTER, "0x%lx");
      PRINT_AST_OPT_IMM (*pointer, STRING_POINTER, "'%s'");
      break;

    case PCL_AST_DOC_STRING:
      IPRINTF ("DOCSTR::\n");

      PRINT_AST_IMM (length, DOC_STRING_LENGTH, "%lu");
      PRINT_AST_IMM (pointer, DOC_STRING_POINTER, "0x%lx");
      PRINT_AST_OPT_IMM (*pointer, DOC_STRING_POINTER, "'%s'");
      break;

    case PCL_AST_EXP:
      {
        /* XXX: replace this with an #include hack.  */
        /* Please keep the following array in sync with the enum
           pcl_ast_op in pcl-ast.h.  */
        static char *opcodes[] =
          { "OR", "IOR", "XOR", "AND", "BAND", "EQ", "NE",
            "SL", "SR", "ADD", "SUB", "MUL", "DIV", "MOD",
            "LT", "GT", "LE", "GE", "INC", "DEC", "SIZEOF",
            "ADDRESS", "POS", "NEG", "BNOT", "NOT", "ASSIGN",
            "MULA", "DIVA", "MODA", "ADDA", "SUBA", "SLA",
            "SRA", "BANDA", "XORA", "IORA"
          };

        IPRINTF ("EXPRESSION::\n");
        IPRINTF ("opcode: %s\n",
                 PCL_AST_EXP_CODE (ast) <= PCL_MAX_OPERATOR
                 ? opcodes[PCL_AST_EXP_CODE (ast)] : "unknown");
        PRINT_AST_IMM (numops, EXP_NUMOPS, "%d");
        IPRINTF ("operands:\n");
        for (i = 0; i < PCL_AST_EXP_NUMOPS (ast); i++)
          pcl_ast_print_1 (fd, PCL_AST_EXP_OPERAND (ast, i),
                         indent + 2);
        break;
      }

    case PCL_AST_COND_EXP:
      IPRINTF ("COND_EXPRESSION::\n");

      PRINT_AST_SUBAST (condition, COND_EXP_COND);
      PRINT_AST_OPT_SUBAST (thenexp, COND_EXP_THENEXP);
      PRINT_AST_OPT_SUBAST (elseexp, COND_EXP_ELSEEXP);
      break;

    case PCL_AST_ENUMERATOR:
      IPRINTF ("ENUMERATOR::\n");

      PRINT_AST_SUBAST (identifier, ENUMERATOR_IDENTIFIER);
      PRINT_AST_SUBAST (value, ENUMERATOR_VALUE);
      PRINT_AST_OPT_SUBAST (docstr, ENUMERATOR_DOCSTR);
      break;

    case PCL_AST_ENUM:
      IPRINTF ("ENUM::\n");

      PRINT_AST_SUBAST (type, ENUM_TYPE);
      PRINT_AST_SUBAST (tag, ENUM_TAG);
      PRINT_AST_OPT_SUBAST (docstr, ENUM_DOCSTR);
      IPRINTF ("values:\n");
      PRINT_AST_SUBAST_CHAIN (ENUM_VALUES);
      break;

    case PCL_AST_STRUCT:
      IPRINTF ("STRUCT::\n");

      IPRINTF ("STRUCT:: endian=%s\n",
               PCL_AST_STRUCT_ENDIAN (ast)
               == PCL_AST_MSB ? "msb" : "lsb");
      PRINT_AST_SUBAST (tag, STRUCT_TAG);
      PRINT_AST_OPT_SUBAST (docstr, STRUCT_DOCSTR);
      IPRINTF ("fields:\n");
      PRINT_AST_SUBAST_CHAIN (STRUCT_FIELDS);
      break;

    case PCL_AST_FIELD:
      IPRINTF ("FIELD::\n");

      IPRINTF ("endian:\n");
      IPRINTF ("  %s\n",
               PCL_AST_FIELD_ENDIAN (ast) == PCL_AST_MSB
               ? "msb" : "lsb");
      PRINT_AST_SUBAST (name, FIELD_NAME);
      PRINT_AST_SUBAST (type, FIELD_TYPE);
      PRINT_AST_OPT_SUBAST (num_ents, FIELD_NUM_ENTS);
      PRINT_AST_OPT_SUBAST (docstr, FIELD_DOCSTR);
      
      break;

    case PCL_AST_TYPE:
      IPRINTF ("TYPE::\n");

      PRINT_AST_IMM (signed, TYPE_SIGNED_P, "%d");
      PRINT_AST_OPT_SUBAST (width, TYPE_WIDTH);

      if (PCL_AST_TYPE_ENUMERATION (ast))
        {
          IPRINTF ("enumeration:\n");
          IPRINTF ("  '%s'\n",
                   PCL_AST_ENUM_TAG (PCL_AST_TYPE_ENUMERATION (ast)));
        }

      if (PCL_AST_TYPE_STRUCT (ast))
        IPRINTF ("struct: tag '%s'\n",
                 PCL_AST_STRUCT_TAG (PCL_AST_TYPE_STRUCT (ast)));
      break;

    default:
      IPRINTF ("UNKNOWN:: code=%d\n", PCL_AST_CODE (ast));
      break;
    }
}

/* Dump a printable representation of AST to the file descriptor FD.
   This function is intended to be useful to debug the PCL
   compiler.  */

void
pcl_ast_print (FILE *fd, pcl_ast ast)
{
  pcl_ast_print_1 (fd, ast, 0);
}

#undef IPRINTF
#undef PRINT_AST_IMM
#undef PRINT_AST_SUBAST
#undef PRINT_AST_OPT_FIELD
#undef PRINT_AST_OPT_SUBAST

#endif /* PCL_DEBUG */
