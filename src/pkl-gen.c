/* pkl-gen.c - Code generation phase for the poke compiler.  */

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
#include <stdio.h>
#include <assert.h>
#include <jitter/jitter.h>

#include "pkl.h"
#include "pkl-gen.h"
#include "pkl-ast.h"
#include "pkl-pass.h"
#include "pkl-asm.h"
#include "pvm.h"

/* The following macros are used in the rules below, to reduce
   verbosity.  */

#define PKL_GEN_PAYLOAD ((pkl_gen_payload) PKL_PASS_PAYLOAD)

#define PKL_GEN_AN_ASM(ASM)                             \
  (PKL_GEN_PAYLOAD->ASM[PKL_GEN_PAYLOAD->cur_##ASM])

#define PKL_GEN_ASM  PKL_GEN_AN_ASM(pasm)
#define PKL_GEN_ASM2 PKL_GEN_AN_ASM(pasm2)

#define PKL_GEN_PUSH_AN_ASM(ASM,new_pasm)                               \
  do                                                                    \
    {                                                                   \
      assert (PKL_GEN_PAYLOAD->cur_##ASM < PKL_GEN_MAX_PASM);           \
      PKL_GEN_PAYLOAD->ASM[++(PKL_GEN_PAYLOAD->cur_##ASM)] = (new_pasm); \
    }                                                                   \
  while (0)
  
#define PKL_GEN_PUSH_ASM(new_pasm)  PKL_GEN_PUSH_AN_ASM(pasm,new_pasm)
#define PKL_GEN_PUSH_ASM2(new_pasm) PKL_GEN_PUSH_AN_ASM(pasm2,new_pasm)

#define PKL_GEN_POP_AN_ASM(ASM)                  \
  do                                             \
    {                                            \
      assert (PKL_GEN_PAYLOAD->cur_##ASM > 0);   \
      PKL_GEN_PAYLOAD->cur_##ASM -= 1;           \
    }                                            \
  while (0)

#define PKL_GEN_POP_ASM  PKL_GEN_POP_AN_ASM(pasm)
#define PKL_GEN_POP_ASM2 PKL_GEN_POP_AN_ASM(pasm2)

/*
 * PROGRAM
 * | PROGRAM_ELEM
 * | ...
 *
 * This function initializes the payload and also generates the
 * standard prologue.
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_program)
{
  PKL_GEN_ASM = pkl_asm_new (PKL_PASS_AST,
                             PKL_GEN_PAYLOAD->compiler,
                             1 /* prologue */);
}
PKL_PHASE_END_HANDLER

/*
 * | PROGRAM_ELEM
 * | ...
 * PROGRAM
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_program)
{
  /* Make sure there is always some value returned in the stack, since
     that is expected in the PVM.  */
  if (!pkl_compiling_expression_p (PKL_GEN_PAYLOAD->compiler))
    pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, PVM_NULL);
  
  PKL_GEN_PAYLOAD->program = pkl_asm_finish (PKL_GEN_ASM,
                                             1 /* prologue */);
}
PKL_PHASE_END_HANDLER

/*
 * DECL
 * | INITIAL
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_decl)
{
  pkl_ast_node decl = PKL_PASS_NODE;
  pkl_ast_node initial = PKL_AST_DECL_INITIAL (decl);

  switch (PKL_AST_DECL_KIND (decl))
    {
    case PKL_AST_DECL_KIND_TYPE:
      if (PKL_AST_TYPE_CODE (initial) == PKL_TYPE_STRUCT)
        {
          /* INITIAL is a struct type.  We need to compile two
             functions from it:

             - A mapper function.
             - A constructor function.

             Push assemblers for both functions and continue
             processing the initial.  */

          /* Assembler for the mapper.  */
          PKL_GEN_PUSH_ASM (pkl_asm_new (PKL_PASS_AST,
                                         PKL_GEN_PAYLOAD->compiler,
                                         0 /* prologue */));

          /* Assembler for the constructor.  */
          PKL_GEN_PUSH_ASM2 (pkl_asm_new (PKL_PASS_AST,
                                          PKL_GEN_PAYLOAD->compiler,
                                          0 /* prologue */));

          PKL_GEN_PAYLOAD->in_struct_decl = 1;
        }
      break;
    case PKL_AST_DECL_KIND_FUNC:

      /* INITIAL is either a PKL_AST_FUNC, that will compile into a
         program containing the function code.  Push a new assembler
         to the stack of assemblers in the payload and use it to
         process INITIAL.  */
      PKL_GEN_PUSH_ASM (pkl_asm_new (PKL_PASS_AST,
                                     PKL_GEN_PAYLOAD->compiler,
                                     0 /* prologue */));
      break;
    default:
      break;
    }
}
PKL_PHASE_END_HANDLER

/*
 * | INITIAL
 * DECL
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_decl)
{
  pkl_ast_node decl = PKL_PASS_NODE;
  pkl_ast_node initial = PKL_AST_DECL_INITIAL (decl);

  switch (PKL_AST_DECL_KIND (decl))
    {
    case PKL_AST_DECL_KIND_VAR:
      /* The value is in the stack.  Just register the variable.  */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);
      break;
    case PKL_AST_DECL_KIND_TYPE:
      if (PKL_AST_TYPE_CODE (initial) == PKL_TYPE_STRUCT)
        {
          /* Finish both the struct constructor and struct mapper
             functions.  Note that this should be done in the same
             order than the registration of declarations in the
             compile-time environment (in pkl-tab.y).  */

          pvm_program program;
          pvm_val closure;

          
          /* XXX */
          /*          {
            char *foo = pkl_type_str (initial, 0);
            
            pkl_print_type (stdout,initial, 0);
            printf ("\n%s\n", foo);
            free (foo);
            } */

          /* Finish the struct mapper.  */
          {
            program = pkl_asm_finish (PKL_GEN_ASM,
                                      0 /* epilogue */);
          
            PKL_GEN_POP_ASM;
            pvm_specialize_program (program);
            closure = pvm_make_cls (program);
          
            /*XXX*/
            /* pvm_print_program (stdout, program); */
            
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, closure);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PEC);

            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);
          }
            
          /* Finish the struct constructor.  */
          {
            program = pkl_asm_finish (PKL_GEN_ASM2,
                                      0 /* epilogue */);
            
            PKL_GEN_POP_ASM2;
            pvm_specialize_program (program);
            closure = pvm_make_cls (program);
            
            /*XXX*/
            /* pvm_print_program (stdout, program); */
            
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, closure);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PEC);

            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);
          }

          PKL_GEN_PAYLOAD->in_struct_decl = 0;
        }
      break;
    case PKL_AST_DECL_KIND_FUNC:
      {
        /* At this point the code for the function specification
           INITIAL has been assembled in the current macroassembler.
           Finalize the program and put it in a PVM closure, along
           with the current environment.  */

        pvm_program program = pkl_asm_finish (PKL_GEN_ASM,
                                              0 /* epilogue */);
        pvm_val closure;

        PKL_GEN_POP_ASM;
        pvm_specialize_program (program);
        closure = pvm_make_cls (program);

        /*XXX*/
        /* pvm_print_program (stdout, program); */

        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, closure);
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PEC);
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);
        break;
      }
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

/*
 * VAR
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_var)
{
  pkl_ast_node var = PKL_PASS_NODE;

  if (PKL_PASS_PARENT
      && PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_ASS_STMT)
    {
      /* This is a l-value in an assignment.  Generate nothing, as
         this node is only used as a recipient for the lexical address
         of the variable.  */
      /* XXX: the call to WRITE will most probably belong here, after
         the aref or sref or whatever.  */
    }
  else
    {
      pkl_ast_node var_type = PKL_AST_TYPE (var);

      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR,
                    PKL_AST_VAR_BACK (var), PKL_AST_VAR_OVER (var));

      /* If the value holds a value that could be mapped, then use the
         REMAP instruction.  */
      if (PKL_AST_TYPE_CODE (var_type) == PKL_TYPE_ARRAY
          || PKL_AST_TYPE_CODE (var_type) == PKL_TYPE_STRUCT)
        {
          /* XXX: handle exceptions from the mapper function.  */
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REMAP);
        }
    }
}
PKL_PHASE_END_HANDLER

/*
 * NULL_STMT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_null_stmt)
{
  /* Null is nothing, nada.  */
}
PKL_PHASE_END_HANDLER

/*
 * COMP_STMT
 * | (STMT | DECL)
 * | ...
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_comp_stmt)
{
  pkl_ast_node comp_stmt = PKL_PASS_NODE;

  if (PKL_AST_COMP_STMT_BUILTIN (comp_stmt) == PKL_AST_BUILTIN_NONE)
    {  
      /* Push a frame into the environment.  */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHF);
    }
}
PKL_PHASE_END_HANDLER

