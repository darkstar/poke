/* pkl-opt.h - Optimizer for PKL.  */

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

#include "pkl-opt.h"

/* Constant folding.  */

static pkl_ast
pkl_opt_constfold (pkl_ast ast)
{
  /* integer literals can be folded.
     enumerator constants can be folded.
     0 * n can't be folded, because of possible IO side effects.
     1 * n can be folded, because it keeps the IO access.  */
  
  if (ast == NULL)
    return ast;

  if (PKL_AST_CODE (ast->ast) == PKL_AST_EXP)
    {
      /* If all the operands are leafs and literals.  */

    }
  
  return ast;
}

/* Run optimizations on AST and return it.  */

pkl_ast
pkl_opt (pkl_ast ast)
{
  ast = pkl_opt_constfold (ast);
  return ast;
}
