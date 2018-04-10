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
#include <assert.h>
#include <jitter/jitter.h>

#include "pkl.h"
#include "pkl-asm.h"
#include "pvm.h"


/* In order to allow nested multi-function macros, like conditionals
   and loops, the assembler supports the notion of "nesting levels".
   For example, consider the following conditional code:

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
   LEVEL is a pointer to the top of a stack of levels.

   AST is for creating ast nodes whenever needed.
   UNIT_TYPE is an AST type for an offset unit.

   START_LABEL marks the beginning of the user code.

   DIVZERO_LABEL marks the division-by-zero error handler defined in
   the standard prologue.

   ERROR_LABEL marks the generic error handler defined in the standard
   prologue.

   EXIT_LABEL marks the return handler defined in the standard
   prologue.  */

struct pkl_asm
{
  pvm_program program;
  struct pkl_asm_level *level;

  pkl_ast ast;
  pkl_ast_node unit_type;

  jitter_label start_label;
  jitter_label divzero_label;
  jitter_label error_label;
  jitter_label exit_label;
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
         /* Source is int.  */
         {
          /* Destination is int.  */
          {
           {PKL_INSN_IUTOIU, PKL_INSN_IUTOI},
           {PKL_INSN_ITOIU, PKL_INSN_ITOI}
          },
          /* Destination is long. */
          {
           {PKL_INSN_IUTOLU, PKL_INSN_IUTOL},
           {PKL_INSN_ITOLU, PKL_INSN_ITOL}
          },
         },
         /* Source is long.  */
         {
          /* Destination is int.  */
          {
           {PKL_INSN_LUTOIU, PKL_INSN_LUTOI},
           {PKL_INSN_LTOIU, PKL_INSN_LTOI}
          },
          {
           /* Destination is long.  */
           {PKL_INSN_LUTOLU, PKL_INSN_LUTOL},
           {PKL_INSN_LTOLU, PKL_INSN_LTOL}
          },
         }
        };

      int fl = !!((from_type_size - 1) & ~0x1f);
      int fs = from_type_sign;
      int tl = !!((to_type_size - 1) & ~0x1f);
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
         {PKL_INSN_PEEKIU, PKL_INSN_PEEKI},
         {PKL_INSN_PEEKLU, PKL_INSN_PEEKL}
        };

      int tl = !!((size - 1) & ~0x1f);

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

/* Macro-instruction: NEG type
   Stack: VAL -> VAL

   Macro-instruction: ADD type
   Stack: VAL VAL -> VAL
   
   Macro-instruction: SUB type
   Stack: VAL VAL -> VAL

   Macro-instruction: MUL type
   Stack: VAL VAL -> VAL

   Macro-instruction: DIV type
   Stack: VAL VAL -> VAL

   Macro-instruction: MOD type
   Stack: VAL VAL -> VAL

   Generate code for performing negation, addition, subtraction,
   multiplication, division and remainder to integral operands.  INSN
   identifies the operation to perform, and TYPE the type of the
   operands and the result.  */

static void
pkl_asm_insn_intop (pkl_asm pasm,
                    enum pkl_asm_insn insn,
                    pkl_ast_node type)
{
  static int neg_table[2][2] =
    {
     { PKL_INSN_NEGIU, PKL_INSN_NEGI },
     { PKL_INSN_NEGLU, PKL_INSN_NEGL },
    };
  
  static int add_table[2][2] =
    {
     { PKL_INSN_ADDIU, PKL_INSN_ADDI },
     { PKL_INSN_ADDLU, PKL_INSN_ADDL },
    };

  static int sub_table[2][2] =
    {
     { PKL_INSN_SUBIU, PKL_INSN_SUBI },
     { PKL_INSN_SUBLU, PKL_INSN_SUBL },
    };

  static int mul_table[2][2] =
    {
     { PKL_INSN_MULIU, PKL_INSN_MULI },
     { PKL_INSN_MULLU, PKL_INSN_MULL },
    };

  static int div_table[2][2] =
    {
     { PKL_INSN_DIVIU, PKL_INSN_DIVI },
     { PKL_INSN_DIVLU, PKL_INSN_DIVL },
    };

  static int mod_table[2][2] =
    {
     { PKL_INSN_MODIU, PKL_INSN_MODI },
     { PKL_INSN_MODLU, PKL_INSN_MODL },
    };

  uint64_t size = PKL_AST_TYPE_I_SIZE (type);
  int signed_p = PKL_AST_TYPE_I_SIGNED (type);
  int tl = !!((size - 1) & ~0x1f);

  switch (insn)
    {
    case PKL_INSN_NEG:
      pkl_asm_insn (pasm, neg_table[tl][signed_p]);
      break;
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
      /* Fallthrough.  */
    case PKL_INSN_MOD:

      pkl_asm_insn (pasm, PKL_INSN_BZ, type,
                    pasm->divzero_label);
      
      if (insn == PKL_INSN_DIV)
        pkl_asm_insn (pasm, div_table[tl][signed_p]);
      else
        pkl_asm_insn (pasm, mod_table[tl][signed_p]);
      
      break;
    default:
      assert (0);
    }
}