/*
 * | (STMT | DECL)
 * | ...
 * COMP_STMT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_comp_stmt)
{
  pkl_ast_node comp_stmt = PKL_PASS_NODE;
  int comp_stmt_builtin
    = PKL_AST_COMP_STMT_BUILTIN (comp_stmt);

  if (comp_stmt_builtin != PKL_AST_BUILTIN_NONE)
    {
      switch (comp_stmt_builtin)
        {
        case PKL_AST_BUILTIN_PRINT:
          {
            /* defun print = (string s) __builtin_print __ */
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 0);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PRINT);
            break;
          }
        default:
            assert (0);
        }
    }
  else
    /* Pop the lexical frame created by the compound statement.  */
    pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPF, 1);
}
PKL_PHASE_END_HANDLER

/*
 * | LVALUE
 * | EXP
 * ASS_STMT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_ass_stmt)
{
  pkl_ast_node ass_stmt = PKL_PASS_NODE;
  pkl_ast_node lvalue = PKL_AST_ASS_STMT_LVALUE (ass_stmt);

  /* At this point the r-value, generated from executing EXP, is in
     the stack.  */
  switch (PKL_AST_CODE (lvalue))
    {
    case PKL_AST_VAR:
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPVAR,
                    PKL_AST_VAR_BACK (lvalue), PKL_AST_VAR_OVER (lvalue));
      break;
    case PKL_AST_ARRAY_REF:
      /* Stack: ARRAY INDEX VAL */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ASET);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_WRITE);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP); /* The array
                                                    value.  */
      break;
    case PKL_AST_STRUCT_REF:
      /* Stack: SCT ID VAL */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SSET);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_WRITE);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP); /* The struct
                                                    value.  */
      break;
    default:
      break;
    }
}
PKL_PHASE_END_HANDLER

/*
 * IF_STMT
 * | EXP
 * | THEN_STMT
 * | [ELSE_STMT]
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_if_stmt)
{
  pkl_ast_node if_stmt = PKL_PASS_NODE;
  pkl_ast_node if_exp = PKL_AST_IF_STMT_EXP (if_stmt);
  pkl_ast_node if_then_stmt = PKL_AST_IF_STMT_THEN_STMT (if_stmt);
  pkl_ast_node if_else_stmt = PKL_AST_IF_STMT_ELSE_STMT (if_stmt);

  pkl_asm_if (PKL_GEN_ASM, if_exp);
  {
    PKL_PASS_SUBPASS (if_exp);
  }
  pkl_asm_then (PKL_GEN_ASM);
  {
    PKL_PASS_SUBPASS (if_then_stmt);
  }
  pkl_asm_else (PKL_GEN_ASM);
  {
    if (if_else_stmt)
      PKL_PASS_SUBPASS (if_else_stmt);
  }
  pkl_asm_endif (PKL_GEN_ASM);

  PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

/*
 * LOOP_STMT
 * | CONDITION
 * | BODY
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_loop_stmt)
{
  pkl_ast_node loop_stmt = PKL_PASS_NODE;
  pkl_ast_node condition = PKL_AST_LOOP_STMT_CONDITION (loop_stmt);
  pkl_ast_node iterator = PKL_AST_LOOP_STMT_ITERATOR (loop_stmt);
  pkl_ast_node container = PKL_AST_LOOP_STMT_CONTAINER (loop_stmt);
  pkl_ast_node body = PKL_AST_LOOP_STMT_BODY (loop_stmt);

  if (condition && !iterator  && !container)
    {
      /* This is a WHILE loop.  */
      pkl_asm_while (PKL_GEN_ASM);
      {
        PKL_PASS_SUBPASS (condition);
      }
      pkl_asm_loop (PKL_GEN_ASM);
      {
        PKL_PASS_SUBPASS (body);
      }
      pkl_asm_endloop (PKL_GEN_ASM);
    }
  else if (iterator && container)
    {
      /* This is a FOR-IN[-WHERE] loop.  */
      pkl_asm_for (PKL_GEN_ASM, condition);
      {
        PKL_PASS_SUBPASS (container);
      }
      pkl_asm_for_where (PKL_GEN_ASM);
      {
        if (condition)
          PKL_PASS_SUBPASS (condition);
      }
      pkl_asm_for_loop (PKL_GEN_ASM);
      {
        PKL_PASS_SUBPASS (body);
      }
      pkl_asm_for_endloop (PKL_GEN_ASM);
    }
  else
    /* This should not happen.  */
    assert (0);

  PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

/*
 * | EXP
 * RETURN
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_return_stmt)
{
  /* Return from the function: pop N frames and generate a return
     instruction.  Note the popf below includes the function's
     frame.  */

  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPF,
                PKL_AST_RETURN_STMT_NFRAMES (PKL_PASS_NODE) + 1);
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RETURN);
}
PKL_PHASE_END_HANDLER

/*
 * | EXP
 * EXP_STMT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_exp_stmt)
{
  /* Pop the expression from the stack.  */
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POP);
}
PKL_PHASE_END_HANDLER

/*
 * | [EXP]
 * RAISE_STMT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_raise_stmt)
{
  pkl_ast_node raise_stmt = PKL_PASS_NODE;

  /* If the `raise' statement was anonymous, then we need to push the
     exception to raise, which by default, is 0.  */
  if (PKL_AST_RAISE_STMT_EXP (raise_stmt) == NULL)
    pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_int (0, 32));

  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RAISE);
}
PKL_PHASE_END_HANDLER

/*
 * | CODE
 * | HANDLER
 * | [ARG]
 * | [EXP]
 * TRY_CATCH_STMT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_try_catch_stmt)
{
  pkl_ast_node try_catch_stmt = PKL_PASS_NODE;
  pkl_ast_node code = PKL_AST_TRY_CATCH_STMT_CODE (try_catch_stmt);
  pkl_ast_node handler = PKL_AST_TRY_CATCH_STMT_HANDLER (try_catch_stmt);
  pkl_ast_node catch_arg = PKL_AST_TRY_CATCH_STMT_ARG (try_catch_stmt);
  pkl_ast_node catch_exp = PKL_AST_TRY_CATCH_STMT_EXP (try_catch_stmt);

  /* Push the exception number that will be catched by the sentence.
     This is EXP if it is defined, or 0 (catch-all) if it isnt.  */
  if (catch_exp)
    PKL_PASS_SUBPASS (catch_exp);
  else
    pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_int (0, 32));
  
  pkl_asm_try (PKL_GEN_ASM, catch_arg);
  {
    PKL_PASS_SUBPASS (code);
  }
  pkl_asm_catch (PKL_GEN_ASM);
  {
    PKL_PASS_SUBPASS (handler);
  }
  pkl_asm_endtry (PKL_GEN_ASM);

  PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

/*
 * | EXP
 * FUNCALL_ARG
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_funcall_arg)
{
  /* Do nothing, the argument is alread pushed in the stack.  */
}
PKL_PHASE_END_HANDLER

/* FUNCALL
 * | [ARG]
 * | ...
 * | FUNCTION
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_funcall)
{
  /* Save the used registers before calling the function.  */
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 0);
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 1);
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 2);
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 3);
}
PKL_PHASE_END_HANDLER

/*
 * | [ARG]
 * | ...
 * | FUNCTION
 * FUNCALL
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_funcall)
{
  /* At this point the closure for FUNCTION and the actuals are pushed
     in the stack.  Just call the bloody function.  */

  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_CALL);

  /* Restore the used registers after calling the function.  */
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 3);
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 2);
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 1);
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 0);
}
PKL_PHASE_END_HANDLER

/*
 * FUNC
 * | [TYPE]
 * | [FUNC_ARG]
 * | ...
 * | BODY
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_func)
{
  /* This is a function prologue.  */
  pkl_asm_note (PKL_GEN_ASM,
                PKL_AST_FUNC_NAME (PKL_PASS_NODE));
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PROLOG);

  /* Push the function environment, for the arguments.  The
     compound-statement that is the body for the function will create
     it's own frame.  */
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHF);
}
PKL_PHASE_END_HANDLER

/*
 * FUNC_ARG
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_func_arg)
{
  /* Pop an actual argument from the stack and put it in the current
     environment.  */

  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);
}
PKL_PHASE_END_HANDLER

/*
 * | [TYPE]
 * | [FUNC_ARG]
 * | ...
 * | BODY
 * FUNC
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_func)
{
  /* Function epilogue.  */

  pkl_ast_node function = PKL_PASS_NODE;
  pkl_ast_node function_type = PKL_AST_TYPE (function);


  /* In a void function, return PVM_NULL in the stack.  Otherwise, it
     is a run-time error to reach this point.  */
  if (PKL_AST_TYPE_CODE (PKL_AST_TYPE_F_RTYPE (function_type))
      == PKL_TYPE_VOID)
    pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, PVM_NULL);
  else
    {
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH,
                    pvm_make_int (PVM_E_NO_RETURN, 32));
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RAISE);
    }
     
  /* Pop the function's environment and return.  */
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPF, 1);
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RETURN);
}
PKL_PHASE_END_HANDLER

