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

#include <config.h>

#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "pkl-ast.h"

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

#define OP_BINARY_INT(emul)                                             \
  do                                                                    \
    {                                                                   \
      if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)                        \
        {                                                               \
          new = pkl_ast_make_integer (emul (PKL_AST_INTEGER_VALUE (op1), \
                                            PKL_AST_INTEGER_VALUE (op2))); \
          PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));             \
                                                                        \
          pkl_ast_node_free (ast);                                      \
          ast = new;                                                    \
        }                                                               \
    }                                                                   \
  while (0)

#define OP_BINARY_STR(emul)                                             \
  do                                                                    \
    {                                                                   \
      if (PKL_AST_CODE (op1) == PKL_AST_STRING)                         \
        {                                                               \
          new = pkl_ast_make_integer (emul (PKL_AST_STRING_POINTER (op1), \
                                            PKL_AST_STRING_POINTER (op2))); \
          PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));             \
                                                                        \
          pkl_ast_node_free (ast);                                      \
          ast = new;                                                    \
        }                                                               \
    }                                                                   \
  while (0)


/* Note that in the following macro an l-value should be passed for
   CHAIN.  */
#define PKL_FOLD_CHAIN(CHAIN)                           \
  do                                                    \
    {                                                   \
      pkl_ast_node elem, last, next;                    \
                                                        \
      /* Process first element in the chain.  */        \
      elem = (CHAIN);                                   \
      next = PKL_AST_CHAIN (elem);                      \
      CHAIN = pkl_fold_1 (elem);                        \
      last = (CHAIN);                                   \
      elem = next;                                      \
                                                        \
      /* Process rest of the chain.  */                 \
      while (elem)                                      \
        {                                               \
          next = PKL_AST_CHAIN (elem);                  \
          PKL_AST_CHAIN (last) = pkl_fold_1 (elem);     \
          last = PKL_AST_CHAIN (last);                  \
          elem = next;                                  \
        }                                               \
    } while (0)

