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
#include <jitter/jitter.h>

#include "pkl-gen.h"
#include "pkl-ast.h"
#include "pvm.h"

/* The macro-assembler provides constants, enumerations, C macros and
   functions to make it easier to program the Poke Virtual Machine.  */

typedef struct pkl_asm *pkl_asm; /* This struct is defined in
                                    pkl-asm.c */

/* Instructions.  */

enum pkl_asm_insn
{
 /* Real PVM instructions.  */
 PKL_ASM_INSN_ADDI,
 PKL_ASM_INSN_ADDIU,
 PKL_ASM_INSN_ADDL,
 PKL_ASM_INSN_ADDLU,
 /* Macro-instructions follow.  */
 PKL_ASM_INSN_ADD_OFFSET,
 PKL_ASM_INSN_SUB_OFFSET,
 PKL_ASM_INSN_MUL_OFFSET,
 PKL_ASM_INSN_DIV_OFFSET,
};

/* The following function inserts the given instruction.  */

void pkl_asm_insn (pkl_asm asm, enum pkl_asm insn);

/* Conditionals.
 *
 *  pkl_asm_if (asm, EXP);
 *
 *  ...
 *
 *  pkl_asm_else (asm);
 *
 *  ...
 *
 *  pkl_asm_endif (asm);
 */

#endif /* PKL_ASM_H */