/*
 * INTEGER
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_integer)
{
  pkl_ast_node integer = PKL_PASS_NODE;
  pkl_ast_node type;
  pvm_val val;
  int size;
  uint64_t value;

  type = PKL_AST_TYPE (integer);
  assert (type != NULL
          && PKL_AST_TYPE_CODE (type) == PKL_TYPE_INTEGRAL);

  size = PKL_AST_TYPE_I_SIZE (type);
  value = PKL_AST_INTEGER_VALUE (integer);

  if ((size - 1) & ~0x1f)
    {
      if (PKL_AST_TYPE_I_SIGNED (type))
        val = pvm_make_long (value, size);
      else
        val = pvm_make_ulong (value, size);
    }
  else
    {
      if (PKL_AST_TYPE_I_SIGNED (type))
        val = pvm_make_int (value, size);
      else
        val = pvm_make_uint (value, size);
    }
  
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, val);
}
PKL_PHASE_END_HANDLER

/*
 * IDENTIFIER
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_identifier)
{
  pkl_ast_node identifier = PKL_PASS_NODE;
  pvm_val val
    = pvm_make_string (PKL_AST_IDENTIFIER_POINTER (identifier));

  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, val);
}
PKL_PHASE_END_HANDLER

/*
 * STRING
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_string)
{
  pkl_ast_node string = PKL_PASS_NODE;
  pvm_val val
    = pvm_make_string (PKL_AST_STRING_POINTER (string));

  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, val);
}
PKL_PHASE_END_HANDLER

/*
 * TYPE
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_type)
{
  /* Type nodes are handled by the code generator only in certain
     circumstances.  In any other cases, break the pass to avoid
     post-order hooks to be invoked.  */

  if (PKL_PASS_PARENT)
    {
      switch (PKL_AST_CODE (PKL_PASS_PARENT))
        {
        case PKL_AST_DECL:
          /* Declared struct type nodes should be processed in order to
             generate the code for mapper and construction functions.  */
          if (PKL_AST_TYPE_CODE (PKL_PASS_NODE) == PKL_TYPE_STRUCT)
            break;
        case PKL_AST_STRUCT:
        case PKL_AST_INTEGER:
        case PKL_AST_STRING:
        case PKL_AST_OFFSET:
        case PKL_AST_FUNC:
        case PKL_AST_FUNC_ARG:
        case PKL_AST_VAR:
        case PKL_AST_ARRAY_REF:
        case PKL_AST_SCONS:
          PKL_PASS_BREAK;
          break;
        default:
          break;
        }
    }
}
PKL_PHASE_END_HANDLER

/*
 * TYPE_OFFSET
 * | BASE_TYPE
 * | UNIT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_type_offset)
{
  /* XXX: is this break really necessary considering the break in
     pkl_gen_pr_type?  */
  if (!PKL_GEN_PAYLOAD->in_mapper
      || !PKL_GEN_PAYLOAD->in_writer)
    PKL_PASS_BREAK;

  if (PKL_GEN_PAYLOAD->in_writer)
    {
      /* Stack: OFF VAL */
      /* The offset to poke is stored in the TOS.  Replace the offset
         at the TOS with the magnitude of the offset and let the
         BASE_TYPE handler to tackle it.  */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM); /* OFF VAL VMAG */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);   /* OFF VMAG */
    }
}
PKL_PHASE_END_HANDLER

/*
 * | BASE_TYPE
 * | UNIT
 * TYPE_OFFSET
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_type_offset)
{
  if (PKL_GEN_PAYLOAD->in_mapper)
    pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKO);
}
PKL_PHASE_END_HANDLER

/*
 * | TYPE
 * | MAGNITUDE
 * | UNIT
 * OFFSET
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_offset)
{
  pkl_asm pasm = PKL_GEN_ASM;

  pkl_asm_insn (pasm, PKL_INSN_MKO);
}
PKL_PHASE_END_HANDLER

/*
 * | EXP
 * CAST
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_cast)
{
  pkl_asm pasm = PKL_GEN_ASM;
  pkl_ast_node node = PKL_PASS_NODE;

  pkl_ast_node exp;
  pkl_ast_node to_type;
  pkl_ast_node from_type;

  exp = PKL_AST_CAST_EXP (node);

  to_type = PKL_AST_CAST_TYPE (node);
  from_type = PKL_AST_TYPE (exp);
  
  if (PKL_AST_TYPE_CODE (from_type) == PKL_TYPE_INTEGRAL
      && PKL_AST_TYPE_CODE (to_type) == PKL_TYPE_INTEGRAL)
    {
      pkl_asm_insn (pasm, PKL_INSN_NTON,
                    from_type, to_type);
      pkl_asm_insn (pasm, PKL_INSN_NIP);
    }
  else if (PKL_AST_TYPE_CODE (from_type) == PKL_TYPE_OFFSET
           && PKL_AST_TYPE_CODE (to_type) == PKL_TYPE_OFFSET)
    {
      pkl_ast_node from_base_type = PKL_AST_TYPE_O_BASE_TYPE (from_type);
      pkl_ast_node from_base_unit = PKL_AST_TYPE_O_UNIT (from_type);
      pkl_ast_node from_base_unit_type = PKL_AST_TYPE (from_base_unit);

      pkl_ast_node to_base_type = PKL_AST_TYPE_O_BASE_TYPE (to_type);
      pkl_ast_node to_base_unit = PKL_AST_TYPE_O_UNIT (to_type);
      pkl_ast_node to_base_unit_type = PKL_AST_TYPE (to_base_unit);

      /* Get the magnitude of the offset, cast it to the new base type
         and convert to new unit.  */
      /* XXX: use OGETMC here.  */
      /* XXX: we have to do the arithmetic in base_unit_types, then
         convert to to_base_type, to assure that to_base_type can hold
         the to_base_unit.  Otherwise weird division by zero occurs.  */

      /* Stack: OFFSET */
      pkl_asm_insn (pasm, PKL_INSN_OGETM);

      /* Stack: OFFSET MAGNITUDE */
      if (!pkl_ast_type_equal (from_base_type, to_base_type))
        {
          pkl_asm_insn (pasm, PKL_INSN_NTON,
                        from_base_type, to_base_type);
          pkl_asm_insn (pasm, PKL_INSN_NIP);
        }

      /* Stack: OFFSET MAGNITUDE */
      PKL_PASS_SUBPASS (from_base_unit);
      
      /* Stack: OFFSET MAGNITUDE UNIT */
      if (!pkl_ast_type_equal (from_base_unit_type, to_base_type))
        {
          pkl_asm_insn (pasm, PKL_INSN_NTON,
                        from_base_unit_type, to_base_type);
          pkl_asm_insn (pasm, PKL_INSN_NIP);

        }

      /* Stack: OFFSET MAGNITUDE UNIT */
      pkl_asm_insn (pasm, PKL_INSN_MUL, to_base_type);
      pkl_asm_insn (pasm, PKL_INSN_NIP2);

      /* Stack: OFFSET (MAGNITUDE*UNIT) */
      PKL_PASS_SUBPASS (to_base_unit);

      /* Stack: OFFSET (MAGNITUDE*UNIT) NEWUNIT */
      if (!pkl_ast_type_equal (to_base_unit_type, to_base_type))
        {
          pkl_asm_insn (pasm, PKL_INSN_NTON,
                        to_base_unit_type, to_base_type);
          pkl_asm_insn (pasm, PKL_INSN_NIP);
        }

      /* Stack: OFFSET (MAGNITUDE*UNIT) NEWUNIT */
      pkl_asm_insn (pasm, PKL_INSN_DIV, to_base_type);
      pkl_asm_insn (pasm, PKL_INSN_NIP2);

      /* Stack: OFFSET (MAGNITUDE*UNIT/NEWUNIT) */
      pkl_asm_insn (pasm, PKL_INSN_SWAP);

      /* Push the new unit.  */

      /* Stack: (MAGNITUDE*UNIT/NEWUNIT) OFFSET */
      PKL_PASS_SUBPASS (to_base_unit);

      /* Stack: (MAGNITUDE*UNIT/NEWUNIT) OFFSET NEWUNIT */

      /* Get rid of the original offset.  */
      pkl_asm_insn (pasm, PKL_INSN_NIP);

      /* Stack: (MAGNITUDE*UNIT/NEWUNIT) NEWUNIT */

      /* And create the new one.  */
      pkl_asm_insn (pasm, PKL_INSN_MKO);

      /* Stack: OFFSET */
    }
  else
    /* XXX: handle casts to structs and arrays.  For structs,
       reorder fields.  */
    assert (0);

}
PKL_PHASE_END_HANDLER

/*
 * | SCONS_VALUE
 * SCONS
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_scons)
{
  pkl_ast_node scons = PKL_PASS_NODE;

  /* Call the constructor function of the struct type, whose lexical
     address is stored in the scons AST node, passing SCONS_VALUE as
     an argument.  The constructor will return a struct on the top of
     the stack.

     XXX: install an exception handler for constraint errors, etc.  */

  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR,
                PKL_AST_SCONS_CONSTRUCTOR_BACK (scons),
                PKL_AST_SCONS_CONSTRUCTOR_OVER (scons));
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_CALL);
}
PKL_PHASE_END_HANDLER

