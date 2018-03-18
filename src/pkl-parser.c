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

pkl_bind_level
pkl_bind_level_new (void)
{
  pkl_bind_level bl = xmalloc (sizeof (struct pkl_bind_level));
  memset (bl, 0, sizeof (struct pkl_bind_level));

  return bl;
}


void
pkl_push_level (struct pkl_parser *parser)
{
  pkl_bind_level new = pkl_bind_level_new ();

  PKL_BIND_LEVEL_PARENT (new) = parser->current_bind_level;
  parser->current_bind_level = new;
}

void
pkl_pop_level (struct pkl_parser *parser)
{
  pkl_ast_node t;
  pkl_bind_level current_bind_level;

  current_bind_level = parser->current_bind_level;
  
  /* The local variables of this level do not have any meaning
     anymore.  */
  for (t = PKL_BIND_LEVEL_NAMES (current_bind_level);
       t;
       t = PKL_AST_CHAIN (t))
    PKL_AST_IDENTIFIER_LOCAL_VALUE (PKL_AST_DECL_NAME (t)) = 0;

#if 0  
  /* Outer names are not shadowed anymore.  */
  for (t = PKL_BIND_LEVEL_SHADOWED (current_bind_level);
       t;
       t = PKL_AST_CHAIN (t))
    PKL_AST_IDENTIFIER_LOCAL_VALUE (PKL_AST_PURPOSE (t)) = PKL_AST_VALUE (t);
#endif

  /* If the level being exited is the top-level of a function, match
     all goto statements with their labels.  */

  /* XXX: but this doesn't work for poke, since we allow nested
     functions.  */

  if (PKL_BIND_LEVEL_PARENT (current_bind_level)
      == parser->global_bind_level)
    {
      
    }

  /* Pop the current level.  */
  parser->current_bind_level
    = PKL_BIND_LEVEL_PARENT (current_bind_level);
  free (current_bind_level);
}

/* Allocate and initialize a parser.  */

static struct pkl_parser *
pkl_parser_init (void)
{
  struct pkl_parser *parser;

  parser = xmalloc (sizeof (struct pkl_parser));
  memset (parser, 0, sizeof (struct pkl_parser));
  
  pkl_tab_lex_init (&(parser->scanner));
  pkl_tab_set_extra (parser, parser->scanner);

  parser->ast = pkl_ast_init ();
  parser->interactive = 0;
  parser->filename = NULL;
  parser->nchars = 0;

  /* Create the global binding level.  */
  parser->global_bind_level = pkl_bind_level_new ();

  return parser;
}

/* Free resources used by a parser, exceptuating the AST.  */

void
pkl_parser_free (struct pkl_parser *parser)
{
  pkl_bind_level l, n;
  
  pkl_tab_lex_destroy (parser->scanner);
  free (parser->filename);

  for (l = parser->current_bind_level; l; l = n)
    {
      n = PKL_BIND_LEVEL_PARENT (l);
      free (l);
    }
  free (parser->global_bind_level);

  free (parser);

  return;
}

/* Read input from the command line, one line at a time, parsing its
   contents as a PKL program.  The parser stops and returns when a PKL
   construct is the last non-blank and non-comment contents of a
   line.

   Return 0 if the parsing was successful, 1 if there as a syntax
   error and 2 if there was a memory exhaustion.  */

int
pkl_parse_cmdline (pkl_ast *ast)
{
  int ret;
  struct pkl_parser *parser;

  parser = pkl_parser_init ();
  parser->interactive = 1;

  pkl_tab_set_in (stdin, parser->scanner);
  ret = pkl_tab_parse (parser);
  *ast = parser->ast;
  pkl_parser_free (parser);

  return ret;
}


/* Read from FD until end of file, parsing its contents as a PKL
   program.  Return 0 if the parsing was successful, 1 if there was a
   syntax error and 2 if there was a memory exhaustion.  */

int
pkl_parse_file (pkl_ast *ast, int what, FILE *fd, const char *fname)
{
  int ret;
  struct pkl_parser *parser;

  parser = pkl_parser_init ();
  parser->filename = xstrdup (fname);
  parser->what = what;

  pkl_tab_set_in (fd, parser->scanner);
  ret = pkl_tab_parse (parser);
  *ast = parser->ast;
  pkl_parser_free (parser);

  return ret;
}

/* Parse the contents of BUFFER as a PKL program.  If END is not NULL,
   set it to the first character after the parsed string.  Return 0 if
   the parsing was successful, 1 if there was a syntax error and 2 if
   there was a memory exhaustion.  */

int
pkl_parse_buffer (pkl_ast *ast, int what, char *buffer, char **end)
{
  YY_BUFFER_STATE yybuffer;
  struct pkl_parser *parser;
  int ret;

  parser = pkl_parser_init ();
  parser->what = what;
  parser->interactive = 1;

  yybuffer = pkl_tab__scan_string(buffer, parser->scanner);

  /* XXX */
  /*  pkl_tab_debug = 1; */
  ret = pkl_tab_parse (parser);
  /* XXX */
  /*  pkl_ast_print (stdout, parser->ast->ast); */

  *ast = parser->ast;
  if (end != NULL)
    *end = buffer + parser->nchars;

  pkl_tab__delete_buffer (yybuffer, parser->scanner);
  pkl_parser_free (parser);

  return ret;
}

