/* pkl.h - Poke compiler.  */

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

#ifndef PKL_H
#define PKL_H

#include <config.h>
#include <stdio.h>
#include <stdarg.h>

#include "pkl-ast.h"
#include "pvm.h"

/* Compile a poke expression, or a program, from a NULL-terminated
   string BUFFER.  Return 0 in case of a compilation error.  Return 1
   otherwise.  If not NULL, END is set to the first character in
   BUFFER that is not part of the program/expression.  */

#define PKL_PROGRAM 0
#define PKL_EXPRESSION 1

int pkl_compile_buffer (pvm_program *prog, int what, char *buffer,
                        char **end);

/* Compile a poke program from a file.  Return 0 in case of a
   compilation error.  Return 1 otherwise.  */

int pkl_compile_file (pvm_program *prog, FILE *fd, const char *fname);

/* Diagnostic routines.  */

void pkl_error (pkl_ast ast, pkl_ast_loc loc, const char *fmt, ...);
void pkl_warning (pkl_ast_loc loc, const char *fmt, ...);
void pkl_ice (pkl_ast ast, pkl_ast_loc loc, const char *fmt, ...);

#endif /* ! PKL_H */