/*
 * MAP
 * | MAP_OFFSET
 * | MAP_TYPE
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_map)
{
  pkl_ast_node map = PKL_PASS_NODE;
  pkl_ast_node map_type = PKL_AST_MAP_TYPE (map);
  pkl_ast_node map_offset = PKL_AST_MAP_OFFSET (map);

  /* Push the offset of the map.  */
  PKL_PASS_SUBPASS (map_offset);

  /* Generate code to peek from the offset generated above.

     If there is a lexical address stored in the map AST node, then it
     refers to a mapper function.  In that case, we should call the
     mapper function.  The map's offset is passed to the function,
     which will return the mapped value.

     Otherwise (there is not a lexical address in the map AST node)
     let the code generator generate code for peeking the mapped type.
     The generated code expects the map offset at the top of the
     stack.  

     XXX: install exception handlers for constraints etc.  */
  if (PKL_AST_MAP_MAPPER_P (map))
    {
      int mapper_back = PKL_AST_MAP_MAPPER_BACK (map);
      int mapper_over = PKL_AST_MAP_MAPPER_OVER (map);

      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR,
                    mapper_back, mapper_over);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_CALL);
    }
  else
    {
      PKL_GEN_PAYLOAD->in_mapper = 1;
      PKL_PASS_SUBPASS (map_type);
      PKL_GEN_PAYLOAD->in_mapper = 0;
    }
        
  PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

/*
 * | ARRAY_INITIALIZER_INDEX
 * | ARRAY_INITIALIZER_EXP
 * ARRAY_INITIALIZER
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_array_initializer)
{
  /* Nothing to do.  */
}
PKL_PHASE_END_HANDLER

/*
 *  | ARRAY_TYPE
 *  | ARRAY_INITIALIZER
 *  | ...
 *  ARRAY
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_array)
{
  pkl_asm pasm = PKL_GEN_ASM;
  pkl_ast_node node = PKL_PASS_NODE;

  pkl_asm_insn (pasm, PKL_INSN_PUSH,
                pvm_make_ulong (PKL_AST_ARRAY_NELEM (node), 64));

  pkl_asm_insn (pasm, PKL_INSN_PUSH,
                pvm_make_ulong (PKL_AST_ARRAY_NINITIALIZER (node), 64));

  pkl_asm_insn (pasm, PKL_INSN_MKA);
}
PKL_PHASE_END_HANDLER

/*
 * | ARRAY_REF_ARRAY
 * | ARRAY_REF_INDEX
 * ARRAY_REF
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_array_ref)
{
  if (PKL_PASS_PARENT
      && PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_ASS_STMT)
    {
      /* This is a l-value in an assignment.  The array and the index
         are pushed to the stack for the ass_stmt PS handler.  Nothing
         else to do here.  */
     }
  else
    {
      pkl_ast_node array_ref = PKL_PASS_NODE;
      pkl_ast_node array_ref_type = PKL_AST_TYPE (array_ref);
      
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_AREF);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);

      /* To cover cases where the referenced array is not mapped, but
         the value stored in it is a mapped value, we issue a
         REMAP.  */
      switch (PKL_AST_TYPE_CODE (array_ref_type))
        {
        case PKL_TYPE_ARRAY:
        case PKL_TYPE_STRUCT:
          /* XXX: this is redundant IO for many (most?) cases.  */
          /* XXX: handle exceptions from the mapper function.  */
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REMAP);
          break;
        default:
          break;
        }
    }
}
PKL_PHASE_END_HANDLER

/*
 *  | STRUCT_ELEM
 *  | ...
 *  STRUCT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_struct)
{
  pkl_asm pasm = PKL_GEN_ASM;

  pkl_asm_insn (pasm, PKL_INSN_PUSH,
                pvm_make_ulong (PKL_AST_STRUCT_NELEM (PKL_PASS_NODE), 64));

  pkl_asm_insn (pasm, PKL_INSN_MKSCT);
}
PKL_PHASE_END_HANDLER

/*
 *  STRUCT_ELEM
 *  | [STRUCT_ELEM_NAME]
 *  | STRUCT_ELEM_EXP
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_struct_elem)
{
  /* If the struct initializer doesn't include a name, generate a null
     value as expected by the mksct instruction.  */
  if (!PKL_AST_STRUCT_ELEM_NAME (PKL_PASS_NODE))
    pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, PVM_NULL);
}
PKL_PHASE_END_HANDLER

/*
 * | STRUCT
 * | IDENTIFIER
 * STRUCT_REF
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_struct_ref)
{
  if (PKL_PASS_PARENT
      && PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_ASS_STMT)
    {
      /* This is a -lvalue in an assignment.  The struct and the
         identifier are pushed to the stack for the ass_stmt PS
         handler.  Nothing else to do here.  */
    }
  else
    {
      pkl_ast_node struct_ref = PKL_PASS_NODE;
      pkl_ast_node struct_ref_type = PKL_AST_TYPE (struct_ref);

      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SREF);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);

      /* To cover cases where the referenced struct is not mapped, but
         the value stored in it is a mapped value, we issue a
         REMAP.  */
      switch (PKL_AST_TYPE_CODE (struct_ref_type))
        {
        case PKL_TYPE_ARRAY:
        case PKL_TYPE_STRUCT:
          /* XXX: this is redundant IO for many (most?) cases.  */
          /* XXX: handle exceptions from the mapper function.  */
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REMAP);
          break;
        default:
          break;
        }
    }
}
PKL_PHASE_END_HANDLER

/*
 * TYPE_INTEGRAL
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_type_integral)
  PKL_PHASE_PARENT (5,
                    PKL_AST_ARRAY,
                    PKL_AST_OFFSET,
                    PKL_AST_TYPE,
                    PKL_AST_MAP,
                    PKL_AST_STRUCT_ELEM_TYPE)
{
  pkl_asm pasm = PKL_GEN_ASM;
  pkl_ast_node integral_type = PKL_PASS_NODE;

  if (PKL_GEN_PAYLOAD->in_mapper)
    /* Stack: OFF */
    pkl_asm_insn (pasm, PKL_INSN_PEEKD, integral_type);
  else if (PKL_GEN_PAYLOAD->in_writer)
    /* Stack: OFF VAL */
    pkl_asm_insn (pasm, PKL_INSN_POKED, integral_type);
  else
    {
      pkl_asm_insn (pasm, PKL_INSN_PUSH,
                    pvm_make_ulong (PKL_AST_TYPE_I_SIZE (integral_type),
                                    64));

      pkl_asm_insn (pasm, PKL_INSN_PUSH,
                    pvm_make_uint (PKL_AST_TYPE_I_SIGNED (integral_type),
                                   32));

      pkl_asm_insn (pasm, PKL_INSN_MKTYI);
    }
}
PKL_PHASE_END_HANDLER

/*
 * FUNC_TYPE_ARG
 * | FUNC_TYPE_ARG_TYPE
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_func_type_arg)
{
  /* XXX: why is it needed a PR handler and a subpass here???  The
     FUNC_TYPE_ARG_TYPE doesn't get traversed in post-order, wtf.  */
  PKL_PASS_SUBPASS (PKL_AST_FUNC_TYPE_ARG_TYPE (PKL_PASS_NODE));
}
PKL_PHASE_END_HANDLER

