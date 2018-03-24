/* pkl-trans.c - Transformation phases for the poke compiler.  */

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
#include "pkl-trans.h"

/* This file implements several transformation compiler phases which,
   generally speaking, are not restartable.

   `trans1' is run immediately after parsing.
   `trans2' is run before anal2.

  See the handlers below for detailed information about the specific
  transformations these phases perform.  */



struct pkl_phase pkl_phase_trans1 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_trans_bf_program),
  };

struct pkl_phase pkl_phase_trans2 =
  {
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_trans_bf_program),
  };
