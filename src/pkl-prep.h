/* pkl-prep.h - Environment creation phase for the poke compiler.  */

/* Copyright (C) 2018 Jose E. Marchesi  */

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

#ifndef PKL_PREP_H
#define PKL_PREP_H

#include <config.h>
#include "pkl-pass.h"
#include "pkl-env.h"

struct pkl_prep_payload
{
  pkl_env env;
  int errors;
};

typedef struct pkl_prep_payload *pkl_prep_payload;

extern struct pkl_phase pkl_phase_prep;

#endif /* !PKL_PREP_H */
