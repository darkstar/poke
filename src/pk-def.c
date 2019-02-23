/* pk-def.c - commands related to definitions.  */

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

static void
print_var_decl (pkl_ast_node decl, void *data)
{
  pkl_ast_node decl_name = PKL_AST_DECL_NAME (decl);
  pkl_ast_loc loc = PKL_AST_LOC (decl);
  char *source =  PKL_AST_DECL_SOURCE (decl);
  int back, over;
  pvm_val val;

  pkl_env compiler_env = pkl_get_env (poke_compiler);
  pvm_env runtime_env = pvm_get_env (poke_vm);

  /* XXX this is not needed.  */
  if (PKL_AST_DECL_KIND (decl) != PKL_AST_DECL_KIND_VAR)
    return;

  assert (pkl_env_lookup (compiler_env,
                          PKL_AST_IDENTIFIER_POINTER (decl_name),
                          &back, &over) != NULL);

  val = pvm_env_lookup (runtime_env, back, over);
  assert (val != PVM_NULL);
                              
  /* Print the name and the current value of the variable.  */
  fputs (PKL_AST_IDENTIFIER_POINTER (decl_name), stdout);
  fputs ("\t\t", stdout);
  /* XXX: support different bases with a /[xbo] cmd flag.  */
  pvm_print_val (stdout, val, 10, 0);
  fputs ("\t\t", stdout);

  /* Print information about the site where the variable was
     declared.  */
  if (source)
    printf ("%s:%d\n", basename (source), loc.first_line);
  else
    fputs ("<stdin>\n", stdout);
}

static void
print_fun_decl (pkl_ast_node decl, void *data)
{
  pkl_ast_node decl_name = PKL_AST_DECL_NAME (decl);
  pkl_ast_node func = PKL_AST_DECL_INITIAL (decl);
  pkl_ast_loc loc = PKL_AST_LOC (decl);
  char *source =  PKL_AST_DECL_SOURCE (decl);
  int back, over;

  pkl_env compiler_env = pkl_get_env (poke_compiler);

  /* Skip mappers, i.e. function declarations whose initials are
     actually struct types, and not function literals.  */

  if (PKL_AST_CODE (func) != PKL_AST_FUNC)
    return;
    
  assert (pkl_env_lookup (compiler_env,
                          PKL_AST_IDENTIFIER_POINTER (decl_name),
                          &back, &over) != NULL);

  /* Print the name and the type of the function.  */
  fputs (PKL_AST_IDENTIFIER_POINTER (decl_name), stdout);
  fputs ("  ", stdout);
  pkl_print_type (stdout, PKL_AST_TYPE (func), 1);
  fputs ("  ", stdout);

  /* Print information about the site where the function was
     declared.  */
  if (source)
    printf ("%s:%d\n", basename (source), loc.first_line);
  else
    fputs ("<stdin>\n", stdout);
}

static int
pk_cmd_info_var (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  printf (_("Name\t\tValue\t\t\tDeclared at\n"));
  pkl_env_map_decls (pkl_get_env (poke_compiler), PKL_AST_DECL_KIND_VAR,
                     print_var_decl, NULL);
  return 1;
}

static int
pk_cmd_info_fun (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  pkl_env_map_decls (pkl_get_env (poke_compiler), PKL_AST_DECL_KIND_FUNC,
                     print_fun_decl, NULL);

  return 1;
}

struct pk_cmd info_var_cmd =
  {"variable", "", "", 0, NULL, pk_cmd_info_var,
   "info variable"};

struct pk_cmd info_fun_cmd =
  {"function", "", "", 0, NULL, pk_cmd_info_fun,
   "info funtion"};
