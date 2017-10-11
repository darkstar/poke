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


/* The following macros are used in the functions below in order to
   append PVM values to a program.  */

#define pvm_append_val_parameter(program,val)                           \
  do                                                                    \
    {                                                                   \
     pvm_append_unsigned_literal_parameter ((program),                  \
                                            (jitter_uint) (val));       \
    } while (0)

#define pvm_append_type_parameter(program,type)                         \
  do                                                                    \
    {                                                                   \
     pvm_append_unsigned_literal_parameter ((program),                  \
                                            (jitter_uint) (type));      \
    } while (0)

/* Forward declaration.  */
static int pkl_gen_1 (pkl_ast_node ast,
                      pvm_program program,
                      size_t *label);

static int
pkl_gen_integer (pkl_ast_node ast,
                 pvm_program program,
                 size_t *label)
{
  pkl_ast_node type;
  pvm_val val;

  type = PKL_AST_TYPE (ast);
  assert (type != NULL);
  
  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_CHAR:
    case PKL_TYPE_BYTE:
    case PKL_TYPE_UINT8:
    case PKL_TYPE_UINT16:
    case PKL_TYPE_UINT32:
      val = pvm_make_uint (PKL_AST_INTEGER_VALUE (ast));
      break;
    case PKL_TYPE_INT8:
    case PKL_TYPE_INT16:
    case PKL_TYPE_INT32:
    case PKL_TYPE_INT:
      val = pvm_make_int (PKL_AST_INTEGER_VALUE (ast));
      break;
    case PKL_TYPE_UINT64:
      val = pvm_make_ulong (PKL_AST_INTEGER_VALUE (ast));
      break;
    case PKL_TYPE_LONG:
    case PKL_TYPE_INT64:
      val = pvm_make_long (PKL_AST_INTEGER_VALUE (ast));
      break;
    case PKL_TYPE_STRING:
    default:
      assert (0);
      break;
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
pkl_gen_op_and (pkl_ast_node ast,
               pvm_program program,
               size_t *label)
{
  switch (PKL_AST_TYPE_CODE (PKL_AST_TYPE (ast)))
    {
    case PKL_TYPE_INT:
    case PKL_TYPE_INT32:
      PVM_APPEND_INSTRUCTION (program, and);
      break;

    default:
      assert (0);
      break;
    }

  return 1;
}

static int
pkl_gen_op_or (pkl_ast_node ast,
               pvm_program program,
               size_t *label)
{
  switch (PKL_AST_TYPE_CODE (PKL_AST_TYPE (ast)))
    {
    case PKL_TYPE_INT:
    case PKL_TYPE_INT32:
      PVM_APPEND_INSTRUCTION (program, or);
      break;

    default:
      assert (0);
      break;
    }

  return 1;
}

static int
pkl_gen_op_not (pkl_ast_node ast,
               pvm_program program,
               size_t *label)
{
  switch (PKL_AST_TYPE_CODE (PKL_AST_TYPE (ast)))
    {
    case PKL_TYPE_INT:
    case PKL_TYPE_INT32:
      PVM_APPEND_INSTRUCTION (program, not);
      break;

    default:
      assert (0);
      break;
    }

  return 1;
}



static int
pkl_gen_op_add (pkl_ast_node ast,
                pvm_program program,
                size_t *label)
{
  pvm_val masku8 = pvm_make_uint (0xff);
  pvm_val maski8 = pvm_make_int (0xff);
  pvm_val masku16 = pvm_make_uint (0xffff);
  pvm_val maski16 = pvm_make_int (0xffff);

  
  switch (PKL_AST_TYPE_CODE (PKL_AST_TYPE (ast)))
    {
    case PKL_TYPE_CHAR:
    case PKL_TYPE_BYTE:
    case PKL_TYPE_UINT8:
      PVM_APPEND_INSTRUCTION (program, addiu);

      PVM_APPEND_INSTRUCTION (program, push);
      pvm_append_val_parameter (program, masku8);
      PVM_APPEND_INSTRUCTION (program, bandiu);
      break;

    case PKL_TYPE_INT8:
      PVM_APPEND_INSTRUCTION (program, addi);

      PVM_APPEND_INSTRUCTION (program, push);
      pvm_append_val_parameter (program, maski8);
      PVM_APPEND_INSTRUCTION (program, bandiu);
      break;
      
    case PKL_TYPE_UINT16:
      PVM_APPEND_INSTRUCTION (program, addiu);

      PVM_APPEND_INSTRUCTION (program, push);
      pvm_append_val_parameter (program, masku16);
      PVM_APPEND_INSTRUCTION (program, bandiu);
      break;

    case PKL_TYPE_SHORT:
    case PKL_TYPE_INT16:
      PVM_APPEND_INSTRUCTION (program, addi);

      PVM_APPEND_INSTRUCTION (program, push);
      pvm_append_val_parameter (program, maski16);
      PVM_APPEND_INSTRUCTION (program, bandiu);
      break;

    case PKL_TYPE_UINT32:
      PVM_APPEND_INSTRUCTION (program, addiu);
      break;

    case PKL_TYPE_INT:
    case PKL_TYPE_INT32:
      PVM_APPEND_INSTRUCTION (program, addi);
      break;

    case PKL_TYPE_UINT64:
      PVM_APPEND_INSTRUCTION (program, addlu);
      break;

    case PKL_TYPE_LONG:
    case PKL_TYPE_INT64:
      PVM_APPEND_INSTRUCTION (program, addl);
      break;

    case PKL_TYPE_STRING:
      PVM_APPEND_INSTRUCTION (program, sconc);
      break;

    default:
      assert (0);
      break;
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

#define GEN_BINARY_OP(OP,T1,T2)                                         \
  do                                                                    \
    {                                                                   \
      PVM_APPEND_INSTRUCTION (program, bnt);                            \
      pvm_append_type_parameter (program, (T1));                        \
                                                                        \
      pvm_append_symbolic_label_parameter (program, "Lerror");          \
                                                                        \
      PVM_APPEND_INSTRUCTION (program, bntut);                          \
      pvm_append_type_parameter (program, (T2));                        \
                                                                        \
      pvm_append_symbolic_label_parameter (program, "Lerror");          \
                                                                        \
      PVM_APPEND_INSTRUCTION (program, OP);                             \
    } while (0)
  
#define GEN_UNARY_OP_IL(OP)                                             \
  do                                                                    \
    {                                                                   \
      char label_0[100], label_1[100];                                  \
      sprintf (label_0, "L%li", (*label)++);                            \
                                                                        \
      PVM_APPEND_INSTRUCTION (program, bnt);                            \
      pvm_append_type_parameter (program, PVM_VAL_TAG_INT);             \
      pvm_append_symbolic_label_parameter (program, label_0);           \
                                                                        \
      PVM_APPEND_INSTRUCTION (program, OP##i);                          \
                                                                        \
      PVM_APPEND_INSTRUCTION (program, ba);                             \
      pvm_append_symbolic_label_parameter (program, label_1);           \
      pvm_append_symbolic_label (program, label_0);                     \
                                                                        \
      PVM_APPEND_INSTRUCTION (program, bnt);                            \
      pvm_append_type_parameter (program, PVM_VAL_TAG_LONG);            \
      pvm_append_symbolic_label_parameter (program, "Lerror");          \
                                                                        \
      PVM_APPEND_INSTRUCTION (program, OP##l);                          \
                                                                        \
      pvm_append_symbolic_label (program, label_1);                     \
    } while (0)

#define GEN_UNARY_OP_I(OP)                                              \
  do                                                                    \
    {                                                                   \
      PVM_APPEND_INSTRUCTION (program, bnt);                            \
      pvm_append_type_parameter (program, PVM_VAL_TAG_INT);             \
      pvm_append_symbolic_label_parameter (program, "Lerror");          \
                                                                        \
      PVM_APPEND_INSTRUCTION (program, OP);                             \
    } while (0)

#define GEN_BINARY_OP_II(OP)                    \
  GEN_BINARY_OP (OP, PVM_VAL_TAG_LONG, PVM_VAL_TAG_LONG)


  switch (PKL_AST_EXP_CODE (ast))
    {
    case PKL_AST_OP_AND:
      return pkl_gen_op_and (ast, program, label);
      break;
    case PKL_AST_OP_OR:
      return pkl_gen_op_or (ast, program, label);
      break;
    case PKL_AST_OP_NOT:
      return pkl_gen_op_not (ast, program, label);
      break;

    case PKL_AST_OP_BAND: GEN_BINARY_OP_II (bandl); break;
    case PKL_AST_OP_IOR:  GEN_BINARY_OP_II (borl); break;
    case PKL_AST_OP_XOR:  GEN_BINARY_OP_II (bxorl); break;

    case PKL_AST_OP_EQ:   GEN_BINARY_OP_II (eql); break;
    case PKL_AST_OP_NE:   GEN_BINARY_OP_II (nel); break;
    case PKL_AST_OP_SL:   GEN_BINARY_OP_II (bsll); break;
    case PKL_AST_OP_SR:   GEN_BINARY_OP_II (bsrl); break;
    case PKL_AST_OP_ADD:
      return pkl_gen_op_add (ast, program, label);
      break;
    case PKL_AST_OP_SUB:  GEN_BINARY_OP_II (subl); break;
    case PKL_AST_OP_MUL:  GEN_BINARY_OP_II (mull); break;
    case PKL_AST_OP_DIV:  GEN_BINARY_OP_II (divl); break;
    case PKL_AST_OP_MOD:  GEN_BINARY_OP_II (modl); break;
    case PKL_AST_OP_LT:   GEN_BINARY_OP_II (ltl); break;
    case PKL_AST_OP_GT:   GEN_BINARY_OP_II (gtl); break;
    case PKL_AST_OP_LE:   GEN_BINARY_OP_II (lel); break;
    case PKL_AST_OP_GE:   GEN_BINARY_OP_II (gel); break;
      
    case PKL_AST_OP_NEG:     GEN_UNARY_OP_IL (neg); break;
    case PKL_AST_OP_BNOT:    GEN_UNARY_OP_IL (bnot); break;

      
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
  /* INT <- INT
       Sign extension or zero extension.
     TUPLE <- TUPLE
       Reordering. */
  
  return 0; 
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
    
    pvm_append_symbolic_label (program, "Lerror");
    
    /* The exit status is ERROR.  */
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
