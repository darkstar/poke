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
#include "pvm.h"

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

/* Generate code for pushing INTEGER to program.  */

static void
append_integer (pvm_program program,
                pkl_ast_node integer)
{
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
  
  pvm_push_val (program, val);
}

/* Generate code to convert a value from FROM_TYPE to TO_TYPE.  */

static void
append_int_cast (pvm_program program,
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
      /* Push the proper conversion instruction.  */
      if ((from_type_size - 1) & ~0x1f)
        {
          if ((to_type_size - 1) & ~0x1f)
            {
              if (from_type_sign && to_type_sign)
                /* From pvm_long to pvm_long  */
                PVM_APPEND_INSTRUCTION (program, ltol);
              else if (from_type_sign && !to_type_sign)
                /* From pvm_long to pvm_ulong */
                PVM_APPEND_INSTRUCTION (program, ltolu);
              else if (!from_type_sign && to_type_sign)
                /* From pvm_ulong to pvm_long */
                PVM_APPEND_INSTRUCTION (program, lutol);
              else
                /* From pvm_ulong to pvm_ulong */
                PVM_APPEND_INSTRUCTION (program, lutolu);
            }
          else
            {
              if (from_type_sign && to_type_sign)
                /* From pvm_long to pvm_int  */
                PVM_APPEND_INSTRUCTION (program, ltoi);
              else if (from_type_sign && !to_type_sign)
                /* From pvm_long to pvm_uint */
                PVM_APPEND_INSTRUCTION (program, ltou);
              else if (!from_type_sign && to_type_sign)
                /* From pvm_ulong to pvm_int */
                PVM_APPEND_INSTRUCTION (program, lutoi);
              else
                /* From pvm_ulong to pvm_uint */
                PVM_APPEND_INSTRUCTION (program, lutou);
            }
        }
      else
        {
          if ((to_type_size - 1) & ~0x1f)
            {
              if (from_type_sign && to_type_sign)
                /* From pvm_int to pvm_long  */
                PVM_APPEND_INSTRUCTION (program, itol);
              else if (from_type_sign && !to_type_sign)
                /* From pvm_int to pvm_ulong */
                PVM_APPEND_INSTRUCTION (program, itolu);
              else if (!from_type_sign && to_type_sign)
                /* From pvm_uint to pvm_long */
                PVM_APPEND_INSTRUCTION (program, utol);
              else
                /* From pvm_uint to pvm_ulong */
                PVM_APPEND_INSTRUCTION (program, utolu);
            }
          else
            {
              if (from_type_sign && to_type_sign)
                /* From pvm_int to pvm_int  */
                PVM_APPEND_INSTRUCTION (program, itoi);
              else if (from_type_sign && !to_type_sign)
                /* From pvm_int to pvm_uint */
                PVM_APPEND_INSTRUCTION (program, itou);
              else if (!from_type_sign && to_type_sign)
                /* From pvm_uint to pvm_int */
                PVM_APPEND_INSTRUCTION (program, utoi);
              else
                /* From pvm_uint to pvm_uint */
                PVM_APPEND_INSTRUCTION (program, utou);
            }
        }

      /* And its argument.  */
      pvm_append_unsigned_literal_parameter (program,
                                             (jitter_uint) to_type_size);
    }
}


/***** Generation phase handlers  *****/

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
  
  val = pvm_make_int (PVM_EXIT_EDIVZ, 32);
  pvm_push_val (program, val);
  
  PVM_APPEND_INSTRUCTION (program, ba);
  pvm_append_symbolic_label_parameter (program, "Lexit");
  
  pvm_append_symbolic_label (program, "Lerror");
  
  val = pvm_make_int (PVM_EXIT_ERROR, 32);
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
  val = pvm_make_int (PVM_EXIT_OK, 32);
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

  append_integer (payload->program, PKL_PASS_NODE);
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

