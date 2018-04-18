/* pkl-gen.c - Code generation phase for the poke compiler.  */

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
#include <stdio.h>
#include <assert.h>
#include <jitter/jitter.h>

#include "pkl.h"
#include "pkl-gen.h"
#include "pkl-ast.h"
#include "pkl-pass.h"
#include "pkl-asm.h"
#include "pvm.h"

/*
 * PROGRAM
 * | PROGRAM_ELEM
 * | ...
 *
 * This function initializes the payload and also generates the
 * standard prologue.
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_program)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  payload->pasm = pkl_asm_new (PKL_PASS_AST);
}
PKL_PHASE_END_HANDLER

/*
 * | PROGRAM_ELEM
 * | ...
 * PROGRAM
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_program)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  payload->program = pkl_asm_finish (payload->pasm);
}
PKL_PHASE_END_HANDLER

/*
 * DECL
 * | INITIAL
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_decl)
{
  /* if DEFUN

       - Save a copy of the partial program in payload->program.
       - Start a new pvm_program for the function body, and
       put it in the payload.  */

  /* XXX: stop for now.  */
  PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

/*
 * | INITIAL
 * DECL
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_decl)
{
  /* if DEFUN

       - Especialize payload->program
       - Make a pvm_val for a closure, containing payload->program
         and the current environment.
       - Push a new environment, if parent != PROGRAM.
       - Register the pvm_val fun in the environment.

      if DEFVAR

       - INITIAL pushed a value in the stack.
       - Push a new environment, if parent != PROGRAM.
       - Register it in the environment.

      if DEFTYPE  (???)
       
       - INITIAL pushed a value in the stack.
       - Push a new environment, if parent != PROGRAM.
       - Register it in the environment.
    */

  /* XXX */
}
PKL_PHASE_END_HANDLER

/*
 * COMP_STMT
 * | (STMT | DECL)
 * | ...
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_comp_stmt)
{
  /* Push a frame into the environment.  */
  /* XXX */
}
PKL_PHASE_END_HANDLER

/*
 * | (STMT | DECL)
 * | ...
 * COMP_STMT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_comp_stmt)
{
  /* Pop N+1 frames from the environment.  */
  /* XXX */
}
PKL_PHASE_END_HANDLER

/*
 * FUNC
 * | [TYPE]
 * | [FUNC_ARG]...
 * | BODY
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_func)
{
  /* Function prologue:
     - Push an environment.
  */

  /* XXX */
}
PKL_PHASE_END_HANDLER

/*
 * FUNC_ARG
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_func_arg)
{
  /* Pop the argument from the stack and put it in the current
     environment.  */

  /* XXX  */
}
PKL_PHASE_END_HANDLER

/*
 * | [TYPE]
 * | [FUNC_ARG]...
 * | BODY
 * FUNC
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_func)
{
  /* Function epilogue:

     - Push the return value in the stack, if the function returns a
       value.
     - Pop the function's environment.
     - Return to the caller: link.
   */
}
PKL_PHASE_END_HANDLER

/*
 * INTEGER
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_integer)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

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
  
  pkl_asm_insn (payload->pasm, PKL_INSN_PUSH, val);
}
PKL_PHASE_END_HANDLER

/*
 * IDENTIFIER
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_identifier)
{
  /* XXX this doesn't feel right.  */

  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node identifier = PKL_PASS_NODE;
  pvm_val val
    = pvm_make_string (PKL_AST_IDENTIFIER_POINTER (identifier));

  pkl_asm_insn (payload->pasm, PKL_INSN_PUSH, val);
}
PKL_PHASE_END_HANDLER

/*
 * STRING
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_string)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node string = PKL_PASS_NODE;
  pvm_val val
    = pvm_make_string (PKL_AST_STRING_POINTER (string));

  pkl_asm_insn (payload->pasm, PKL_INSN_PUSH, val);
}
PKL_PHASE_END_HANDLER

/*
 * TYPE
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_type)
{
  /* Avoid generating type nodes in certain circumstances.  */
  if (PKL_PASS_PARENT
      && (PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_STRUCT
          || PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_INTEGER
          || PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_STRING
          || PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_OFFSET
          || PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_MAP))
    PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

/*
 * TYPE_OFFSET
 * | BASE_TYPE
 * | UNIT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_type_offset)
{
  /* We do not need to generate code for the offset type.  */
  PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

