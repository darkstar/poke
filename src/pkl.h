/* pkl.h - Poke compiler.  */

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

#ifndef PKL_H
#define PKL_H

#include <config.h>
#include <stdio.h>
#include <stdarg.h>

#include "pkl-ast.h"
#include "pkl-env.h"
#include "pvm.h"

/* This is the main header file for the Poke Compiler.  The Poke
   Compiler is an "incremental compiler", i.e. it is designed to
   compile poke programs incrementally.

   A poke program is a sequence of declarations of several classes of
   entities, namely variables, types and functions.  Unlike in many
   other programming languages, there is not a main function or
   procedure where execution starts by default.  Poke programs, as
   such, are not executable.

   Instead, the poke compiler works as follows:

   First, a compiler is created and initialized with `pkl_new'.  At
   this point, the internal program is almost empty, but not quite:
   part of the compiler is written in poke itself, and thus it needs
   to bootstrap itself defining some variables, types and functions,
   that compose the run-time environment.

   Then, subsequent calls to `pkl_compile_buffer' and
   `pkl_compile_file (..., PKL_PROGRAM, ...)' expands the
   internally-maintained program, with definitions of variables,
   types, function etc from the user.  They return a PVM program that
   should be executed to complete the declaration.

   At any point, the user can request to compile a poke expression
   with `pkl_compile_expression'.  This returns a PVM program that,
   can be executed in a virtual machine.  It is up to the user to free
   the returned PVM program when it is not useful anymore.

   `pkl_compile_buffer', `pkl_compile_file' and
   `pkl_compile_expression' can be called any number of times, in any
   possible combination.

   Finally, `pkl_free' should be invoked when the compiler is no
   longer needed, in order to do some finalization tasks and free
   resources.  */

typedef struct pkl_compiler *pkl_compiler; /* This data structure is
                                              defined in pkl.c */

/* Initialization and finalization functions.  */

pkl_compiler pkl_new (void);
void pkl_free (pkl_compiler compiler);

/* Compile a poke program from the given file FNAME.  Return NULL in
   case of a compilation error.  */

pvm_program pkl_compile_file (pkl_compiler compiler, const char *fname);

/* Compile a poke declaration from a NULL-terminated string BUFFER.
   Return NULL in case of a compilation error.  If not NULL, END is
   set to the first character in BUFFER that is not part of the
   declaration.  */

pvm_program pkl_compile_declaration (pkl_compiler compiler,
                                     char *buffer, char **end);

/* Compile a poke expression from a NULL-terminated string BUFFER.
   Return NULL in case of a compilation error.  If not NULL, END is
   set to the first character in BUFFER that is not part of the
   expression.  */

pvm_program pkl_compile_expression (pkl_compiler compiler,
                                    char *buffer, char **end);

/* Compile a poke statement from a NULL-terminated string BUFFER.
   Return NULL in case of a compilation error.  If not NULL, END is
   set to the first character in BUFFER that is not part of the
   expression.  */

pvm_program pkl_compile_statement (pkl_compiler compiler,
                                   char *buffer, char **end);

/* Return the current compile-time environment in COMPILER.  */

pkl_env pkl_get_env (pkl_compiler compiler);

/* Returns a boolean telling whether the compiler has been
   bootstrapped.  */

int pkl_bootstrapped_p (pkl_compiler compiler);

/* Returns a boolean telling whether the compiler is compiling an
   expression.  */

int pkl_compiling_expression_p (pkl_compiler compiler);

/* Diagnostic routines.  */

void pkl_error (pkl_ast ast, pkl_ast_loc loc, const char *fmt, ...);
void pkl_warning (pkl_ast_loc loc, const char *fmt, ...);
void pkl_ice (pkl_ast ast, pkl_ast_loc loc, const char *fmt, ...);

#endif /* ! PKL_H */
