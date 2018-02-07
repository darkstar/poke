/* pk-peek.c - `print' command.  */

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

#include "pk-cmd.h"

#define PK_PRINT_UFLAGS "xbo"
#define PK_PRINT_F_HEX 0x1
#define PK_PRINT_F_BIN 0x2
#define PK_PRINT_F_OCT 0x4

static int
pk_cmd_print (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* print EXP */

  pvm_program prog;
  pvm_val val;
  int pvm_ret;
  int base;

  assert (argc == 1);
  assert (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_EXP);

  prog = PK_CMD_ARG_EXP (argv[0]);

  /* Numeration base to use while printing values.  Default is
     decimal.  Command flags can be used to change this.  */
  base = 10;
  if (!!(uflags & PK_PRINT_F_HEX)
      + !!(uflags & PK_PRINT_F_BIN)
      + !!(uflags & PK_PRINT_F_OCT) > 1)
    {
      printf (_("print: only one of `x', `b' or `o' may be specified.\n"));
      return 0;
    }

  if (uflags & PK_PRINT_F_HEX)
    base = 16;
  else if (uflags & PK_PRINT_F_BIN)
    base = 2;
  else if (uflags & PK_PRINT_F_OCT)
    base = 8;
  
  /* jitter_disassemble_program (prog, true, JITTER_CROSS_OBJDUMP, NULL); */
  pvm_ret = pvm_run (prog, &val);
  if (pvm_ret != PVM_EXIT_OK)
    goto rterror;
  
  pvm_print_val (stdout, val, base);
  printf ("\n");
  return 1;

 rterror:
  printf (_("run-time error: %s\n"), pvm_error (pvm_ret));
  return 0;
}

struct pk_cmd print_cmd =
  {"print", "e", PK_PRINT_UFLAGS, 0, NULL, pk_cmd_print,
   "print EXP\n\n\
Flags: x (print numbers in hexadecimal)"};
