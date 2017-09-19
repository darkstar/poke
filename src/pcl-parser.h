/* pcl-parser.h - Parser for PCL.  */

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

#ifndef PCL_PARSER_H
#define PCL_PARSER_H

#include <config.h>
#include <stdio.h>

#include "pcl-ast.h"

/* The following struct holds the parser state.  */

#define HASH_TABLE_SIZE 1008
typedef pcl_ast pcl_hash[HASH_TABLE_SIZE];

struct pcl_parser
{
  /* Flex scanner.  */
  void *scanner;
  
  /* Abstract syntax tree built by the parser.  */
  pcl_ast ast;

  /* The abstract syntax tree points to entries in the hash tables
     below, which are created during parsing.  */
  pcl_hash ids_hash_table;
  pcl_hash types_hash_table;
  pcl_hash enums_hash_table;
  pcl_hash structs_hash_table;
};

/* Exported functions defined in pcl-parser.c.  */

int pcl_parse_file (FILE *fd);
int pcl_parse_buffer (char *buffer, size_t size);

pcl_ast pcl_parser_get_identifier (struct pcl_parser *parser,
                                   const char *str);
pcl_ast pcl_parser_get_registered (struct pcl_parser *parser,
                                   const char *name,
                                   enum pcl_ast_code code);
pcl_ast pcl_parser_register (struct pcl_parser *parser,
                             const char *name, pcl_ast ast);

#endif /* !PCL_PARSER_H */
