/* pk-dump.c - `dump' command.  */

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

#include <config.h>
#include <ctype.h>
#include <assert.h>
#include <gettext.h>
#define _(str) dgettext (PACKAGE, str)

#include "poke.h"
#include "pk-term.h"
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

static int
pk_cmd_dump (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* dump [ADDR] [,COUNT]  */

  int c = 0;
  char string[18];
  string[0] = ' ';
  string[17] = '\0';
  ios_off address, count, top;
  static ios_off last_address;

  assert (argc == 2);

  if (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_NULL)
    address = last_address;
  else
    address = PK_CMD_ARG_ADDR (argv[0]);

  if (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_NULL)
    count = 8;
  else
    count = PK_CMD_ARG_INT (argv[1]);

  top = address + 0x10 * count;

  /* Dump the requested address.  */

  last_address = address;

  if (poke_interactive_p)
    printf ("%s87654321  0011 2233 4455 6677 8899 aabb ccdd eeff  0123456789ABCDEF%s\n",
            KBOLD, KNONE);
  
  for (; 0 <= c && address < top; address += 0x10)
    {
      int i;
      for (i = 0; i < 16; i++)
        {
          if (0 <= c)
            {
              uint64_t value;
              int ret;

              ret = ios_read_uint (ios_cur (), (address + i) * 8, 0, 8,
                                   IOS_ENDIAN_MSB /* irrelevant */,
                                   &value);
              if (ret == IOS_OK)
                ;
              else if (ret == IOS_EIOFF || ret == IOS_EIOBJ)
                {
                  fputc ('\n', stdout);
                  goto done;
                }
              else
                {
                  printf ("error reading from IO\n");
                  return 0;
                }

              c = (char) value;
            }
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
              
              printf (GREEN "%02x" NOATTR, c + 0u);
            }

          if (i & 0x1)
            putchar (' ');
        }
      
      if (i)
        printf ("%s%s%s\n", KYEL, string, KNONE);
    }
 done:
  
  return 1;
}

struct pk_cmd dump_cmd =
  {"dump", "?a,?n", "", PK_CMD_F_REQ_IO, NULL, pk_cmd_dump,
   "dump [ADDRESS] [,COUNT]"};
