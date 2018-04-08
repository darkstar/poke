/* pkl-asm.c - Macro-assembler for the Poke Virtual Machine.  */

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

#include <config.h>
#include <xalloc.h>
#include <stdarg.h>

#include "pkl.h"
#include "pvm.h"
#include "pkl-asm.h"

/* In order to allow nested multi-function macros, like conditionals
   and loops, the assembler supports the notion of "nest levels".  For
   example, consider the following conditional nested in a loop:

      ... top-level ...

      pkl_asm_dotimes (pasm, exp);
      {
         ... level-1 ...
   
         pkl_asm_if (pasm, exp);
         {
            ... level-2 ...
         }
         pkl_asm_end_if (pasm);
      }
      pkl_asm_end_dotimes (pasm);

   Levels are stacked and managed using the `pkl_asm_pushlevel' and
   `pkl_asm_poplevel' functions defined below.

   CURRENT_ENV identifies what kind of instruction created the level.
   This can be either PKL_ASM_ENV_NULL, PKL_ASM_ENV_CONDITIONAL or
   PKL_ASM_ENV_LOOP.  PKL_ASM_ENV_NULL should only be used at the
   top-level.

   PARENT is the parent level, i.e. the level containing this one.
   This is NULL at the top-level.  */

#define PKL_ASM_ENV_NULL 0
#define PKL_ASM_ENV_CONDITIONAL 1
#define PKL_ASM_ENV_LOOP 2

struct pkl_asm_level
{
  enum pkl_asm_insn current_env;
  struct pkl_asm_level *parent;
};

/* An assembler instance.

   PROGRAM is the PVM program being assembled.
   LEVEL is a pointer to the top of a stack of levels.  */

struct pkl_asm
{
  pvm_program program;
  struct pkl_asm_level *level;
};

static void
pkl_asm_pushlevel (pkl_asm pasm, int env)
{
  struct pkl_asm_level *level
    = xmalloc (sizeof (struct pkl_asm_level));

  memset (level, 0, sizeof (struct pkl_asm_level));
  level->parent = pasm->level;
  pasm->level = level;
}

static void __attribute__((unused))
pkl_asm_poplevel (pkl_asm pasm)
{
  struct pkl_asm_level *level = pasm->level;

  pasm->level = level->parent;
  free (level);
}

pkl_asm
pkl_asm_new ()
{
  pkl_asm pasm = xmalloc (sizeof (struct pkl_asm));

  memset (pasm, 0, sizeof (struct pkl_asm));

  pkl_asm_pushlevel (pasm, PKL_ASM_ENV_NULL);
  return pasm;
}

pvm_program
pkl_asm_get_program (pkl_asm pasm)
{
  return pasm->program;
}

void
pkl_asm_free (pkl_asm pasm)
{
  free (pasm);
}

void
pkl_asm_insn (pkl_asm pasm, enum pkl_asm_insn insn, ...)
{
  static const char *insn_names[] =
    {
#define PKL_DEF_INSN(SYM, ARGS, NAME) NAME,
#define PKL_DEF_MACRO_INSN(SYM, ARGS)
#  include "pkl-insn.def"
#undef PKL_DEF_INSN
#undef PKL_DEF_MACRO_INSN
    };

  //  va_list valist;

  if (insn < PKL_INSN_MACRO)
    {
      /* This is a normal instruction.  Process its arguments and
         append it to the jitter program.  */

      const char *insn_name = insn_names[insn];

      pvm_append_instruction_name (pasm->program, insn_name);
    }
  else if (insn > PKL_INSN_MACRO)
    {
      /* This is a macro-instruction.  Process its arguments and
         dispach to the corresponding macro handler.  */
    }
  else
    assert (0);
}