/*
 * | FUNC_TYPE_ARG
 * | ...
 * | FUNC_TYPE_RTYPE
 * TYPE_FUNCTION
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_type_function)
{
  pkl_ast_node ftype = PKL_PASS_NODE;

  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH,
                pvm_make_ulong (PKL_AST_TYPE_F_NARG (ftype), 64));
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKTYC);
}
PKL_PHASE_END_HANDLER

/*
 * TYPE_ARRAY
 * | ETYPE
 * | NELEM
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_type_array)
{
  assert (!(PKL_GEN_PAYLOAD->in_mapper
            && PKL_GEN_PAYLOAD->in_writer));

  if (PKL_GEN_PAYLOAD->in_writer)
    {
      /* Stack: OFF ARR */
      /* XXX: handle exceptions from the writer.  */

      /* Note that we don't use the offset, because it should be the
         same than the mapped offset in the array.  */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_WRITE);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP); /* The array.  */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP); /* The offset. */
      PKL_PASS_BREAK;
    }

  if (PKL_GEN_PAYLOAD->in_mapper)
    {
      /* Generate code to create a mapped array of the given type.  At
         this point the offset to use is in the top of the stack.  */

      pkl_ast_node array_type = PKL_PASS_NODE;
      pkl_ast_node array_type_nelem = PKL_AST_TYPE_A_NELEM (array_type);

      pvm_program mapper_program;
      pvm_val mapper_closure;

      /* Compile a mapper function to a closure.  */
      PKL_GEN_PUSH_ASM (pkl_asm_new (PKL_PASS_AST,
                                     PKL_GEN_PAYLOAD->compiler,
                                     0 /* prologue */));

      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PROLOG);

      /* Push a new frame and register the passed object as a local
         variable.  */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHF);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR,
                    0, 0); /* XXX: use this variable in the code
                              below, instead of %aoff.  */

      if (array_type_nelem)
        {
          /* The size of the array is variable.  Peek that many
             elements.

             XXX: rewrite to use ADDO.

             ; Four scratch registers are used in this code:
             ;
             ;   %aoff is %r0 and contains the offset of the
             ;   array.
             ;   %off is %r1 and contains the offset of the
             ;   array element being processed, relative to the start
             ;   of the array, in bits.
             ;   %idx is %r2 and contains the index of the
             ;   array element being processed.
             ;   %nelem is %r3 and holds the total number of
             ;   elements contained in the array.
             SAVER %aoff
             SAVER %off
             SAVER %idx
             SAVER %nelem

                                      ; OFF (must be offset<uint<64>,*>)          
             ; Initialize registers.
             OGETM		      ; OFF OMAG
             SWAP                     ; OMAG OFF
             OGETU                    ; OMAG OFF OUNIT
             ROT                      ; OFF OUNIT OMAG
             MULLU                    ; OFF OUNIT OMAG (OUNIT*OMAG)
             NIP2                     ; OFF (OUNIT*OMAG)
             POPR %aoff               ; OFF
             PUSH 0UL                 ; OFF 0UL
             POPR %off                ; OFF
             PUSH 0UL                 ; OFF 0UL
             POPR %idx                ; OFF
             SUBPASS array_type_nelem ; OFF NELEM
             POPR %nelem              ; OFF ATYPE
             SUBPASS array_type       ; OFF ATYPE

           .while
             PUSHR %idx               ; OFF ATYPE I
             PUSHR %nelemd            ; OFF ATYPE I NELEM
             LTLU                     ; OFF ATYPE NELEM I (NELEM<I)
             NIP2                     ; OFF ATYPE (NELEM<I)
           .loop
                                      ; OFF ATYPE

             ; Mount the Ith element triplet: [EOFF EIDX EVAL]
             PUSHR %aoff              ; ... AOFFMAG
             PUSHR %off               ; ... AOFFMAG EOMAG
             ADDLU                    ; ... AOFFMAG EOMAG (AOFFMAG+EOMAG)
             NIP2                     ; ... (AOFFMAG+EOMAG)
             PUSH 1UL                 ; ... (AOFFMAG+EOMAG) EOUNIT
             MKO                      ; ... EOFF
             DUP                      ; ... EOFF EOFF
             SUBPASS array_type       ; ... EOFF EVAL

             ; XXX EOFF = EOFF - %aoff

             ; Update the current offset with the size of the value just
             ; peeked.
             SIZ                      ; ... EOFF EVAL ESIZ
             ROT                      ; ... EVAL ESIZ EOFF
             OGETM                    ; ... EVAL ESIZ EOFF EOMAG
             ROT                      ; ... EVAL EOFF EOMAG ESIZ
             OGETM                    ; ... EVAL EOFF EOMAG ESIZ ESIGMAG
             ROT                      ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
             ADDLU                    ; ... EVAL EOFF ESIZ ESIGMAG EOMAG (ESIGMAG+EOMAG)
             POPR %off                ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
             DROP                     ; ... EVAL EOFF ESIZ ESIGMAG
             DROP                     ; ... EVAL EOFF ESIZ
             DROP                     ; ... EVAL EOFF
             PUSHR %idx               ; ... EVAL EOFF EIDX
             ROT                      ; ... EOFF EIDX EVAL

             ; Increase the current index and process the next
             ; element.
             PUSHR %idx              ; ... EOFF EIDX EVAL EIDX
             PUSH 1UL                ; ... EOFF EIDX EVAL EIDX 1UL
             ADDLU                   ; ... EOFF EIDX EVAL EDIX 1UL (EIDX+1UL)
             NIP2                    ; ... EOFF EIDX EVAL (EIDX+1UL)
             POPR %idx               ; ... EOFF EIDX EVAL
           .endloop                                     
                                  
             PUSHR %nelem            ; OFF ATYPE [EOFF EIDX EVAL]... NELEM
             DUP                     ; OFF ATYPE [EOFF EIDX EVAL]... NELEM NINITIALIZER
             MKMA                    ; ARRAY

             RESTORER %nelem
             RESTORER %idx
             RESTORER %off
             RESTORER %aoff  */

          int aoffreg = 0;
          int offreg = 1;
          int idxreg = 2;
          int nelemreg = 3;

          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETU);
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MULLU);
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, aoffreg);
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (0, 64));
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, offreg);

          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH,
                        pvm_make_ulong (0, 64));
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, idxreg);

          PKL_GEN_PAYLOAD->in_mapper = 0;
          PKL_PASS_SUBPASS (array_type_nelem);
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, nelemreg);
          PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));
          PKL_GEN_PAYLOAD->in_mapper = 1;

          pkl_asm_while (PKL_GEN_ASM);
          {
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, nelemreg);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_LTLU);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);
          }
          pkl_asm_loop (PKL_GEN_ASM);
          {
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, aoffreg);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, offreg);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (1, 64));
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKO);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DUP);


            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 0);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 1);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 2);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 3);

            //            pkl_asm_note (PKL_GEN_ASM, "before array_type"); /* XXX */
            PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));
            //            pkl_asm_note (PKL_GEN_ASM, "after array_type"); /* XXX */

            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 3);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 2);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 1);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 0);

            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SIZ);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, offreg);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);

            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (1, 64));
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);
            pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, idxreg);
          }
          pkl_asm_endloop (PKL_GEN_ASM);

          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, nelemreg);
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DUP);
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKMA);
        }
      else
        {
          /* The size of the array is unspecified.  Peek array
             elements until EOF(=null).  XXX: or until a constraint
             exception is raised.  */

          assert (0); /* WRITEME */
        }

      /* Finish the mapper function.  */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPF, 1);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RETURN);

      /* All right, the mapper function is compiled.  */
      mapper_program = pkl_asm_finish (PKL_GEN_ASM,
                                       0 /* epilogue */);
      
      PKL_GEN_POP_ASM;
      pvm_specialize_program (mapper_program);
      mapper_closure = pvm_make_cls (mapper_program);

      /* XXX */
      /*printf ("BEGIN MAPPER\n");
      pvm_print_program (stdout, mapper_program);
      printf ("END MAPPER\n"); */

      /* Complete the mapper closure with the current environment,
         call it to obtain the mapped array itself, and install it in
         the array as its mapper.  */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, mapper_closure); /* OFF CLS */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PEC);                  /* OFF CLS */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DUP);                  /* OFF CLS CLS */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NROT);                 /* CLS OFF CLS */

      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 0);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 1);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 2);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 3);

      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_CALL);                 /* CLS ARR */

      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 3);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 2);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 1);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 0);

      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                 /* ARR CLS */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ASETM);                /* ARR */

      /* Yay!, we are done ;) */
      PKL_PASS_BREAK;

      /* XXX do the same with the writer.  */
    }
}
PKL_PHASE_END_HANDLER

/*
 * | ETYPE
 * | NELEM
 * TYPE_ARRAY
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_type_array)
  PKL_PHASE_PARENT (4,
                    PKL_AST_ARRAY,
                    PKL_AST_OFFSET,
                    PKL_AST_TYPE,
                    PKL_AST_STRUCT_ELEM_TYPE)
{
  if (PKL_AST_TYPE_A_NELEM (PKL_PASS_NODE))
    pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP); /* XXX: drop the number
                                                  of elements, as it
                                                  isn't used at the
                                                  PVM level.  */
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKTYA);
}
PKL_PHASE_END_HANDLER

/*
 * TYPE_STRING
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_type_string)
  PKL_PHASE_PARENT (4,
                    PKL_AST_ARRAY,
                    PKL_AST_OFFSET,
                    PKL_AST_TYPE,
                    PKL_AST_STRUCT_ELEM_TYPE)
{
  if (PKL_GEN_PAYLOAD->in_mapper)
    /* Stack: OFF */
    pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PEEKS);
  else if (PKL_GEN_PAYLOAD->in_writer)
    /* Stack: OFF STR */
    pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POKES);
  else
    pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKTYS);
}
PKL_PHASE_END_HANDLER

