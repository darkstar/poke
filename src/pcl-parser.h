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

/* The following struct holds the parser state.  */

#define HASH_TABLE_SIZE 1008

struct pcl_parser
{
  /* Flex scanner.  */
  void *scanner;
  
  /* Abstract syntax tree built by the parser.  */
  pcl_ast ast;

  /* The abstract syntax tree points to entries in the hash tables
     below, which are created during parsing.  */
  pcl_ast ids_hash_table[HASH_TABLE_SIZE];
  pcl_ast types_hash_table[HASH_TABLE_SIZE];
  pcl_ast enums_hash_table[HASH_TABLE_SIZE];
};

/* Exported functions defined in pcl-parser.c.  */

int pcl_parse_file (FILE *fd);
int pcl_parse_buffer (char *buffer, size_t size);

#endif /* !PCL_PARSER_H */
