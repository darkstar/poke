/* pk-peek.c - `print' command.  */

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
#include <assert.h>
#include <gettext.h>
#define _(str) dgettext (PACKAGE, str)

#include "poke.h"
#include "pk-cmd.h"
#include "pk-term.h"

#define PK_PRINT_UFLAGS "xbom"
#define PK_PRINT_F_HEX 0x1
#define PK_PRINT_F_BIN 0x2
#define PK_PRINT_F_OCT 0x4
#define PK_PRINT_F_MAP 0x8

static int
pk_cmd_print (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* print EXP */

  pvm_routine routine;
  pvm_val val;
  int pvm_ret;
  int base;
  int pflags = 0;

  assert (argc == 1);
  assert (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_EXP);

  routine = PK_CMD_ARG_EXP (argv[0]);

  /* Numeration base to use while printing values.  Default is
     decimal.  Command flags can be used to change this.  */
  base = 10;
  if (!!(uflags & PK_PRINT_F_HEX)
      + !!(uflags & PK_PRINT_F_BIN)
      + !!(uflags & PK_PRINT_F_OCT) > 1)
    {
      pk_printf (_("print: only one of `x', `b' or `o' may be specified.\n"));
      return 0;
    }

  if (uflags & PK_PRINT_F_HEX)
    base = 16;
  else if (uflags & PK_PRINT_F_BIN)
    base = 2;
  else if (uflags & PK_PRINT_F_OCT)
    base = 8;

  if (uflags & PK_PRINT_F_MAP)
    pflags |= PVM_PRINT_F_MAPS;

  pvm_ret = pvm_run (poke_vm, routine, &val);
  if (pvm_ret != PVM_EXIT_OK)
    goto rterror;

  pvm_print_val (val, base, pflags);
  pk_puts ("\n");
  return 1;

 rterror:
  return 0;
}

struct pk_cmd print_cmd =
  {"print[/xobm]", "e", PK_PRINT_UFLAGS, 0, NULL, pk_cmd_print,
   "print EXP.\n\
Flags:\n\
  x (print numbers in hexadecimal)\n\
  o (print numbers in octal)\n\
  b (print numbers in binary)"};
