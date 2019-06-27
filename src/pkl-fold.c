/* pkl-fold.c - Constant folding phase for the poke compiler. */

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

/* This file implements a constant folding phase.  */

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

PKL_PHASE_BEGIN_HANDLER (pkl_fold_ps_cast)
{
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node exp = PKL_AST_CAST_EXP (node);

  pkl_ast_node to_type = PKL_AST_CAST_TYPE (node);
  pkl_ast_node from_type = PKL_AST_TYPE (exp);

  if (PKL_AST_TYPE_CODE (from_type) == PKL_TYPE_INTEGRAL
      && PKL_AST_TYPE_CODE (to_type) == PKL_TYPE_INTEGRAL)
    {
      //      size_t from_type_size = PKL_AST_TYPE_I_SIZE (from_type);
      //      int from_type_sign = PKL_AST_TYPE_I_SIGNED (from_type);
      
      //      size_t to_type_size = PKL_AST_TYPE_I_SIZE (to_type);
      //      int to_type_sign = PKL_AST_TYPE_I_SIGNED (to_type);

      pkl_ast_node op1 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);
      pkl_ast_node new;
      uint64_t new_value;

      new_value = PKL_AST_INTEGER_VALUE (op1);

      new = pkl_ast_make_integer (PKL_PASS_AST, new_value);
      PKL_AST_TYPE (new) = ASTREF (to_type);
      PKL_AST_LOC (new) = PKL_AST_LOC (op1);
      
      pkl_ast_node_free (PKL_PASS_NODE);
      PKL_PASS_NODE = new;
      PKL_PASS_RESTART = 1; /* XXX ??? */
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_fold =
  {
   PKL_PHASE_PS_HANDLER (PKL_AST_CAST, pkl_fold_ps_cast),
#define ENTRY(ops, fs)\
   PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_##ops, pkl_fold_##fs)

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