/* Macro-instruction: EQ type
   Stack: VAL VAL -> INT

   Macro-instruction: NE type
   Stack: VAL VAL -> INT
 
   Macro-instruction: LT type
   Stack VAL VAL -> INT
 
   Macro-instruction: GT type
   Stack VAL VAL -> INT

   Macro-instruction: GE type
   Stack VAL VAL -> INT

   Macro-instruction: LE type
   Stack VAL VAL -> INT

   Generate code for perfoming a comparison operation, to either
   integral or string operands.  INSN identifies the operation to
   perform, and TYPE the type of the operands.  */

static void
pkl_asm_insn_cmp (pkl_asm pasm,
                  enum pkl_asm_insn insn,
                  pkl_ast_node type)
{
  enum pkl_asm_insn oinsn;
  
  /* Decide what instruction to assembly.  */
  if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_STRING)
    {
      switch (insn)
        {
        case PKL_INSN_EQ: oinsn = PKL_INSN_EQS; break;
        case PKL_INSN_NE: oinsn = PKL_INSN_NES; break;
        case PKL_INSN_LT: oinsn = PKL_INSN_LTS; break;
        case PKL_INSN_GT: oinsn = PKL_INSN_GTS; break;
        case PKL_INSN_GE: oinsn = PKL_INSN_GES; break;
        case PKL_INSN_LE: oinsn = PKL_INSN_LES; break;
        default:
          assert (0);
        }
    }
  else if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_INTEGRAL)
    {
      static int eq_table[2][2] =
        {
         { PKL_INSN_EQIU, PKL_INSN_EQI },
         { PKL_INSN_EQLU, PKL_INSN_EQL }
        };
      
      static int ne_table[2][2] =
        {
         { PKL_INSN_NEIU, PKL_INSN_NEI },
         { PKL_INSN_NELU, PKL_INSN_NEL }
        };
      
      static int lt_table[2][2] =
        {
         { PKL_INSN_LTIU, PKL_INSN_LTI },
         { PKL_INSN_LTLU, PKL_INSN_LTL }
        };
      
      static int gt_table[2][2] =
        {
         { PKL_INSN_GTIU, PKL_INSN_GTI },
         { PKL_INSN_GTLU, PKL_INSN_GTL }
        };
      
      static int ge_table[2][2] =
        {
         { PKL_INSN_GEIU, PKL_INSN_GEI },
         { PKL_INSN_GELU, PKL_INSN_GEL }
        };
      
      static int le_table[2][2] =
        {
         { PKL_INSN_LEIU, PKL_INSN_LEI },
         { PKL_INSN_LELU, PKL_INSN_LEL }
        };

      uint64_t size = PKL_AST_TYPE_I_SIZE (type);
      int signed_p = PKL_AST_TYPE_I_SIGNED (type);
      int tl = !!((size - 1) & ~0x1f);

      switch (insn)
        {
        case PKL_INSN_EQ: oinsn = eq_table[tl][signed_p]; break;
        case PKL_INSN_NE: oinsn = ne_table[tl][signed_p]; break;
        case PKL_INSN_LT: oinsn = lt_table[tl][signed_p]; break;
        case PKL_INSN_GT: oinsn = gt_table[tl][signed_p]; break;
        case PKL_INSN_GE: oinsn = ge_table[tl][signed_p]; break;
        case PKL_INSN_LE: oinsn = le_table[tl][signed_p]; break;
        default:
          assert (0);
          break;
        }
    }
  else
    assert (0);

  /* Assembly the instruction.  */
  pkl_asm_insn (pasm, oinsn);
}

