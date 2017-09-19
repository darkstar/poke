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

static struct pcl_parser *
pcl_parser_init (void)
{
  size_t i;
  struct pcl_parser *parser;

  parser = xmalloc (sizeof (struct pcl_parser));
  pcl_tab_lex_init (&(parser->scanner));
  pcl_tab_set_extra (parser, parser->scanner);

  parser->ast = pcl_ast_init ();

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
        pcl_ast_node t = pcl_ast_make_type (type->code,
                                            1, /* signed_p  */
                                            type->size,
                                            NULL /* enumeration */,
                                            NULL /* strct */);
        pcl_ast_register (parser->ast, type->id, t);
      }
  }
  
  return parser;
}


/* Free resources used by a parser, exceptuating the AST.  */

void
pcl_parser_free (struct pcl_parser *parser)
{
  size_t i;
  pcl_ast_node t, n;
  
  pcl_tab_lex_destroy (parser->scanner);
  free (parser);

  return;
}

/* Read from FD until end of file, parsing its contents as a PCL
   program.  Return 0 if the parsing was successful, 1 if there was a
   syntax error and 2 if there was a memory exhaustion.  */

int
pcl_parse_file (pcl_ast *ast, FILE *fd)
{
  int ret;
  struct pcl_parser *parser;

  parser = pcl_parser_init ();

  pcl_tab_set_in (fd, parser->scanner);
  ret = pcl_tab_parse (parser);
  *ast = parser->ast;
  pcl_parser_free (parser);

  return ret;
}

/* Parse the contents of BUFFER as a PCL program.  Return 0 if the
   parsing was successful, 1 if there was a syntax error and 2 if
   there was a memory exhaustion.  */

int
pcl_parse_buffer (pcl_ast *ast, char *buffer, size_t size)
{
  YY_BUFFER_STATE yybuffer;
  void *pcl_scanner;
  struct pcl_parser *parser;
  int ret;

  parser = pcl_parser_init ();

  yybuffer = pcl_tab__scan_buffer(buffer, size, parser->scanner);

  ret = pcl_tab_parse (pcl_scanner);
  *ast = parser->ast;

  pcl_tab__delete_buffer (yybuffer, pcl_scanner);
  pcl_parser_free (parser);

  return ret;
}

