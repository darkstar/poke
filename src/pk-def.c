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
#include <stdio.h>
#include <gettext.h>
#define _(str) dgettext (PACKAGE, str)

#include "pkl.h"
#include "pvm.h"
#include "poke.h"
#include "pk-cmd.h"

static int
pk_cmd_def (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  int pvm_ret;
  pvm_program prog;
  pvm_val val;
  
  assert (argc == 1);
  assert (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_DEF);

  /* Execute the program with the definition in the pvm and check for
     execution errors, but ignore the returned value.  */
  
  prog = PK_CMD_ARG_DEF (argv[0]);
  /* XXX */
  /* pvm_print_program (stdout, prog); */
  pvm_ret = pvm_run (poke_pvm, prog, &val);
  if (pvm_ret != PVM_EXIT_OK)
    {
      printf (_("run-time error: %s\n"), pvm_error (pvm_ret));
      return 0;
    }

  return 1;
}

static void
print_var_decl (pkl_ast_node decl, void *data)
{
  pkl_ast_node decl_name = PKL_AST_DECL_NAME (decl);
  pkl_ast_loc loc = PKL_AST_LOC (decl);
  /* XXX pvm_val = pvm_env_lookup (..); */

  printf ("%s\t\t%s\t\t\tXXX:%d\n",
          PKL_AST_IDENTIFIER_POINTER (decl_name),
          "XXX", loc.first_line);
}

static int
pk_cmd_info_var (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  printf (_("Name\t\tValue\t\t\tDeclared at\n"));
  pkl_map_decls (poke_compiler, PKL_AST_DECL_KIND_VAR,
                 print_var_decl, NULL);
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

struct pk_cmd info_var_cmd =
  {"variable", "", "", 0, NULL, pk_cmd_info_var,
   "info variable"};