/* Macro-instruction: OGETMC base_type
   Stack: OFFSET UNIT -> OFFSET CONVERTED_MAGNITUDE

   Given an offset and an unit in the stack, generate code to push its
   magnitude converted to the given unit.  */

static void
pkl_asm_insn_ogetmc (pkl_asm pasm,
                     pkl_ast_node base_type)
{
  /* Stack: OFF TOUNIT */
  pkl_asm_insn (pasm, PKL_INSN_SWAP);
  pkl_asm_insn (pasm, PKL_INSN_DUP);

  /* Stack: TOUNIT OFF OFF */
  pkl_asm_insn (pasm, PKL_INSN_OGETM);
  pkl_asm_insn (pasm, PKL_INSN_SWAP);
  pkl_asm_insn (pasm, PKL_INSN_OGETU);
  pkl_asm_insn (pasm, PKL_INSN_NTON, pasm->unit_type, base_type);
  pkl_asm_insn (pasm, PKL_INSN_NIP);

  /* Stack: TOUNIT OFF MAGNITUDE UNIT */
  pkl_asm_insn (pasm, PKL_INSN_MUL, base_type);

  /* Stack: TOUNIT OFF (MAGNITUDE*UNIT) */
  pkl_asm_insn (pasm, PKL_INSN_ROT);

  /* Stack: OFF (MAGNITUDE*UNIT) TOUNIT */
  pkl_asm_insn (pasm, PKL_INSN_NTON, pasm->unit_type, base_type);
  pkl_asm_insn (pasm, PKL_INSN_DIV, base_type);
}

/* Macro-instruction: BZ type, label
   Stack: _ -> _

   Branch to LABEL if the integer value of type TYPE at the top of the
   stack is zero.  */

static void
pkl_asm_insn_bz (pkl_asm pasm,
                 pkl_ast_node type,
                 jitter_label label)
{
  static int bz_table[2][2] =
    {
     {PKL_INSN_BZIU, PKL_INSN_BZI},
     {PKL_INSN_BZLU, PKL_INSN_BZL}
    };

  size_t size = PKL_AST_TYPE_I_SIZE (type);
  int sign = PKL_AST_TYPE_I_SIGNED (type);

  int tl = !!((size - 1) & ~0x1f);

  pkl_asm_insn (pasm, bz_table[tl][sign], label);
}

/* Macro-instruction: BNZ type, label
   Stack: _ -> _

   Branch to LABEL if the integer value of type TYPE at the top of the
   stack is not zero.  */

static void
pkl_asm_insn_bnz (pkl_asm pasm,
                  pkl_ast_node type,
                  jitter_label label)
{
  static int bnz_table[2][2] =
    {
     {PKL_INSN_BNZIU, PKL_INSN_BNZI},
     {PKL_INSN_BNZLU, PKL_INSN_BNZL}
    };

  size_t size = PKL_AST_TYPE_I_SIZE (type);
  int sign = PKL_AST_TYPE_I_SIGNED (type);

  int tl = !!((size - 1) & ~0x1f);

  pkl_asm_insn (pasm, bnz_table[tl][sign], label);
}

/* Create a new instance of an assembler.  This initializes a new
   program.  */

pkl_asm
pkl_asm_new (pkl_ast ast)
{
  pkl_asm pasm = xmalloc (sizeof (struct pkl_asm));
  pvm_program program;

  memset (pasm, 0, sizeof (struct pkl_asm));
  pkl_asm_pushlevel (pasm, PKL_ASM_ENV_NULL);

  pasm->ast = ast;
  pasm->unit_type
    = pkl_ast_make_integral_type (pasm->ast, 64, 0);

  program = pvm_make_program ();
  pasm->start_label = jitter_fresh_label (program);
  pasm->divzero_label = jitter_fresh_label (program);
  pasm->error_label = jitter_fresh_label (program);
  pasm->exit_label = jitter_fresh_label (program);

  /* XXX: note begin prologue */
  
  /* Standard prologue.  */
  PVM_APPEND_INSTRUCTION (program, ba);
  pvm_append_label_parameter (program, pasm->start_label);
  
  pvm_append_label (program, pasm->divzero_label);
  
  pkl_asm_push_val (program, pvm_make_int (PVM_EXIT_EDIVZ, 32));

  PVM_APPEND_INSTRUCTION (program, ba);
  pvm_append_label_parameter (program, pasm->exit_label);
  pvm_append_label (program, pasm->error_label);
  pkl_asm_push_val (program, pvm_make_int (PVM_EXIT_ERROR, 32));
  
  pvm_append_label (program, pasm->exit_label);
  PVM_APPEND_INSTRUCTION (program, exit);
  
  pvm_append_label (program, pasm->start_label);

  /* XXX: note end prologue  */
  pasm->program = program;
    
  return pasm;
}

