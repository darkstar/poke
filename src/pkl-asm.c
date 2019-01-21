/* pkl-asm.c - Macro-assembler for the Poke Virtual Machine.  */

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

#include <config.h>

#include <xalloc.h>
#include <stdarg.h>
#include <assert.h>
#include <jitter/jitter.h>

#include "pkl.h"
#include "pkl-asm.h"
#include "pvm.h"
#include "ios.h"

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
   This can be either PKL_ASM_ENV_NULL, PKL_ASM_ENV_CONDITIONAL,
   PKL_ASM_ENV_LOOP or PKL_ASM_ENV_TRY.  PKL_ASM_ENV_NULL should only
   be used at the top-level.

   PARENT is the parent level, i.e. the level containing this one.
   This is NULL at the top-level.
   
   The meaning of the LABEL* and NODE* fields depend on the particular
   kind of environment.  See the details in the implementation of the
   functions below.  */

#define PKL_ASM_ENV_NULL 0
#define PKL_ASM_ENV_CONDITIONAL 1
#define PKL_ASM_ENV_LOOP 2
#define PKL_ASM_ENV_TRY 3

struct pkl_asm_level
{
  int current_env;
  struct pkl_asm_level *parent;
  jitter_label label1;
  jitter_label label2;
  jitter_label label3;
  pkl_ast_node node1;
};

/* An assembler instance.

   COMPILER is the PKL compiler using the macro-assembler.

   PROGRAM is the PVM program being assembled.
   LEVEL is a pointer to the top of a stack of levels.

   AST is for creating ast nodes whenever needed.
   UNIT_TYPE is an AST type for an offset unit.

   ERROR_LABEL marks the generic error handler defined in the standard
   prologue.  */

#define PKL_ASM_LEVEL(PASM) ((PASM)->level)

struct pkl_asm
{
  pkl_compiler compiler;

  pvm_program program;
  struct pkl_asm_level *level;

  pkl_ast ast;
  pkl_ast_node unit_type;

  jitter_label error_label;
};

/* Push a new level to PASM's level stack with ENV.  */

static void
pkl_asm_pushlevel (pkl_asm pasm, int env)
{
  struct pkl_asm_level *level
    = xmalloc (sizeof (struct pkl_asm_level));

  memset (level, 0, sizeof (struct pkl_asm_level));
  level->parent = pasm->level;
  level->current_env = env;
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

/* Macro-instruction: PEEK type, endian, nenc
   Stack: _ -> VAL

   Generate code for a peek operation to TYPE, which should be an
   integral type.  */

static void
pkl_asm_insn_peek (pkl_asm pasm, pkl_ast_node type,
                   jitter_uint nenc, jitter_uint endian)
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

      if (sign)
        pkl_asm_insn (pasm, peek_table[tl][sign],
                      nenc, endian,
                      (jitter_uint) size);
      else
        pkl_asm_insn (pasm, peek_table[tl][sign],
                      endian,
                      (jitter_uint) size);
    }
  else
    assert (0);
}

/* Macro-instruction: PEEKD type
   Stack: _ -> VAL

   Generate code for a peek operation to TYPE, which should be an
   integral type.  */

