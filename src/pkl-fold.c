/* pkl-fold.c - Constant folding phase for the poke compiler. */

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

/* This file implements a constant folding phase.

   XXX: document phase pre-requisites.  */

#include <config.h>

#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "pkl-ast.h"
#include "pkl-pass.h"

static inline uint64_t
emul_or (uint64_t op1, uint64_t op2)
{
  return op1 || op2;
}

static inline uint64_t
emul_ior (uint64_t op1, uint64_t op2)
{
  return op1 | op2;
}

static inline uint64_t
emul_xor (uint64_t op1, uint64_t op2)
{
  return op1 ^ op2;
}

static inline uint64_t
emul_and (uint64_t op1, uint64_t op2)
{
  return op1 && op2;
}

static inline uint64_t
emul_band (uint64_t op1, uint64_t op2)
{
  return op1 & op2;
}

static inline uint64_t
emul_eq (uint64_t op1, uint64_t op2)
{
  return op1 == op2;
}

static inline uint64_t
emul_eqs (const char *op1, const char *op2)
{
  return (strcmp (op1, op2) == 0);
}

static inline uint64_t
emul_ne (uint64_t op1, uint64_t op2)
{
  return (op1 != op2);
}

static inline uint64_t
emul_sl (uint64_t op1, uint64_t op2)
{
  /* XXX writeme */
  assert (0);
  return 0;
}

static inline uint64_t
emul_sr (uint64_t op1, uint64_t op2)
{
  /* XXX writeme */
  assert (0);
  return 0;
}

static inline uint64_t
emul_add (uint64_t op1, uint64_t op2)
{
  return op1 + op2;
}

static inline uint64_t
emul_sub (uint64_t op1, uint64_t op2)
{
  return op1 - op2;
}

static inline uint64_t
emul_mul (uint64_t op1, uint64_t op2)
{
  return op1 * op2;
}

static inline uint64_t
emul_div (uint64_t op1, uint64_t op2)
{
  return op1 / op2;
}

static inline uint64_t
emul_mod (uint64_t op1, uint64_t op2)
{
  return op1 % op2;
}

static inline uint64_t
emul_lt (uint64_t op1, uint64_t op2)
{
  return op1 < op2;
}

static inline uint64_t
emul_gt (uint64_t op1, uint64_t op2)
{
  return op1 > op2;
}

static inline uint64_t
emul_le (uint64_t op1, uint64_t op2)
{
  return op1 <= op2;
}

static inline uint64_t
emul_ge (uint64_t op1, uint64_t op2)
{
  return op1 >= op2;
}

/* Auxiliary macros used in the handlers below.  */

#define OP_BINARY_INT(emul)                                             \
  do                                                                    \
    {                                                                   \
      pkl_ast_node op1 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);        \
      pkl_ast_node op2 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 1);        \
                                                                        \
      if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)                        \
        {                                                               \
          pkl_ast_node new;                                             \
                                                                        \
          new = pkl_ast_make_integer (PKL_PASS_AST,                     \
                                      emul (PKL_AST_INTEGER_VALUE (op1), \
                                            PKL_AST_INTEGER_VALUE (op2))); \
          PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));             \
                                                                        \
          pkl_ast_node_free (PKL_PASS_NODE);                            \
          PKL_PASS_NODE = new;                                          \
          PKL_PASS_RESTART = 1;                                         \
        }                                                               \
    }                                                                   \
  while (0)

#define OP_BINARY_STR(emul)                                             \
  do                                                                    \
    {                                                                   \
      pkl_ast_node op1 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);        \
      pkl_ast_node op2 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 1);        \
                                                                        \
      if (PKL_AST_CODE (op1) == PKL_AST_STRING)                         \
        {                                                               \
          pkl_ast_node new;                                             \
          new = pkl_ast_make_integer (PKL_PASS_AST,                     \
                                      emul (PKL_AST_STRING_POINTER (op1), \
                                            PKL_AST_STRING_POINTER (op2))); \
          PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));             \
                                                                        \
          pkl_ast_node_free (PKL_PASS_NODE);                            \
          PKL_PASS_NODE = new;                                          \
          PKL_PASS_RESTART = 1;                                         \
        }                                                               \
    }                                                                   \
  while (0)

