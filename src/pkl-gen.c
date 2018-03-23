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

#include "pvm.h"
#include "pkl-gen.h"
#include "pkl-ast.h"
#include "pkl-pass.h"

/* The following function is used to push pvm_val values to the PVM
   stack.  */

static inline void
pvm_push_val (pvm_program program, pvm_val val)
{
#if __WORDSIZE == 64
  PVM_APPEND_INSTRUCTION (program, push);
  pvm_append_unsigned_literal_parameter (program,
                                         (jitter_uint) val);
#else
  /* Use the push-hi and push-lo instructions, to overcome jitter's
     limitation of only accepting a jitter_uint value as a literal
     argument, which is 32-bit long in 32-bit hosts.  */

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

  pvm_val val;
  pvm_program program;
  size_t label;
  
  program = pvm_make_program ();
  label = 0;

  /* Standard prologue.  */
  PVM_APPEND_INSTRUCTION (program, ba);
  pvm_append_symbolic_label_parameter (program,
                                       "Lstart");
  
  pvm_append_symbolic_label (program, "Ldivzero");
  
  val = pvm_make_int (PVM_EXIT_EDIVZ);
  pvm_push_val (program, val);
  
  PVM_APPEND_INSTRUCTION (program, ba);
  pvm_append_symbolic_label_parameter (program, "Lexit");
  
  pvm_append_symbolic_label (program, "Lerror");
  
  val = pvm_make_int (PVM_EXIT_ERROR);
  pvm_push_val (program, val);
  
  pvm_append_symbolic_label (program, "Lexit");
  PVM_APPEND_INSTRUCTION (program, exit);
  
  pvm_append_symbolic_label (program, "Lstart");

  /* Initialize payload.  */
  payload->program = program;
  payload->label = label;
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
  struct pvm_program *program = payload->program;
  pvm_val val;

  /* Standard epilogue.  */
  val = pvm_make_int (PVM_EXIT_OK);
  pvm_push_val (program, val);
    
  PVM_APPEND_INSTRUCTION (program, ba);
  pvm_append_symbolic_label_parameter (program, "Lexit");
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

  type = PKL_AST_TYPE (integer);
  assert (type != NULL
          && PKL_AST_TYPE_CODE (type) == PKL_TYPE_INTEGRAL);

  switch (PKL_AST_TYPE_I_SIZE (type))
    {
    case 64:
      if (PKL_AST_TYPE_I_SIGNED (type))
        val = pvm_make_long (PKL_AST_INTEGER_VALUE (integer));
      else
        val = pvm_make_ulong (PKL_AST_INTEGER_VALUE (integer));
      break;

    case 32:
      if (PKL_AST_TYPE_I_SIGNED (type))
        val = pvm_make_int (PKL_AST_INTEGER_VALUE (integer));
      else
        val = pvm_make_uint (PKL_AST_INTEGER_VALUE (integer));
      break;

    case 16:
      if (PKL_AST_TYPE_I_SIGNED (type))
        val = pvm_make_half (PKL_AST_INTEGER_VALUE (integer));
      else
        val = pvm_make_uhalf (PKL_AST_INTEGER_VALUE (integer));
      break;

    case 8:
      if (PKL_AST_TYPE_I_SIGNED (type))
        val = pvm_make_byte (PKL_AST_INTEGER_VALUE (integer));
      else
        val = pvm_make_ubyte (PKL_AST_INTEGER_VALUE (integer));
      break;

    default:
      assert (0);
      break;
    }

  pvm_push_val (payload->program, val);
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

  pvm_push_val (payload->program, val);
}
PKL_PHASE_END_HANDLER

