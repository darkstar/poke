/* pkl-trans.h - Transformation phases for the poke compiler.  */

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

#ifndef PKL_TRANS_H
#define PKL_TRANS_H

#include <config.h>
#include "pkl-pass.h"

struct pkl_trans_payload
{
  int errors;
};

typedef struct pkl_trans_payload *pkl_trans_payload;

extern struct pkl_phase pkl_phase_trans1;
extern struct pkl_phase pkl_phase_trans2;
extern struct pkl_phase pkl_phase_trans3;
extern struct pkl_phase pkl_phase_trans4;

#endif /* PKL_TRANS_H */
