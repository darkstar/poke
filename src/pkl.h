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
struct pvm_mem
{
  pvm_prog type; /* Program for the type.  */
  pvm_layout layout;  /* Recalculated running the type above.  */
  pvm_addr addr;
};

struct pvm_var
{
  union
  {
    pvm_int integer;
    pvm_mem mem;
  };
};
#endif /* 0 */

#endif /* ! PKL_H */
