/* pcl-parser.c - Parser for PCL.  */

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

#include <xalloc.h>
#include <string.h>
#include <assert.h>

#include "pcl-ast.h"
#include "pcl-parser.h"
#include "pcl-tab.h"
#include "pcl-lex.h"

/* Allocate and initialize a parser.  */

struct pcl_parser *
pcl_parser_init (void)
{
  size_t i;
  struct pcl_parser *parser;

  parser = xmalloc (sizeof (struct pcl_parser));
  pcl_tab_lex_init (&(parser->scanner));
  pcl_tab_set_extra (parser, parser->scanner);

  /* Zero the hash tables.  */
  memset (parser->ids_hash_table, 0, HASH_TABLE_SIZE);
  memset (parser->types_hash_table, 0, HASH_TABLE_SIZE);
  memset (parser->enums_hash_table, 0, HASH_TABLE_SIZE);
  memset (parser->structs_hash_table, 0, HASH_TABLE_SIZE);

  /* Register standard types.  */
  {
    static struct
    {
      int code;
      char *id;
      size_t size;
    } *type, stdtypes[] =
        {
#define PCL_DEF_TYPE(CODE,ID,SIZE) {CODE, ID, SIZE},
# include "pcl-types.def"
#undef PCL_DEF_TYPE
          { PCL_TYPE_NOTYPE, NULL, 0 }
        };

    for (type = stdtypes; type->code != PCL_TYPE_NOTYPE; type++)
      {
        pcl_ast t = pcl_ast_make_type (type->code,
                                       1, /* signed_p  */
                                       type->size,
                                       NULL /* enumeration */,
                                       NULL /* strct */);
        pcl_parser_register (parser, type->id, t);
      }
  }
  
  return parser;
}


/* Free resources used by a parser.  */

static void
free_hash_table (pcl_hash *hash_table)
{
  size_t i;
  pcl_ast t, n;

  for (i = 0; i < HASH_TABLE_SIZE; ++i)
    if ((*hash_table)[i])
      for (t = (*hash_table)[i]; t; t = n)
        {
          n = PCL_AST_CHAIN (t);
          pcl_ast_free (t);
        }
}
                 

void
pcl_parser_destroy (struct pcl_parser *parser)
{
  size_t i;
  pcl_ast t, n;
  
  pcl_ast_free (parser->ast);

  free_hash_table (&parser->ids_hash_table);
  free_hash_table (&parser->types_hash_table);
  free_hash_table (&parser->enums_hash_table);
  free_hash_table (&parser->structs_hash_table);

  pcl_tab_lex_destroy (parser->scanner);
  free (parser);

  return;
}

/* Read from FD until end of file, parsing its contents as a PCL
   program.  Return 0 if the parsing was successful, 1 if there was a
   syntax error and 2 if there was a memory exhaustion.  */

int
pcl_parse_file (FILE *fd)
{
  int ret;
  struct pcl_parser *pcl_parser;

  pcl_parser = pcl_parser_init ();

  pcl_tab_set_in (fd, pcl_parser->scanner);
  ret = pcl_tab_parse (pcl_parser);

                  
#ifdef PCL_DEBUG
                  pcl_ast_print (stdout, pcl_parser->ast);
                  /* pcl_gen ($$); */
#endif                  
  
  pcl_parser_destroy (pcl_parser);
  return ret;
}

/* Parse the contents of BUFFER as a PCL program.  Return 0 if the
   parsing was successful, 1 if there was a syntax error and 2 if
   there was a memory exhaustion.  */