static pkl_ast_node
pkl_fold_1 (pkl_ast_node ast)
{
  pkl_ast_node ast_orig = ast;
  
  switch (PKL_AST_CODE (ast))
    {
    case PKL_AST_EXP:
      {
        pkl_ast_node new, op1 = NULL, op2 = NULL;

        if (PKL_AST_EXP_NUMOPS (ast) > 1)
          PKL_AST_EXP_OPERAND (ast, 0)
              = pkl_fold_1 (PKL_AST_EXP_OPERAND (ast, 0));
        if (PKL_AST_EXP_NUMOPS (ast) == 2)
          PKL_AST_EXP_OPERAND (ast, 1)
            = pkl_fold_1 (PKL_AST_EXP_OPERAND (ast, 1));

        op1 = PKL_AST_EXP_OPERAND (ast, 0);
        op2 = PKL_AST_EXP_OPERAND (ast, 1);

        switch (PKL_AST_EXP_CODE (ast))
          {
            /* Binary operators.  */
          case PKL_AST_OP_OR:
            OP_BINARY_INT (emul_or);
            break;
          case PKL_AST_OP_IOR:
            OP_BINARY_INT (emul_ior);
            break;
          case PKL_AST_OP_XOR:
            OP_BINARY_INT (emul_xor);
            break;
          case PKL_AST_OP_AND:
            OP_BINARY_INT (emul_and);
            break;
          case PKL_AST_OP_BAND:
            OP_BINARY_INT (emul_band);
            break;
          case PKL_AST_OP_EQ:
            OP_BINARY_INT (emul_eq);
            OP_BINARY_STR (emul_eqs);
            break;
          case PKL_AST_OP_NE:
            OP_BINARY_INT (emul_ne);
            break;
          case PKL_AST_OP_SL:
            OP_BINARY_INT (emul_sl);
            break;
          case PKL_AST_OP_SR:
            OP_BINARY_INT (emul_sr);
            break;
          case PKL_AST_OP_ADD:
            OP_BINARY_INT (emul_add);
            break;
          case PKL_AST_OP_SUB:
            OP_BINARY_INT (emul_sub);
            break;
          case PKL_AST_OP_MUL:
            OP_BINARY_INT (emul_mul);
            break;
          case PKL_AST_OP_DIV:
            OP_BINARY_INT (emul_div);
            break;
          case PKL_AST_OP_MOD:
            OP_BINARY_INT (emul_mod);
            break;
          case PKL_AST_OP_LT:
            OP_BINARY_INT (emul_lt);
            break;
          case PKL_AST_OP_GT:
            OP_BINARY_INT (emul_gt);
            break;
          case PKL_AST_OP_LE:
            OP_BINARY_INT (emul_le);
            break;
          case PKL_AST_OP_GE:
            OP_BINARY_INT (emul_ge);
            break;
          case PKL_AST_OP_SCONC:
            /* XXX writeme */
            break;
          case PKL_AST_OP_MAP:
            /* XXX: writeme */
          case PKL_AST_OP_CAST:
            {
              pkl_ast_node to_type;
              pkl_ast_node from_type;
              PKL_AST_EXP_OPERAND (ast, 0)
                = pkl_fold_1 (PKL_AST_EXP_OPERAND (ast, 0));
              op1 = PKL_AST_EXP_OPERAND (ast, 0);

#define CAST_TO(T)                                                      \
              do                                                        \
                {                                                       \
                  new = pkl_ast_make_integer ((T) PKL_AST_INTEGER_VALUE (op1)); \
                  PKL_AST_TYPE (new) = ASTREF (to_type);                \
                                                                        \
                  pkl_ast_node_free (ast);                              \
                  ast = new;                                            \
                } while (0)

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
                        break;
                      
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

              break;
            }
            /* Unary operators.  */
          case PKL_AST_OP_PREINC:
          case PKL_AST_OP_PREDEC:
          case PKL_AST_OP_POSTINC:
          case PKL_AST_OP_POSTDEC:
          case PKL_AST_OP_SIZEOF:
          case PKL_AST_OP_ELEMSOF:
          case PKL_AST_OP_TYPEOF:
            /* There is no need to handle this here, since the
               corresponding rules in pkl-tab.y do the folding.  */
            break;
          case PKL_AST_OP_POS:
          case PKL_AST_OP_NEG:
          case PKL_AST_OP_BNOT:
          case PKL_AST_OP_NOT:
            break;
            
          default:
            assert (0);
          }

        break;
      }
    case PKL_AST_PROGRAM:
      PKL_FOLD_CHAIN (PKL_AST_PROGRAM_ELEMS (ast));
      break;
    case PKL_AST_COND_EXP:
      PKL_AST_COND_EXP_COND (ast)
        = pkl_fold_1 (PKL_AST_COND_EXP_COND (ast));
      PKL_AST_COND_EXP_THENEXP (ast)
        = pkl_fold_1 (PKL_AST_COND_EXP_THENEXP (ast));
      PKL_AST_COND_EXP_ELSEEXP (ast)
        = pkl_fold_1 (PKL_AST_COND_EXP_ELSEEXP (ast));
      break;
    case PKL_AST_ARRAY:
      PKL_FOLD_CHAIN (PKL_AST_ARRAY_ELEMS (ast));
      break;
    case PKL_AST_ARRAY_ELEM:
      PKL_AST_ARRAY_ELEM_EXP (ast)
        = pkl_fold_1 (PKL_AST_ARRAY_ELEM_EXP (ast));
      break;
    case PKL_AST_ARRAY_REF:
      PKL_AST_ARRAY_REF_ARRAY (ast)
        = pkl_fold_1 (PKL_AST_ARRAY_REF_ARRAY (ast));
      PKL_AST_ARRAY_REF_INDEX (ast)
        = pkl_fold_1 (PKL_AST_ARRAY_REF_INDEX (ast));
      break;
    case PKL_AST_STRUCT:
      PKL_FOLD_CHAIN (PKL_AST_STRUCT_ELEMS (ast));
      break;
    case PKL_AST_STRUCT_ELEM:
      PKL_AST_STRUCT_ELEM_NAME (ast)
        = pkl_fold_1 (PKL_AST_STRUCT_ELEM_NAME (ast));
      PKL_AST_STRUCT_ELEM_EXP (ast)
        = pkl_fold_1 (PKL_AST_STRUCT_ELEM_EXP (ast));
      break;
    case PKL_AST_STRUCT_REF:
      PKL_AST_STRUCT_REF_STRUCT (ast)
        = pkl_fold_1 (PKL_AST_STRUCT_REF_STRUCT (ast));
      PKL_AST_STRUCT_REF_IDENTIFIER (ast)
        = pkl_fold_1 (PKL_AST_STRUCT_REF_IDENTIFIER (ast));
      break;
    case PKL_AST_OFFSET:
      PKL_AST_OFFSET_MAGNITUDE (ast)
        = pkl_fold_1 (PKL_AST_OFFSET_MAGNITUDE (ast));
      break;
    case PKL_AST_TYPE:
      {
        switch (PKL_AST_TYPE_CODE (ast))
          {
          case PKL_TYPE_ARRAY:
            PKL_AST_TYPE_A_NELEM (ast)
              = pkl_fold_1 (PKL_AST_TYPE_A_NELEM (ast));
            PKL_AST_TYPE_A_ETYPE (ast)
              = pkl_fold_1 (PKL_AST_TYPE_A_ETYPE (ast));
            break;
          case PKL_TYPE_STRUCT:
            PKL_AST_TYPE_S_ELEMS (ast)
              = pkl_fold_1 (PKL_AST_TYPE_S_ELEMS (ast));
            break;
          case PKL_TYPE_OFFSET:
            PKL_AST_TYPE_O_BASE_TYPE (ast)
              = pkl_fold_1 (PKL_AST_TYPE_O_BASE_TYPE (ast));
            break;
          case PKL_TYPE_INTEGRAL:
          case PKL_TYPE_STRING:
            /* Nothing to fold.  */
            break;
          case PKL_TYPE_NOTYPE:
          default:
            assert (0);
          }
        break;
      }
    case PKL_AST_STRUCT_TYPE_ELEM:
      PKL_AST_STRUCT_TYPE_ELEM_NAME (ast)
        = pkl_fold_1 (PKL_AST_STRUCT_TYPE_ELEM_NAME (ast));
      PKL_AST_STRUCT_TYPE_ELEM_TYPE (ast)
        = pkl_fold_1 (PKL_AST_STRUCT_TYPE_ELEM_TYPE (ast));
      break;
    case PKL_AST_INTEGER:
    case PKL_AST_STRING:
    case PKL_AST_IDENTIFIER:
      /* Nothing to fold.  */
      break;
    case PKL_AST_DECL:
    case PKL_AST_ENUM:
    case PKL_AST_ENUMERATOR:
      /* Not yet needed/implemented.  */
      assert (0);
      break;
    default:
      break;
    }

  /* If a new node was created to replace the incoming node, increase
     its reference counter.  This assumes that the node returned by
     this function will be stored in some other node (or the top-level
     AST structure).  */
  if (ast != ast_orig)
    ASTREF (ast);
  
  return ast;
}

pkl_ast
pkl_fold (pkl_ast ast)
{
  ast->ast = pkl_fold_1 (ast->ast);
  return ast;
}
