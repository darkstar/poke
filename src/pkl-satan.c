/* pkl-satan.c - Example phase for the poke compiler.  */

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

#include "pkl-ast.h"
#include "pkl-pass.h"

/* This is Satan's compiler phase: it turns every integral constant in
   the program into 666, and it also overflows 8-bit constants!  Also,
   it triggers a compilation error if the anti-demonic constant 999 is
   found in the program.  */

PKL_PHASE_HANDLER (pkl_satanize_integer)
{
  if (PKL_AST_INTEGER_VALUE (PKL_PASS_AST) == 999)
    {
      printf ("error: Satan doesn't like 999\n");
      PKL_PASS_ERROR;
    }

  PKL_AST_INTEGER_VALUE (PKL_PASS_AST) = 666;
  return PKL_PASS_AST;
}

struct pkl_phase satanize =
  { .code_df_handlers[PKL_AST_INTEGER] = pkl_satanize_integer };