/*
 * | TYPE
 * | MAGNITUDE
 * | UNIT
 * OFFSET
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_offset)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;
  pkl_asm pasm = payload->pasm;

  pkl_asm_insn (pasm, PKL_INSN_MKO);
}
PKL_PHASE_END_HANDLER

/*
 * | EXP
 * CAST
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_cast)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm pasm = payload->pasm;
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
      pkl_asm_insn (pasm, PKL_INSN_OGETM);
      pkl_asm_insn (pasm, PKL_INSN_NTON,
                    from_base_type, to_base_type);

      PKL_PASS_SUBPASS (from_base_unit);
      pkl_asm_insn (pasm, PKL_INSN_NTON,
                    from_base_unit_type, to_base_type);

      pkl_asm_insn (pasm, PKL_INSN_MUL, to_base_type);

      PKL_PASS_SUBPASS (to_base_unit);
      pkl_asm_insn (pasm, PKL_INSN_NTON,
                    to_base_unit_type, to_base_type);

      pkl_asm_insn (pasm, PKL_INSN_DIV, to_base_type);
      pkl_asm_insn (pasm, PKL_INSN_SWAP);

      /* Push the new unit.  */
      PKL_PASS_SUBPASS (to_base_unit);
      pkl_asm_insn (pasm, PKL_INSN_SWAP);

      /* Get rid of the original offset.  */
      pkl_asm_insn (pasm, PKL_INSN_DROP);
      /* And create the new one.  */
      pkl_asm_insn (pasm, PKL_INSN_MKO);
    }
  else
    /* XXX: handle casts to structs and arrays.  For structs,
       reorder fields.  */
    assert (0);

}
PKL_PHASE_END_HANDLER

/*
 * | MAP_OFFSET
 * MAP
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_map)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;
  pkl_asm pasm = payload->pasm;

  pkl_ast_node map = PKL_PASS_NODE;
  pkl_ast_node map_type = PKL_AST_MAP_TYPE (map);

  switch (PKL_AST_TYPE_CODE (map_type))
    {
    case PKL_TYPE_INTEGRAL:
    case PKL_TYPE_STRING:
      pkl_asm_insn (pasm, PKL_INSN_PEEK, map_type);
      break;
    case PKL_TYPE_OFFSET:
      pkl_asm_insn (pasm, PKL_INSN_PEEK,
                    PKL_AST_TYPE_O_BASE_TYPE (map_type));
      PKL_PASS_SUBPASS (PKL_AST_TYPE_O_UNIT (map_type));
      pkl_asm_insn (pasm, PKL_INSN_MKO);
      break;
    case PKL_TYPE_ARRAY:
      /* XXX: call to the std function std_map_array.  Error if we are
         bootstrapping and this operation is not yet available.  */
    case PKL_TYPE_STRUCT:
      /* XXX: call to the std function std_map_struct.  Error if we
         are bootstrapping and this operation is not yet
         available.  */
    default:
      pkl_ice (PKL_PASS_AST, PKL_AST_LOC (map_type),
               "unhandled node type in codegen for node map #%" PRIu64,
               PKL_AST_UID (map));
      break;
    }
}
PKL_PHASE_END_HANDLER

/*
 * | ARRAY_INITIALIZER_INDEX
 * | ARRAY_INITIALIZER_EXP
 * ARRAY_INITIALIZER
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_array_initializer)
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

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_array)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;
  pkl_asm pasm = payload->pasm;
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

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_array_ref)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm_insn (payload->pasm, PKL_INSN_AREF);
}
PKL_PHASE_END_HANDLER

/*
 *  | STRUCT_ELEM
 *  | ...
 *  STRUCT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_struct)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;
  pkl_asm pasm = payload->pasm;

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

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_struct_elem)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  /* If the struct initializer doesn't include a name, generate a null
     value as expected by the mksct instruction.  */
  if (!PKL_AST_STRUCT_ELEM_NAME (PKL_PASS_NODE))
    pkl_asm_insn (payload->pasm, PKL_INSN_PUSH, PVM_NULL);
}
PKL_PHASE_END_HANDLER