/* OFFSET
 * | BASE_TYPE
 * | MAGNITUDE
 * | UNIT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_offset)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;
  pvm_program program = payload->program;
  pkl_ast_node offset = PKL_PASS_NODE;
  pkl_ast_node offset_type = PKL_AST_TYPE (offset);
  pkl_ast_node base_type = PKL_AST_TYPE_O_BASE_TYPE (offset_type);

  /* We should generate the base type associated with the offset type,
     not the offset type itself.  This relies on the BF handler for
     type offsets below, that does a PASS_BREAK to prevent generating
     the full thing.  */
  
  assert (PKL_AST_TYPE_CODE (base_type) == PKL_TYPE_INTEGRAL);

  pvm_push_val (program,
                pvm_make_ulong (PKL_AST_TYPE_I_SIZE (base_type), 64));

  pvm_push_val (program,
                pvm_make_uint (PKL_AST_TYPE_I_SIGNED (base_type), 32));
  
  PVM_APPEND_INSTRUCTION (program, mktyi);
}
PKL_PHASE_END_HANDLER

/*
 * TYPE_OFFSET
 * | BASE_TYPE
 * | UNIT
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_type_offset)
{
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
  pvm_program program = payload->program;

  PVM_APPEND_INSTRUCTION (program, mko);
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

  pvm_program program = payload->program;
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
      append_int_cast (program, from_type, to_type);
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

      /* XXX: do not do unneeded operations.  */

      /* Push the new base type.  XXX remove this once mko is modified
         to not get the superfluous base type.  */
      pvm_push_val (program,
                    pvm_make_ulong (PKL_AST_TYPE_I_SIZE (to_base_type), 64));
      pvm_push_val (program,
                    pvm_make_uint (PKL_AST_TYPE_I_SIGNED (to_base_type), 32));
      PVM_APPEND_INSTRUCTION (program, mktyi);
      PVM_APPEND_INSTRUCTION (program, swap);

      /* Get the magnitude of the offset, cast it to the new base type
         and convert to new unit.  */

      PVM_APPEND_INSTRUCTION (program, ogetm);
      append_int_cast (program, from_base_type, to_base_type);
      PKL_PASS_SUBPASS (from_base_unit);
      append_int_cast (program, from_base_unit_type, to_base_type);
      /* XXX push mul */
      PVM_APPEND_INSTRUCTION (program, muliu);
      PKL_PASS_SUBPASS (to_base_unit);
      append_int_cast (program, to_base_unit_type, to_base_type);
      /* XXX push div */
      PVM_APPEND_INSTRUCTION (program, diviu);
      PVM_APPEND_INSTRUCTION (program, swap);

      /* Push the new unit.  */
      /* XXX: the unit is an expression!  */
      PKL_PASS_SUBPASS (to_base_unit);
      /*      assert (PKL_AST_CODE (to_base_unit) == PKL_AST_INTEGER); */
      /*      append_integer (program, to_base_unit); */
      PVM_APPEND_INSTRUCTION (program, swap);

      /* Get rid of the original offset.  */
      PVM_APPEND_INSTRUCTION (program, drop);
      /* And create the new one.  */
      PVM_APPEND_INSTRUCTION (program, mko);
    }
  else
    /* XXX: handle casts to structs and arrays.  For structs,
       reorder fields.  */
    assert (0);

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
  pvm_program program = payload->program;
  pkl_ast_node node = PKL_PASS_NODE;

  pvm_push_val (program,
                pvm_make_ulong (PKL_AST_ARRAY_NELEM (node), 64));

  pvm_push_val (program,
                pvm_make_ulong (PKL_AST_ARRAY_NINITIALIZER (node), 64));

  PVM_APPEND_INSTRUCTION (program, mka);
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

  PVM_APPEND_INSTRUCTION (payload->program, aref);
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
                pvm_make_ulong (PKL_AST_STRUCT_NELEM (PKL_PASS_NODE), 64));

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
 * | STRUCT
 * | IDENTIFIER
 * STRUCT_REF
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_struct_ref)
{
  pkl_gen_payload payload
    = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  PVM_APPEND_INSTRUCTION (payload->program, sctref);
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
  pvm_program program = payload->program;
  pkl_ast_node node = PKL_PASS_NODE;

  pvm_push_val (program,
                pvm_make_ulong (PKL_AST_TYPE_I_SIZE (node), 64));

  pvm_push_val (program,
                pvm_make_uint (PKL_AST_TYPE_I_SIGNED (node), 32));
  
  PVM_APPEND_INSTRUCTION (program, mktyi);
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

  PVM_APPEND_INSTRUCTION (payload->program, mktya);
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

  PVM_APPEND_INSTRUCTION (payload->program, mktys);
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
  pvm_program program = payload->program;

  PVM_APPEND_INSTRUCTION (program, mktyo);
}
PKL_PHASE_END_HANDLER
#endif

