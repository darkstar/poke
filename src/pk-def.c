/* pk-def.c - `def*' commands.  */

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

#include "pk-cmd.h"

static int
pk_cmd_def (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  assert (argc == 1);
  assert (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_DEF);

  /* Nothing to do.  */
  return 1;
}

struct pk_cmd deftype_cmd =
  {"deftype", "d", 0, 0, NULL, pk_cmd_def,
   "deftype NAME = TYPE"};

struct pk_cmd defvar_cmd =
  {"defvar", "d", 0, 0, NULL, pk_cmd_def,
   "defvar NAME = EXP"};

struct pk_cmd defun_cmd =
  {"defun", "d", 0, 0, NULL, pk_cmd_def,
   "defun NAME = (ARGS) [: TYPE] { BODY }"};

