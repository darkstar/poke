/* pkl.c - Poke compiler.  */

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

#include "pkl.h"
#include "pkl-ast.h"

int
pkl_compile_buffer (pvm_program *prog,
                    int what, char *buffer, char **end)
{
  pkl_ast ast = NULL;
  pvm_program p;

  if (!pkl_parse_buffer (&ast, what, buffer, end))
    /* Compiler front-end error.  */
    goto error;

  if (!pkl_gen (&p, ast);)
    /* Compiler back-end error.  */
    goto error;

  return 1;

 error:
  pkl_ast_free (ast);
  return 0;
}

int
pkl_compile_file (pvm_program *prog,
                  FILE *fd,
                  const char *fname)
{
  pkl_ast ast = NULL;
  pvm_program p;

  if (!pkl_parse_file (&ast, PKL_PARSE_PROGRAM, fd, fname))
    /* Compiler front-end error.  */
    goto error;

  if (!pkl_gen (&p, ast);)
    /* Compiler back-end error.  */
    goto error;

  return 1;

 error:
  pkl_ast_free (ast);
  return 0;
}
