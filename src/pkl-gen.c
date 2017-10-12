/* pkl-gen.c - Code generator for Poke.  */

/* Copyright (C) 2017 Jose E. Marchesi */

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
#include <stdio.h>
#include <assert.h>

#include "pvm.h"
#include "pkl-gen.h"

/* The following macro is used in the functions below in order to
   append PVM values to a program.  */

#define pvm_append_val_parameter(program,val)                           \
  do                                                                    \
    {                                                                   \
     pvm_append_unsigned_literal_parameter ((program),                  \
                                            (jitter_uint) (val));       \
    } while (0)

/* Forward declaration.  */
static int pkl_gen_1 (pkl_ast_node ast, pvm_program program,
                      size_t *label);

static int
pkl_gen_integer (pkl_ast_node ast,
                 pvm_program program,
                 size_t *label)
{
  pkl_ast_node type;
  pvm_val val;

  type = PKL_AST_TYPE (ast);
  assert (type != NULL && PKL_AST_TYPE_INTEGRAL (type));

  if (PKL_AST_TYPE_SIZE (type) == 64)
    {
      if (PKL_AST_TYPE_SIGNED (type))
        val = pvm_make_ulong (PKL_AST_INTEGER_VALUE (ast));
      else
        val = pvm_make_long (PKL_AST_INTEGER_VALUE (ast));
    }
  else
    {
      if (PKL_AST_TYPE_SIGNED (type))
        val = pvm_make_int (PKL_AST_INTEGER_VALUE (ast));
      else
        val = pvm_make_uint (PKL_AST_INTEGER_VALUE (ast));
    }
    
  PVM_APPEND_INSTRUCTION (program, push);
  pvm_append_val_parameter (program, val);

  return 1;
}

static int
pkl_gen_string (pkl_ast_node ast,
                pvm_program program,
                size_t *label)
{
  pvm_val val;

  val = pvm_make_string (xstrdup (PKL_AST_STRING_POINTER (ast)));

  PVM_APPEND_INSTRUCTION (program, push);
  pvm_append_val_parameter (program, val);

  return 1;
}

