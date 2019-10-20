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
   entities, namely variables, types and functions, and statements.

   The PKL compiler works as follows:

   First, a compiler is created and initialized with `pkl_new'.  At
   this point, the internal program is almost empty, but not quite:
   part of the compiler is written in poke itself, and thus it needs
   to bootstrap itself defining some variables, types and functions,
   that compose the run-time environment.

   Then, subsequent calls to `pkl_compile_buffer' and
   `pkl_compile_file (..., PKL_PROGRAM, ...)' expands the
   internally-maintained program, with definitions of variables,
   types, function etc from the user.

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

/* Compile a poke program from the given file FNAME.  Return 1 if the
   compilation was successful, 0 otherwise.  */

int pkl_compile_file (pkl_compiler compiler, const char *fname);

/* Compile a Poke program from a NULL-terminated string BUFFER.
   Return 0 in case of a compilation error, 1 otherwise.  If not NULL,
   END is set to the first character in BUFFER that is not part of the
   compiled entity.  */

int pkl_compile_buffer (pkl_compiler compiler, char *buffer, char **end);

/* Like pkl_compile_buffer but compile a single Poke statement, which
   may generate a value in VAL if it is an "expression statement".  */

int pkl_compile_statement (pkl_compiler compiler, char *buffer, char **end,
                           pvm_val *val);


/* Like pkl_compile_buffer, but compile a Poke expression and return a
   PVM program that evaluates to the expression.  In case of error
   return NULL.  */

pvm_program pkl_compile_expression (pkl_compiler compiler,
                                    char *buffer, char **end,
                                    void **pointers);

/* Return the current compile-time environment in COMPILER.  */

pkl_env pkl_get_env (pkl_compiler compiler);

/* Returns a boolean telling whether the compiler has been
   bootstrapped.  */

int pkl_bootstrapped_p (pkl_compiler compiler);

/* Returns a boolean telling whether the compiler is compiling a
   single xexpression or a statement, respectively.  */

int pkl_compiling_expression_p (pkl_compiler compiler);
int pkl_compiling_statement_p (pkl_compiler compiler);

/* Set/get the error-on-warning flag in/from the compiler.  If this
   flag is set, then warnings are handled like errors.  By default,
   the flag is not set.  */

int pkl_error_on_warning (pkl_compiler compiler);
void pkl_set_error_on_warning (pkl_compiler compiler,
                               int error_on_warning);

/* Diagnostic routines.  */

void pkl_error (pkl_compiler compiler, pkl_ast ast, pkl_ast_loc loc,
                const char *fmt, ...);
void pkl_warning (pkl_compiler compiler, pkl_ast ast,
                  pkl_ast_loc loc, const char *fmt, ...);
void pkl_ice (pkl_ast ast, pkl_ast_loc loc, const char *fmt, ...);

#endif /* ! PKL_H */
