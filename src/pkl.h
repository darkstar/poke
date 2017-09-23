/* pkl.h - Poke language support  */

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

#ifndef PKL_H
#define PKL_H

#include <config.h>

#if 0
struct pvm_struct
{
  pvm_prog type; /* Program for the type.  */
  pio_layout layout;  /* Recalculated running the type above.  */
  pio_addr addr;
};

enum pvm_var_type
{
  PVM_VT_NUMBER,
  PVM_VT_ARRAY,
  PVM_VT_STRUCT
};

struct pvm_var
{
  enum pvm_var_type type;
  
  union
  {
    pvm_int integer;
    pvm_array array;
    pvm_struct strct;
    pio_addr addr;
  } v;
};

struct pvm_scope
{
  /* Variables defined in this scope.  */
  pvm_var *vars;
};

#endif /* 0 */

#endif /* ! PKL_H */
