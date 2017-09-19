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

#include "pcl-ast.h"
#include "pcl-parser.h"
#include "pcl-tab.h"
#include "pcl-lex.h"


/* Read from FD until end of file, parsing its contents as a PCL
   program.  Return 0 if the parsing was successful, 1 if there was a
   syntax error and 2 if there was a memory exhaustion.  */

int
pcl_parse_file (FILE *fd)
{
  int ret;
  struct pcl_parser *pcl_parser;

  /* Initialize the PCL parser.  */
  pcl_parser = xmalloc (sizeof (struct pcl_parser));
  
  pcl_tab_lex_init (&(pcl_parser->scanner));
  pcl_tab_set_extra (pcl_parser, pcl_parser->scanner);

  pcl_tab_set_in (fd, pcl_parser->scanner);

  ret = pcl_tab_parse (pcl_parser);

  /* Free resources.  */
  /* XXX: free pcl_parser->ast.  */
  pcl_tab_lex_destroy (pcl_parser->scanner);
  free (pcl_parser);

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