static void
pkl_asm_insn_peekd (pkl_asm pasm, pkl_ast_node type)
{
  int type_code = PKL_AST_TYPE_CODE (type);

  if (type_code == PKL_TYPE_INTEGRAL)
    {
      size_t size = PKL_AST_TYPE_I_SIZE (type);
      int sign = PKL_AST_TYPE_I_SIGNED (type);

      static int peekd_table[2][2] =
        {
         {PKL_INSN_PEEKDIU, PKL_INSN_PEEKDI},
         {PKL_INSN_PEEKDLU, PKL_INSN_PEEKDL}
        };

      int tl = !!((size - 1) & ~0x1f);

      pkl_asm_insn (pasm, peekd_table[tl][sign],
                    (jitter_uint) size);
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
   
   Macro-instruction: BNOT type
   Stack: VAL -> VAL

   Macro-instruction: BAND type
   Stack: VAL VAL -> VAL

   Macro-instruction: BOR type
   Stack: VAL VAL -> VAL

   Macro-instruction: BXOR type
   Stack: VAL VAL -> VAL

   Macro-instruction: SL type
   Stack: VAL VAL -> VAL

   Macro-instruction: SR type
   Stack: VAL VAL -> VAL

   Generate code for performing negation, addition, subtraction,
   multiplication, division, remainder and bit shift to integral
   operands.  INSN identifies the operation to perform, and TYPE the
   type of the operands and the result.  */

static void
pkl_asm_insn_intop (pkl_asm pasm,
                    enum pkl_asm_insn insn,
                    pkl_ast_node type)
{
  static int neg_table[2][2] = {{ PKL_INSN_NEGIU, PKL_INSN_NEGI },
                                { PKL_INSN_NEGLU, PKL_INSN_NEGL }};

  static int add_table[2][2] = {{ PKL_INSN_ADDIU, PKL_INSN_ADDI },
                                { PKL_INSN_ADDLU, PKL_INSN_ADDL }};

  static int sub_table[2][2] = {{ PKL_INSN_SUBIU, PKL_INSN_SUBI },
                                { PKL_INSN_SUBLU, PKL_INSN_SUBL }};

  static int mul_table[2][2] = {{ PKL_INSN_MULIU, PKL_INSN_MULI },
                                { PKL_INSN_MULLU, PKL_INSN_MULL }};

  static int div_table[2][2] = {{ PKL_INSN_DIVIU, PKL_INSN_DIVI },
                                { PKL_INSN_DIVLU, PKL_INSN_DIVL }};

  static int mod_table[2][2] = {{ PKL_INSN_MODIU, PKL_INSN_MODI },
                                { PKL_INSN_MODLU, PKL_INSN_MODL }};

  static int bnot_table[2][2] = {{ PKL_INSN_BNOTIU, PKL_INSN_BNOTI },
                                 { PKL_INSN_BNOTLU, PKL_INSN_BNOTL }};

  static int band_table[2][2] = {{ PKL_INSN_BANDIU, PKL_INSN_BANDI },
                                 { PKL_INSN_BANDLU, PKL_INSN_BANDL }};

  static int bor_table[2][2] = {{ PKL_INSN_BORIU, PKL_INSN_BORI },
                                { PKL_INSN_BORLU, PKL_INSN_BORL }};

  static int bxor_table[2][2] = {{ PKL_INSN_BXORIU, PKL_INSN_BXORI },
                                 { PKL_INSN_BXORLU, PKL_INSN_BXORL }};

  static int sl_table[2][2] = {{ PKL_INSN_SLIU, PKL_INSN_SLI },
                               { PKL_INSN_SLLU, PKL_INSN_SLL }};

  static int sr_table[2][2] = {{ PKL_INSN_SRIU, PKL_INSN_SRI },
                               { PKL_INSN_SRLU, PKL_INSN_SRL }};

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
    case PKL_INSN_MOD:

      if (insn == PKL_INSN_DIV)
        pkl_asm_insn (pasm, div_table[tl][signed_p]);
      else
        pkl_asm_insn (pasm, mod_table[tl][signed_p]);
      
      break;
    case PKL_INSN_BNOT:
      pkl_asm_insn (pasm, bnot_table[tl][signed_p]);
      break;
    case PKL_INSN_BAND:
      pkl_asm_insn (pasm, band_table[tl][signed_p]);
      break;
    case PKL_INSN_BOR:
      pkl_asm_insn (pasm, bor_table[tl][signed_p]);
      break;
    case PKL_INSN_BXOR:
      pkl_asm_insn (pasm, bxor_table[tl][signed_p]);
      break;
    case PKL_INSN_SL:
      pkl_asm_insn (pasm, sl_table[tl][signed_p]);
      break;
    case PKL_INSN_SR:
      pkl_asm_insn (pasm, sr_table[tl][signed_p]);
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
      static int eq_table[2][2] = {{ PKL_INSN_EQIU, PKL_INSN_EQI },
                                   { PKL_INSN_EQLU, PKL_INSN_EQL }};
      
      static int ne_table[2][2] = {{ PKL_INSN_NEIU, PKL_INSN_NEI },
                                   { PKL_INSN_NELU, PKL_INSN_NEL }};
      static int lt_table[2][2] = {{ PKL_INSN_LTIU, PKL_INSN_LTI },
                                   { PKL_INSN_LTLU, PKL_INSN_LTL }};
      
      static int gt_table[2][2] = {{ PKL_INSN_GTIU, PKL_INSN_GTI },
                                   { PKL_INSN_GTLU, PKL_INSN_GTL }};
      
      static int ge_table[2][2] = {{ PKL_INSN_GEIU, PKL_INSN_GEI },
                                   { PKL_INSN_GELU, PKL_INSN_GEL }};
      
      static int le_table[2][2] = {{ PKL_INSN_LEIU, PKL_INSN_LEI },
                                   { PKL_INSN_LELU, PKL_INSN_LEL }};

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
  static int bz_table[2][2] = {{PKL_INSN_BZIU, PKL_INSN_BZI},
                               {PKL_INSN_BZLU, PKL_INSN_BZL}};

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
  static int bnz_table[2][2] = {{PKL_INSN_BNZIU, PKL_INSN_BNZI},
                                {PKL_INSN_BNZLU, PKL_INSN_BNZL}};

  size_t size = PKL_AST_TYPE_I_SIZE (type);
  int sign = PKL_AST_TYPE_I_SIGNED (type);

  int tl = !!((size - 1) & ~0x1f);

  pkl_asm_insn (pasm, bnz_table[tl][sign], label);
}

/* Create a new instance of an assembler.  This initializes a new
   program.  */

pkl_asm
pkl_asm_new (pkl_ast ast, pkl_compiler compiler,
             int guard_stack, int prologue)
{
  pkl_asm pasm = xmalloc (sizeof (struct pkl_asm));
  pvm_program program;

  memset (pasm, 0, sizeof (struct pkl_asm));
  pkl_asm_pushlevel (pasm, PKL_ASM_ENV_NULL);

  pasm->compiler = compiler;
  pasm->ast = ast;
  pasm->unit_type
    = pkl_ast_make_integral_type (pasm->ast, 64, 0);

  program = pvm_make_program ();
  pasm->error_label = jitter_fresh_label (program);
  pasm->program = program;
  
  if (prologue)
    {
      /* Standard prologue.  */
      pkl_asm_note (pasm, "#begin prologue");
      
      /* XXX: initialize the base register to [0 b] and other PVM
         registers.  */
      
      /* Push the stack centinel value.  */
      if (guard_stack)
        pkl_asm_insn (pasm, PKL_INSN_PUSH, PVM_NULL);

      /* Install the default signal handler.  */
      pkl_asm_insn (pasm, PKL_INSN_PUSHE, 0, pasm->error_label);
      pkl_asm_note (pasm, "#end prologue");
    }

  return pasm;
}

/* Finish the assembly of the current program and return it.  This
   function frees all resources used by the assembler instance, and
   `pkl_asm_new' should be called again in order to assemble another
   program.  */

pvm_program
pkl_asm_finish (pkl_asm pasm, int epilogue)
{
  pvm_program program = pasm->program;

  if (epilogue)
    {
      pkl_asm_note (pasm, "#begin epilogue");

      /* Successful program finalization.  */
      pkl_asm_insn (pasm, PKL_INSN_POPE);
      pkl_asm_push_val (program, pvm_make_int (PVM_EXIT_OK, 32));
      pkl_asm_insn (pasm, PKL_INSN_EXIT);      

      pvm_append_label (pasm->program, pasm->error_label);

      /* Default exception handler.  If we are bootstrapping the
         compiler, then use a very simple one inlined here in
         assembly.  Otherwise, call the _pkl_exception_handler
         function which is part of the compiler run-time.  */
      if (pkl_bootstrapped_p (pasm->compiler))
        {
          pkl_asm_push_val (program, pvm_make_int (0, 32)); /* XXX: exception number
                                                               from the stack.  */
          pkl_asm_call (pasm, "_pkl_exception_handler");
        }
      else
        {
          // XXX: discard exception number from the stack.
          // JITTER_DROP_STACK ();
          pkl_asm_insn (pasm, PKL_INSN_PUSH,
                        pvm_make_string ("unhandled exception while bootstrapping\n"));
          pkl_asm_insn (pasm, PKL_INSN_PRINT);

        }

      /* Set the exit status to ERROR and exit the PVM.  */
      pkl_asm_push_val (program, pvm_make_int (PVM_EXIT_ERROR, 32));
      pkl_asm_insn (pasm, PKL_INSN_EXIT);  

      pkl_asm_note (pasm, "#end epilogue");
    }      
  
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
                /* XXX: this doesn't work in 32-bit  */
                pvm_append_unsigned_literal_parameter (pasm->program,
                                                       (jitter_uint) val);
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

      const char *note_begin_prefix = "#begin ";
      const char *note_end_prefix = "#end ";
      const char *macro_name = insn_names[insn];
      char *note_begin = xmalloc (strlen (note_begin_prefix)
                                  + strlen (macro_name) + 1);
      char *note_end = xmalloc (strlen (note_end_prefix)
                                + strlen (macro_name) + 1);

      strcpy (note_begin, note_begin_prefix);
      strcat (note_begin, macro_name);

      strcpy (note_end, note_end_prefix);
      strcat (note_end, macro_name);

      /* pkl_asm_note (pasm, note_begin); */
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
            jitter_uint endian, nenc;

            va_start (valist, insn);
            peek_type = va_arg (valist, pkl_ast_node);
            nenc = va_arg (valist, jitter_uint);
            endian = va_arg (valist, jitter_uint);
            va_end (valist);

            pkl_asm_insn_peek (pasm, peek_type, nenc, endian);
            break;
          }
        case PKL_INSN_PEEKD:
          {
            pkl_ast_node peek_type;

            va_start (valist, insn);
            peek_type = va_arg (valist, pkl_ast_node);
            va_end (valist);

            pkl_asm_insn_peekd (pasm, peek_type);
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
        case PKL_INSN_ADD:
        case PKL_INSN_SUB:
        case PKL_INSN_MUL:
        case PKL_INSN_DIV:
        case PKL_INSN_MOD:
        case PKL_INSN_BNOT:
        case PKL_INSN_BAND:
        case PKL_INSN_BOR:
        case PKL_INSN_BXOR:
        case PKL_INSN_SL:
        case PKL_INSN_SR:
          {
            pkl_ast_node type;

            va_start (valist, insn);
            type = va_arg (valist, pkl_ast_node);
            va_end (valist);

            pkl_asm_insn_intop (pasm, insn, type);
            break;
          }
        case PKL_INSN_EQ:
        case PKL_INSN_NE:
        case PKL_INSN_LT:
        case PKL_INSN_GT:
        case PKL_INSN_GE:
        case PKL_INSN_LE:
          {
            pkl_ast_node type;

            va_start (valist, insn);
            type = va_arg (valist, pkl_ast_node);
            va_end (valist);

            pkl_asm_insn_cmp (pasm, insn, type);
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

      /* pkl_asm_note (pasm, note_end); */
      free (note_begin);
      free (note_end);
    }
}

/* Emit a .note directive with STR as its contents.  */

void
pkl_asm_note (pkl_asm pasm, const char *str)
{
  /* XXX: this doesn't work in 32-bit because of jitter's inability to
     pass 64-bit pointers as arguments to instructions in 32-bit.  */
#if __WORDSIZE == 64
  pkl_asm_insn (pasm, PKL_INSN_NOTE, pvm_make_string (str));
#endif
}

/* The following functions implement conditional constructions.  The
   code generated is:

        ... condition expression ...
        BZ label1;
        POP the condition expression
        ... then body ...
        BA label2;
     label1:
        POP the condition expression
        ... else body ...
     label2:

     Thus, conditionals use two labels.  */

void
pkl_asm_if (pkl_asm pasm, pkl_ast_node exp)
{
  pkl_asm_pushlevel (pasm, PKL_ASM_ENV_CONDITIONAL);

  pasm->level->label1 = jitter_fresh_label (pasm->program);
  pasm->level->label2 = jitter_fresh_label (pasm->program);
  pasm->level->node1 = ASTREF (exp);
}

void
pkl_asm_then (pkl_asm pasm)
{
  assert (pasm->level->current_env == PKL_ASM_ENV_CONDITIONAL);

  pkl_asm_insn (pasm, PKL_INSN_BZ,
                PKL_AST_TYPE (pasm->level->node1),
                pasm->level->label1);
  /* Pop the expression condition from the stack.  */
  pkl_asm_insn (pasm, PKL_INSN_POP);
}

void
pkl_asm_else (pkl_asm pasm)
{
  assert (pasm->level->current_env == PKL_ASM_ENV_CONDITIONAL);

  pkl_asm_insn (pasm, PKL_INSN_BA, pasm->level->label2);
  pvm_append_label (pasm->program, pasm->level->label1);
  /* Pop the expression condition from the stack.  */
  pkl_asm_insn (pasm, PKL_INSN_POP);
}

void
pkl_asm_endif (pkl_asm pasm)
{
  assert (pasm->level->current_env == PKL_ASM_ENV_CONDITIONAL);
  pvm_append_label (pasm->program, pasm->level->label2);
  
  /* Cleanup and pop the current level.  */
  pkl_ast_node_free (pasm->level->node1);
  pkl_asm_poplevel (pasm);
}

/* The following functions implement try-catch blocks.  The code
   generated is:

     PUSH-REGISTERS
     PUSH-E-HANDLER label1
     ... code ...
     POP-E-HANDLER
     POP-REGISTERS
     BA label2
   label1:
     ... handler ...
   label2:

   Thus, try-catch blocks use two labels.  */

void
pkl_asm_try (pkl_asm pasm)
{
  pkl_asm_pushlevel (pasm, PKL_ASM_ENV_TRY);

  pasm->level->label1 = jitter_fresh_label (pasm->program);
  pasm->level->label2 = jitter_fresh_label (pasm->program);

  /* pkl_asm_note (pasm, "PUSH-REGISTERS"); */
  pkl_asm_insn (pasm, PKL_INSN_PUSHE, /* XXX */ 0, pasm->level->label1);
}

void
pkl_asm_catch (pkl_asm pasm)
{
  assert (pasm->level->current_env == PKL_ASM_ENV_TRY);

  pkl_asm_insn (pasm, PKL_INSN_POPE);
  /* XXX pkl_asm_note (pasm, "POP-REGISTERS"); */
  pkl_asm_insn (pasm, PKL_INSN_BA, pasm->level->label2);
  pvm_append_label (pasm->program, pasm->level->label1);
}

void
pkl_asm_endtry (pkl_asm pasm)
{
  assert (pasm->level->current_env == PKL_ASM_ENV_TRY);
  pvm_append_label (pasm->program, pasm->level->label2);
}

/* The following functions implement while loops.  The code generated
   is:

   label1:
   ... loop condition expression ...
   BZ label2;
   POP the condition expression
   ... loop body ...
   BA label1;
   label2:
   POP the condition expression
  
   Thus, loops use two labels.  */

void
pkl_asm_while (pkl_asm pasm)
{
  pkl_asm_pushlevel (pasm, PKL_ASM_ENV_LOOP);

  pasm->level->label1 = jitter_fresh_label (pasm->program);
  pasm->level->label2 = jitter_fresh_label (pasm->program);

  pvm_append_label (pasm->program, pasm->level->label1);
}

void
pkl_asm_loop (pkl_asm pasm)
{
  pkl_asm_insn (pasm, PKL_INSN_BZI, pasm->level->label2);
  /* Pop the loop condition from the stack.  */
  pkl_asm_insn (pasm, PKL_INSN_POP);
}

void
pkl_asm_endloop (pkl_asm pasm)
{
  pkl_asm_insn (pasm, PKL_INSN_BA, pasm->level->label1);
  pvm_append_label (pasm->program, pasm->level->label2);
  /* Pop the loop condition from the stack.  */
  pkl_asm_insn (pasm, PKL_INSN_POP);

  /* Cleanup and pop the current level.  */
  pkl_asm_poplevel (pasm);
}

void
pkl_asm_call (pkl_asm pasm, const char *funcname)
{
  pkl_env compiler_env = pkl_get_env (pasm->compiler);
  int back, over;
  
  assert (pkl_env_lookup (compiler_env, funcname,
                          &back, &over) != NULL);

  pkl_asm_insn (pasm, PKL_INSN_PUSHVAR, back, over);
  pkl_asm_insn (pasm, PKL_INSN_CALL);
}