/*
 * ARRAY_INITIALIZER
 * | ARRAY_INITIALIZER_EXP
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_array_initializer)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;
  pkl_ast_node node = PKL_PASS_NODE;
  pvm_val idx
    = pvm_make_ulong (PKL_AST_ARRAY_INITIALIZER_INDEX (node));

  pvm_push_val (payload->program, idx);
}
PKL_PHASE_END_HANDLER

/*
 *  | ARRAY_INITIALIZER
 *  | ...
 *  | ARRAY_TYPE
 *  ARRAY
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_array)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;
  pvm_program program = payload->program;
  pkl_ast_node node = PKL_PASS_NODE;

  pvm_push_val (program,
                pvm_make_ulong (PKL_AST_ARRAY_NELEM (node)));

  pvm_push_val (program,
                pvm_make_ulong (PKL_AST_ARRAY_NINITIALIZER (node)));

  PVM_APPEND_INSTRUCTION (program, mka);
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
  pvm_program program = payload->program;

  pvm_push_val (program,
                pvm_make_ulong (PKL_AST_STRUCT_NELEM (PKL_PASS_NODE)));

  PVM_APPEND_INSTRUCTION (program, mksct);
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
    pvm_push_val (payload->program, PVM_NULL);
}
PKL_PHASE_END_HANDLER

/*
 * TYPE_INTEGRAL
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_type_integral)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;
  pvm_program program = payload->program;
  pkl_ast_node node = PKL_PASS_NODE;

  pvm_push_val (program,
                pvm_make_ulong (PKL_AST_TYPE_I_SIZE (node)));

  pvm_push_val (program,
                pvm_make_uint (PKL_AST_TYPE_I_SIGNED (node)));
  
  PVM_APPEND_INSTRUCTION (program, mktyi);
}
PKL_PHASE_END_HANDLER

/*
 * | ETYPE
 * | NELEM
 * TYPE_ARRAY
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_type_array)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  PVM_APPEND_INSTRUCTION (payload->program, mktya);
}
PKL_PHASE_END_HANDLER

/* 
 * All the expression handlers below have this AST context:
 *
 * | OPERAND1
 * | [OPERAND2]
 * EXP
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_add)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  pvm_program program = payload->program;
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (node);

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      {
        uint64_t size = PKL_AST_TYPE_I_SIZE (type);
        int signed_p = PKL_AST_TYPE_I_SIGNED (type);

        /* XXX: use a table with pvm_append_instruction_name (program,
           "addb") */
        
        switch (size)
          {
          case 8:
            if (signed_p)
              PVM_APPEND_INSTRUCTION (program, addb);
            else
              PVM_APPEND_INSTRUCTION (program, addbu);
            break;
            
          case 16:
            if (signed_p)
              PVM_APPEND_INSTRUCTION (program, addh);
            else
              PVM_APPEND_INSTRUCTION (program, addhu);
            break;
            
          case 32:
            if (signed_p)
              PVM_APPEND_INSTRUCTION (program, addi);
            else
              PVM_APPEND_INSTRUCTION (program, addiu);
            break;
            
          case 64:
            if (signed_p)
              PVM_APPEND_INSTRUCTION (program, addl);
            else
              PVM_APPEND_INSTRUCTION (program, addlu);
            break;
          }
        break;
      }
    case PKL_TYPE_STRING:
      PVM_APPEND_INSTRUCTION (program, sconc);
      break;
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

