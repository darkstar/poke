/* pk-peek.c - `print' command.  */

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
#include <assert.h>
#include <stdio.h> /* For stdout */

#include "pk-cmd.h"

static int
pk_cmd_print (int argc, struct pk_cmd_arg argv[])
{
  /* print EXP */

  pvm_program prog;

  assert (argc == 1);
  assert (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_EXP);
  prog = PK_CMD_ARG_EXP (argv[0]);

  /* Run the program in the pvm.  */
  jitter_disassemble_program (prog, true, JITTER_CROSS_OBJDUMP, NULL);
  pvm_execute (prog);
  
  return 1;
}

struct pk_cmd print_cmd =
  {"print", "e", 0, NULL, pk_cmd_print, "print EXP"};