/*
 * | STRUCT
 * | IDENTIFIER
 * STRUCT_REF
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_struct_ref)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm_insn (payload->pasm, PKL_INSN_SREF);
}
PKL_PHASE_END_HANDLER

/*
 * (PKL_AST_ARRAY, PKL_AST_OFFSET, PKL_AST_TYPE,
    PKL_AST_STRUCT_ELEM_TYPE)
 * | TYPE_INTEGRAL
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_type_integral)
  PKL_PHASE_PARENT (4,
                    PKL_AST_ARRAY,
                    PKL_AST_OFFSET,
                    PKL_AST_TYPE,
                    PKL_AST_STRUCT_ELEM_TYPE)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;
  pkl_asm pasm = payload->pasm;
  pkl_ast_node node = PKL_PASS_NODE;

  pkl_asm_insn (pasm, PKL_INSN_PUSH,
                pvm_make_ulong (PKL_AST_TYPE_I_SIZE (node), 64));

  pkl_asm_insn (pasm, PKL_INSN_PUSH,
                pvm_make_uint (PKL_AST_TYPE_I_SIGNED (node), 32));

  pkl_asm_insn (pasm, PKL_INSN_MKTYI);
}
PKL_PHASE_END_HANDLER

/*
 * (PKL_AST_ARRAY, PKL_AST_OFFSET, PKL_AST_TYPE,
    PKL_AST_STRUCT_ELEM_TYPE)
 * | | ETYPE
 * | | NELEM
 * | TYPE_ARRAY
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_type_array)
  PKL_PHASE_PARENT (4,
                    PKL_AST_ARRAY,
                    PKL_AST_OFFSET,
                    PKL_AST_TYPE,
                    PKL_AST_STRUCT_ELEM_TYPE)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm_insn (payload->pasm, PKL_INSN_MKTYA);
}
PKL_PHASE_END_HANDLER

/*
 * (PKL_AST_ARRAY, PKL_AST_OFFSET, PKL_AST_TYPE,
 *  PKL_AST_STRUCT_ELEM_TYPE)
 * | TYPE_STRING
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_type_string)
  PKL_PHASE_PARENT (4,
                    PKL_AST_ARRAY,
                    PKL_AST_OFFSET,
                    PKL_AST_TYPE,
                    PKL_AST_STRUCT_ELEM_TYPE)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm_insn (payload->pasm, PKL_INSN_MKTYS);
}
PKL_PHASE_END_HANDLER

#if 0
/*
 * (PKL_AST_ARRAY, PKL_AST_OFFSET, PKL_AST_TYPE,
 *  PKL_AST_STRUCT_ELEM_TYPE)
 * | | BASE_TYPE
 * | | UNIT
 * | TYPE_OFFSET
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_type_offset)
  PKL_PHASE_PARENT (4,
                    PKL_AST_ARRAY,
                    PKL_AST_OFFSET,
                    PKL_AST_TYPE,
                    PKL_AST_STRUCT_ELEM_TYPE)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm_insn (payload->pasm, PKL_INSN_MKTYO);
}
PKL_PHASE_END_HANDLER
#endif

/*
 * TYPE_STRUCT
 * | STRUCT_ELEM_TYPE
 * | ...
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_type_struct)
{
  /* Push a frame to the environment.  */
  /* XXX */
}
PKL_PHASE_END_HANDLER

/*
 * (PKL_AST_ARRAY, PKL_AST_OFFSET, PKL_AST_TYPE,
 *  PKL_AST_STRUCT_ELEM_TYPE)
 * | | STRUCT_ELEM_TYPE
 * | | ...
 * | TYPE_STRUCT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_type_struct)
  PKL_PHASE_PARENT (4,
                    PKL_AST_ARRAY,
                    PKL_AST_OFFSET,
                    PKL_AST_TYPE,
                    PKL_AST_STRUCT_ELEM_TYPE)
{
 pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;
 pkl_asm pasm = payload->pasm;

 pkl_asm_insn (pasm, PKL_INSN_PUSH,
               pvm_make_ulong (PKL_AST_TYPE_S_NELEM (PKL_PASS_NODE), 64));
 pkl_asm_insn (pasm, PKL_INSN_MKTYSCT);

 /* XXX: pop N+1 frames from the environment.  */
}
PKL_PHASE_END_HANDLER

