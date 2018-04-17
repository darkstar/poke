/* pkl-parser.c - Parser for Poke.  */

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

#include <xalloc.h>
#include <string.h>
#include <assert.h>

#include "pkl-ast.h"
#include "pkl-parser.h"
#include "pkl-tab.h"
#include "pkl-lex.h"

/* Allocate and initialize a parser.  */

static struct pkl_parser *
pkl_parser_init (void)
{
  struct pkl_parser *parser;

  parser = xmalloc (sizeof (struct pkl_parser));
  memset (parser, 0, sizeof (struct pkl_parser));
  
  pkl_tab_lex_init (&(parser->scanner));
  pkl_tab_set_extra (parser, parser->scanner);

  parser->env = NULL;
  parser->ast = pkl_ast_init ();
  parser->interactive = 0;
  parser->filename = NULL;
  parser->nchars = 0;

  return parser;
}

/* Free resources used by a parser, exceptuating the AST.  */

void
pkl_parser_free (struct pkl_parser *parser)
{
  pkl_tab_lex_destroy (parser->scanner);
  free (parser->filename);

  free (parser);

  return;
}

/* Read from FD until end of file, parsing its contents as a PKL
   program.  Return 0 if the parsing was successful, 1 if there was a
   syntax error and 2 if there was a memory exhaustion.  */

int
pkl_parse_file (pkl_env env,
                pkl_ast *ast, FILE *fd, const char *fname)
{
  int ret;
  struct pkl_parser *parser;

  parser = pkl_parser_init ();
  parser->filename = xstrdup (fname);
  parser->start_token = START_PROGRAM;

  parser->env = env;
  parser->ast->file = fd;
  parser->ast->filename = xstrdup (fname);
  pkl_tab_set_in (fd, parser->scanner);
  ret = pkl_tab_parse (parser);
  *ast = parser->ast;
  pkl_parser_free (parser);

  return ret;
}

/* Parse the contents of BUFFER as a PKL program, or an expression
   depending on the value of WHAT.  If END is not NULL, set it to the
   first character after the parsed string.  Return 0 if the parsing
   was successful, 1 if there was a syntax error and 2 if there was a
   memory exhaustion.  */

int
pkl_parse_buffer (pkl_env env,
                  pkl_ast *ast, int what, char *buffer, char **end)
{
  YY_BUFFER_STATE yybuffer;
  struct pkl_parser *parser;
  int ret;

  parser = pkl_parser_init ();
  parser->interactive = 1;

  if (what == PKL_PARSE_PROGRAM)
    parser->start_token = START_PROGRAM;
  else if (what == PKL_PARSE_EXPRESSION)
    parser->start_token = START_EXP;
  else
    assert (0);

  yybuffer = pkl_tab__scan_string(buffer, parser->scanner);

  /* XXX */
  /* pkl_tab_debug = 1; */
  parser->env = env;
  parser->ast->buffer = xstrdup (buffer);
  ret = pkl_tab_parse (parser);
  *ast = parser->ast;
  if (end != NULL)
    *end = buffer + parser->nchars;
  pkl_tab__delete_buffer (yybuffer, parser->scanner);
  pkl_parser_free (parser);

  return ret;
}

