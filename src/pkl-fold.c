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

#include "pkl.h"
#include "pkl-ast.h"
#include "pkl-pass.h"

/* Roll out our own GCD from gnulib.  */
#define WORD_T uint64_t
#define GCD pkl_gcd
#include <gcd.c>

/* Emulation routines.

   The letter-codes after EMUL_ specify the number and kind of
   arguments that the operations receive and return.  The type of the
   returned value comes last.
   
   So, for example, EMUL_III declares an int64 OP int64 -> int64
   operation, whereas EMUL_SSI declares a string OP string -> int64
   operation.  */

#define EMUL_UNA_PROTO(OP,SIGN,TYPE,RTYPE)              \
  static inline RTYPE emul_##SIGN##_##OP (TYPE op)

#define EMUL_BIN_PROTO(OP,SIGN,TYPE,RTYPE)                      \
  static inline RTYPE emul_##SIGN##_##OP (TYPE op1, TYPE op2)

#define EMUL_II(OP)                       \
  EMUL_UNA_PROTO (OP,s,int64_t,int64_t)
#define EMUL_UU(OP)                       \
  EMUL_UNA_PROTO (OP,u,uint64_t,uint64_t)
#define EMUL_III(OP)                      \
  EMUL_BIN_PROTO (OP,s,int64_t,int64_t)
#define EMUL_UUU(OP)                      \
  EMUL_BIN_PROTO (OP,u,uint64_t,uint64_t)
#define EMUL_UUI(OP)                      \
  EMUL_BIN_PROTO (OP,u,uint64_t,int64_t)
#define EMUL_SSI(OP)                          \
  EMUL_BIN_PROTO (OP,s,const char *,int64_t)

EMUL_II (neg) { return -op; }
EMUL_UU (neg) { return -op; }
EMUL_II (pos) { return op; }
EMUL_UU (pos) { return op; }
EMUL_II (not) { return !op; }
EMUL_UU (not) { return !op; }
EMUL_II (bnot) { return ~op; }
EMUL_UU (bnot) { return ~op; }

EMUL_UUU (or) { return op1 || op2; }
EMUL_III (or) { return op1 || op2; }
EMUL_UUU (ior) { return op1 | op2; }
EMUL_III (ior) { return op1 | op2; }
EMUL_UUU (xor) { return op1 ^ op2; }
EMUL_III (xor) { return op1 ^ op2; }
EMUL_UUU (and) { return op1 && op2; }
EMUL_III (and) { return op1 && op2; }
EMUL_UUU (band) { return op1 & op2; }
EMUL_III (band) { return op1 & op2; }
EMUL_UUU (eq) { return op1 == op2; }
EMUL_III (eq) { return op1 == op2; }
EMUL_UUU (ne) { return op1 != op2; }
EMUL_III (ne) { return op1 != op2; }
EMUL_UUU (add) { return op1 + op2; }
EMUL_III (add) { return op1 + op2; }
EMUL_UUU (sub) { return op1 - op2; }
EMUL_III (sub) { return op1 - op2; }
EMUL_UUU (mul) { return op1 * op2; }
EMUL_III (mul) { return op1 * op2; }
EMUL_UUU (div) { return op1 / op2; }
EMUL_III (div) { return op1 / op2; }
EMUL_UUU (mod) { return op1 % op2; }
EMUL_III (mod) { return op1 % op2; }
EMUL_UUU (lt) { return op1 < op2; }
EMUL_III (lt) { return op1 < op2; }
EMUL_UUU (gt) { return op1 > op2; }
EMUL_III (gt) { return op1 > op2; }
EMUL_UUU (le) { return op1 <= op2; }
EMUL_III (le) { return op1 <= op2; }
EMUL_UUU (ge) { return op1 >= op2; }
EMUL_III (ge) { return op1 >= op2; }

EMUL_UUU (sl) { assert (0); return 0; } /* XXX WRITEME */
EMUL_III (sl) { assert (0); return 0; } /* XXX WRITEME */
EMUL_UUU (sr) { assert (0); return 0; } /* XXX WRITEME */
EMUL_III (sr) { assert (0); return 0; } /* XXX WRITEME */

