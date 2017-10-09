/* pk-vm.c - PVM related commands.  */

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

#include "poke.h"
#include "pk-cmd.h"
#include "pvm.h"

static int
pk_cmd_disas_exp (int argc, struct pk_cmd_arg argv[])
{
  printf ("HOLA\n");
  return 1;
}

extern struct pk_cmd null_cmd; /* pk-cmd.c  */

struct pk_cmd disas_exp_cmd =
  {"expression", "e", 0, NULL, pk_cmd_disas_exp, "disassemble expression EXP"};

struct pk_cmd *disas_cmds[] =
  {
    &disas_exp_cmd,
    &null_cmd
  };

struct pk_trie *disas_trie;

struct pk_cmd disas_cmd =
  {"disassemble", "", 0, &disas_trie, NULL, "disassemble (expression)"};

