/* pk-peek.c - `peek' command.  */

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
#include <assert.h>
#include <stdio.h> /* For stdout */
#include <gettext.h>
#define _(str) dgettext (PACKAGE, str)

#include "ios.h"
#include "poke.h"
#include "pk-cmd.h"

static int
pk_cmd_peek (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* peek [ADDR]  */

  pvm_program prog;
  ios_off address;
  char c;
  pvm_val val;
  int pvm_ret;
  uint64_t value;

  assert (argc == 1);

  if (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_NULL)
    {
      address = ios_tell (ios_cur ());
    }
  else
    {
      assert (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_EXP);
      prog = PK_CMD_ARG_EXP (argv[0]);

      pvm_ret = pvm_run (poke_vm, prog, &val);
      if (pvm_ret != PVM_EXIT_OK)
        goto rterror;

      if (!PVM_IS_OFF (val) || PVM_VAL_OFF (val) < 0)
        {
          printf (_("Bad ADDRESS.\n"));
          return 0;
        }

      /* Get the offset in bits.  */
      address = (PVM_VAL_INTEGRAL (PVM_VAL_OFF_MAGNITUDE (val))
                 * PVM_VAL_INTEGRAL (PVM_VAL_OFF_UNIT (val)));
    }
  
  /* XXX: endianness, and what not.  */

  switch (ios_read_uint (ios_cur (), address, 0, 8,
                         IOS_ENDIAN_MSB /* irrelevant  */,
                         &value))
    {
    case IOS_OK:
      break;
    case IOS_EIOBJ:
      printf ("EOF\n");
      break;
    default:
      printf ("error reading from IO\n");
      return 0;
    }

  c = (char) value;
  printf ("0x%08jx 0x%x\n", address, c);

  return 1;

 rterror:
  printf (_("run-time error\n"));
  return 0;
}

static int
pk_cmd_help_peek (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* help peek  */

  assert (argc == 0);

  puts (_("The peek command fetches a value from the current IO"));
  puts ("stream.");
  
  return 1;
}

struct pk_cmd peek_cmd =
  {"peek", "?e", "", PK_CMD_F_REQ_IO, NULL, pk_cmd_peek, "peek [ADDRESS]"};

struct pk_cmd help_peek_cmd =
  {"peek", "", "", 0, NULL, pk_cmd_help_peek, "help peek"};
