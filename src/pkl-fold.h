/* pkl-fold.h - Constant folding for the poke compiler. */

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

#ifndef PKL_FOLD_H
#define PKL_FOLD_H

#include <config.h>

#include "pkl-ast.h"
#include "pkl-parser.h"

/* Perform constant-folding in the given AST and return it.  Note that
   when the function finds an ast node that is not an expression or an
   expression primary, or an unkown operator, it stops the process.
   Also, it assumes AST is well formed, i.e. these are valid operands
   for the operators.  */
pkl_ast_node pkl_fold (struct pkl_parser *parser, pkl_ast_node ast);

#endif /* PKL_FOLD_H */
