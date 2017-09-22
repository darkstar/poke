/* pkl-parser.c - Parser for PKL.  */

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

#include "pkl-ast.h"
#include "pkl-parser.h"
#include "pkl-tab.h"
#include "pkl-lex.h"

/* Allocate and initialize a parser.  */

static struct pkl_parser *
pkl_parser_init (void)
{
  size_t i;
  struct pkl_parser *parser;

  parser = xmalloc (sizeof (struct pkl_parser));
  pkl_tab_lex_init (&(parser->scanner));
  pkl_tab_set_extra (parser, parser->scanner);

  parser->ast = pkl_ast_init ();

  parser->ps1 = "(poke) ";
  parser->ps2 = "> ";
  parser->eof = 0;
  parser->error = NULL;
  parser->at_start = 1;
  parser->at_end = 0;

  /* Register standard types.  */
  {
    static struct
    {
      int code;
      char *id;
      size_t size;
    } *type, stdtypes[] =
        {
#define PKL_DEF_TYPE(CODE,ID,SIZE) {CODE, ID, SIZE},
# include "pkl-types.def"
#undef PKL_DEF_TYPE
          { PKL_TYPE_NOTYPE, NULL, 0 }
        };

    for (type = stdtypes; type->code != PKL_TYPE_NOTYPE; type++)
      {
        pkl_ast_node t = pkl_ast_make_type (type->code,
                                            1, /* signed_p  */
                                            type->size,
                                            NULL /* enumeration */,
                                            NULL /* strct */);
        pkl_ast_register (parser->ast, type->id, t);
      }
  }
  
  return parser;
}


/* Free resources used by a parser, exceptuating the AST.  */

void
pkl_parser_free (struct pkl_parser *parser)
{
  size_t i;
  pkl_ast_node t, n;
  
  pkl_tab_lex_destroy (parser->scanner);
  free (parser);

  return;
}

/* Read from FD until end of file, parsing its contents as a PKL
   program.  Return 0 if the parsing was successful, 1 if there was a
   syntax error and 2 if there was a memory exhaustion.  */

int
pkl_parse_file (pkl_ast *ast, FILE *fd)
{
  int ret;
  struct pkl_parser *parser;

  parser = pkl_parser_init ();

  pkl_tab_set_in (fd, parser->scanner);
  ret = pkl_tab_parse (parser);
  *ast = parser->ast;
  pkl_parser_free (parser);

  return ret;
}

/* Parse the contents of BUFFER as a PKL program.  Return 0 if the
   parsing was successful, 1 if there was a syntax error and 2 if
   there was a memory exhaustion.  */

int
pkl_parse_buffer (pkl_ast *ast, char *buffer, size_t size)
{
  YY_BUFFER_STATE yybuffer;
  void *pkl_scanner;
  struct pkl_parser *parser;
  int ret;

  parser = pkl_parser_init ();

  yybuffer = pkl_tab__scan_buffer(buffer, size, parser->scanner);

  ret = pkl_tab_parse (pkl_scanner);
  *ast = parser->ast;

  pkl_tab__delete_buffer (yybuffer, pkl_scanner);
  pkl_parser_free (parser);

  return ret;
}

