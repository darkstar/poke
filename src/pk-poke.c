/* pk-poke.c - `poke' command.  */

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

#include "pk-cmd.h"
#include "pk-io.h"

static int
pk_cmd_poke (int argc, struct pk_cmd_arg argv[])
{
  /* poke ADDR, VAL */

  pk_io_off address;
  int value;

  if (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_NULL)
    address = pk_io_tell (pk_io_cur ());
  else if (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_EXP)
    {
      return 1;
    }
  else
    address = PK_CMD_ARG_ADDR (argv[0]);
  
  if (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_NULL)
    value = 0;
  else if (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_EXP)
    {
      return 1;
    }
  else
    value = PK_CMD_ARG_INT (argv[1]);

  if (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_EXP
      || PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_EXP)
    return 1;

  /* XXX: endianness, and what not.   */
  pk_io_seek (pk_io_cur (), address, PK_SEEK_SET);
  if (pk_io_putc ((int) value) == PK_EOF)
    {
      printf ("Error writing byte 0x%x to 0x%08jx\n",
              value, address);
    }
  else
    printf ("0x%08jx <- 0x%x\n", address, value);

  return 1;
}

struct pk_cmd poke_cmd =
  {"poke", "ea,?ei", PK_CMD_F_REQ_IO | PK_CMD_F_REQ_W, NULL,
   pk_cmd_poke, "poke ADDRESS [,VALUE]"};