/*
 * TYPE_STRUCT
 * | STRUCT_ELEM_TYPE
 * | ...
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_type_struct)
  PKL_PHASE_PARENT (5,
                    PKL_AST_DECL,
                    PKL_AST_ARRAY,
                    PKL_AST_OFFSET,
                    PKL_AST_TYPE,
                    PKL_AST_STRUCT_ELEM_TYPE)
{
  if (PKL_GEN_PAYLOAD->in_writer)
    {
      /* Stack: OFF SCT */
      /* XXX: handle exceptions from the writer.  */

      /* Note that we don't use the offset, because it should be the
         same than the mapped offset in the struct.  */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_WRITE);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP); /* The struct.  */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP); /* The offset. */
      PKL_PASS_BREAK;
    }
      
  if (PKL_GEN_PAYLOAD->in_mapper)
    {
      /* XXX: Generate code like in the case below for in_struct_decl.
         The code should expect the offset in the stack, followed by
         the struct arguments, and should build a mapped struct at the
         top of the main stack.

         In this schema, in_struct_decl is only used to emit the
         FUNCTION prologue and epilogue in the PS handler.  The code
         in between is common (for function and inline cases.)  */

      assert (0);
    }
  
  if (PKL_GEN_PAYLOAD->in_struct_decl)
    {
      /* Generating code for the struct mapper and constructor
         functions.  */

      pkl_ast_node decl_name = PKL_AST_DECL_NAME (PKL_PASS_PARENT);
      char *type_name = PKL_AST_IDENTIFIER_POINTER (decl_name);
      char *mapper_name = xmalloc (strlen (type_name) +
                                    strlen ("_pkl_mapper_") + 1);
      char *constructor_name = xmalloc (strlen (type_name) +
                                        strlen ("_pkl_constructor_") + 1);

      strcpy (mapper_name, "_pkl_mapper_");
      strcat (mapper_name, type_name);
      strcpy (constructor_name, "_pkl_constructor_");
      strcat (constructor_name, type_name);

      /* Build the struct mapper prologue.  */
      {
        pkl_asm_note (PKL_GEN_ASM, mapper_name);
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PROLOG);
        
        /* Push the struct environment, for the arguments and local
           variables.  */
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHF);

        /* Put the arguments in the current environment:
         
           OFFSET: offset of the struct to map.  */
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);

        /* Push the offset to the stack.  */
        //        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 0);
      }

      /* Build the struct constructor prologue.  */
      {
        pkl_asm_note (PKL_GEN_ASM2, constructor_name);
        pkl_asm_insn (PKL_GEN_ASM2, PKL_INSN_PROLOG);
        
        /* Push the struct environment, for the arguments and local
           variables.  */
        pkl_asm_insn (PKL_GEN_ASM2, PKL_INSN_PUSHF);

        /* Put the arguments in the current environment:
         
           BASE_VALUE: struct value.  */
        pkl_asm_insn (PKL_GEN_ASM2, PKL_INSN_REGVAR);
      }

      free (mapper_name);
      free (constructor_name);
    }
}
PKL_PHASE_END_HANDLER

/*
 * | STRUCT_ELEM_TYPE
 * | ...
 * TYPE_STRUCT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_type_struct)
{
  if (PKL_GEN_PAYLOAD->in_struct_decl)
    {
      /* Generating code for the struct mapper and constructor
         functions.  */

      /* Struct mapper epilogue.  */
      {
        /* XXX: create an empty struct for the moment.  */
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH,
                      pvm_make_ulong (0, 64));
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKSCT);

        /* XXX: register IOS callback for updates, covering the whole
           mapped area, i.e. [base-offset,current-offset].  */
      
        /* Pop the struct's environment and return.  */
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPF, 1);
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RETURN);
      }

      /* Struct constructor epilogue.  */
      {
        /* XXX: create an empty struct for the moment.  */
        pkl_asm_insn (PKL_GEN_ASM2, PKL_INSN_PUSH,
                      pvm_make_ulong (0, 64));
        pkl_asm_insn (PKL_GEN_ASM2, PKL_INSN_MKSCT);
      
        /* Pop the struct's environment and return.  */
        pkl_asm_insn (PKL_GEN_ASM2, PKL_INSN_POPF, 1);
        pkl_asm_insn (PKL_GEN_ASM2, PKL_INSN_RETURN);
      }
    }
  else
    {
      /* We are generating a PVM struct type.  */
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH,
                    pvm_make_ulong (PKL_AST_TYPE_S_NELEM (PKL_PASS_NODE), 64));
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKTYSCT);
    }
}
PKL_PHASE_END_HANDLER

/*
 * STRUCT_ELEM_TYPE
 * | [STRUCT_ELEM_TYPE_NAME]
 * | STRUCT_ELEM_TYPE_TYPE
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_pr_struct_elem_type)
{
  if (PKL_GEN_PAYLOAD->in_struct_decl)
    {
      /* XXX: add mapper code for a struct elem type.  */
      /* XXX: add constructor code for a struct elem type.  */
      /* Do not process the child nodes.  */
      PKL_PASS_BREAK;
    }
  else
    {
      /* We are generating a PVM struct type.  */

      /* If the struct type element doesn't include a name, generate a
         null value as expected by the mktysct instruction.  */
      if (!PKL_AST_STRUCT_ELEM_TYPE_NAME (PKL_PASS_NODE))
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, PVM_NULL);
    }
}
PKL_PHASE_END_HANDLER