EMUL_SSI (eqs) { return (strcmp (op1, op2) == 0); }
EMUL_SSI (nes) { return (strcmp (op1, op2) != 0); }
EMUL_SSI (gts) { return (strcmp (op1, op2) > 0); }
EMUL_SSI (lts) { return (strcmp (op1, op2) < 0); }
EMUL_SSI (les) { return (strcmp (op1, op2) <= 0); }
EMUL_SSI (ges) { return (strcmp (op1, op2) >= 0); }

/* The following emulation routines work on offset magnitudes
   normalized to bits.  */
EMUL_UUI (eqo) { return op1 == op2; }
EMUL_UUI (neo) { return op1 != op2; }
EMUL_UUI (gto) { return op1 > op2; }
EMUL_UUI (lto) { return op1 < op2; }
EMUL_UUI (leo) { return op1 <= op2; }
EMUL_UUI (geo) { return op1 >= op2; }
EMUL_III (eqo) { return op1 == op2; }
EMUL_III (neo) { return op1 != op2; }
EMUL_III (gto) { return op1 > op2; }
EMUL_III (lto) { return op1 < op2; }
EMUL_III (leo) { return op1 <= op2; }
EMUL_III (geo) { return op1 >= op2; }

/* Auxiliary macros used in the handlers below.  */

#define OP_UNARY_II(OP)                         \
  do                                                                    \
    {                                                                   \
      pkl_ast_node type = PKL_AST_TYPE (PKL_PASS_NODE);                 \
      pkl_ast_node op = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);         \
                                                                        \
      if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_INTEGRAL)                \
        {                                                               \
          pkl_ast_node new;                                             \
          uint64_t result;                                              \
                                                                        \
          if (PKL_AST_CODE (op) != PKL_AST_INTEGER)                     \
            /* We cannot fold this expression.  */                      \
            PKL_PASS_DONE;                                              \
                                                                        \
          if (PKL_AST_TYPE_I_SIGNED (type))                             \
            result = emul_s_##OP (PKL_AST_INTEGER_VALUE (op));          \
          else                                                          \
            result = emul_u_##OP (PKL_AST_INTEGER_VALUE (op));          \
                                                                        \
          new = pkl_ast_make_integer (PKL_PASS_AST, result);            \
          PKL_AST_TYPE (new) = ASTREF (type);                           \
          PKL_AST_LOC (new) = PKL_AST_LOC (PKL_PASS_NODE);              \
                                                                        \
          pkl_ast_node_free (PKL_PASS_NODE);                            \
          PKL_PASS_NODE = new;                                          \
        }                                                               \
    }                                                                   \
  while (0)