int
pcl_parse_buffer (char *buffer, size_t size)
{
  YY_BUFFER_STATE yybuffer;
  void *pcl_scanner;
  struct pcl_parser *parser;
  int ret;

  /* Initialize the PCL parser.  */
  
  pcl_tab_lex_init (&pcl_scanner);
  pcl_tab_set_extra (parser, pcl_scanner);

  yybuffer = pcl_tab__scan_buffer(buffer, size, pcl_scanner);

  ret = pcl_tab_parse (pcl_scanner);
  if (ret == 1)
    printf ("SYNTAX ERROR\n");
  else if (ret == 2)
    printf ("MEMORY EXHAUSTION\n");

  /* Free resources.  */
  pcl_tab__delete_buffer (yybuffer, pcl_scanner);
  pcl_tab_lex_destroy (pcl_scanner);

  return ret;
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

/* Return a PCL_AST_IDENTIFIER node whose name is the NULL-terminated
   string STR.  If an identifier with that name has previously been
   referred to, the name node is returned this time.  */

pcl_ast
pcl_parser_get_identifier (struct pcl_parser *parser,
                           const char *str)
{
  pcl_ast id;
  int hash;
  size_t len;

  /* Compute the hash code for the identifier string.  */
  len = strlen (str);

  hash = hash_string (str);

  /* Search the hash table for the identifier.  */
  for (id = parser->ids_hash_table[hash];
       id != NULL;
       id = PCL_AST_CHAIN (id))
    if (PCL_AST_IDENTIFIER_LENGTH (id) == len
        && !strcmp (PCL_AST_IDENTIFIER_POINTER (id), str))
      return id;

  /* Create a new node for this identifier, and put it in the hash
     table.  */
  id = pcl_ast_make_identifier (str);
  PCL_AST_REGISTERED_P (id) = 1;
  PCL_AST_CHAIN (id) = parser->ids_hash_table[hash];
  parser->ids_hash_table[hash] = id;

  return id;

}

/* Register an AST under the given NAME in the corresponding hash
   table maintained by the parser, and return a pointer to it.  */

pcl_ast
pcl_parser_register (struct pcl_parser *parser,
                     const char *name, pcl_ast ast)
{
  enum pcl_ast_code code;
  enum pcl_ast_type_code type_code;
  pcl_hash *hash_table;
  int hash;
  pcl_ast t;

  code = PCL_AST_CODE (ast);
  assert (code == PCL_AST_TYPE || code == PCL_AST_ENUM
          || code == PCL_AST_STRUCT);

  if (code == PCL_AST_ENUM)
    hash_table = &parser->enums_hash_table;
  else if (code == PCL_AST_STRUCT)
    hash_table = &parser->structs_hash_table;
  else
    {
      type_code = PCL_AST_TYPE_CODE (ast);
      
      if (type_code == PCL_TYPE_ENUM)
        hash_table = &parser->enums_hash_table;
      else if (type_code == PCL_TYPE_STRUCT)
        hash_table = &parser->structs_hash_table;
      else
        hash_table = &parser->types_hash_table;
    }

  hash = hash_string (name);

  for (t = (*hash_table)[hash]; t != NULL; t = PCL_AST_CHAIN (t))
    if ((code == PCL_AST_TYPE
         && (PCL_AST_TYPE_NAME (t)
             && !strcmp (PCL_AST_TYPE_NAME (t), name)))
        || (code == PCL_AST_ENUM
            && PCL_AST_ENUM_TAG (t)
            && PCL_AST_IDENTIFIER_POINTER (PCL_AST_ENUM_TAG (t))
            && !strcmp (PCL_AST_IDENTIFIER_POINTER (PCL_AST_ENUM_TAG (t)), name))
        || (code == PCL_AST_STRUCT
            && PCL_AST_STRUCT_TAG (t)
            && PCL_AST_IDENTIFIER_POINTER (PCL_AST_STRUCT_TAG (t))
            && !strcmp (PCL_AST_IDENTIFIER_POINTER (PCL_AST_STRUCT_TAG (t)), name)))
      return NULL;

  /* Set the type as registered.  */
  PCL_AST_REGISTERED_P (ast) = 1;

  if (code == PCL_AST_TYPE)
    /* Put the passed type in the hash table.  */
    PCL_AST_TYPE_NAME (ast) = xstrdup (name);

  PCL_AST_CHAIN (ast) = (*hash_table)[hash];
  (*hash_table)[hash] = ast;

  return ast;
}

/* Return the AST registered under the name NAME, of type CODE has not
   been registered, return NULL.  */

pcl_ast
pcl_parser_get_registered (struct pcl_parser *parser,
                           const char *name,
                           enum pcl_ast_code code)
{
  int hash;
  pcl_ast t;
  pcl_hash *hash_table;

  assert (code == PCL_AST_TYPE || code == PCL_AST_ENUM
          || code == PCL_AST_STRUCT);

  if (code == PCL_AST_ENUM)
    hash_table = &parser->enums_hash_table;
  else if (code == PCL_AST_STRUCT)
    hash_table = &parser->structs_hash_table;
  else
    hash_table = &parser->types_hash_table;
    
  hash = hash_string (name);

  /* Search the hash table for the type.  */
  for (t = (*hash_table)[hash]; t != NULL; t = PCL_AST_CHAIN (t))
    if ((code == PCL_AST_TYPE
         && (PCL_AST_TYPE_NAME (t)
             && !strcmp (PCL_AST_TYPE_NAME (t), name)))
        || (code == PCL_AST_ENUM
            && PCL_AST_ENUM_TAG (t)
            && PCL_AST_IDENTIFIER_POINTER (PCL_AST_ENUM_TAG (t))
            && !strcmp (PCL_AST_IDENTIFIER_POINTER (PCL_AST_ENUM_TAG (t)), name))
        || (code == PCL_AST_STRUCT
            && PCL_AST_STRUCT_TAG (t)
            && PCL_AST_IDENTIFIER_POINTER (PCL_AST_STRUCT_TAG (t))
            && !strcmp (PCL_AST_IDENTIFIER_POINTER (PCL_AST_STRUCT_TAG (t)), name)))
      return t;

  return NULL;
}