/* 
 * Expression handlers.
 *
 * | OPERAND1
 * | [OPERAND2]
 * EXP
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_op_add)
{
  pkl_asm pasm = PKL_GEN_ASM;
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (node);

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      pkl_asm_insn (pasm, PKL_INSN_ADD, type);
      pkl_asm_insn (pasm, PKL_INSN_NIP2);
      break;
    case PKL_TYPE_STRING:
      pkl_asm_insn (pasm, PKL_INSN_SCONC);
      pkl_asm_insn (pasm, PKL_INSN_NIP2);
      break;
    case PKL_TYPE_OFFSET:
      {
        /* Calculate the magnitude of the new offset, which is the
           addition of both magnitudes.  The unit used for the result
           is the greatest common divisor of the operand's units.

           Note that since addition is commutative we can process OFF2
           first and save a swap.  */

        pkl_ast_node op1 = PKL_AST_EXP_OPERAND (node, 0);
        pkl_ast_node op1_type = PKL_AST_TYPE (op1);
        
        pkl_ast_node op2 = PKL_AST_EXP_OPERAND (node, 1);
        pkl_ast_node op2_type = PKL_AST_TYPE (op2);
        
        pkl_ast_node base_type = PKL_AST_TYPE_O_BASE_TYPE (type);

        PKL_PASS_SUBPASS (PKL_AST_TYPE_O_UNIT (op1_type));
        PKL_PASS_SUBPASS (PKL_AST_TYPE_O_UNIT (op2_type));
        pkl_asm_call (pasm, "_pkl_gcd");
        pkl_asm_insn (pasm, PKL_INSN_OGETMC, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP);
        pkl_asm_insn (pasm, PKL_INSN_SWAP);

        PKL_PASS_SUBPASS (PKL_AST_TYPE_O_UNIT (op1_type));
        PKL_PASS_SUBPASS (PKL_AST_TYPE_O_UNIT (op2_type));
        pkl_asm_call (pasm, "_pkl_gcd");
        pkl_asm_insn (pasm, PKL_INSN_OGETMC, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP);
        pkl_asm_insn (pasm, PKL_INSN_ADD, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP2);

        /* PKL_PASS_SUBPASS (res_unit); */
        PKL_PASS_SUBPASS (PKL_AST_TYPE_O_UNIT (op1_type));
        PKL_PASS_SUBPASS (PKL_AST_TYPE_O_UNIT (op2_type));
        pkl_asm_call (pasm, "_pkl_gcd");
        pkl_asm_insn (pasm, PKL_INSN_MKO);
      }
      break;
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_op_sub)
{
  pkl_asm pasm = PKL_GEN_ASM;
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (node);

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      pkl_asm_insn (pasm, PKL_INSN_SUB, type);
      pkl_asm_insn (pasm, PKL_INSN_NIP2);
      break;
    case PKL_TYPE_OFFSET:
      {
        /* Calculate the magnitude of the new offset, which is the
           subtraction of both magnitudes, once normalized to bits. */

        pkl_ast_node base_type = PKL_AST_TYPE_O_BASE_TYPE (type);
        pkl_ast_node res_unit = PKL_AST_TYPE_O_UNIT (type);

        pkl_asm_insn (pasm, PKL_INSN_SWAP);

        PKL_PASS_SUBPASS (res_unit);
        pkl_asm_insn (pasm, PKL_INSN_OGETMC, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP);
        pkl_asm_insn (pasm, PKL_INSN_SWAP);
        PKL_PASS_SUBPASS (res_unit);
        pkl_asm_insn (pasm, PKL_INSN_OGETMC, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP);
        pkl_asm_insn (pasm, PKL_INSN_SUB, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP2);

        PKL_PASS_SUBPASS (res_unit);
        pkl_asm_insn (pasm, PKL_INSN_MKO);
      }
      break;
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_op_mul)
{
  pkl_asm pasm = PKL_GEN_ASM;
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (node);

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      pkl_asm_insn (pasm, PKL_INSN_MUL, type);
      pkl_asm_insn (pasm, PKL_INSN_NIP2);
      break;
    case PKL_TYPE_OFFSET:
      {       
        pkl_ast_node op1 = PKL_AST_EXP_OPERAND (node, 0);
        pkl_ast_node op2 = PKL_AST_EXP_OPERAND (node, 1);
        pkl_ast_node op1_type = PKL_AST_TYPE (op1);
        pkl_ast_node op2_type = PKL_AST_TYPE (op2);
        int op1_type_code = PKL_AST_TYPE_CODE (op1_type);
        int op2_type_code = PKL_AST_TYPE_CODE (op2_type);

        pkl_ast_node offset_type, offset_unit, base_type;
        pkl_ast_node offset_op = NULL;

        if (op2_type_code == PKL_TYPE_OFFSET)
          {
            pkl_asm_insn (pasm, PKL_INSN_OGETM); /* OP1 OP2 M2 */
            pkl_asm_insn (pasm, PKL_INSN_NIP);   /* OP1 M2 */

            offset_op = op2;
          }

        pkl_asm_insn (pasm, PKL_INSN_SWAP); /* M2 OP1 */

        if (op1_type_code == PKL_TYPE_OFFSET)
          {
            pkl_asm_insn (pasm, PKL_INSN_OGETM); /* M2 OP1 M1 */
            pkl_asm_insn (pasm, PKL_INSN_NIP);   /* M2 M1 */

            offset_op = op1;
          }

        assert (offset_op != NULL);
        offset_type = PKL_AST_TYPE (offset_op);
        offset_unit = PKL_AST_TYPE_O_UNIT (offset_type);
        base_type = PKL_AST_TYPE_O_BASE_TYPE (offset_type);

        pkl_asm_insn (pasm, PKL_INSN_MUL, base_type); /* M2 M1 MR */
        pkl_asm_insn (pasm, PKL_INSN_NIP2); /* MR */
          
        PKL_PASS_SUBPASS (offset_unit); /* MR UNIT */
        pkl_asm_insn (pasm, PKL_INSN_MKO); /* MR UNIT OFFSET */
      }
      break;
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_op_div)
{
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_asm pasm = PKL_GEN_ASM;
  pkl_ast_node type = PKL_AST_TYPE (node);
  pkl_ast_node op2 = PKL_AST_EXP_OPERAND (node, 0);
  pkl_ast_node op2_type = PKL_AST_TYPE (op2);

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      {
        if (PKL_AST_TYPE_CODE (op2_type) == PKL_TYPE_OFFSET)
          {
            /* Calculate the resulting integral value, which is the
               division of both magnitudes, once normalized to
               bits. */

            pkl_ast_node unit_type = pkl_ast_make_integral_type (PKL_PASS_AST, 64, 0);
            pkl_ast_node unit_bits = pkl_ast_make_integer (PKL_PASS_AST, 1);
            PKL_AST_TYPE (unit_bits) = ASTREF (unit_type);

            pkl_asm_insn (pasm, PKL_INSN_SWAP);

            PKL_PASS_SUBPASS (unit_bits);
            pkl_asm_insn (pasm, PKL_INSN_OGETMC, type);
            pkl_asm_insn (pasm, PKL_INSN_NIP);
            pkl_asm_insn (pasm, PKL_INSN_SWAP);
            PKL_PASS_SUBPASS (unit_bits);
            pkl_asm_insn (pasm, PKL_INSN_OGETMC, type);
            pkl_asm_insn (pasm, PKL_INSN_NIP);

            pkl_asm_insn (pasm, PKL_INSN_DIV, type);
            pkl_asm_insn (pasm, PKL_INSN_NIP2);

            ASTREF (unit_bits); pkl_ast_node_free (unit_bits);
          }
        else
          {
            pkl_asm_insn (pasm, PKL_INSN_DIV, type);
            pkl_asm_insn (pasm, PKL_INSN_NIP2);
          }
        break;
      }
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_op_mod)
{
  pkl_ast_node node = PKL_PASS_NODE;

  pkl_asm pasm = PKL_GEN_ASM;
  pkl_ast_node type = PKL_AST_TYPE (node);
  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (node, 0);
  pkl_ast_node op1_type = PKL_AST_TYPE (op1);

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      pkl_asm_insn (pasm, PKL_INSN_MOD, type);
      pkl_asm_insn (pasm, PKL_INSN_NIP2);
      break;
    case PKL_TYPE_OFFSET:
      {
        /* Calculate the magnitude of the new offset, which is the
           modulus of both magnitudes, the second argument converted
           to first's units.  */

        pkl_ast_node base_type = PKL_AST_TYPE_O_BASE_TYPE (type);
        pkl_ast_node op1_unit = PKL_AST_TYPE_O_UNIT (op1_type);

        pkl_asm_insn (pasm, PKL_INSN_SWAP);

        pkl_asm_insn (pasm, PKL_INSN_OGETM);
        pkl_asm_insn (pasm, PKL_INSN_NIP);
        pkl_asm_insn (pasm, PKL_INSN_SWAP);
        PKL_PASS_SUBPASS (op1_unit);
        pkl_asm_insn (pasm, PKL_INSN_OGETMC, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP);

        pkl_asm_insn (pasm, PKL_INSN_MOD, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP2);

        PKL_PASS_SUBPASS (op1_unit);
        pkl_asm_insn (pasm, PKL_INSN_MKO);
      }
      break;
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_op_intexp)
{
  pkl_asm pasm = PKL_GEN_ASM;

  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (node);

  enum pkl_asm_insn insn;

  switch (PKL_AST_EXP_CODE (node))
    {
    case PKL_AST_OP_BAND: insn = PKL_INSN_BAND; break;
    case PKL_AST_OP_BNOT: insn = PKL_INSN_BNOT; break;
    case PKL_AST_OP_NEG: insn = PKL_INSN_NEG; break;
    case PKL_AST_OP_IOR: insn = PKL_INSN_BOR; break;
    case PKL_AST_OP_XOR: insn = PKL_INSN_BXOR; break;
    case PKL_AST_OP_SL: insn = PKL_INSN_SL; break;
    case PKL_AST_OP_SR: insn = PKL_INSN_SR; break;
    default:
      assert (0);
      break;
    }
          
  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      pkl_asm_insn (pasm, insn, type);
      pkl_asm_insn (pasm, PKL_INSN_NIP);
      if (insn != PKL_INSN_NEG)
        pkl_asm_insn (pasm, PKL_INSN_NIP);
      break;
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_op_and)
{
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_AND);
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_op_or)
{
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OR);
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_op_not)
{
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NOT);
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_op_rela)
{
  pkl_asm pasm = PKL_GEN_ASM;
  pkl_ast_node exp = PKL_PASS_NODE;
  int exp_code = PKL_AST_EXP_CODE (exp);
  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (exp, 0);
  pkl_ast_node op1_type = PKL_AST_TYPE (op1);

  enum pkl_asm_insn rela_insn;

  switch (exp_code)
    {
    case PKL_AST_OP_EQ: rela_insn = PKL_INSN_EQ; break;
    case PKL_AST_OP_NE: rela_insn = PKL_INSN_NE; break;
    case PKL_AST_OP_LT: rela_insn = PKL_INSN_LT; break;
    case PKL_AST_OP_GT: rela_insn = PKL_INSN_GT; break;
    case PKL_AST_OP_LE: rela_insn = PKL_INSN_LE; break;
    case PKL_AST_OP_GE: rela_insn = PKL_INSN_GE; break;
    default:
      assert (0);
      break;
    }

  switch (PKL_AST_TYPE_CODE (op1_type))
    {
    case PKL_TYPE_INTEGRAL:
    case PKL_TYPE_STRING:
      pkl_asm_insn (pasm, rela_insn, op1_type);
      pkl_asm_insn (pasm, PKL_INSN_NIP2);
      break;
    case PKL_TYPE_OFFSET:
      {
        /* Calculate the resulting integral value, which is the
           comparison of both magnitudes, once normalized to bits.
           Note that at this point the magnitude types of both offset
           operands are the same.  */

        pkl_ast_node base_type = PKL_AST_TYPE_O_BASE_TYPE (op1_type);
        pkl_ast_node unit_type = pkl_ast_make_integral_type (PKL_PASS_AST, 64, 0);
        pkl_ast_node unit_bits = pkl_ast_make_integer (PKL_PASS_AST, 1);
        PKL_AST_TYPE (unit_bits) = ASTREF (unit_type);

        /* Equality and inequality are commutative, so we can save an
           instruction here.  */
        if (exp_code != PKL_AST_OP_EQ && exp_code != PKL_AST_OP_NE)
          pkl_asm_insn (pasm, PKL_INSN_SWAP);

        PKL_PASS_SUBPASS (unit_bits);
        pkl_asm_insn (pasm, PKL_INSN_OGETMC, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP);
        pkl_asm_insn (pasm, PKL_INSN_SWAP);
        PKL_PASS_SUBPASS (unit_bits);
        pkl_asm_insn (pasm, PKL_INSN_OGETMC, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP);

        pkl_asm_insn (pasm, rela_insn, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP2);

        ASTREF (unit_bits); pkl_ast_node_free (unit_bits);
      }
      break;
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

/*
 * | OPERAND1
 * EXP
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_op_attr)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node operand = PKL_AST_EXP_OPERAND (exp, 0);
  pkl_ast_node operand_type = PKL_AST_TYPE (operand);
  enum pkl_ast_attr attr = PKL_AST_EXP_ATTR (exp);

  switch (attr)
    {
    case PKL_AST_ATTR_SIZE:
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SIZ);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);
      break;
    case PKL_AST_ATTR_MAGNITUDE:
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);
      break;
    case PKL_AST_ATTR_UNIT:
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETU);
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);
      break;
    case PKL_AST_ATTR_SIGNED:
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);
      if (PKL_AST_TYPE_I_SIGNED (operand_type))
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_int (1, 32));
      else
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_int (0, 32));
      break;
    case PKL_AST_ATTR_LENGTH:
      switch (PKL_AST_TYPE_CODE (operand_type))
        {
        case PKL_TYPE_STRING:
        case PKL_TYPE_ARRAY:
        case PKL_TYPE_STRUCT:
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SEL);
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);
          break;
        default:
          /* This should not happen.  */
          assert (0);
        }
      break;
    case PKL_AST_ATTR_ALIGNMENT:
      /* XXX writeme */
    case PKL_AST_ATTR_OFFSET:
      /* XXX writeme */
    default:
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (exp),
               "unhandled attribute expression code #%d in code generator",
               attr);
      PKL_PASS_ERROR;
      break;
    }
}
PKL_PHASE_END_HANDLER

