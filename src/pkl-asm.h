/* pkl-asm.h - Macro-assembler for the poke compiler.  */

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

#ifndef PKL_ASM_H
#define PKL_ASM_H

#include <config.h>
#include <stdarg.h>
#include <jitter/jitter.h>

#include "pkl.h" /* For pkl_compiler */
#include "pkl-ast.h"
#include "ios.h" /* For IOS_NENC_* and IOS_ENDIAN_* */
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

/* The following values are intended to be used as arguments to
   PKL_INSN_PEEK instructions.  */

#define PKL_ASM_ENDIAN_LSB IOS_ENDIAN_LSB
#define PKL_ASM_ENDIAN_MSB IOS_ENDIAN_MSB
#define PKL_ASM_NENC_1 IOS_NENC_1
#define PKL_ASM_NENC_2 IOS_NENC_2

/* Enumeration with machine registers. */

enum pkl_asm_reg
{
 PKL_REG_FOO,
 /* XXX: fill me.  */
};

/* Opaque data structure for an assembler instance.  The struct is
   defined in pkl-asm.c.  */

typedef struct pkl_asm *pkl_asm;

/* Create a new instance of an assembler.  This initializes a new
   program.  */

pkl_asm pkl_asm_new (pkl_ast ast, pkl_compiler compiler,
                     int prologue);

/* Finish the assembly of the current program and return it.  This
   function frees all resources used by the assembler instance, and
   `pkl_asm_new' should be called again in order to assemble another
   program.

   If POINTERS is not NULL, then it is an array of pointers to boxed
   values stored in the returned program.  */

pvm_program pkl_asm_finish (pkl_asm pasm, int epilogue,
                            void **pointers);

/* Assemble an instruction INSN and append it to the program being
   assembled in PASM.  If the instruction takes any argument, they
   follow after INSN.  */

void pkl_asm_insn (pkl_asm pasm, enum pkl_asm_insn insn, ...);

/* Emit assembly code for calling the function FUNCNAME, which should
   be defined in the global environment.  */

void pkl_asm_call (pkl_asm pasm, const char *funcname);

/* Conditionals.
 *
 *  pkl_asm_if (pasm, exp);
 *
 *  ... exp ...
 *
 *  pkl_asm_then (pasm);
 *
 *  ... whatever ...
 *
 *  pkl_asm_else (pasm);
 *
 *  ... whatever ...
 *
 *  pkl_asm_endif (pasm);
 */

void pkl_asm_if (pkl_asm pasm, pkl_ast_node exp);
void pkl_asm_then (pkl_asm pasm);
void pkl_asm_else (pkl_asm pasm);
void pkl_asm_endif (pkl_asm pasm);

/* While loops.
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

void pkl_asm_while (pkl_asm pasm);
void pkl_asm_loop (pkl_asm pasm);
void pkl_asm_endloop (pkl_asm pasm);

/* For-in-where loops.
 *
 * pkl_asm_for (pasm, container_type, selector)
 *
 * ... container ...
 *
 * pkl_asm_for_where (pasm);
 *
 * ... selector ...
 *
 * pkl_asm_for_loop (pasm);
 *
 * ... body ...
 *
 * pkl_asm_for_endloop (pasm);
 *
 */

void pkl_asm_for (pkl_asm pasm, int container_type,
                  pkl_ast_node selector);
void pkl_asm_for_where (pkl_asm pasm);
void pkl_asm_for_loop (pkl_asm pasm);
void pkl_asm_for_endloop (pkl_asm pasm);

/* Try-catch blocks.
 *
 * pkl_asm_try (pasm);
 * 
 * ... stmt ...
 *
 * pkl_asm_catch (pasm);
 *
 * ... stmt ...
 *
 * pkl_asm_endtry (pasm);
 */

void pkl_asm_try (pkl_asm pasm, pkl_ast_node type);
void pkl_asm_catch (pkl_asm pasm);
void pkl_asm_endtry (pkl_asm pasm);

/* The following function returns the label associated to the
   enclosing break-able construction (such as a loop or a switch).  If
   there is not such enclosign environment, this function aborts.  */

jitter_label pkl_asm_break_label (pkl_asm pasm);

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

void pkl_asm_note (pkl_asm pasm, const char *str);

/* Allocate a fresh jitter label and return it.  */
jitter_label pkl_asm_fresh_label (pkl_asm pasm);

/* Append a label.  */
void pkl_asm_label (pkl_asm pasm, jitter_label label);

#endif /* PKL_ASM_H */
