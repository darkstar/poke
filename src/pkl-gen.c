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

#define pvm_append_pointer_parameter \
  pvm_append_unsigned_literal_parameter
#define pvm_pointer \
  jitter_uint

/* Forward declaration.  */
static int pkl_gen_1 (pkl_ast_node ast,
                      pvm_program program,
                      size_t *label);

static int
pkl_gen_integer (pkl_ast_node ast,
                 pvm_program program,
                 size_t *label)
{
  pvm_stack s;
  
  s = pvm_stack_new ();
  PVM_STACK_TYPE (s) = PVM_STACK_INT;
  PVM_STACK_INTEGER (s) = PKL_AST_INTEGER_VALUE (ast);

  /* PUSH int_cst */
  PVM_APPEND_INSTRUCTION (program, push);
  pvm_append_pointer_parameter (program, (pvm_pointer) s);

  return 1;
}

static int
pkl_gen_string (pkl_ast_node ast,
                pvm_program program,
                size_t *label)
{
  pvm_stack s;
  
  s = pvm_stack_new ();
  PVM_STACK_TYPE (s) = PVM_STACK_STR;
  PVM_STACK_STRING (s) = xstrdup (PKL_AST_STRING_POINTER (ast));
  
  /* PUSH str_cst */
  PVM_APPEND_INSTRUCTION (program, push);
  pvm_append_pointer_parameter (program, (pvm_pointer) s);

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
    do                                                                  \
      {                                                                 \
        PVM_APPEND_INSTRUCTION (program, bnt);                          \
        pvm_append_unsigned_literal_parameter (program,                 \
                                               (jitter_uint) (T1));     \
        pvm_append_symbolic_label_parameter (program, "Lerror");         \
                                                                        \
        PVM_APPEND_INSTRUCTION (program, bntut);                        \
        pvm_append_unsigned_literal_parameter (program,                 \
                                               (jitter_uint) (T2));     \
        pvm_append_symbolic_label_parameter (program, "Lerror");         \
                                                                        \
        PVM_APPEND_INSTRUCTION (program, OP);                           \
      } while (0)
  
#define GEN_UNARY_OP(OP,T)                                              \
    do                                                                  \
      {                                                                 \
        PVM_APPEND_INSTRUCTION (program, bnt);                          \
        pvm_append_unsigned_literal_parameter (program,                 \
                                               (jitter_uint) (T));      \
        pvm_append_symbolic_label_parameter (program, "Lerror");         \
                                                                        \
        PVM_APPEND_INSTRUCTION (program, OP);                           \
      } while (0)

#define GEN_BINARY_OP_II(OP)                                            \
    GEN_BINARY_OP (OP, PVM_STACK_INT, PVM_STACK_INT)

#define GEN_UNARY_OP_I(OP)                      \
    GEN_UNARY_OP (OP, PVM_STACK_INT)
      
  switch (PKL_AST_EXP_CODE (ast))
    {
    case PKL_AST_OP_OR:   GEN_BINARY_OP_II (or); break;
    case PKL_AST_OP_BAND: GEN_BINARY_OP_II (band); break;
    case PKL_AST_OP_IOR:  GEN_BINARY_OP_II (bior); break;
    case PKL_AST_OP_XOR:  GEN_BINARY_OP_II (bxor); break;
    case PKL_AST_OP_AND:  GEN_BINARY_OP_II (and); break;
    case PKL_AST_OP_EQ:   GEN_BINARY_OP_II (ieq); break;
    case PKL_AST_OP_NE:   GEN_BINARY_OP_II (ine); break;
    case PKL_AST_OP_SL:   GEN_BINARY_OP_II (bsl); break;
    case PKL_AST_OP_SR:   GEN_BINARY_OP_II (bsr); break;
    case PKL_AST_OP_SUB:  GEN_BINARY_OP_II (sub); break;
    case PKL_AST_OP_MUL:  GEN_BINARY_OP_II (mul); break;
    case PKL_AST_OP_DIV:  GEN_BINARY_OP_II (div); break;
    case PKL_AST_OP_MOD:  GEN_BINARY_OP_II (mod); break;
    case PKL_AST_OP_LT:   GEN_BINARY_OP_II (ilt); break;
    case PKL_AST_OP_GT:   GEN_BINARY_OP_II (igt); break;
    case PKL_AST_OP_LE:   GEN_BINARY_OP_II (ile); break;
    case PKL_AST_OP_GE:   GEN_BINARY_OP_II (ige); break;
      
    case PKL_AST_OP_PREINC:  GEN_UNARY_OP_I (preinc); break;
    case PKL_AST_OP_PREDEC:  GEN_UNARY_OP_I (predec); break;
    case PKL_AST_OP_POS:     GEN_UNARY_OP_I (pos); break;
    case PKL_AST_OP_NEG:     GEN_UNARY_OP_I (neg); break;
    case PKL_AST_OP_BNOT:    GEN_UNARY_OP_I (bnot); break;
    case PKL_AST_OP_NOT:     GEN_UNARY_OP_I (not); break;
      
    case PKL_AST_OP_ADD:
      {
        char label_0[100], label_1[100];
        
        sprintf (label_0, "L%li", (*label)++);
        sprintf (label_1, "L%li", (*label)++);
        
        PVM_APPEND_INSTRUCTION (program, bntut);
        pvm_append_unsigned_literal_parameter (program,
                                               (jitter_uint) (PVM_STACK_INT));
        pvm_append_symbolic_label_parameter (program, label_0);
        
        /* Arithmetic addition.  */
        PVM_APPEND_INSTRUCTION (program, bnt);
        pvm_append_unsigned_literal_parameter (program,
                                               (jitter_uint) (PVM_STACK_INT));
        pvm_append_symbolic_label_parameter (program, "Lerror");
        PVM_APPEND_INSTRUCTION (program, add);
        
        PVM_APPEND_INSTRUCTION (program, ba);
        pvm_append_symbolic_label_parameter (program, label_1);
        pvm_append_symbolic_label (program, label_0);
        
        /* String concatenation.  */
        PVM_APPEND_INSTRUCTION (program, bnt);
        pvm_append_unsigned_literal_parameter (program,
                                               (jitter_uint) (PVM_STACK_STR));
        pvm_append_symbolic_label_parameter (program, "Lerror");
        PVM_APPEND_INSTRUCTION (program, sconc);
        
        pvm_append_symbolic_label (program, label_1);
        break;
      }

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
    pvm_stack s;
    
    PVM_APPEND_INSTRUCTION (program, ba);
    pvm_append_symbolic_label_parameter (program, "Lstart");
    
    pvm_append_symbolic_label (program, "Lerror");
    
    /* The exit status is ERROR.  */
    s = pvm_stack_new ();
    PVM_STACK_TYPE (s) = PVM_STACK_INT;
    PVM_STACK_INTEGER (s) = PVM_EXIT_ERROR;
    PVM_APPEND_INSTRUCTION (program, push);
    pvm_append_pointer_parameter (program, (pvm_pointer) s);
    
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
    pvm_stack s;

    /* The exit status is OK.  */
    s = pvm_stack_new ();
    PVM_STACK_TYPE (s) = PVM_STACK_INT;
    PVM_STACK_INTEGER (s) = PVM_EXIT_OK;
    PVM_APPEND_INSTRUCTION (program, push);
    pvm_append_pointer_parameter (program, (pvm_pointer) s);
    
    PVM_APPEND_INSTRUCTION (program, ba);
    pvm_append_symbolic_label_parameter (program, "Lexit");
  }

  *prog = program;
  return 1;
  
 error:
  return 0;
}