/* | OPERAND1
 * | OPERAND2
 * EXP
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_ps_op_bconc)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (exp, 0);
  pkl_ast_node op2 = PKL_AST_EXP_OPERAND (exp, 1);

  pkl_ast_node op1_type = PKL_AST_TYPE (op1);
  pkl_ast_node op2_type = PKL_AST_TYPE (op2);
  pkl_ast_node exp_type = PKL_AST_TYPE (exp);

  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BCONC,
                op1_type, op2_type, exp_type);
  pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);
}
PKL_PHASE_END_HANDLER

/* The handler below generates and ICE if a given node isn't handled
   by the code generator.  */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_noimpl)
{
  pkl_ast_node node = PKL_PASS_NODE;

  if (PKL_AST_CODE (node) == PKL_AST_EXP)
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (node),
               "unhandled node #%" PRIu64 " with code %d opcode %d in code generator",
               PKL_AST_UID (node), PKL_AST_CODE (node), PKL_AST_EXP_CODE (node));
    }
  else if (PKL_AST_CODE (node) == PKL_AST_TYPE)
    {
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (node),
               "unhandled node #%" PRIu64 " with code %d typecode %d in code generator",
               PKL_AST_UID (node), PKL_AST_CODE (node), PKL_AST_TYPE_CODE (node));
    }
  else
    pkl_ice (PKL_PASS_AST, PKL_AST_LOC (node),
             "unhandled node #%" PRIu64 " with code %d in code generator",
             PKL_AST_UID (node), PKL_AST_CODE (node));

  PKL_PASS_ERROR;
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_gen =
  {
   PKL_PHASE_PR_HANDLER (PKL_AST_DECL, pkl_gen_pr_decl),
   PKL_PHASE_PS_HANDLER (PKL_AST_DECL, pkl_gen_ps_decl),
   PKL_PHASE_PS_HANDLER (PKL_AST_VAR, pkl_gen_ps_var),
   PKL_PHASE_PR_HANDLER (PKL_AST_COMP_STMT, pkl_gen_pr_comp_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_COMP_STMT, pkl_gen_ps_comp_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_NULL_STMT, pkl_gen_ps_null_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_ASS_STMT, pkl_gen_ps_ass_stmt),
   PKL_PHASE_PR_HANDLER (PKL_AST_IF_STMT, pkl_gen_pr_if_stmt),
   PKL_PHASE_PR_HANDLER (PKL_AST_LOOP_STMT, pkl_gen_pr_loop_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_RETURN_STMT, pkl_gen_ps_return_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_EXP_STMT, pkl_gen_ps_exp_stmt),
   PKL_PHASE_PS_HANDLER (PKL_AST_RAISE_STMT, pkl_gen_ps_raise_stmt),
   PKL_PHASE_PR_HANDLER (PKL_AST_TRY_CATCH_STMT, pkl_gen_pr_try_catch_stmt),
   PKL_PHASE_PR_HANDLER (PKL_AST_FUNCALL, pkl_gen_pr_funcall),
   PKL_PHASE_PS_HANDLER (PKL_AST_FUNCALL, pkl_gen_ps_funcall),
   PKL_PHASE_PS_HANDLER (PKL_AST_FUNCALL_ARG, pkl_gen_ps_funcall_arg),
   PKL_PHASE_PR_HANDLER (PKL_AST_FUNC, pkl_gen_pr_func),
   PKL_PHASE_PS_HANDLER (PKL_AST_FUNC, pkl_gen_ps_func),
   PKL_PHASE_PS_HANDLER (PKL_AST_FUNC_ARG, pkl_gen_ps_func_arg),
   PKL_PHASE_PR_HANDLER (PKL_AST_FUNC_TYPE_ARG, pkl_gen_pr_func_type_arg),
   PKL_PHASE_PR_HANDLER (PKL_AST_TYPE, pkl_gen_pr_type),
   PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_gen_pr_program),
   PKL_PHASE_PS_HANDLER (PKL_AST_PROGRAM, pkl_gen_ps_program),
   PKL_PHASE_PS_HANDLER (PKL_AST_INTEGER, pkl_gen_ps_integer),
   PKL_PHASE_PS_HANDLER (PKL_AST_IDENTIFIER, pkl_gen_ps_identifier),
   PKL_PHASE_PS_HANDLER (PKL_AST_STRING, pkl_gen_ps_string),
   PKL_PHASE_PR_TYPE_HANDLER (PKL_TYPE_OFFSET, pkl_gen_pr_type_offset),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_OFFSET, pkl_gen_ps_type_offset),
   PKL_PHASE_PS_HANDLER (PKL_AST_OFFSET, pkl_gen_ps_offset),
   PKL_PHASE_PS_HANDLER (PKL_AST_CAST, pkl_gen_ps_cast),
   PKL_PHASE_PR_HANDLER (PKL_AST_MAP, pkl_gen_pr_map),
   PKL_PHASE_PS_HANDLER (PKL_AST_SCONS, pkl_gen_ps_scons),
   PKL_PHASE_PS_HANDLER (PKL_AST_ARRAY, pkl_gen_ps_array),
   PKL_PHASE_PS_HANDLER (PKL_AST_ARRAY_REF, pkl_gen_ps_array_ref),
   PKL_PHASE_PR_HANDLER (PKL_AST_ARRAY_INITIALIZER, pkl_gen_ps_array_initializer),
   PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT, pkl_gen_ps_struct),
   PKL_PHASE_PR_HANDLER (PKL_AST_STRUCT_ELEM, pkl_gen_pr_struct_elem),
   PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT_REF, pkl_gen_ps_struct_ref),
   PKL_PHASE_PR_HANDLER (PKL_AST_STRUCT_ELEM_TYPE, pkl_gen_pr_struct_elem_type),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_ADD, pkl_gen_ps_op_add),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_SUB, pkl_gen_ps_op_sub),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_MUL, pkl_gen_ps_op_mul),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_MOD, pkl_gen_ps_op_mod),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_BAND, pkl_gen_ps_op_intexp),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_BNOT, pkl_gen_ps_op_intexp),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_NEG, pkl_gen_ps_op_intexp),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_IOR, pkl_gen_ps_op_intexp),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_XOR, pkl_gen_ps_op_intexp),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_SL, pkl_gen_ps_op_intexp),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_SR, pkl_gen_ps_op_intexp),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_DIV, pkl_gen_ps_op_div),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_AND, pkl_gen_ps_op_and),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_OR, pkl_gen_ps_op_or),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_NOT, pkl_gen_ps_op_not),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_EQ, pkl_gen_ps_op_rela),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_NE, pkl_gen_ps_op_rela),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_LT, pkl_gen_ps_op_rela),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_LE, pkl_gen_ps_op_rela),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_GT, pkl_gen_ps_op_rela),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_GE, pkl_gen_ps_op_rela),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_ATTR, pkl_gen_ps_op_attr),
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_BCONC, pkl_gen_ps_op_bconc),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_INTEGRAL, pkl_gen_ps_type_integral),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_FUNCTION, pkl_gen_ps_type_function),
   PKL_PHASE_PR_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_gen_pr_type_array),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_gen_ps_type_array),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_STRING, pkl_gen_ps_type_string),
   PKL_PHASE_PR_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_gen_pr_type_struct),
   PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_gen_ps_type_struct),
   PKL_PHASE_ELSE_HANDLER (pkl_gen_noimpl),
  };