/*
 * (PKL_AST_ARRAY, PKL_AST_OFFSET, PKL_AST_TYPE,
 *  PKL_AST_STRUCT_ELEM_TYPE)
 * | STRUCT_ELEM_TYPE
 * | | [STRUCT_ELEM_TYPE_NAME]
 * | | STRUCT_ELEM_TYPE_TYPE
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_struct_elem_type)
  PKL_PHASE_PARENT (4,
                    PKL_AST_ARRAY,
                    PKL_AST_OFFSET,
                    PKL_AST_TYPE,
                    PKL_AST_STRUCT_ELEM_TYPE)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  /* If the struct type element doesn't include a name, generate a
     null value as expected by the mktysct instruction.  */
  if (!PKL_AST_STRUCT_ELEM_TYPE_NAME (PKL_PASS_NODE))
    pkl_asm_insn (payload->pasm, PKL_INSN_PUSH, PVM_NULL);
}
PKL_PHASE_END_HANDLER

/* 
 * Expression handlers.
 *
 * | OPERAND1
 * | [OPERAND2]
 * EXP
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_add)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm pasm = payload->pasm;
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (node);

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      pkl_asm_insn (pasm, PKL_INSN_ADD, type);
      break;
    case PKL_TYPE_STRING:
      pkl_asm_insn (pasm, PKL_INSN_SCONC);
      break;
    case PKL_TYPE_OFFSET:
      {
        /* Calculate the magnitude of the new offset, which is the
           addition of both magnitudes, once normalized to bits.
           Since addition is commutative we can process OFF2 first and
           save a swap.  */

        pkl_ast_node base_type = PKL_AST_TYPE_O_BASE_TYPE (type);
        pkl_ast_node res_unit = PKL_AST_TYPE_O_UNIT (type);

        PKL_PASS_SUBPASS (res_unit);
        pkl_asm_insn (pasm, PKL_INSN_OGETMC, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP);
        pkl_asm_insn (pasm, PKL_INSN_SWAP);
        PKL_PASS_SUBPASS (res_unit);
        pkl_asm_insn (pasm, PKL_INSN_OGETMC, base_type);
        pkl_asm_insn (pasm, PKL_INSN_NIP);
        pkl_asm_insn (pasm, PKL_INSN_ADD, base_type);

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

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_sub)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm pasm = payload->pasm;
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (node);

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      pkl_asm_insn (pasm, PKL_INSN_SUB, type);
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

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_mul)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm pasm = payload->pasm;
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (node);

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      pkl_asm_insn (pasm, PKL_INSN_MUL, type);
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

        /* The operation is commutative, so there is no need to swap
           the arguments.  */

        if (op2_type_code == PKL_TYPE_OFFSET)
          {
            pkl_asm_insn (pasm, PKL_INSN_OGETM);
            pkl_asm_insn (pasm, PKL_INSN_NIP);

            offset_op = op2;
          }

        pkl_asm_insn (pasm, PKL_INSN_SWAP);

        if (op1_type_code == PKL_TYPE_OFFSET)
          {
            pkl_asm_insn (pasm, PKL_INSN_OGETM);
            pkl_asm_insn (pasm, PKL_INSN_NIP);

            offset_op = op1;
          }

        assert (offset_op != NULL);
        offset_type = PKL_AST_TYPE (offset_op);
        offset_unit = PKL_AST_TYPE_O_UNIT (offset_type);
        base_type = PKL_AST_TYPE_O_BASE_TYPE (offset_type);

        pkl_asm_insn (pasm, PKL_INSN_MUL, base_type);
          
        PKL_PASS_SUBPASS (offset_unit);
        pkl_asm_insn (pasm, PKL_INSN_MKO);
      }
      break;
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_div)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_ast_node node = PKL_PASS_NODE;
  pkl_asm pasm = payload->pasm;
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

            ASTREF (unit_bits); pkl_ast_node_free (unit_bits);
          }
        else
          pkl_asm_insn (pasm, PKL_INSN_DIV, type);
        break;
      }
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_mod)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;
  pkl_ast_node node = PKL_PASS_NODE;

  pkl_asm pasm = payload->pasm;
  pkl_ast_node type = PKL_AST_TYPE (node);
  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (node, 0);
  pkl_ast_node op1_type = PKL_AST_TYPE (op1);

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      pkl_asm_insn (pasm, PKL_INSN_MOD, type);
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

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_intexp)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;
  pkl_asm pasm = payload->pasm;

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
      break;
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_and)
{
  pkl_gen_payload payload
      = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm_insn (payload->pasm, PKL_INSN_AND);
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_or)
{
  pkl_gen_payload payload
      = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm_insn (payload->pasm, PKL_INSN_OR);
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_not)
{
  pkl_gen_payload payload
      = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm_insn (payload->pasm, PKL_INSN_NOT);
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_rela)
{
  pkl_gen_payload payload
      = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm pasm = payload->pasm;
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

        ASTREF (unit_bits); pkl_ast_node_free (unit_bits);
      }
      break;
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

