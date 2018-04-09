/* pkl-asm.h - Macro-assembler for the poke compiler.  */

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

/* The user of the assembler refers to specific instructions using the
   PKL_INSN_* symbols defined below.  See the file pkl-insn.def for
   detailed information on the supported instructions.  */

enum pkl_asm_insn
{
#define PKL_DEF_INSN(SYM, ARGS, NAME) SYM,
#  include "pkl-insn.def"
#undef PKL_DEF_INSN
};

/* Opaque data structure for an assembler instance.  The struct is
   defined in pkl-asm.c.  */

typedef struct pkl_asm *pkl_asm;

/* Create a new instance of an assembler.  This initializes a new
   program.  */

pkl_asm pkl_asm_new (void);

/* Finish the assembly of the current program and return it.  This
   function frees all resources used by the assembler instance, and
   `pkl_asm_new' should be called again in order to assemble another
   program.  */

pvm_program pkl_asm_finish (pkl_asm pasm);

/* Assemble an instruction INSN and append it to the program being
   assembled in PASM.  If the instruction takes any argument, they
   follow after INSN.  */

void pkl_asm_insn (pkl_asm pasm, enum pkl_asm_insn insn, ...);

/* Conditionals.
 *
 *  pkl_asm_if (pasm, EXP);
 *
 *  ... then body ...
 *
 *  pkl_asm_else (pasm);
 *
 *  ... else body ...
 *
 *  pkl_asm_endif (pasm);
 */

/* For loop:
 *
 * pkl_asm_dotimes (pasm, INTEGER)
 *
 * pkl_asm_loop (pasm);
 *
 * ... loop body ...
 *
 * pkl_asm_end_loop (pasm);
 *
 * While loop:
 *
 * pkl_asm_while (pasm);
 * 
 *   ... condition ...
 *
 * pkl_asm_loop (pasm);
 *
 *   ... loop body ...
 * 
 * pkl_asm_end_loop (pasm);
 */

/* Assembler directives:
 *
 * pkl_asm_note (pasm, STR);
 * pkl_asm_loc (pasm, LOC);
 *
 * XXX: how to use pretty-printers in jitter in order to print
 * directives like:
 *
 * .note "foobar"      Does nothing.
 * .loc lb,le,cb,ce    Updates the current location in the PVM.
 *                     Used for debugging and error reporting.
 *
 * instead of using instructions like:
 *
 *       note "foobar"
 *       loc lb,le,cb,ce
 */

#endif /* PKL_ASM_H */