#define OP_BINARY_OOI(OP)                                               \
  do                                                                    \
    {                                                                   \
      pkl_ast_node op1 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);        \
      pkl_ast_node op2 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 1);        \
      pkl_ast_node type = PKL_AST_TYPE (PKL_PASS_NODE);                 \
      pkl_ast_node op1_type = PKL_AST_TYPE (op1);                       \
      pkl_ast_node op2_type = PKL_AST_TYPE (op2);                       \
                                                                        \
      if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_INTEGRAL                 \
          && PKL_AST_TYPE_CODE (op1_type) == PKL_TYPE_OFFSET            \
          && PKL_AST_TYPE_CODE (op2_type) == PKL_TYPE_OFFSET)           \
        {                                                               \
          pkl_ast_node new;                                             \
          pkl_ast_node op1_magnitude = PKL_AST_OFFSET_MAGNITUDE (op1);  \
          pkl_ast_node op1_unit = PKL_AST_OFFSET_UNIT (op1);            \
          pkl_ast_node op2_magnitude = PKL_AST_OFFSET_MAGNITUDE (op2);  \
          pkl_ast_node op2_unit = PKL_AST_OFFSET_UNIT (op2);            \
          uint64_t result;                                              \
          uint64_t op1_magnitude_bits;                                  \
          uint64_t op2_magnitude_bits;                                  \
                                                                        \
          if (PKL_AST_CODE (op1) != PKL_AST_OFFSET                      \
              || PKL_AST_CODE (op2) != PKL_AST_OFFSET                   \
              || PKL_AST_CODE (op1_magnitude) != PKL_AST_INTEGER        \
              || PKL_AST_CODE (op1_unit) != PKL_AST_INTEGER             \
              || PKL_AST_CODE (op2_magnitude) != PKL_AST_INTEGER        \
              || PKL_AST_CODE (op2_unit) != PKL_AST_INTEGER)            \
            /* We cannot fold this expression.  */                      \
            PKL_PASS_DONE;                                              \
                                                                        \
          op1_magnitude_bits = (PKL_AST_INTEGER_VALUE (op1_magnitude)   \
                                * PKL_AST_INTEGER_VALUE (op1_unit));    \
          op2_magnitude_bits = (PKL_AST_INTEGER_VALUE (op2_magnitude)   \
                                * PKL_AST_INTEGER_VALUE (op2_unit));    \
                                                                        \
          if (PKL_AST_TYPE_I_SIGNED (op1_type))                         \
            result = emul_s_##OP (op1_magnitude_bits,                   \
                                  op2_magnitude_bits);                  \
          else                                                          \
            result = emul_u_##OP (op1_magnitude_bits,                   \
                                  op2_magnitude_bits);                  \
                                                                        \
          new = pkl_ast_make_integer (PKL_PASS_AST, result);            \
          PKL_AST_TYPE (new) = ASTREF (type);                           \
          PKL_AST_LOC (new) = PKL_AST_LOC (PKL_PASS_NODE);              \
                                                                        \
          pkl_ast_node_free (PKL_PASS_NODE);                            \
          PKL_PASS_NODE = new;                                          \
          PKL_PASS_DONE;                                                \
        }                                                               \
    }                                                                   \
  while (0)

#define OP_BINARY_III(OP)                                               \
  do                                                                    \
    {                                                                   \
      pkl_ast_node op1 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);        \
      pkl_ast_node op2 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 1);        \
      pkl_ast_node type = PKL_AST_TYPE (PKL_PASS_NODE);                 \
      pkl_ast_node op1_type = PKL_AST_TYPE (op1);                       \
      pkl_ast_node op2_type = PKL_AST_TYPE (op2);                       \
                                                                        \
      if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_INTEGRAL                 \
          && PKL_AST_TYPE_CODE (op1_type) == PKL_TYPE_INTEGRAL          \
          && PKL_AST_TYPE_CODE (op2_type) == PKL_TYPE_INTEGRAL)         \
        {                                                               \
          pkl_ast_node new;                                             \
          uint64_t result;                                              \
                                                                        \
          if (PKL_AST_CODE (op1) != PKL_AST_INTEGER                     \
              || PKL_AST_CODE (op2) != PKL_AST_INTEGER)                 \
            /* We cannot fold this expression.  */                      \
            PKL_PASS_DONE;                                              \
                                                                        \
          if (PKL_AST_TYPE_I_SIGNED (type))                             \
            result = emul_s_##OP (PKL_AST_INTEGER_VALUE (op1),          \
                                  PKL_AST_INTEGER_VALUE (op2));         \
          else                                                          \
            result = emul_u_##OP (PKL_AST_INTEGER_VALUE (op1),          \
                                  PKL_AST_INTEGER_VALUE (op2));         \
                                                                        \
          new = pkl_ast_make_integer (PKL_PASS_AST, result);            \
          PKL_AST_TYPE (new) = ASTREF (type);                           \
          PKL_AST_LOC (new) = PKL_AST_LOC (PKL_PASS_NODE);              \
                                                                        \
          pkl_ast_node_free (PKL_PASS_NODE);                            \
          PKL_PASS_NODE = new;                                          \
          PKL_PASS_DONE;                                                \
        }                                                               \
    }                                                                   \
  while (0)

