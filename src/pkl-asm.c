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

/* Push a new level to PASM's level stack with ENV.  */

static void
pkl_asm_pushlevel (pkl_asm pasm, int env)
{
  struct pkl_asm_level *level
    = xmalloc (sizeof (struct pkl_asm_level));

  memset (level, 0, sizeof (struct pkl_asm_level));
  level->parent = pasm->level;
  pasm->level = level;
}

/* Pop the innermost level from PASM's level stack.  */

static void __attribute__((unused))
pkl_asm_poplevel (pkl_asm pasm)
{
  struct pkl_asm_level *level = pasm->level;

  pasm->level = level->parent;
  free (level);
}

/* Append instructions to PROGRAM to push VAL into the stack.  */

static inline void
pkl_asm_push_val (pvm_program program, pvm_val val)
{
#if __WORDSIZE == 64
  PVM_APPEND_INSTRUCTION (program, push);
  pvm_append_unsigned_literal_parameter (program,
                                         (jitter_uint) val);
#else
  /* Use the push-hi and push-lo instructions, to overcome jitter's
     limitation of only accepting a jitter_uint value as a literal
     argument, whose size is 32-bit in 32-bit hosts.  */

  if (val & ~0xffffffffLL)
    {
      PVM_APPEND_INSTRUCTION (program, pushhi);
      pvm_append_unsigned_literal_parameter (program,
                                             ((jitter_uint) (val >> 32)));

      PVM_APPEND_INSTRUCTION (program, pushlo);
      pvm_append_unsigned_literal_parameter (program,
                                             ((jitter_uint) (val & 0xffffffff)));
    }
  else
    {
      PVM_APPEND_INSTRUCTION (program, push32);
      pvm_append_unsigned_literal_parameter (program,
                                             ((jitter_uint) (val & 0xffffffff)));
    }
#endif
}

/* Macro-instruction: NTON from_type, to_type
   Stack: VAL(from_type) -> VAL(to_type)

   Generate code to convert an integer value from FROM_TYPE to
   TO_TYPE.  Both types should be integral types.  */

static void
pkl_asm_insn_nton  (pkl_asm pasm,
                    pkl_ast_node from_type,
                    pkl_ast_node to_type)
{
  size_t from_type_size = PKL_AST_TYPE_I_SIZE (from_type);
  int from_type_sign = PKL_AST_TYPE_I_SIGNED (from_type);
      
  size_t to_type_size = PKL_AST_TYPE_I_SIZE (to_type);
  int to_type_sign = PKL_AST_TYPE_I_SIGNED (to_type);
  
  if (from_type_size == to_type_size
      && from_type_sign == to_type_sign)
    /* Wheee, nothing to do.  */
    return;
  else
    {
      static int cast_table[2][2][2][2] =
        {
         {
          {
           {PKL_INSN_LTOLU, PKL_INSN_LTOIU},
           {PKL_INSN_LTOL, PKL_INSN_LTOI}
          },
          {
           {PKL_INSN_LUTOLU, PKL_INSN_LUTOIU},
           {PKL_INSN_LUTOL, PKL_INSN_LUTOI}
          },
         },
         {
          {
           {PKL_INSN_ITOLU, PKL_INSN_ITOIU},
           {PKL_INSN_ITOL, PKL_INSN_ITOI}
          },
          {
           {PKL_INSN_IUTOLU, PKL_INSN_IUTOIU},
           {PKL_INSN_IUTOL, PKL_INSN_IUTOI}
          },
         }
        };

      int fl = ((from_type_size - 1) & ~0x1f);
      int fs = from_type_sign;
      int tl = ((to_type_size - 1) & ~0x1f);
      int ts = to_type_sign;
         
      pkl_asm_insn (pasm,
                    cast_table [fl][tl][fs][ts],
                    (jitter_uint) to_type_size);
    }
}

/* Macro-instruction: PEEK type
   Stack: _ -> VAL

   Generate code for a peek operation to TYPE, which should be a
   simple type, i.e. integral or string.  */

