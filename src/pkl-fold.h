/* pkl-fold.h - Constant folding phase for the poke compiler. */

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

/* Perform constant-folding in the given AST and return it.  This pass
   works based on the following assumptions:

   - All operators have legal arguments, i.e. the `promote' pass has
     been executed in the AST.
*/
pkl_ast pkl_fold (pkl_ast ast);

#endif /* PKL_FOLD_H */