#define OP_BINARY_SSI(OP)                                               \
  do                                                                    \
    {                                                                   \
      pkl_ast_node op1 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 0);        \
      pkl_ast_node op2 = PKL_AST_EXP_OPERAND (PKL_PASS_NODE, 1);        \
      pkl_ast_node type = PKL_AST_TYPE (PKL_PASS_NODE);                 \
      pkl_ast_node op1_type = PKL_AST_TYPE (op1);                       \
      pkl_ast_node op2_type = PKL_AST_TYPE (op2);                       \
                                                                        \
      if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_INTEGRAL                 \
          && PKL_AST_TYPE_CODE (op1_type) == PKL_TYPE_STRING            \
          && PKL_AST_TYPE_CODE (op2_type) == PKL_TYPE_STRING)           \
        {                                                               \
          pkl_ast_node new;                                             \
                                                                        \
          if (PKL_AST_CODE (op1) != PKL_AST_STRING                      \
              || PKL_AST_CODE (op2) != PKL_AST_STRING)                  \
            /* We cannot fold this expression.  */                      \
            PKL_PASS_DONE;                                              \
                                                                        \
          new = pkl_ast_make_integer (PKL_PASS_AST,                     \
                                      emul_s_##OP (PKL_AST_STRING_POINTER (op1), \
                                                   PKL_AST_STRING_POINTER (op2))); \
          PKL_AST_TYPE (new) = ASTREF (type);                           \
          PKL_AST_LOC (new) = PKL_AST_LOC (PKL_PASS_NODE);              \
                                                                        \
          pkl_ast_node_free (PKL_PASS_NODE);                            \
          PKL_PASS_NODE = new;                                          \
          PKL_PASS_DONE;                                                \
        }                                                               \
    }                                                                   \
  while (0)

/* Handlers for the several expression codes.  */