/* The handler below genrates and ICE if a node couldn't be processed
   by the code generator.  */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_noimpl)
{
  printf ("error: unrecognized node code %d in code generator\n",
          PKL_AST_CODE (PKL_PASS_NODE));
  PKL_PASS_ERROR;
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_gen =
  { .default_handler = pkl_gen_noimpl,
    .code_bf_handlers[PKL_AST_PROGRAM] = pkl_gen_bf_program,
    .code_df_handlers[PKL_AST_PROGRAM] = pkl_gen_df_program,
    .code_df_handlers[PKL_AST_INTEGER] = pkl_gen_df_integer,
    .code_df_handlers[PKL_AST_STRING] = pkl_gen_df_string,
    .code_df_handlers[PKL_AST_ARRAY] = pkl_gen_df_array,
    .code_bf_handlers[PKL_AST_ARRAY_INITIALIZER] = pkl_gen_bf_array_initializer,
    .code_df_handlers[PKL_AST_STRUCT] = pkl_gen_df_struct,
    .code_bf_handlers[PKL_AST_STRUCT_ELEM] = pkl_gen_bf_struct_elem,
    .op_df_handlers[PKL_AST_OP_ADD] = pkl_gen_df_op_add,
    .type_df_handlers[PKL_TYPE_INTEGRAL] = pkl_gen_df_type_integral,
    .type_df_handlers[PKL_TYPE_ARRAY] = pkl_gen_df_type_array,
  };

#if 0

static int
pkl_gen_op (pkl_ast_node ast,
            pvm_program program,
            size_t *label,
            enum pkl_ast_op what)
{
  pkl_ast_node type = PKL_AST_TYPE (ast);

#define PVM_APPEND_ARITH_INSTRUCTION(what,suffix)               \
  do                                                            \
    {                                                           \
      if (what == PKL_AST_OP_DIV || what == PKL_AST_OP_MOD)     \
        {                                                       \
          PVM_APPEND_INSTRUCTION (program, bz);                 \
          pvm_append_symbolic_label_parameter (program,         \
                                               "Ldivzero");     \
        }                                                       \
                                                                \
      switch ((what))                                           \
        {                                                       \
        case PKL_AST_OP_NEG:                                    \
          PVM_APPEND_INSTRUCTION (program, neg##suffix);        \
          break;                                                \
        case PKL_AST_OP_ADD:                                    \
          PVM_APPEND_INSTRUCTION (program, add##suffix);        \
          break;                                                \
        case PKL_AST_OP_SUB:                                    \
          PVM_APPEND_INSTRUCTION (program, sub##suffix);        \
          break;                                                \
        case PKL_AST_OP_MUL:                                    \
          PVM_APPEND_INSTRUCTION (program, mul##suffix);        \
          break;                                                \
        case PKL_AST_OP_DIV:                                    \
          PVM_APPEND_INSTRUCTION (program, div##suffix);        \
          break;                                                \
        case PKL_AST_OP_MOD:                                    \
          PVM_APPEND_INSTRUCTION (program, mod##suffix);        \
          break;                                                \
        case PKL_AST_OP_BAND:                                   \
          PVM_APPEND_INSTRUCTION (program, band##suffix);       \
          break;                                                \
        case PKL_AST_OP_IOR:                                    \
          PVM_APPEND_INSTRUCTION (program, bor##suffix);        \
          break;                                                \
        case PKL_AST_OP_XOR:                                    \
          PVM_APPEND_INSTRUCTION (program, bxor##suffix);       \
          break;                                                \
        case PKL_AST_OP_BNOT:                                   \
          PVM_APPEND_INSTRUCTION (program, bnot##suffix);       \
          break;                                                \
        case PKL_AST_OP_SL:                                     \
          PVM_APPEND_INSTRUCTION (program, bsl##suffix);        \
          break;                                                \
        case PKL_AST_OP_SR:                                     \
          PVM_APPEND_INSTRUCTION (program, bsr##suffix);        \
          break;                                                \
        case PKL_AST_OP_EQ:                                     \
          PVM_APPEND_INSTRUCTION (program, eq##suffix);         \
          break;                                                \
        case PKL_AST_OP_NE:                                     \
          PVM_APPEND_INSTRUCTION (program, ne##suffix);         \
          break;                                                \
        case PKL_AST_OP_LT:                                     \
          PVM_APPEND_INSTRUCTION (program, lt##suffix);         \
          break;                                                \
        case PKL_AST_OP_LE:                                     \
          PVM_APPEND_INSTRUCTION (program, le##suffix);         \
          break;                                                \
        case PKL_AST_OP_GT:                                     \
          PVM_APPEND_INSTRUCTION (program, gt##suffix);         \
          break;                                                \
        case PKL_AST_OP_GE:                                     \
          PVM_APPEND_INSTRUCTION (program, ge##suffix);         \
          break;                                                \
        default:                                                \
          break;                                                \
        }                                                       \
    } while (0)
  
  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      {
        uint64_t size = PKL_AST_TYPE_I_SIZE (type);
        int signed_p = PKL_AST_TYPE_I_SIGNED (type);
        
        switch (size)
        {
        case 8:
          if (signed_p)
            PVM_APPEND_ARITH_INSTRUCTION (what, b);
          else
            PVM_APPEND_ARITH_INSTRUCTION (what, bu);
          break;

        case 16:
          if (signed_p)
              PVM_APPEND_ARITH_INSTRUCTION (what, h);
          else
            PVM_APPEND_ARITH_INSTRUCTION (what, hu);
          break;

        case 32:
          if (signed_p)
            PVM_APPEND_ARITH_INSTRUCTION (what, i);
          else
            PVM_APPEND_ARITH_INSTRUCTION (what, iu);
          break;

        case 64:
          if (signed_p)
            PVM_APPEND_ARITH_INSTRUCTION (what, l);
          else
            PVM_APPEND_ARITH_INSTRUCTION (what, lu);
          break;

        default:
          assert (0);
          break;
        }

        break;
      }
    case PKL_TYPE_STRING:
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
    case PKL_TYPE_ARRAY:
      /* Fallthrough.  */
    case PKL_TYPE_STRUCT:
      /* Fallthrough.  */
      break;
    default:
      assert (0);
      break;
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

  if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_INTEGRAL
      && PKL_AST_TYPE_I_SIZE (type) == 32)
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

static int pkl_gen_exp (pkl_ast_node ast,
                        pvm_program program,
                        size_t *label);

static int pkl_gen_type (pkl_ast_node ast,
                         pvm_program program,
                         size_t *label);

static int
pkl_gen_type_struct (pkl_ast_node ast,
                     pvm_program program,
                     size_t *label)
{
  pkl_ast_node t;
  
  /* Basic case: all the struct elements are completely defined at
     compilation time.  This means:

     - All the struct elements are field definitions.  No
       statements, no declarations.

     - The struct type has no arguments.

     - There are no array sizes depending on other fields.  */

  if (1)
    {
      for (t = PKL_AST_TYPE_S_ELEMS (ast); t; t = PKL_AST_CHAIN (t))
        {
          pkl_ast_node struct_type_elem_name
            = PKL_AST_STRUCT_TYPE_ELEM_NAME (t);
          pkl_ast_node struct_type_elem_type
            = PKL_AST_STRUCT_TYPE_ELEM_TYPE (t);
          char *ename
            = (struct_type_elem_name
               ? PKL_AST_IDENTIFIER_POINTER (struct_type_elem_name)
               : NULL);
          
          /* Push the struct element name.  */
          if (ename == NULL)
            pvm_push_val (program, PVM_NULL);
          else
            pvm_push_val (program,
                          pvm_make_string (ename));
          
          /* Push the struct element type.  */
          if (!pkl_gen_type (struct_type_elem_type, program, label))
            return 0;
        }

      pvm_push_val (program,
                    pvm_make_ulong (PKL_AST_TYPE_S_NELEM (ast)));
      
      PVM_APPEND_INSTRUCTION (program, mktysct);
    }
  else
    {
      assert (0);
      
      /* Complex case: this must create a _program_ containing three
         functions setting things up, creating a struct type and
         returning it.
         
         The struct functions are invoked by:
         
         - Initialized to zero:
           let packet p;
         
         - Casts to structs:
           (packet) {1,2,3}
           (struct { int i; long j;}) {10}
 
         - Mappings:
           packet @ 0x0

         So we need several functions defined here:
            - A function that creates the struct type from 0.
            - A function that creates the struct type from
              another struct value.
            - A function that creates the struct type from
              an IO space.
      */

      /* XXX: BA to skip the function definition.  */
      /* XXX: label and function prologue  */
      /* XXX: begin lexical scope.  */
      
      /* XXX: end lexical scope.  */
      /* XXX: function epilogue.  */
      /* XXX: label after function.  */
    }

  return 1;
}

static int
pkl_gen_type (pkl_ast_node ast,
              pvm_program program,
              size_t *label)
{
  if (PKL_AST_TYPE_CODE (ast) == PKL_TYPE_INTEGRAL)
    {
      pvm_push_val (program,
                    pvm_make_ulong (PKL_AST_TYPE_I_SIZE (ast)));

      pvm_push_val (program,
                    pvm_make_uint (PKL_AST_TYPE_I_SIGNED (ast)));

      PVM_APPEND_INSTRUCTION (program, mktyi);
    }
  else if (PKL_AST_TYPE_CODE (ast) == PKL_TYPE_STRING)
    {
      PVM_APPEND_INSTRUCTION (program, mktys);
    }
  else if (PKL_AST_TYPE_CODE (ast) == PKL_TYPE_OFFSET)
    {
      PVM_APPEND_INSTRUCTION (program, mktyo);
    }
  else if (PKL_AST_TYPE_CODE (ast) == PKL_TYPE_ARRAY)
    {
      pkl_ast_node nelem = PKL_AST_TYPE_A_NELEM (ast);
      
      if (!pkl_gen_type (PKL_AST_TYPE_A_ETYPE (ast),
                         program, label))
        return 0;

      /* The number of elements of the array type can be either an
         INTEGER or an EXPRESSION.  */
      switch (PKL_AST_CODE (nelem))
        {
        case PKL_AST_INTEGER:
          if (!pkl_gen_integer (nelem, program, label))
            return 0;
          break;
        case PKL_AST_EXP:
          if (!pkl_gen_exp (PKL_AST_TYPE_A_NELEM (ast),
                            program, label))
            return 0;
          break;
        default:
          assert (0);
          break;
        }
      
      PVM_APPEND_INSTRUCTION (program, mktya);
    }
  else if (PKL_AST_TYPE_CODE (ast) == PKL_TYPE_STRUCT)
    return pkl_gen_type_struct (ast, program, label);
  else
    assert (0);
    
  return 1;
}

static int
pkl_gen_exp_cast (pkl_ast_node ast,
                  pvm_program program,
                  size_t *label)
{
  pkl_ast_node to_type;
  pkl_ast_node from_type;

  to_type = PKL_AST_TYPE (ast);
  from_type = PKL_AST_TYPE (PKL_AST_EXP_OPERAND (ast, 0));
  
  if (PKL_AST_TYPE_CODE (from_type) == PKL_TYPE_INTEGRAL
      && PKL_AST_TYPE_CODE (to_type) == PKL_TYPE_INTEGRAL)
    {
      size_t from_type_size = PKL_AST_TYPE_I_SIZE (from_type);
      int from_type_sign = PKL_AST_TYPE_I_SIGNED (from_type);
      
      size_t to_type_size = PKL_AST_TYPE_I_SIZE (to_type);
      int to_type_sign = PKL_AST_TYPE_I_SIGNED (to_type);

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
                  switch (from_type_size)
                    {
                    case 8: /* uint8  -> int8 */
                      PVM_APPEND_INSTRUCTION (program, butob);
                      break;
                    case 16: /* uint16 -> int16 */
                      PVM_APPEND_INSTRUCTION (program, hutoh);
                      break;
                    case 32: /* uint32 -> int32 */
                      PVM_APPEND_INSTRUCTION (program, iutoi);
                      break;
                    default:
                      assert (0);
                      break;
                    }
                }
              else
                {
                  switch (from_type_size)
                    {
                    case 8: /* int8 -> uint8 */
                      PVM_APPEND_INSTRUCTION (program, btobu);
                      break;
                    case 16: /* int16 -> uint16 */
                      PVM_APPEND_INSTRUCTION (program, htohu);
                      break;
                    case 32: /* int32 -> uint32 */
                      PVM_APPEND_INSTRUCTION (program, itoiu);
                      break;
                    default:
                      assert (0);
                      break;
                    }
                }
            }
        }
      else /* from_type_size != to_type_size */
        {
          switch (from_type_size)
            {
            case 64:
              switch (to_type_size)
                {
                case 8:
                  if (from_type_sign && to_type_sign)
                    /* int64 -> int8 */
                    PVM_APPEND_INSTRUCTION (program, ltob);
                  else if (from_type_sign && !to_type_sign)
                    /* int64 -> uint8 */
                    PVM_APPEND_INSTRUCTION (program, ltobu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint64 -> int8 */
                    PVM_APPEND_INSTRUCTION (program, lutob);
                  else
                    /* uint64 -> uint8 */
                    PVM_APPEND_INSTRUCTION (program, lutobu);
                  break;

                case 16:
                  if (from_type_sign && to_type_sign)
                    /* int64 -> int16 */
                    PVM_APPEND_INSTRUCTION (program, ltoh);
                  else if (from_type_sign && !to_type_sign)
                    /* int64 -> uint16 */
                    PVM_APPEND_INSTRUCTION (program, ltohu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint64 -> int16 */
                    PVM_APPEND_INSTRUCTION (program, lutoh);
                  else
                    /* uint64 -> uint16 */
                    PVM_APPEND_INSTRUCTION (program, lutohu);
                  break;

                case 32:
                  if (from_type_sign && to_type_sign)
                    /* int64 -> int32 */
                    PVM_APPEND_INSTRUCTION (program, ltoi);
                  else if (from_type_sign && !to_type_sign)
                    /* int64 -> uint32 */
                    PVM_APPEND_INSTRUCTION (program, ltoiu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint64 -> int32 */
                    PVM_APPEND_INSTRUCTION (program, lutoi);
                  else
                    /* uint64 -> uint32 */
                    PVM_APPEND_INSTRUCTION (program, lutoiu);
                  break;

                default:
                  assert (0);
                  break;
                }
              break;

            case 32:
              switch (to_type_size)
                {
                case 8:
                  if (from_type_sign && to_type_sign)
                    /* int32 -> int8 */
                    PVM_APPEND_INSTRUCTION (program, itob);
                  else if (from_type_sign && !to_type_sign)
                    /* int32 -> uint8 */
                    PVM_APPEND_INSTRUCTION (program, itobu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint32 -> int8 */
                    PVM_APPEND_INSTRUCTION (program, iutob);
                  else
                    /* uint32 -> uint8 */
                    PVM_APPEND_INSTRUCTION (program, iutobu);
                  break;

                case 16:
                  if (from_type_sign && to_type_sign)
                    /* int32 -> int16 */
                    PVM_APPEND_INSTRUCTION (program, itoh);
                  else if (from_type_sign && !to_type_sign)
                    /* int32 -> uint16 */
                    PVM_APPEND_INSTRUCTION (program, itohu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint32 -> int16 */
                    PVM_APPEND_INSTRUCTION (program, iutoh);
                  else
                    /* uint32 -> uint16 */
                    PVM_APPEND_INSTRUCTION (program, iutohu);
                  break;

                case 64:
                  if (from_type_sign && to_type_sign)
                    /* int32 -> int64 */
                    PVM_APPEND_INSTRUCTION (program, itol);
                  else if (from_type_sign && !to_type_sign)
                    /* int32 -> uint64 */
                    PVM_APPEND_INSTRUCTION (program, itolu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint32 -> int64 */
                    PVM_APPEND_INSTRUCTION (program, iutol);
                  else
                    /* uint32 -> uint64 */
                    PVM_APPEND_INSTRUCTION (program, iutolu);
                  break;

                default:
                  assert (0);
                  break;
                }
              break;

            case 16:
              switch (to_type_size)
                {
                case 8:
                  if (from_type_sign && to_type_sign)
                    /* int16 -> int8 */
                    PVM_APPEND_INSTRUCTION (program, htob);
                  else if (from_type_sign && !to_type_sign)
                    /* int16 -> uint8 */
                    PVM_APPEND_INSTRUCTION (program, htobu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint16 -> int8 */
                    PVM_APPEND_INSTRUCTION (program, hutob);
                  else
                    /* uint16 -> uint8 */
                    PVM_APPEND_INSTRUCTION (program, hutobu);
                  break;

                case 32:
                  if (from_type_sign && to_type_sign)
                    /* int16 -> int32 */
                    PVM_APPEND_INSTRUCTION (program, htoi);
                  else if (from_type_sign && !to_type_sign)
                    /* int16 -> uint32 */
                    PVM_APPEND_INSTRUCTION (program, htoiu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint16 -> int32 */
                    PVM_APPEND_INSTRUCTION (program, hutoi);
                  else
                    /* uint16 -> uint32 */
                    PVM_APPEND_INSTRUCTION (program, hutoiu);
                  break;

                case 64:
                  if (from_type_sign && to_type_sign)
                    /* int16 -> int64 */
                    PVM_APPEND_INSTRUCTION (program, htol);
                  else if (from_type_sign && !to_type_sign)
                    /* int16 -> uint64 */
                    PVM_APPEND_INSTRUCTION (program, htolu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint16 -> int64 */
                    PVM_APPEND_INSTRUCTION (program, hutol);
                  else
                    /* uint16 -> uint64 */
                    PVM_APPEND_INSTRUCTION (program, hutolu);
                  break;

                default:
                  assert (0);
                  break;
                }
              break;

            case 8:
              switch (to_type_size)
                {
                case 16:
                  if (from_type_sign && to_type_sign)
                    /* int8 -> int16 */
                    PVM_APPEND_INSTRUCTION (program, btoh);
                  else if (from_type_sign && !to_type_sign)
                    /* int8 -> uint16 */
                    PVM_APPEND_INSTRUCTION (program, btohu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint8 -> int16 */
                    PVM_APPEND_INSTRUCTION (program, butoh);
                  else
                    /* uint8 -> uint16 */
                    PVM_APPEND_INSTRUCTION (program, butohu);
                  break;

                case 32:
                  if (from_type_sign && to_type_sign)
                    /* int8 -> int32 */
                    PVM_APPEND_INSTRUCTION (program, btoi);
                  else if (from_type_sign && !to_type_sign)
                    /* int8 -> uint32 */
                    PVM_APPEND_INSTRUCTION (program, btoiu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint8-> int32 */
                    PVM_APPEND_INSTRUCTION (program, butoi);
                  else
                    /* uint8 -> uint32 */
                    PVM_APPEND_INSTRUCTION (program, butoiu);
                  break;

                case 64:
                  if (from_type_sign && to_type_sign)
                    /* int8 -> int64 */
                    PVM_APPEND_INSTRUCTION (program, btol);
                  else if (from_type_sign && !to_type_sign)
                    /* int8 -> uint64 */
                    PVM_APPEND_INSTRUCTION (program, btolu);
                  else if (!from_type_sign && to_type_sign)
                    /* uint8 -> int64 */
                    PVM_APPEND_INSTRUCTION (program, butol);
                  else
                    /* uint8 -> uint64 */
                    PVM_APPEND_INSTRUCTION (program, butolu);
                  break;

                default:
                  assert (0);
                  break;
                }
              break;
            default:
              assert (0);
              break;
            }
        }
    }
  else
    /* XXX: handle casts to structs and arrays.  For structs, reorder
       fields.  */
    assert (0);

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

    case PKL_AST_OP_SIZEOF:
      PVM_APPEND_INSTRUCTION (program, siz);
      break;
    case PKL_AST_OP_ELEMSOF:
      PVM_APPEND_INSTRUCTION (program, sel);
      break;
    case PKL_AST_OP_TYPEOF:
      PVM_APPEND_INSTRUCTION (program, typof);
      break;

    case PKL_AST_OP_MAP:
      PVM_APPEND_INSTRUCTION (program, mkm);
      break;

    case PKL_AST_OP_CAST:
      return pkl_gen_exp_cast (ast, program, label);
      break;

    default:
      fprintf (stderr, "gen: unhandled expression code %d\n",
               PKL_AST_EXP_CODE (ast));
      return 0;
    }

  return 1;
}

static int
pkl_gen_array_ref (pkl_ast_node ast,
                   pvm_program program,
                   size_t *label)
{
  pkl_gen_1 (PKL_AST_ARRAY_REF_ARRAY (ast), program, label);
  pkl_gen_1 (PKL_AST_ARRAY_REF_INDEX (ast), program, label);
  PVM_APPEND_INSTRUCTION (program, aref);

  return 1;
}

static int
pkl_gen_struct (pkl_ast_node ast,
                pvm_program program,
                size_t *label)
{
  pkl_ast_node e;
  /* DONE */

  for (e = PKL_AST_STRUCT_ELEMS (ast);
       e;
       e = PKL_AST_CHAIN (e))
    {
      pvm_val name;

      if (PKL_AST_STRUCT_ELEM_NAME (e) == NULL)
        name = PVM_NULL;
      else
        name
          = pvm_make_string (PKL_AST_IDENTIFIER_POINTER (PKL_AST_STRUCT_ELEM_NAME (e)));
      
      pvm_push_val (program, name);
      pkl_gen_1 (PKL_AST_STRUCT_ELEM_EXP (e), program, label);
    }

  pvm_push_val (program,
                pvm_make_ulong (PKL_AST_STRUCT_NELEM (ast)));

  PVM_APPEND_INSTRUCTION (program, mksct);
  return 1;
}

static int
pkl_gen_struct_ref (pkl_ast_node ast,
                    pvm_program program,
                    size_t *label)
{
  char *name
    = PKL_AST_IDENTIFIER_POINTER (PKL_AST_STRUCT_REF_IDENTIFIER (ast));
  
  pkl_gen_1 (PKL_AST_STRUCT_REF_STRUCT (ast), program, label);

  pvm_push_val (program, pvm_make_string (name));
  PVM_APPEND_INSTRUCTION (program, sctref);

  return 1;
}

static int
pkl_gen_offset (pkl_ast_node ast,
                pvm_program program,
                size_t *label)
{
  pkl_ast_node type;
  pvm_val val;

  type = PKL_AST_TYPE (ast);
  if (!pkl_gen_type (PKL_AST_TYPE_O_BASE_TYPE (type),
                     program, label))
    return 0;

  if (!pkl_gen_1 (PKL_AST_OFFSET_MAGNITUDE (ast),
                  program, label))
    return 0;

  switch (PKL_AST_OFFSET_UNIT (ast))
    {
    case PKL_AST_OFFSET_UNIT_BITS:
      val = pvm_make_ulong (PVM_VAL_OFF_UNIT_BITS);
      break;
    case PKL_AST_OFFSET_UNIT_BYTES:
      val = pvm_make_ulong (PVM_VAL_OFF_UNIT_BYTES);
      break;
    default:
      /* Invalid unit. */
      assert (0);
    }
  pvm_push_val (program, val);
      
  PVM_APPEND_INSTRUCTION (program, mko);
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

    case PKL_AST_ARRAY:
      if (!pkl_gen_array (ast, program, label))
        goto error;
      break;

    case PKL_AST_ARRAY_REF:
      if (!pkl_gen_array_ref (ast, program, label))
        goto error;
      break;

    case PKL_AST_STRUCT_REF:
      if (!pkl_gen_struct_ref (ast, program, label))
        goto error;
      break;

    case PKL_AST_STRUCT:
      if (!pkl_gen_struct (ast, program, label))
        goto error;
      break;

    case PKL_AST_TYPE:
      if (!pkl_gen_type (ast, program, label))
        goto error;
      break;

    case PKL_AST_OFFSET:
      if (!pkl_gen_offset (ast, program, label))
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

#endif