static int
pkl_gen_op (pkl_ast_node ast,
            pvm_program program,
            size_t *label,
            enum pkl_ast_op what)
{
  pvm_val masku8 = pvm_make_uint (0xff);
  pvm_val maski8 = pvm_make_int (0xff);
  pvm_val masku16 = pvm_make_uint (0xffff);
  pvm_val maski16 = pvm_make_int (0xffff);

#define PVM_APPEND_ARITH_INSTRUCTION(what,suffix)       \
                                                        \
  if (what == PKL_AST_OP_DIV || what == PKL_AST_OP_MOD) \
    {                                                   \
      PVM_APPEND_INSTRUCTION (program, bz);             \
      pvm_append_symbolic_label_parameter (program,     \
                                           "Ldivzero"); \
    }                                                   \
                                                        \
  switch ((what))                                       \
    {                                                   \
    case PKL_AST_OP_NEG:                                \
      PVM_APPEND_INSTRUCTION (program, neg##suffix);    \
      break;                                            \
    case PKL_AST_OP_ADD:                                \
      PVM_APPEND_INSTRUCTION (program, add##suffix);    \
      break;                                            \
    case PKL_AST_OP_SUB:                                \
      PVM_APPEND_INSTRUCTION (program, sub##suffix);    \
      break;                                            \
    case PKL_AST_OP_MUL:                                \
      PVM_APPEND_INSTRUCTION (program, mul##suffix);    \
      break;                                            \
    case PKL_AST_OP_DIV:                                \
      PVM_APPEND_INSTRUCTION (program, div##suffix);    \
      break;                                            \
    case PKL_AST_OP_MOD:                                \
      PVM_APPEND_INSTRUCTION (program, mod##suffix);    \
      break;                                            \
    case PKL_AST_OP_BAND:                               \
      PVM_APPEND_INSTRUCTION (program, band##suffix);   \
      break;                                            \
    case PKL_AST_OP_IOR:                                \
      PVM_APPEND_INSTRUCTION (program, bor##suffix);    \
      break;                                            \
    case PKL_AST_OP_XOR:                                \
      PVM_APPEND_INSTRUCTION (program, bxor##suffix);   \
      break;                                            \
    case PKL_AST_OP_BNOT:                               \
      PVM_APPEND_INSTRUCTION (program, bnot##suffix);   \
      break;                                            \
    case PKL_AST_OP_SL:                                 \
      PVM_APPEND_INSTRUCTION (program, bsl##suffix);    \
      break;                                            \
    case PKL_AST_OP_SR:                                 \
      PVM_APPEND_INSTRUCTION (program, bsr##suffix);    \
      break;                                            \
    case PKL_AST_OP_EQ:                                 \
      PVM_APPEND_INSTRUCTION (program, eq##suffix);     \
      break;                                            \
    case PKL_AST_OP_NE:                                 \
      PVM_APPEND_INSTRUCTION (program, ne##suffix);     \
      break;                                            \
    case PKL_AST_OP_LT:                                 \
      PVM_APPEND_INSTRUCTION (program, lt##suffix);     \
      break;                                            \
    case PKL_AST_OP_LE:                                 \
      PVM_APPEND_INSTRUCTION (program, le##suffix);     \
      break;                                            \
    case PKL_AST_OP_GT:                                 \
      PVM_APPEND_INSTRUCTION (program, gt##suffix);     \
      break;                                            \
    case PKL_AST_OP_GE:                                 \
      PVM_APPEND_INSTRUCTION (program, ge##suffix);     \
      break;                                            \
    default:                                            \
      break;                                            \
     }

  pkl_ast_node type = PKL_AST_TYPE (ast);
  
  if (PKL_AST_TYPE_INTEGRAL (type))
    {
      switch (PKL_AST_TYPE_SIZE (type))
        {
        case 8:
          if (PKL_AST_TYPE_SIGNED (type))
            {
              PVM_APPEND_ARITH_INSTRUCTION (what, i);
              
              PVM_APPEND_INSTRUCTION (program, push);
              pvm_append_val_parameter (program, maski8);
              PVM_APPEND_INSTRUCTION (program, bandiu);
            }
          else
            {
              PVM_APPEND_ARITH_INSTRUCTION (what, iu);
              
              PVM_APPEND_INSTRUCTION (program, push);
              pvm_append_val_parameter (program, masku8);
              PVM_APPEND_INSTRUCTION (program, bandiu);
            }
          break;

        case 16:
          if (PKL_AST_TYPE_SIGNED (type))
            {
              PVM_APPEND_ARITH_INSTRUCTION (what, i);
              
              PVM_APPEND_INSTRUCTION (program, push);
              pvm_append_val_parameter (program, maski16);
              PVM_APPEND_INSTRUCTION (program, bandiu);
            }
          else
            {
              PVM_APPEND_ARITH_INSTRUCTION (what, iu);
              
              PVM_APPEND_INSTRUCTION (program, push);
              pvm_append_val_parameter (program, masku16);
              PVM_APPEND_INSTRUCTION (program, bandiu);
            }
          break;

        case 32:
          if (PKL_AST_TYPE_SIGNED (type))
            {
              PVM_APPEND_ARITH_INSTRUCTION (what, i);
            }
          else
            {
              PVM_APPEND_ARITH_INSTRUCTION (what, iu);
            }
          break;

        case 64:
          if (PKL_AST_TYPE_SIGNED (type))
            {
              PVM_APPEND_ARITH_INSTRUCTION (what, l);
            }
          else
            {
              PVM_APPEND_ARITH_INSTRUCTION (what, lu);
            }
          break;

        default:
          assert (0);
          break;
        }
    }
  else if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_STRING)
    {
      switch (what)
        {
        case PKL_AST_OP_ADD:
          PVM_APPEND_INSTRUCTION (program, sconc);
          break;
        case PKL_AST_OP_EQ:
          PVM_APPEND_INSTRUCTION (program, eqs);
          break;
        case PKL_AST_OP_NE:
          PVM_APPEND_INSTRUCTION (program, nes);
          break;
        case PKL_AST_OP_LT:
          PVM_APPEND_INSTRUCTION (program, lts);
          break;
        case PKL_AST_OP_LE:
          PVM_APPEND_INSTRUCTION (program, les);
          break;
        case PKL_AST_OP_GT:
          PVM_APPEND_INSTRUCTION (program, gts);
          break;
        case PKL_AST_OP_GE:
          PVM_APPEND_INSTRUCTION (program, ges);
          break;
        default:
          assert (0);
          break;
        }
    }

  return 1;
}

static int
pkl_gen_op_int (pkl_ast_node ast,
                pvm_program program,
                size_t *label,
                enum pkl_ast_op what)
{
  pkl_ast_node type = PKL_AST_TYPE (ast);

  if (PKL_AST_TYPE_INTEGRAL (type)
      && PKL_AST_TYPE_SIZE (type) == 32)
    {
      if (what == PKL_AST_OP_AND)
        PVM_APPEND_INSTRUCTION (program, and);
      else if (what == PKL_AST_OP_OR)
        PVM_APPEND_INSTRUCTION (program, or);
      else if (what == PKL_AST_OP_NOT)
        PVM_APPEND_INSTRUCTION (program, not);
      else
        assert (0);
    }

  return 1;
}

static int
pkl_gen_exp (pkl_ast_node ast,
             pvm_program program,
             size_t *label)
{
  size_t i;
        
  /* Generate operators.  */
  for (i = 0; i < PKL_AST_EXP_NUMOPS (ast); ++i)
    {
      if (!pkl_gen_1 (PKL_AST_EXP_OPERAND (ast, i),
                      program, label))
        return 0;
    }

  switch (PKL_AST_EXP_CODE (ast))
    {
    case PKL_AST_OP_ADD:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_ADD);
      break;
    case PKL_AST_OP_SUB:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_SUB);
      break;
    case PKL_AST_OP_MUL:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_MUL);
      break;
    case PKL_AST_OP_DIV:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_DIV);
      break;
    case PKL_AST_OP_MOD:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_MOD);
      break;
    case PKL_AST_OP_NEG:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_NEG);
      break;

    case PKL_AST_OP_AND:
      return pkl_gen_op_int (ast, program, label, PKL_AST_OP_AND);
      break;
    case PKL_AST_OP_OR:
      return pkl_gen_op_int (ast, program, label, PKL_AST_OP_OR);
      break;
    case PKL_AST_OP_NOT:
      return pkl_gen_op_int (ast, program, label, PKL_AST_OP_NOT);
      break;

    case PKL_AST_OP_BAND:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_BAND);
      break;
    case PKL_AST_OP_IOR:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_IOR);
      break;
    case PKL_AST_OP_XOR:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_XOR);
      break;
    case PKL_AST_OP_BNOT:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_BNOT);
      break;
    case PKL_AST_OP_SL:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_SL);
      break;
    case PKL_AST_OP_SR:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_SR);
      break;

    case PKL_AST_OP_EQ:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_EQ);
    case PKL_AST_OP_NE:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_NE);
    case PKL_AST_OP_LT:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_LT);
    case PKL_AST_OP_LE:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_LE);
    case PKL_AST_OP_GT:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_GT);
    case PKL_AST_OP_GE:
      return pkl_gen_op (ast, program, label, PKL_AST_OP_GE);
      
    default:
      fprintf (stderr, "gen: unhandled expression code %d\n",
               PKL_AST_EXP_CODE (ast));
      return 0;
    }

