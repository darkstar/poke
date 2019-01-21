/* pkl-parser.h - Parser for Poke.  */

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

#ifndef PKL_PARSER_H
#define PKL_PARSER_H

#include <config.h>
#include <stdio.h>

#include "pkl-env.h"
#include "pkl-ast.h"

/* The `pkl_parser' struct holds the parser state.

   SCANNER is a flex scanner.
   AST is the abstract syntax tree created by the bison parser.  */

struct pkl_parser
{
  void *scanner;
  pkl_env env;
  pkl_ast ast;
  int interactive;
  char *filename;
  int start_token;
  size_t nchars;
};

/* Public interface.  */

#define PKL_PARSE_PROGRAM 0
#define PKL_PARSE_EXPRESSION 1
#define PKL_PARSE_DECLARATION 2

int pkl_parse_file (pkl_env *env, pkl_ast *ast, FILE *fd, const char *fname);
int pkl_parse_buffer (pkl_env *env, pkl_ast *ast, int what, char *buffer, char **end);



#endif /* !PKL_PARSER_H */