#undef RELA_EXP_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_sizeof)
{
  pkl_gen_payload payload
      = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pkl_asm_insn (payload->pasm, PKL_INSN_SIZ);
}
PKL_PHASE_END_HANDLER

#undef BIN_INTEGRAL_EXP_HANDLER

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
   PKL_PHASE_BF_HANDLER (PKL_AST_DECL, pkl_gen_bf_decl),
   PKL_PHASE_DF_HANDLER (PKL_AST_DECL, pkl_gen_df_decl),
   PKL_PHASE_BF_HANDLER (PKL_AST_COMP_STMT, pkl_gen_bf_comp_stmt),
   PKL_PHASE_DF_HANDLER (PKL_AST_COMP_STMT, pkl_gen_df_comp_stmt),
   PKL_PHASE_BF_HANDLER (PKL_AST_FUNC, pkl_gen_bf_func),
   PKL_PHASE_DF_HANDLER (PKL_AST_FUNC, pkl_gen_df_func),
   PKL_PHASE_DF_HANDLER (PKL_AST_FUNC_ARG, pkl_gen_df_func_arg),
   PKL_PHASE_BF_HANDLER (PKL_AST_TYPE, pkl_gen_bf_type),
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_gen_bf_program),
   PKL_PHASE_DF_HANDLER (PKL_AST_PROGRAM, pkl_gen_df_program),
   PKL_PHASE_DF_HANDLER (PKL_AST_INTEGER, pkl_gen_df_integer),
   PKL_PHASE_DF_HANDLER (PKL_AST_IDENTIFIER, pkl_gen_df_identifier),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRING, pkl_gen_df_string),
   PKL_PHASE_BF_TYPE_HANDLER (PKL_TYPE_OFFSET, pkl_gen_bf_type_offset),
   PKL_PHASE_DF_HANDLER (PKL_AST_OFFSET, pkl_gen_df_offset),
   PKL_PHASE_DF_HANDLER (PKL_AST_CAST, pkl_gen_df_cast),
   PKL_PHASE_DF_HANDLER (PKL_AST_MAP, pkl_gen_df_map),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY, pkl_gen_df_array),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY_REF, pkl_gen_df_array_ref),
   PKL_PHASE_BF_HANDLER (PKL_AST_ARRAY_INITIALIZER, pkl_gen_df_array_initializer),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT, pkl_gen_df_struct),
   PKL_PHASE_BF_HANDLER (PKL_AST_STRUCT_ELEM, pkl_gen_bf_struct_elem),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT_REF, pkl_gen_df_struct_ref),
   PKL_PHASE_BF_HANDLER (PKL_AST_STRUCT_ELEM_TYPE, pkl_gen_bf_struct_elem_type),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_ADD, pkl_gen_df_op_add),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SUB, pkl_gen_df_op_sub),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_MUL, pkl_gen_df_op_mul),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_MOD, pkl_gen_df_op_mod),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_BAND, pkl_gen_df_op_intexp),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_BNOT, pkl_gen_df_op_intexp),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NEG, pkl_gen_df_op_intexp),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_IOR, pkl_gen_df_op_intexp),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_XOR, pkl_gen_df_op_intexp),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SL, pkl_gen_df_op_intexp),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SR, pkl_gen_df_op_intexp),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_DIV, pkl_gen_df_op_div),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_AND, pkl_gen_df_op_and),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_OR, pkl_gen_df_op_or),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NOT, pkl_gen_df_op_not),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_EQ, pkl_gen_df_op_rela),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NE, pkl_gen_df_op_rela),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LT, pkl_gen_df_op_rela),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LE, pkl_gen_df_op_rela),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GT, pkl_gen_df_op_rela),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GE, pkl_gen_df_op_rela),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SIZEOF, pkl_gen_df_op_sizeof),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_INTEGRAL, pkl_gen_df_type_integral),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_gen_df_type_array),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_STRING, pkl_gen_df_type_string),
   PKL_PHASE_BF_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_gen_bf_type_struct),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_gen_df_type_struct),
   PKL_PHASE_ELSE_HANDLER (pkl_gen_noimpl),
  };