  return 1;
}

static int
pkl_gen_cast (pkl_ast_node ast,
              pvm_program program,
              size_t *label)
{
  pkl_ast_node to_type;
  pkl_ast_node from_type;

  pkl_gen_1 (PKL_AST_CAST_EXP (ast),
             program,
             label);
  
  to_type = PKL_AST_TYPE (ast);
  from_type = PKL_AST_TYPE (PKL_AST_CAST_EXP (ast));

#define SIGN_EXTEND                                                     \
  do                                                                    \
    {                                                                   \
      int shift = 32 - to_type_size;                                    \
      if (shift > 0)                                                    \
        {                                                               \
          /* Extend sign when casting to signed 8-bit and */            \
          /* 16-bit values.  */                                         \
          pvm_val val = pvm_make_int (shift);                           \
                                                                        \
          PVM_APPEND_INSTRUCTION (program, push);                       \
          pvm_append_val_parameter (program, val);                      \
          PVM_APPEND_INSTRUCTION (program, bsli);                       \
          PVM_APPEND_INSTRUCTION (program, push);                       \
          pvm_append_val_parameter (program, val);                      \
          PVM_APPEND_INSTRUCTION (program, bsri);                       \
        }                                                               \
    } while (0)
  
  if (PKL_AST_TYPE_INTEGRAL (from_type)
      && PKL_AST_TYPE_INTEGRAL (to_type))
    {
      size_t from_type_size = PKL_AST_TYPE_SIZE (from_type);
      int from_type_sign = PKL_AST_TYPE_SIGNED (from_type);
      
      size_t to_type_size = PKL_AST_TYPE_SIZE (to_type);
      int to_type_sign = PKL_AST_TYPE_SIGNED (to_type);

      if (from_type_size == to_type_size)
        {
          if (from_type_sign == to_type_sign)
            /* Wheee, nothing to do.  */
            return 1;

          if (from_type_size == 64)
            {
              if (to_type_sign)
                /* uint64 -> int64 */
                PVM_APPEND_INSTRUCTION (program, lutol);
              else
                /* int64 -> uint64 */
                PVM_APPEND_INSTRUCTION (program, ltolu);
            }
          else
            {
              if (to_type_sign)
                {
                  /* uint32 -> int32
                     uint16 -> int16
                     uint8  -> int8 */
                  PVM_APPEND_INSTRUCTION (program, iutoi);
                  SIGN_EXTEND;
                }
              else
                /* int32 -> uint32
                   int16 -> uint16
                   int8  -> uint8 */
                PVM_APPEND_INSTRUCTION (program, itoiu);
            }
        }
      else /* from_type_size != to_type_size */
        {
          if (from_type_size == 64)
            {
              if (from_type_sign && to_type_sign)
                /* int64 -> int{32,16,8} */
                PVM_APPEND_INSTRUCTION (program, ltoi);
              else if (from_type_sign && !to_type_sign)
                /* int64 -> uint{32,16,8} */
                PVM_APPEND_INSTRUCTION (program, ltoiu);
              else if (!from_type_sign && to_type_sign)
                /* uint64 -> int{32,16,8} */
                PVM_APPEND_INSTRUCTION (program, lutoi);
              else
                /* uint64 -> uint{32,16,8} */
                PVM_APPEND_INSTRUCTION (program, lutoiu);

              if (to_type_sign)
                SIGN_EXTEND;
            }
          else
            {
              if (to_type_size == 64)
                {
                  if (from_type_sign && to_type_sign)
                    /* int{32,16,8} -> int64 */
                    PVM_APPEND_INSTRUCTION (program, itol);
                  else if (from_type_sign && !to_type_sign)
                    /* int{32,16,8} -> uint64 */
                    PVM_APPEND_INSTRUCTION (program, itolu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint{32,16,8} -> int64 */
                    PVM_APPEND_INSTRUCTION (program, iutol);
                  else
                    /* uint{32,16,8} -> uint64 */
                    PVM_APPEND_INSTRUCTION (program, iutolu);
                }
              else
                {
                  /* [u]int{32,16,8} -> [u]int{32,16,8} */
                  if (to_type_sign)
                    SIGN_EXTEND;
                }
            }
        }
    }

#undef SIGN_EXTEND
  
  return 1;
}

static int
pkl_gen_1 (pkl_ast_node ast,
           pvm_program program,
           size_t *label)
{
  pkl_ast_node tmp;
  
  if (ast == NULL)
    goto success;

  switch (PKL_AST_CODE (ast))
    {
    case PKL_AST_PROGRAM:
      for (tmp = PKL_AST_PROGRAM_ELEMS (ast); tmp; tmp = PKL_AST_CHAIN (tmp))
        if (!pkl_gen_1 (tmp, program, label))
          goto error;
      break;
      
    case PKL_AST_INTEGER:
      if (!pkl_gen_integer (ast, program, label))
        goto error;     
      break;
    case PKL_AST_STRING:
      if (!pkl_gen_string (ast, program, label))
        goto error;
      break;

    case PKL_AST_EXP:
      if (!pkl_gen_exp (ast, program, label))
        goto error;
      break;

    case PKL_AST_CAST:
      if (!pkl_gen_cast (ast, program, label))
        goto error;
      break;

    case PKL_AST_COND_EXP:
    default:
      fprintf (stderr, "gen: unknown AST node.\n");
      goto error;
    }

 success:
  return 1;
  
 error:
  return 0;
}

int
pkl_gen (pvm_program *prog, pkl_ast ast)
{
  struct pvm_program *program;
  size_t label;

  label = 0;
  program = pvm_make_program ();
  if (program == NULL)
    goto error;

  /* Standard prologue.  */
  {
    pvm_val val;
    
    PVM_APPEND_INSTRUCTION (program, ba);
    pvm_append_symbolic_label_parameter (program, "Lstart");

    pvm_append_symbolic_label (program, "Ldivzero");

    val = pvm_make_int (PVM_EXIT_EDIVZ);
    PVM_APPEND_INSTRUCTION (program, push);
    pvm_append_val_parameter (program, val);

    PVM_APPEND_INSTRUCTION (program, ba);
    pvm_append_symbolic_label_parameter (program, "Lexit");
    
    pvm_append_symbolic_label (program, "Lerror");
    
    val = pvm_make_int (PVM_EXIT_ERROR);
    PVM_APPEND_INSTRUCTION (program, push);
    pvm_append_val_parameter (program, val);
        
    pvm_append_symbolic_label (program, "Lexit");
    PVM_APPEND_INSTRUCTION (program, exit);

    pvm_append_symbolic_label (program, "Lstart");
  }

  if (!pkl_gen_1 (ast->ast, program, &label))
    {
      /* XXX: handle code generation errors.  */
      pvm_destroy_program (program);
      goto error;
    }

  /* Standard epilogue.  */
  {
    pvm_val val;

    /* The exit status is OK.  */
    val = pvm_make_int (PVM_EXIT_OK);
    PVM_APPEND_INSTRUCTION (program, push);
    pvm_append_val_parameter (program, val);
    
    PVM_APPEND_INSTRUCTION (program, ba);
    pvm_append_symbolic_label_parameter (program, "Lexit");
  }

  *prog = program;
  return 1;
  
 error:
  return 0;
}
