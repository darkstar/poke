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
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "poke.h"
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
#define KBOLD (poke_interactive_p ? "\033[1m" : "")
#define KNONE (poke_interactive_p ? "\033[0m" : "")

/* Convenience macros and functions for parsing.  */

static inline char *
skip_blanks (char *p)
{
  while (isblank (*p))
    p++;
  return p;
}

static inline int
pk_atoi (char **p, long int *number)
{
  long int li;
  char *end;

  errno = 0;
  li = strtol (*p, &end, 0);
  if (errno != 0 && li == 0
      || end == *p)
    return 0;

  *number = li;
  *p = end;
  return 1;
}

/* Commands follow.  */

static int
pk_cmd_dump (char *str)
{
  int c = 0;
  char string[18], *p;
  string[0] = ' ';
  string[17] = '\0';
  pk_io_off cur, address, count, top;

  /* Parse arguments:

     dump [ADDR] [,COUNT]

  */

  p = skip_blanks (str);

  /* Get the address.  */
  
  if (*p == '\0')
    address = pk_io_tell ();
  else if (*p == ',')
    ;
  else
    {
      if (!pk_atoi (&p, &address))
        goto usage;
    }

  /* Get the count.  */

  p = skip_blanks (p);
  
  if (*p == '\0')
    count = 8;
  else if (*p == ',')
    {
      p++;  /* Skip ,  */
      if (!pk_atoi (&p, &count))
        goto usage;
    }
  else
    goto usage;

  p = skip_blanks (p);

  if (*p != '\0')
    goto usage;

  if (count < 0)
    goto usage;
  
  top = address + 0x10 * count;
  
  /* Dump the requested address.  */
  
  pk_io_seek (address, PK_SEEK_SET);

  if (poke_interactive_p)
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

 usage:
  puts ("usage: dump [ADDRESS] [,COUNT]");
  return 0;
}

#define MAX_CMD_NAME 18

int
pk_cmd_exec (char *str)
{
  int ret;
  size_t i;
  char cmd_name[MAX_CMD_NAME], *p;

  /* Skip blanks, and return if the command is composed by only blank
     characters.  */

  p = skip_blanks (str);
  if (*p == '\0')
    return 1;

  /* Get the command name.  */
  i = 0;
  while (isalnum (*p) || *p == '_')
    cmd_name[i++] = *(p++);
  cmd_name[i] = '\0';

  /* Dispatch to the appropriate command handler.  */
  ret = 0;
  if (cmd_name[0] == '\0')
    puts ("invalid command");
  if (strcmp (cmd_name, "dump") == 0)
    ret = pk_cmd_dump (p);
  else
    printf ("%s: command not found\n", cmd_name);

  return ret;
}
