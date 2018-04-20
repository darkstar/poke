/* pkl-gen.h - Code generation pass for the poke compiler.  */

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

#ifndef PKL_GEN_H
#define PKL_GEN_H

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "pkl-ast.h"
#include "pkl-pass.h"
#include "pkl-asm.h"
#include "pkl-asm.h"
#include "pvm.h"

/* It would be very unlikely to have more than 24 function
   declarations nested in a poke program... if it ever happens, we
   will just increase this, thats a promise :P */
#define PKL_GEN_MAX_PASM 25

struct pkl_gen_payload
{
  pkl_asm pasm[PKL_GEN_MAX_PASM];
  int cur_pasm;
  pvm_program program;
};

typedef struct pkl_gen_payload *pkl_gen_payload;

extern struct pkl_phase pkl_phase_gen;

static inline void
pkl_gen_init_payload (pkl_gen_payload payload)
{
  memset (payload, 0, sizeof (struct pkl_gen_payload));
}


#endif /* !PKL_GEN_H  */
