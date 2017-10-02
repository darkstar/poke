/* pk-cmd.c - Poke commands.  */

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
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "pk-io.h"
#include "pk-cmd.h"

/* Escape sequences for changing text attributes on the terminal.  */

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define KBOLD "\033[1m"
#define KNONE "\033[0m"

int
pk_cmd_dump (const char *str)
{
  int c = 0;
  char string[18];
  string[0] = ' ';
  string[17] = '\0';
  pk_io_off cur, address, top;

  /* Parse arguments.  */
  if (str[0] == '\0')
    address = pk_io_tell ();
  else
    {
      long int li;
      char *end;
      int i = 0;
      while (str[i] == ' ' || str[i] == '\t')
        i++;
      
      li = strtol (str + i, &end, 0);
      if (str[i] != '\0' && (*end == '\0'))
        {
          address = li;
        }
      else
        return 0;
    }

  top = address + 0x10 * 8;
  
  /* Dump the requested address.  */
  
  pk_io_seek (address, PK_SEEK_SET);
  
  printf ("%s87654321  0011 2233 4455 6677 8899 aabb ccdd eeff  0123456789ABCDEF%s\n",
          KBOLD, KNONE);
  for (; 0 <= c && address < top; address += 0x10)
    {
      int i;
      for (i = 0; i < 16; i++)
        {
          if (0 <= c)
            c = pk_io_getc ();
          if (c < 0)
            {
              if (!i)
                break;
              
              fputs ("  ", stdout);
              string[i + 1] = '\0';
            }
          else
            {
              if (!i)
                printf ("%s%08jx: %s", KBOLD, address, KNONE);
              
              string[i + 1]
                = (c < 0x20 || (0x7F <= c && (c < 0xa0))
                   ? '.'
                   : isprint (c) ? c : '?');
              
              printf ("%02x", c + 0u);
            }

          if (i & 0x1)
            putchar (' ');
        }
      
      if (i)
        puts (string);
    }

  return 1;
}