/* Handlers for the several expression codes.  */

#define PKL_PHASE_HANDLER_BIN_INT(op, emul)          \
  PKL_PHASE_BEGIN_HANDLER (pkl_fold_##op)            \
  {                                                  \
    OP_BINARY_INT (emul);                            \
  }                                                  \
  PKL_PHASE_END_HANDLER

PKL_PHASE_HANDLER_BIN_INT (or, emul_or);
PKL_PHASE_HANDLER_BIN_INT (ior, emul_ior);
PKL_PHASE_HANDLER_BIN_INT (xor, emul_xor);
PKL_PHASE_HANDLER_BIN_INT (and, emul_and);
PKL_PHASE_HANDLER_BIN_INT (band, emul_band);
PKL_PHASE_HANDLER_BIN_INT (ne, emul_ne);
PKL_PHASE_HANDLER_BIN_INT (sl, emul_sl);
PKL_PHASE_HANDLER_BIN_INT (sr, emul_sr);
PKL_PHASE_HANDLER_BIN_INT (add, emul_add);
PKL_PHASE_HANDLER_BIN_INT (sub, emul_sub);
PKL_PHASE_HANDLER_BIN_INT (mul, emul_mul);
PKL_PHASE_HANDLER_BIN_INT (div, emul_div);
PKL_PHASE_HANDLER_BIN_INT (mod, emul_mod);
PKL_PHASE_HANDLER_BIN_INT (lt, emul_lt);
PKL_PHASE_HANDLER_BIN_INT (gt, emul_gt);
PKL_PHASE_HANDLER_BIN_INT (le, emul_le);
PKL_PHASE_HANDLER_BIN_INT (ge, emul_ge);

PKL_PHASE_BEGIN_HANDLER (pkl_fold_eq)
{
  OP_BINARY_INT (emul_eq);
  OP_BINARY_STR (emul_eqs);
}
PKL_PHASE_END_HANDLER

#define PKL_PHASE_HANDLER_UNIMPL(op)            \
  PKL_PHASE_BEGIN_HANDLER (pkl_fold_##op)       \
  {                                             \
    assert (0); /* WRITEME */                   \
  }                                             \
  PKL_PHASE_END_HANDLER

PKL_PHASE_HANDLER_UNIMPL (map);
PKL_PHASE_HANDLER_UNIMPL (elemsof);
PKL_PHASE_HANDLER_UNIMPL (typeof);
PKL_PHASE_HANDLER_UNIMPL (sizeof);
PKL_PHASE_HANDLER_UNIMPL (pos);
PKL_PHASE_HANDLER_UNIMPL (neg);
PKL_PHASE_HANDLER_UNIMPL (bnot);
PKL_PHASE_HANDLER_UNIMPL (not);
PKL_PHASE_HANDLER_UNIMPL (sconc);

PKL_PHASE_BEGIN_HANDLER (pkl_fold_cast)
{
#define CAST_TO(T)                                                      \
  do                                                                    \
    {                                                                   \
      pkl_ast_node op1 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);        \
      pkl_ast_node new;                                                 \
                                                                        \
      new = pkl_ast_make_integer (PKL_PASS_AST,                         \
                                  (T) PKL_AST_INTEGER_VALUE (op1));     \
      PKL_AST_TYPE (new) = ASTREF (to_type);                            \
                                                                        \
      pkl_ast_node_free (PKL_PASS_NODE);                                \
      PKL_PASS_NODE = new;                                              \
      PKL_PASS_RESTART = 1;                                             \
    } while (0)

  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node exp = PKL_AST_CAST_EXP (node);

  pkl_ast_node to_type = PKL_AST_CAST_TYPE (node);
  pkl_ast_node from_type = PKL_AST_TYPE (exp);
  
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
            return PKL_PASS_NODE;
          
          if (from_type_size == 64)
            {
              if (to_type_sign)
                /* uint64 -> int64 */
                CAST_TO (int64_t);
              else
                /* int64 -> uint64 */
                CAST_TO (uint64_t);
            }
          else
            {
              if (to_type_sign)
                {
                  switch (from_type_size)
                    {
                    case 8: /* uint8  -> int8 */
                      CAST_TO (int8_t);
                      break;
                    case 16: /* uint16 -> int16 */
                      CAST_TO (int16_t);
                      break;
                    case 32: /* uint32 -> int32 */
                      CAST_TO (int32_t);
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
                      CAST_TO (uint8_t);
                      break;
                    case 16: /* int16 -> uint16 */
                      CAST_TO (uint16_t);
                      break;
                    case 32: /* int32 -> uint32 */
                      CAST_TO (uint32_t);
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
                    CAST_TO (int8_t);
                  else if (from_type_sign && !to_type_sign)
                    /* int64 -> uint8 */
                    CAST_TO (uint8_t);
                  else if (!from_type_sign && to_type_sign)
                    /* uint64 -> int8 */
                    CAST_TO (int8_t);
                  else
                    /* uint64 -> uint8 */
                    CAST_TO (uint8_t);
                  break;
                  
                case 16:
                  if (from_type_sign && to_type_sign)
                    /* int64 -> int16 */
                    CAST_TO (int16_t);
                  else if (from_type_sign && !to_type_sign)
                    /* int64 -> uint16 */
                    CAST_TO (uint16_t);
                  else if (!from_type_sign && to_type_sign)
                    /* uint64 -> int16 */
                    CAST_TO (int16_t);
                  else
                    /* uint64 -> uint16 */
                    CAST_TO (uint16_t);
                  break;
                  
                case 32:
                  if (from_type_sign && to_type_sign)
                    /* int64 -> int32 */
                    CAST_TO (int32_t);
                  else if (from_type_sign && !to_type_sign)
                    /* int64 -> uint32 */
                    CAST_TO (uint32_t);
                  else if (!from_type_sign && to_type_sign)
                    /* uint64 -> int32 */
                    CAST_TO (int32_t);
                  else
                    /* uint64 -> uint32 */
                    CAST_TO (uint32_t);
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
                    CAST_TO (int8_t);
                  else if (from_type_sign && !to_type_sign)
                    /* int32 -> uint8 */
                    CAST_TO (uint8_t);
                  else if (!from_type_sign && to_type_sign)
                    /* uint32 -> int8 */
                    CAST_TO (int8_t);
                  else
                    /* uint32 -> uint8 */
                    CAST_TO (uint8_t);
                  break;
                              
                case 16:
                  if (from_type_sign && to_type_sign)
                    /* int32 -> int16 */
                    CAST_TO (int16_t);
                  else if (from_type_sign && !to_type_sign)
                    /* int32 -> uint16 */
                    CAST_TO (uint16_t);
                  else if (!from_type_sign && to_type_sign)
                    /* uint32 -> int16 */
                    CAST_TO (int16_t);
                  else
                    /* uint32 -> uint16 */
                    CAST_TO (uint16_t);
                  break;
                              
                case 64:
                  if (from_type_sign && to_type_sign)
                    /* int32 -> int64 */
                    CAST_TO (int64_t);
                  else if (from_type_sign && !to_type_sign)
                    /* int32 -> uint64 */
                    CAST_TO (uint64_t);
                  else if (!from_type_sign && to_type_sign)
                    /* uint32 -> int64 */
                    CAST_TO (int64_t);
                  else
                    /* uint32 -> uint64 */
                    CAST_TO (uint64_t);
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
                    CAST_TO (int8_t);
                  else if (from_type_sign && !to_type_sign)
                    /* int16 -> uint8 */
                    CAST_TO (uint8_t);
                  else if (!from_type_sign && to_type_sign)
                    /* uint16 -> int8 */
                    CAST_TO (int8_t);
                  else
                    /* uint16 -> uint8 */
                    CAST_TO (uint8_t);
                  break;
                              
                case 32:
                  if (from_type_sign && to_type_sign)
                    /* int16 -> int32 */
                    CAST_TO (int32_t);
                  else if (from_type_sign && !to_type_sign)
                    /* int16 -> uint32 */
                    CAST_TO (uint32_t);
                  else if (!from_type_sign && to_type_sign)
                    /* uint16 -> int32 */
                    CAST_TO (int32_t);
                  else
                    /* uint16 -> uint32 */
                    CAST_TO (uint32_t);
                  break;
                              
                case 64:
                  if (from_type_sign && to_type_sign)
                    /* int16 -> int64 */
                    CAST_TO (int64_t);
                  else if (from_type_sign && !to_type_sign)
                    /* int16 -> uint64 */
                    CAST_TO (uint64_t);
                  else if (!from_type_sign && to_type_sign)
                    /* uint16 -> int64 */
                    CAST_TO (int64_t);
                  else
                    /* uint16 -> uint64 */
                    CAST_TO (uint64_t);
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
                    CAST_TO (int16_t);
                  else if (from_type_sign && !to_type_sign)
                    /* int8 -> uint16 */
                    CAST_TO (uint16_t);
                  else if (!from_type_sign && to_type_sign)
                    /* uint8 -> int16 */
                    CAST_TO (int16_t);
                  else
                    /* uint8 -> uint16 */
                    CAST_TO (uint16_t);
                  break;

                case 32:
                  if (from_type_sign && to_type_sign)
                    /* int8 -> int32 */
                    CAST_TO (int32_t);
                  else if (from_type_sign && !to_type_sign)
                    /* int8 -> uint32 */
                    CAST_TO (uint32_t);
                  else if (!from_type_sign && to_type_sign)
                    /* uint8-> int32 */
                    CAST_TO (int32_t);
                  else
                    /* uint8 -> uint32 */
                    CAST_TO (uint32_t);
                  break;

                case 64:
                  if (from_type_sign && to_type_sign)
                    /* int8 -> int64 */
                    CAST_TO (int64_t);
                  else if (from_type_sign && !to_type_sign)
                    /* int8 -> uint64 */
                    CAST_TO (uint64_t);
                  else if (!from_type_sign && to_type_sign)
                    /* uint8 -> int64 */
                    CAST_TO (int64_t);
                  else
                    /* uint8 -> uint64 */
                    CAST_TO (uint64_t);
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
#undef CAST_TO
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_fold =
  {
   PKL_PHASE_DF_HANDLER (PKL_AST_CAST, pkl_fold_cast),
#define ENTRY(ops, fs)\
   PKL_PHASE_DF_OP_HANDLER (PKL_AST_OP_##ops, pkl_fold_##fs)

   ENTRY (OR, or), ENTRY (IOR, ior), ENTRY (ADD, add),
   ENTRY (XOR, xor), ENTRY (AND, and), ENTRY (BAND, band),
   ENTRY (EQ, eq), ENTRY (NE, ne), ENTRY (SL, sl),
   ENTRY (SR, sr), ENTRY (ADD, add), ENTRY (SUB, sub),
   ENTRY (MUL, mul), ENTRY (DIV, div), ENTRY (MOD, mod),
   ENTRY (LT, lt), ENTRY (GT, gt), ENTRY (LE, le),
   ENTRY (GE, ge), ENTRY (SCONC, sconc), ENTRY (MAP, map),
   ENTRY (ELEMSOF, elemsof), ENTRY (TYPEOF, typeof),
   ENTRY (POS, pos), ENTRY (NEG, neg), ENTRY (BNOT, bnot),
   ENTRY (NOT, not), ENTRY (SIZEOF, sizeof),
#undef ENTRY
  };