static void
pkl_asm_insn_peek (pkl_asm pasm, pkl_ast_node type)
{
  int type_code = PKL_AST_TYPE_CODE (type);

  if (type_code == PKL_TYPE_INTEGRAL)
    {
      size_t size = PKL_AST_TYPE_I_SIZE (type);
      int sign = PKL_AST_TYPE_I_SIGNED (type);

      static int peek_table[2][2] =
        {
         {PKL_INSN_PEEKL, PKL_INSN_PEEKLU},
         {PKL_INSN_PEEKI, PKL_INSN_PEEKIU}
        };

      int tl = ((size - 1) & ~0x1f);

      pkl_asm_insn (pasm, peek_table[tl][sign],
                    (jitter_uint) size);
    }
  else if (type_code == PKL_TYPE_STRING)
    {
      pkl_asm_insn (pasm, PKL_INSN_PEEKS);
    }
  else
    assert (0);
}

/* Macro-instruction: ADD type
   Stack: VAL VAL -> VAL
   
   Macro-instruction: SUB type
   Stack: VAL VAL -> VAL

   Macro-instruction: MUL type
   Stack: VAL VAL -> VAL

   Macro-instruction: DIV type
   Stack: VAL VAL -> VAL

   Macro-instruction: MOD type
   Stack: VAL VAL -> VAL

   Generate code for performing addition, subtraction, multiplication,
   division and remainder to integral operands.  INSN identifies the
   operation to perform, and TYPE the type of the operands and the
   result.  */

static void
pkl_asm_insn_intop (pkl_asm pasm,
                    enum pkl_asm_insn insn,
                    pkl_ast_node type)
{
  static int add_table[2][2] =
    {
     { PKL_INSN_ADDLU, PKL_INSN_ADDL },
     { PKL_INSN_ADDIU, PKL_INSN_ADDI },
    };

  static int sub_table[2][2] =
    {
     { PKL_INSN_SUBLU, PKL_INSN_SUBL },
     { PKL_INSN_SUBIU, PKL_INSN_SUBI },
    };

  static int mul_table[2][2] =
    {
     { PKL_INSN_MULLU, PKL_INSN_MULL },
     { PKL_INSN_MULIU, PKL_INSN_MULI },
    };

  static int div_table[2][2] =
    {
     { PKL_INSN_DIVLU, PKL_INSN_DIVL },
     { PKL_INSN_DIVIU, PKL_INSN_DIVI },
    };

  static int mod_table[2][2] =
    {
     { PKL_INSN_MODLU, PKL_INSN_MODL },
     { PKL_INSN_MODIU, PKL_INSN_MODI },
    };

  uint64_t size = PKL_AST_TYPE_I_SIZE (type);
  int signed_p = PKL_AST_TYPE_I_SIGNED (type);
  int tl = ((size - 1) & ~0x1f);

  switch (insn)
    {
    case PKL_INSN_ADD:
      pkl_asm_insn (pasm, add_table[tl][signed_p]);
      break;
    case PKL_INSN_SUB:
      pkl_asm_insn (pasm, sub_table[tl][signed_p]);
      break;
    case PKL_INSN_MUL:
      pkl_asm_insn (pasm, mul_table[tl][signed_p]);
      break;
    case PKL_INSN_DIV:
      pkl_asm_insn (pasm, div_table[tl][signed_p]);
      break;
    case PKL_INSN_MOD:
      pkl_asm_insn (pasm, mod_table[tl][signed_p]);
      break;
    default:
      assert (0);
    }
}

#if 0
/* Macro-instruction: OGETMU base_type, unit_type, to_unit
   Stack: OFFSET -> OFFSET CONVERTED_MAGNITUDE

   Given an offset in the stack, generate code to push its magnitude
   converted to unit TO_UNIT.  */


