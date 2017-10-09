/* pkl-gen.h - Code generator for Poke.  */

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

#ifndef PKL_GEN_H
#define PKL_GEN_H

#include <config.h>
#include <jitter/jitter.h>
#include "pkl-ast.h"
#include "pvm.h"

/* Lower an AST to a PVM program and return it.  Return 0 if a
   compilation error occurs.  Return 1 otherwise.  */

int pkl_gen (pvm_program *prog, pkl_ast ast);

#endif /* !PKL_GEN_H  */