/* Finish the assembly of the current program and return it.  This
   function frees all resources used by the assembler instance, and
   `pkl_asm_new' should be called again in order to assemble another
   program.  */

pvm_program
pkl_asm_finish (pkl_asm pasm)
{
  pvm_program program = pasm->program;

  /* XXX: note begin epilogue. */
  /* Standard epilogue.  */
  pkl_asm_push_val (program, pvm_make_int (PVM_EXIT_OK, 32));
    
  PVM_APPEND_INSTRUCTION (program, ba);
  pvm_append_label_parameter (program, pasm->exit_label);
  /* XXX: note end epilogue.  */

  /* Free the assembler instance and return the assembled program to
     the user.  */
  ASTREF (pasm->unit_type); pkl_ast_node_free (pasm->unit_type);
  free (pasm);
  return program;
}

/* Assemble an instruction INSN and append it to the program being
   assembled in PASM.  If the instruction takes any argument, they
   follow after INSN.  */

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

  if (insn == PKL_INSN_PUSH)
    {
      /* We handle PUSH as a special case, due to some jitter
         limitations.  See the docstring for `pkl_asm_push_val'
         above.  */

      pvm_val val;

      va_start (valist, insn);
      val = va_arg (valist, pvm_val);
      va_end (valist);

      pkl_asm_push_val (pasm->program, val);
    }
  else if (insn < PKL_INSN_MACRO)
    {
      /* This is a PVM instruction.  Process its arguments and append
         it to the jitter program.  */

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
                pvm_append_label_parameter (pasm->program, label);
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

      /* XXX: emit a begin macro note if the assembler is in debugging
         mode.  */
      
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
        case PKL_INSN_BZ:
          {
            pkl_ast_node type;
            jitter_label label;

            va_start (valist, insn);
            type = va_arg (valist, pkl_ast_node);
            label = va_arg (valist, jitter_label);
            va_end (valist);

            pkl_asm_insn_bz (pasm, type, label);
            break;
          }
        case PKL_INSN_BNZ:
          {
            pkl_ast_node type;
            jitter_label label;

            va_start (valist, insn);
            type = va_arg (valist, pkl_ast_node);
            label = va_arg (valist, jitter_label);
            va_end (valist);

            pkl_asm_insn_bnz (pasm, type, label);
            break;
          }
        case PKL_INSN_NEG:
          /* Fallthrough.  */
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
        case PKL_INSN_EQ:
          /* Fallthrough.  */
        case PKL_INSN_NE:
          /* Fallthrough.  */
        case PKL_INSN_LT:
          /* Fallthrough.  */
        case PKL_INSN_GT:
          /* Fallthrough.  */
        case PKL_INSN_GE:
          /* Fallthrough.  */
        case PKL_INSN_LE:
          {
            pkl_ast_node type;

            va_start (valist, insn);
            type = va_arg (valist, pkl_ast_node);
            va_end (valist);

            pkl_asm_insn_cmp (pasm, insn, type);
            break;
          }
        case PKL_INSN_SL:
          /* Fallthrough.  */
        case PKL_INSN_SR:
          {
            assert (0);
            break;
          }
        case PKL_INSN_BNOT:
        case PKL_INSN_BAND:
        case PKL_INSN_BOR:
        case PKL_INSN_BXOR:
          {
            assert (0);
            break;
          }
        case PKL_INSN_OGETMC:
          {
            pkl_ast_node base_type;

            va_start (valist, insn);
            base_type = va_arg (valist, pkl_ast_node);
            va_end (valist);

            pkl_asm_insn_ogetmc (pasm, base_type);
            break;
          }
        case PKL_INSN_MACRO:
        default:
          assert (0);
        }

      /* XXX: emit an end-macro note if the assembler is in debugging
         mode.  */
    }
}
