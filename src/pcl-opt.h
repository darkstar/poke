/* pcl-opt.c - Optimizer for PCL.  */

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

#ifndef PCL_OPT_H
#define PCL_OPT_H

#include <config.h>
#include "pcl-ast.h"

/* `pcl_opt' runs several optimizations to the passed AST and returns
   an optimized AST.  The optimized AST implements exactly the same
   semantics than the original, but (hopefully) it does so more
   efficiently.  */

pcl_ast pcl_opt (pcl_ast ast);

#endif /* !PCL_OPT_H */
