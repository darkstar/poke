/* pk-stmt.c - do (execute sentence) command */

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
#include <stdio.h>
#include <string.h>
#include <gettext.h>
#define _(str) dgettext (PACKAGE, str)

#include "pkl.h"
#include "pvm.h"
#include "poke.h"
#include "pk-cmd.h"

static int
pk_cmd_stmt (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  int pvm_ret;
  pvm_program prog;
  pvm_val val;

  assert (argc == 1);
  assert (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_STMT);

  /* Execute the program with the sentence and check for execution
     errors.  The returned value is ignored.  */

  prog = PK_CMD_ARG_STMT (argv[0]);
  pvm_ret = pvm_run (poke_vm, prog, &val);
  if (pvm_ret != PVM_EXIT_OK)
    {
      printf (_("run-time error\n"));
      return 0;
    }

  return 1;
}

struct pk_cmd stmt_cmd =
  {"do", "T", 0, 0, NULL, pk_cmd_stmt,
   "do STATEMENT"};