#define PKL_PHASE_HANDLER_UNA_INT(OP)           \
  PKL_PHASE_BEGIN_HANDLER (pkl_fold_##OP)       \
  {                                             \
    OP_UNARY_II (OP);                           \
  }                                             \
  PKL_PHASE_END_HANDLER

PKL_PHASE_HANDLER_UNA_INT (neg);
PKL_PHASE_HANDLER_UNA_INT (pos);
PKL_PHASE_HANDLER_UNA_INT (not);
PKL_PHASE_HANDLER_UNA_INT (bnot);

#define PKL_PHASE_HANDLER_BIN_INT(OP)                \
  PKL_PHASE_BEGIN_HANDLER (pkl_fold_##OP)            \
  {                                                  \
    OP_BINARY_III (OP);                              \
  }                                                  \
  PKL_PHASE_END_HANDLER

PKL_PHASE_HANDLER_BIN_INT (or);
PKL_PHASE_HANDLER_BIN_INT (ior);
PKL_PHASE_HANDLER_BIN_INT (xor);
PKL_PHASE_HANDLER_BIN_INT (and);
PKL_PHASE_HANDLER_BIN_INT (band);

#define PKL_PHASE_HANDLER_BIN_RELA(OP)               \
  PKL_PHASE_BEGIN_HANDLER (pkl_fold_##OP)            \
  {                                                  \
    OP_BINARY_III (OP);                              \
    OP_BINARY_OOI (OP##o);                           \
    OP_BINARY_SSI (OP##s);                           \
  }                                                  \
  PKL_PHASE_END_HANDLER

PKL_PHASE_HANDLER_BIN_RELA (eq);
PKL_PHASE_HANDLER_BIN_RELA (ne);
PKL_PHASE_HANDLER_BIN_RELA (lt);
PKL_PHASE_HANDLER_BIN_RELA (gt);
PKL_PHASE_HANDLER_BIN_RELA (le);
PKL_PHASE_HANDLER_BIN_RELA (ge);

PKL_PHASE_HANDLER_BIN_INT (add); /* XXX */
PKL_PHASE_HANDLER_BIN_INT (sub);
PKL_PHASE_HANDLER_BIN_INT (mul);
/* XXX the handler for div and mod should check for division by
   zero.  */
PKL_PHASE_HANDLER_BIN_INT (div);
PKL_PHASE_HANDLER_BIN_INT (mod);

#define PKL_PHASE_HANDLER_UNIMPL(op)            \
  PKL_PHASE_BEGIN_HANDLER (pkl_fold_##op)       \
  {                                             \
    /* WRITEME */                               \
  }                                             \
  PKL_PHASE_END_HANDLER

PKL_PHASE_HANDLER_UNIMPL (sconc);
PKL_PHASE_HANDLER_UNIMPL (bconc);
PKL_PHASE_HANDLER_UNIMPL (sl);
PKL_PHASE_HANDLER_UNIMPL (sr);

PKL_PHASE_BEGIN_HANDLER (pkl_fold_ps_cast)
{
  pkl_ast_node cast = PKL_PASS_NODE;
  pkl_ast_node exp = PKL_AST_CAST_EXP (cast);
  pkl_ast_node from_type = PKL_AST_TYPE (exp);
  pkl_ast_node to_type = PKL_AST_CAST_TYPE (cast);

  pkl_ast_node new = NULL;

  if (PKL_AST_TYPE_CODE (from_type) == PKL_TYPE_INTEGRAL
      && PKL_AST_TYPE_CODE (to_type) == PKL_TYPE_INTEGRAL
      && PKL_AST_CODE (exp) == PKL_AST_INTEGER)
    {
      new = pkl_ast_make_integer (PKL_PASS_AST,
                                  PKL_AST_INTEGER_VALUE (exp));
    }
  else if (PKL_AST_TYPE_CODE (from_type) == PKL_TYPE_OFFSET
           && PKL_AST_TYPE_CODE (to_type) == PKL_TYPE_OFFSET
           && PKL_AST_CODE (exp) == PKL_AST_OFFSET)
    {
      pkl_ast_node magnitude = PKL_AST_OFFSET_MAGNITUDE (exp);
      pkl_ast_node unit = PKL_AST_OFFSET_UNIT (exp);
      pkl_ast_node to_unit = PKL_AST_TYPE_O_UNIT (to_type);
      pkl_ast_node from_base_type = PKL_AST_TYPE_O_BASE_TYPE (from_type);
      pkl_ast_node to_base_type = PKL_AST_TYPE_O_BASE_TYPE (to_type);

      if (PKL_AST_CODE (magnitude) != PKL_AST_INTEGER
          || PKL_AST_CODE (unit) != PKL_AST_INTEGER
          || PKL_AST_CODE (to_unit) != PKL_AST_INTEGER)
        /* We can't fold this cast.  */
        PKL_PASS_DONE;

      /* Transform magnitude to bits.  */
      PKL_AST_INTEGER_VALUE (magnitude)
        = (PKL_AST_INTEGER_VALUE (magnitude) *
           PKL_AST_INTEGER_VALUE (unit));

      /* Calculate the new unit.  */
      PKL_AST_INTEGER_VALUE (unit)
        = PKL_AST_INTEGER_VALUE (to_unit);

      /* We may need to create a new magnitude node, if the base type
         is different.  */
      if (!pkl_ast_type_equal (from_base_type, to_base_type))
        {
          magnitude = pkl_ast_make_integer  (PKL_PASS_AST,
                                             PKL_AST_INTEGER_VALUE (magnitude));
          PKL_AST_TYPE (magnitude) = ASTREF (to_base_type);
          PKL_AST_LOC (magnitude) = PKL_AST_LOC (cast);
        }

      /* Transform magnitude to new unit.  */
      PKL_AST_INTEGER_VALUE (magnitude)
        = (PKL_AST_INTEGER_VALUE (magnitude)
           /  PKL_AST_INTEGER_VALUE (unit));

      new = pkl_ast_make_offset (PKL_PASS_AST,
                                 magnitude, unit);
    }
  else
    PKL_PASS_DONE;

  /* XXX handle array casts.  */

  /* `new' is the node to replace the cast.  */
  PKL_AST_TYPE (new) = ASTREF (to_type);
  PKL_AST_LOC (new) = PKL_AST_LOC (exp);
  pkl_ast_node_free (cast);
  PKL_PASS_NODE = new;
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
   ENTRY (GE, ge), ENTRY (SCONC, sconc),
   ENTRY (BCONC, bconc),
   ENTRY (POS, pos), ENTRY (NEG, neg), ENTRY (BNOT, bnot),
   ENTRY (NOT, not),
#undef ENTRY
  };
