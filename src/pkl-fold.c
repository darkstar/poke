/* pkl-fold.c - Constant folding for the poke compiler. */

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
#include "pkl-parser.h"

static pkl_ast_node
pkl_fold_1 (struct pkl_parser *parser, pkl_ast_node ast)
{
  pkl_ast_node new, op1, op2;

  switch (PKL_AST_CODE (ast))
    {
    case PKL_AST_EXP:
      {
        switch (PKL_AST_EXP_CODE (ast))
          {
            /* Binary operators.  */
          case PKL_AST_OP_OR:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              || PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_IOR:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              | PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }            
          case PKL_AST_OP_XOR:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              ^ PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_AND:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              && PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_BAND:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              & PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_EQ:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              == PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }
              else if (PKL_AST_CODE (op1) == PKL_AST_STRING)
                {
                  new = pkl_ast_make_integer (strcmp (PKL_AST_STRING_POINTER (op1),
                                                      PKL_AST_STRING_POINTER (op2)) == 0);
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_NE:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              != PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_SL:
            break;
          case PKL_AST_OP_SR:
            break;
          case PKL_AST_OP_ADD:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              + PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_SUB:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              - PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_MUL:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              * PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_DIV:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              / PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_MOD:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              % PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_LT:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              < PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_GT:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              > PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_LE:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              <= PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_GE:
            {
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));
              op2 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 1));

              if (PKL_AST_CODE (op1) == PKL_AST_INTEGER)
                {
                  new = pkl_ast_make_integer (PKL_AST_INTEGER_VALUE (op1)
                                              >= PKL_AST_INTEGER_VALUE (op2));
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (op1));

                  pkl_ast_node_free (ast);
                  ast = new;
                }

              break;
            }
          case PKL_AST_OP_SCONC:
            break;
          case PKL_AST_OP_MAP:
          case PKL_AST_OP_CAST:
            {
              pkl_ast_node to_type;
              pkl_ast_node from_type;
              op1 = pkl_fold_1 (parser, PKL_AST_EXP_OPERAND (ast, 0));

#define CAST_TO(T)                                                      \
              do                                                        \
                {                                                       \
                  new = pkl_ast_make_integer ((T) PKL_AST_INTEGER_VALUE (op1)); \
                  PKL_AST_TYPE (new) = ASTREF (PKL_AST_TYPE (to_type)); \
                                                                        \
                  ASTREF (ast);                                         \
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
    case PKL_AST_COND_EXP:
      /* XXX: writeme  */
      assert (0);
    default:
      break;
    }

  return ast;
}

pkl_ast_node
pkl_fold (struct pkl_parser *parser, pkl_ast_node ast)
{
  ASTREF (ast);
  return pkl_fold_1 (parser, ast);
}