/*
 * (PKL_AST_ARRAY, PKL_AST_OFFSET, PKL_AST_TYPE,
 *  PKL_AST_STRUCT_ELEM_TYPE)
 * | | STRUCT_TYPE_ELEM
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
 pvm_program program = payload->program;

 pvm_push_val (program,
               pvm_make_ulong (PKL_AST_TYPE_S_NELEM (PKL_PASS_NODE), 64));
 PVM_APPEND_INSTRUCTION (program, mktysct);
}
PKL_PHASE_END_HANDLER

/*
 * (PKL_AST_ARRAY, PKL_AST_OFFSET, PKL_AST_TYPE,
 *  PKL_AST_STRUCT_ELEM_TYPE)
 * | STRUCT_TYPE_ELEM
 * | | [STRUCT_TYPE_ELEM_NAME]
 * | | STRUCT_TYPE_ELEM_TYPE
 */

PKL_PHASE_BEGIN_HANDLER (pkl_gen_bf_struct_type_elem)
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
    pvm_push_val (payload->program, PVM_NULL);
}
PKL_PHASE_END_HANDLER

/* 
 * Expression handlers.
 *
 * | OPERAND1
 * | [OPERAND2]
 * EXP
 */

#define INTEGRAL_EXP(insn)                              \
  do                                                    \
    {                                                   \
      uint64_t size = PKL_AST_TYPE_I_SIZE (type);       \
      int signed_p = PKL_AST_TYPE_I_SIGNED (type);      \
                                                        \
      if ((size - 1) & ~0x1f)                           \
        {                                               \
          if (signed_p)                                 \
            PVM_APPEND_INSTRUCTION (program, insn##l);  \
          else                                          \
            PVM_APPEND_INSTRUCTION (program, insn##lu); \
        }                                               \
      else                                              \
        {                                               \
          if (signed_p)                                 \
            PVM_APPEND_INSTRUCTION (program, insn##i);  \
          else                                          \
            PVM_APPEND_INSTRUCTION (program, insn##iu); \
        }                                               \
    }                                                   \
  while (0)

#define OFFSET_EXP(insn)                                \
  do                                                    \
    {                                                   \
      pkl_ast_node base_type = PKL_AST_TYPE_O_BASE_TYPE (type); \
      uint64_t size = PKL_AST_TYPE_I_SIZE (base_type);  \
      int signed_p = PKL_AST_TYPE_I_SIGNED (base_type); \
                                                        \
      if ((size - 1) & ~0x1f)                           \
        {                                               \
          if (signed_p)                                 \
            PVM_APPEND_INSTRUCTION (program, insn##l);  \
          else                                          \
            PVM_APPEND_INSTRUCTION (program, insn##lu); \
        }                                               \
      else                                              \
        {                                               \
          if (signed_p)                                 \
            PVM_APPEND_INSTRUCTION (program, insn##i);  \
          else                                          \
            PVM_APPEND_INSTRUCTION (program, insn##iu); \
        }                                               \
    }                                                   \
  while (0)

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
      INTEGRAL_EXP (add);
      break;
    case PKL_TYPE_STRING:
      PVM_APPEND_INSTRUCTION (program, sconc);
      break;
    case PKL_TYPE_OFFSET:
      OFFSET_EXP (addo);
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

  pvm_program program = payload->program;
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node type = PKL_AST_TYPE (node);

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      INTEGRAL_EXP (sub);
      break;
    case PKL_TYPE_STRING:
      PVM_APPEND_INSTRUCTION (program, sconc);
      break;
    case PKL_TYPE_OFFSET:
      OFFSET_EXP (subo);
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
  pvm_program program = payload->program;
  pkl_ast_node type = PKL_AST_TYPE (node);
  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (node, 0);
  pkl_ast_node op1_type = PKL_AST_TYPE (op1);

  if (PKL_AST_TYPE_CODE (op1_type) == PKL_TYPE_OFFSET)
    PVM_APPEND_INSTRUCTION (program, boz);
  else
    PVM_APPEND_INSTRUCTION (program, bz);
  
  pvm_append_symbolic_label_parameter (program,
                                       "Ldivzero");

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      {        
        if (PKL_AST_TYPE_CODE (op1_type) == PKL_TYPE_OFFSET)
          INTEGRAL_EXP (divo);
        else
          INTEGRAL_EXP (div);
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
  pvm_program program = payload->program;
  pkl_ast_node type = PKL_AST_TYPE (node);
  pkl_ast_node op1 = PKL_AST_EXP_OPERAND (node, 0);
  pkl_ast_node op1_type = PKL_AST_TYPE (op1);

  if (PKL_AST_TYPE_CODE (op1_type) == PKL_TYPE_OFFSET)
    PVM_APPEND_INSTRUCTION (program, boz);
  else
    PVM_APPEND_INSTRUCTION (program, bz);
  pvm_append_symbolic_label_parameter (program,
                                       "Ldivzero");

  switch (PKL_AST_TYPE_CODE (type))
    {
    case PKL_TYPE_INTEGRAL:
      INTEGRAL_EXP (mod);
      break;
    case PKL_TYPE_OFFSET:
      OFFSET_EXP (modo);
      break;
    default:
      assert (0);
      break;
    }
}
PKL_PHASE_END_HANDLER

#define BIN_INTEGRAL_EXP_HANDLER(op,insn)                       \
  PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_##op)                  \
  {                                                             \
    pkl_gen_payload payload                                     \
      = (pkl_gen_payload) PKL_PASS_PAYLOAD;                     \
    pkl_ast_node node = PKL_PASS_NODE;                          \
    pvm_program program = payload->program;                     \
    pkl_ast_node type = PKL_AST_TYPE (node);                    \
                                                                \
    switch (PKL_AST_TYPE_CODE (type))                           \
      {                                                         \
      case PKL_TYPE_INTEGRAL:                                   \
        INTEGRAL_EXP (insn);                                    \
        break;                                                  \
      default:                                                  \
        assert (0);                                             \
        break;                                                  \
      }                                                         \
  }                                                             \
  PKL_PHASE_END_HANDLER

BIN_INTEGRAL_EXP_HANDLER (mul, mul);
BIN_INTEGRAL_EXP_HANDLER (band, band);
BIN_INTEGRAL_EXP_HANDLER (bnot, bnot);
BIN_INTEGRAL_EXP_HANDLER (neg, neg);
BIN_INTEGRAL_EXP_HANDLER (ior, bor);
BIN_INTEGRAL_EXP_HANDLER (xor, bxor);
BIN_INTEGRAL_EXP_HANDLER (sl, bsl);
BIN_INTEGRAL_EXP_HANDLER (sr, bsr);

#define LOGIC_EXP_HANDLER(op)                           \
  PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_##op)          \
  {                                                     \
    pkl_gen_payload payload                             \
      = (pkl_gen_payload) PKL_PASS_PAYLOAD;             \
                                                        \
    PVM_APPEND_INSTRUCTION (payload->program, op);      \
  }                                                     \
  PKL_PHASE_END_HANDLER

LOGIC_EXP_HANDLER (and);
LOGIC_EXP_HANDLER (or);
LOGIC_EXP_HANDLER (not);

#undef LOGIC_EXP_HANDLER

#define RELA_EXP_HANDLER(op)                            \
  PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_##op)          \
  {                                                     \
    pkl_gen_payload payload                             \
      = (pkl_gen_payload) PKL_PASS_PAYLOAD;             \
                                                        \
    pvm_program program = payload->program;             \
    pkl_ast_node node = PKL_PASS_NODE;                  \
    pkl_ast_node type = PKL_AST_TYPE (node);            \
                                                        \
    switch (PKL_AST_TYPE_CODE (type))                   \
      {                                                 \
      case PKL_TYPE_INTEGRAL:                           \
        INTEGRAL_EXP (op);                              \
        break;                                          \
      case PKL_TYPE_STRING:                             \
        PVM_APPEND_INSTRUCTION (program, op##s);        \
        break;                                          \
      default:                                          \
        assert (0);                                     \
        break;                                          \
      }                                                 \
  }                                                     \
  PKL_PHASE_END_HANDLER

RELA_EXP_HANDLER (eq);
RELA_EXP_HANDLER (ne);
RELA_EXP_HANDLER (lt);
RELA_EXP_HANDLER (le);
RELA_EXP_HANDLER (gt);
RELA_EXP_HANDLER (ge);

#undef RELA_EXP_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_gen_df_op_sizeof)
{
  pkl_gen_payload payload
      = (pkl_gen_payload) PKL_PASS_PAYLOAD;

  PVM_APPEND_INSTRUCTION (payload->program, siz);
}
PKL_PHASE_END_HANDLER

#undef BIN_INTEGRAL_EXP_HANDLER
#undef INTEGRAL_EXP

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
   PKL_PHASE_ELSE_HANDLER (pkl_gen_noimpl),
   PKL_PHASE_BF_HANDLER (PKL_AST_PROGRAM, pkl_gen_bf_program),
   PKL_PHASE_DF_HANDLER (PKL_AST_PROGRAM, pkl_gen_df_program),
   PKL_PHASE_DF_HANDLER (PKL_AST_INTEGER, pkl_gen_df_integer),
   PKL_PHASE_DF_HANDLER (PKL_AST_IDENTIFIER, pkl_gen_df_identifier),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRING, pkl_gen_df_string),
   PKL_PHASE_BF_HANDLER (PKL_AST_OFFSET, pkl_gen_bf_offset),
   PKL_PHASE_BF_TYPE_HANDLER (PKL_TYPE_OFFSET, pkl_gen_bf_type_offset),
   PKL_PHASE_DF_HANDLER (PKL_AST_OFFSET, pkl_gen_df_offset),
   PKL_PHASE_DF_HANDLER (PKL_AST_CAST, pkl_gen_df_cast),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY, pkl_gen_df_array),
   PKL_PHASE_DF_HANDLER (PKL_AST_ARRAY_REF, pkl_gen_df_array_ref),
   PKL_PHASE_BF_HANDLER (PKL_AST_ARRAY_INITIALIZER, pkl_gen_df_array_initializer),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT, pkl_gen_df_struct),
   PKL_PHASE_BF_HANDLER (PKL_AST_STRUCT_ELEM, pkl_gen_bf_struct_elem),
   PKL_PHASE_DF_HANDLER (PKL_AST_STRUCT_REF, pkl_gen_df_struct_ref),
   PKL_PHASE_BF_HANDLER (PKL_AST_STRUCT_ELEM_TYPE, pkl_gen_bf_struct_type_elem),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_ADD, pkl_gen_df_op_add),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SUB, pkl_gen_df_op_sub),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_MUL, pkl_gen_df_op_mul),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_MOD, pkl_gen_df_op_mod),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_BAND, pkl_gen_df_op_band),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_BNOT, pkl_gen_df_op_bnot),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NEG, pkl_gen_df_op_neg),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_IOR, pkl_gen_df_op_ior),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_XOR, pkl_gen_df_op_xor),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SL, pkl_gen_df_op_sl),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SR, pkl_gen_df_op_sr),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_DIV, pkl_gen_df_op_div),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_AND, pkl_gen_df_op_and),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_OR, pkl_gen_df_op_or),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NOT, pkl_gen_df_op_not),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_EQ, pkl_gen_df_op_eq),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_NE, pkl_gen_df_op_ne),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LT, pkl_gen_df_op_lt),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_LE, pkl_gen_df_op_le),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GT, pkl_gen_df_op_gt),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_GE, pkl_gen_df_op_ge),
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_SIZEOF, pkl_gen_df_op_sizeof),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_INTEGRAL, pkl_gen_df_type_integral),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_gen_df_type_array),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_STRING, pkl_gen_df_type_string),
   PKL_PHASE_DF_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_gen_df_type_struct),
  };