static void
pkl_asm_insn_ogetmu (pkl_asm pasm,
                     pkl_ast_node base_type,
                     pkl_ast_node unit_type,
                     pkl_ast_node to_unit)
{
  pvm_program program = pasm->program;
  
  /* Dup the offset.  */
  pkl_asm_insn (pasm, PKL_INSN_DUP);

  /* Get magnitude and unit.  */
  pkl_asm_insn (pasm, PKL_INSN_OGETM);
  pkl_asm_insn (pasm, PKL_INSN_SWAP);
  pkl_asm_insn (pasm, PKL_INSN_OGETU);
  pkl_asm_insn (pasm, PKL_INSN_NTON, unit_type, base_type);
  pkl_asm_insn (pasm, PKL_INSN_NIP);

  /* (magnitude * unit) / res_unit */
  pkl_asm_insn (pasm, PKL_INSN_MUL, base_type);
  PKL_PASS_SUBPASS (to_unit); /* XXX shit */
  append_int_cast (program, unit_type, base_type);
  append_int_op (program, "bz", base_type);
  pvm_append_symbolic_label_parameter (program,
                                       "Ldivzero");
  append_int_op (program, "div", base_type);
}

#endif

/* The functions below are documented in pkl-asm.h.  */

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
#  include "pkl-insn.def"
#undef PKL_DEF_INSN
    };

  static const char *insn_args[] =
    {
#define PKL_DEF_INSN(SYM, ARGS, NAME) ARGS,
#  include "pkl-insn.def"
#undef PKL_DEF_INSN
    };    

  va_list valist;

  if (insn < PKL_INSN_MACRO)
    {
      /* This is a normal instruction.  Process its arguments and
         append it to the jitter program.  */

      const char *insn_name = insn_names[insn];
      const char *p;

      pvm_append_instruction_name (pasm->program, insn_name);

      va_start (valist, insn);
      for (p = insn_args[insn]; *p != '\0'; ++p)
        {
          char arg_class = *p;
          
          switch (arg_class)
            {
            case 'v':
              {
                pvm_val val = va_arg (valist, pvm_val);
                pkl_asm_push_val (pasm->program, val);
                break;
              }
            case 'n':
              {
                jitter_uint n = va_arg (valist, jitter_uint);
                pvm_append_unsigned_literal_parameter (pasm->program, n);
                break;
              }
            case 'a':
              assert (0); /* XXX */
              break;
            case 'l':
              {
                jitter_label label = va_arg (valist, jitter_label);
                pvm_append_label (pasm->program, label);
                break;
              }
            case 'i':
              assert (0); /* XXX */
              break;
            case 'r':
              assert (0); /* XXX */
              break;
            }
        }
      va_end (valist);
    }
  else
    {
      /* This is a macro-instruction.  Dispatch to the corresponding
         macro-instruction handler.  */

      switch (insn)
        {
        case PKL_INSN_NTON:
          {
            pkl_ast_node from_type;
            pkl_ast_node to_type;

            va_start (valist, insn);
            from_type = va_arg (valist, pkl_ast_node);
            to_type = va_arg (valist, pkl_ast_node);
            va_end (valist);

            pkl_asm_insn_nton (pasm, from_type, to_type);
            break;
          }
        case PKL_INSN_PEEK:
          {
            pkl_ast_node peek_type;

            va_start (valist, insn);
            peek_type = va_arg (valist, pkl_ast_node);
            va_end (valist);

            pkl_asm_insn_peek (pasm, peek_type);
            break;
          }
        case PKL_INSN_ADD:
          /* Fallthrough.  */
        case PKL_INSN_SUB:
          /* Fallthrough.  */
        case PKL_INSN_MUL:
          /* Fallthrough.  */
        case PKL_INSN_DIV:
          /* Fallthrough.  */
        case PKL_INSN_MOD:
          {
            pkl_ast_node type;

            va_start (valist, insn);
            type = va_arg (valist, pkl_ast_node);
            va_end (valist);

            pkl_asm_insn_intop (pasm, insn, type);
            break;
          }
#if 0
        case PKL_INSN_OGETMU:
          {
            pkl_ast_node base_type;
            pkl_ast_node unit_type;
            pkl_ast_node to_unit;

            va_start (valist, insn);
            base_type = va_arg (valist, pkl_ast_node);
            unit_type = va_arg (valist, pkl_ast_node);
            to_unit = va_arg (valist, pkl_ast_node);
            va_end (valist);

            pkl_asm_insn_ogetmu (pasm, base_type, unit_type, to_unit);
            break;
          }
#endif
        case PKL_INSN_MACRO:
        default:
          assert (0);
        }
    }
}
