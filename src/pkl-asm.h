/* pkl-asm.h - Macro-assembler for the Poke Virtual Machine.  */

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

#ifndef PKL_ASM_H
#define PKL_ASM_H

#include <config.h>
#include <stdarg.h>
#include <jitter/jitter.h>

#include "pkl-gen.h"
#include "pkl-ast.h"
#include "pvm.h"

/* The macro-assembler provides constants, enumerations, C macros and
   functions to make it easier to program the Poke Virtual Machine.  */

/* Instructions.  */

#define PKL_DEF_INSN(SYM, ARGS) SYM,
enum pkl_asm_insn
{
#include "pkl-insn.def"
};
#undef PKL_DEF_INSN

/* The following struct defines the state of an assembler
   instance.  */

struct pkl_asm
{
  /* PVM program being assembled.  */
  pvm_program program;

  /* Number of instructions assembled in the program.  */
  size_t insns_added;
};

typedef struct pkl_asm *pkl_asm;

/* Create and return a new assembler instance.  PROGRAM is a PVM
   program that may be partially assembled.  */

pkl_asm pkl_asm_new (pvm_program program);

/* Destroy an assembler instance, freeing all used resources.  */

void pkl_asm_free (pkl_asm pasm);

/* Assemble an instruction INSN and append it to the program being
   assembled in PASM.  If the instruction takes any argument, they
   follow after INSN.  */

void pkl_asm_insn (pkl_asm pasm,
                   enum pkl_asm_insn insn,
                   ... /* args */);

/* Conditionals.
 *
 *  pkl_asm_if (asm, EXP);
 *
 *  ... then body ...
 *
 *  pkl_asm_else (asm);
 *
 *  ... else body ...
 *
 *  pkl_asm_endif (asm);
 */

/* Loops.
 *
 * pkl_asm_dotimes (asm, EXP);
 *
 * ... loop body ...
 *
 * pkl_asm_enddotimes (asm);
 */

#endif /* PKL_ASM_H */
