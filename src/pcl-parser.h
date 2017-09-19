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

/* The `pcl_parser' struct holds the parser state.

   SCANNER is a flex scanner.
   AST is the abstract syntax tree created by the bison parser.  */

struct pcl_parser
{
  void *scanner;
  pcl_ast ast;
};

/* Exported functions defined in pcl-parser.c.  */

int pcl_parse_file (pcl_ast *ast, FILE *fd);
int pcl_parse_buffer (pcl_ast *ast, char *buffer, size_t size);

#endif /* !PCL_PARSER_H */
