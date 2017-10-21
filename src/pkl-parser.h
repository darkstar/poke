/* pkl-parser.h - Parser for Poke.  */

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

#ifndef PKL_PARSER_H
#define PKL_PARSER_H

#include <config.h>
#include <stdio.h>

#include "pkl-ast.h"

/* Binding levels.

   Some syntactic constructions like function bodies, struct bodies
   and compound statements, introduce binding contours.

   There is also a global contour, containing the bindings of global
   symbols.

   When the parser finds a name, its meaning can be found by searching
   the binding levels from the current one out to the global one.  */

#define PKL_BIND_LEVEL_NAMES(L) ((L)->names)
#define PKL_BIND_LEVEL_PARENT(L) ((L)->parent)

struct pkl_bind_level
{
  /* Chain of PKL_AST_DECL nodes for all the variables, constants,
     functions and types defined in this binding level.  */
  pkl_ast_node names;

  /* Parent binding level, i.e. the binding level containing this
     one.  */
  struct pkl_bind_level *parent;
};

typedef struct pkl_bind_level *pkl_bind_level;

pkl_bind_level pkl_bind_level_new (void);

/* The `pkl_parser' struct holds the parser state.

   SCANNER is a flex scanner.
   AST is the abstract syntax tree created by the bison parser.  */

struct pkl_parser
{
  void *scanner;
  pkl_ast ast;
  int interactive;
  char *filename;
  int what; /* What to parse.  */
  size_t nchars;

  /* Outermost binding level.  This is populated when the parser is
     created.  */
  pkl_bind_level global_bind_level;
  
  /* Binding level currently in effect.  */
  pkl_bind_level current_bind_level;
};

/* Enter a new binding level.  */
void pkl_push_level (struct pkl_parser *parser);

/* Exit a binding level.  */
void pkl_pop_level (struct pkl_parser *parser);

/* Public interface.  */

#define PKL_PARSE_PROGRAM 0
#define PKL_PARSE_EXPRESSION 1

int pkl_parse_cmdline (pkl_ast *ast);
int pkl_parse_file (pkl_ast *ast, int what, FILE *fd, const char *fname);
int pkl_parse_buffer (pkl_ast *ast, int what, char *buffer, char **end);



#endif /* !PKL_PARSER_H */
