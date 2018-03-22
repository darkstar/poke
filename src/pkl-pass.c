/* pkl-pass.c - Generic support for compiler passes.  */

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

#include <stdarg.h>
#include "pkl-pass.h"

/* Note that in the following macro an l-value should be passed for
   CHAIN.  */
#define PKL_PASS_CHAIN(CHAIN)                           \
  do                                                    \
    {                                                   \
      pkl_ast_node elem, last, next;                    \
                                                        \
      /* Process first element in the chain.  */        \
      elem = (CHAIN);                                   \
      next = PKL_AST_CHAIN (elem);                      \
      CHAIN = pkl_do_pass_1 (elem, data, transforms);   \
      last = (CHAIN);                                   \
      elem = next;                                      \
                                                        \
      /* Process rest of the chain.  */                 \
      while (elem)                                      \
        {                                               \
          next = PKL_AST_CHAIN (elem);                  \
          PKL_AST_CHAIN (last) = pkl_do_pass_1 (elem,   \
                                                data,   \
                                                transforms);    \
          last = PKL_AST_CHAIN (last);                  \
          elem = next;                                  \
        }                                               \
    } while (0)

static pkl_ast_node
pkl_do_pass_1 (pkl_ast_node ast, void *data, ...)
{
  va_list transforms;
  pkl_ast_node ast_orig = ast;

  va_start (transforms, data);
  
  switch (PKL_AST_CODE (ast))
    {
    case PKL_AST_EXP:
      if (PKL_AST_EXP_NUMOPS (ast) > 1)
        PKL_AST_EXP_OPERAND (ast, 0)
          = pkl_do_pass_1 (PKL_AST_EXP_OPERAND (ast, 0), data,
                           transforms);
      if (PKL_AST_EXP_NUMOPS (ast) == 2)
        PKL_AST_EXP_OPERAND (ast, 1)
          = pkl_do_pass_1 (PKL_AST_EXP_OPERAND (ast, 1), data,
                           transforms);

      switch (PKL_AST_EXP_CODE (ast))
          {
            /* Binary operators.  */
          case PKL_AST_OP_OR:
            /* CALL TRANSFORMS.  */
            break;
          case PKL_AST_OP_IOR:
            /* CALL TRANSFORMS.  */
            break;
          case PKL_AST_OP_XOR:
            /* CALL TRANSFORMS.  */
            break;
          case PKL_AST_OP_AND:
            /* CALL TRANSFORMS.  */
            break;
          case PKL_AST_OP_BAND:
            /* CALL TRASNSFORMS.  */
            break;
          case PKL_AST_OP_EQ:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_NE:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_SL:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_SR:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_ADD:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_SUB:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_MUL:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_DIV:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_MOD:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_LT:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_GT:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_LE:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_GE:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_SCONC:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_MAP:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_CAST:
            {
              PKL_AST_EXP_OPERAND (ast, 0)
                = pkl_do_pass_1 (PKL_AST_EXP_OPERAND (ast, 0), data,
                                 transforms);
              /* CALL TRANSFORMS */
            }
            /* Unary operators.  */
          case PKL_AST_OP_PREINC:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_PREDEC:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_POSTINC:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_POSTDEC:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_SIZEOF:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_ELEMSOF:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_TYPEOF:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_POS:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_NEG:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_BNOT:
            /* CALL TRANSFORMS */
            break;
          case PKL_AST_OP_NOT:
            /* CALL TRANSFORMS */
            break;
          default:
            assert (0);
          }
      break;
    case PKL_AST_PROGRAM:
      PKL_PASS_CHAIN (PKL_AST_PROGRAM_ELEMS (ast));
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_COND_EXP:
      PKL_AST_COND_EXP_COND (ast)
        = pkl_do_pass_1 (PKL_AST_COND_EXP_COND (ast), data,
                         transforms);
      PKL_AST_COND_EXP_THENEXP (ast)
        = pkl_do_pass_1 (PKL_AST_COND_EXP_THENEXP (ast), data,
                         transforms);
      PKL_AST_COND_EXP_ELSEEXP (ast)
        = pkl_do_pass_1 (PKL_AST_COND_EXP_ELSEEXP (ast), data,
                         transforms);
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_ARRAY:
      PKL_PASS_CHAIN (PKL_AST_ARRAY_ELEMS (ast));
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_ARRAY_ELEM:
      PKL_AST_ARRAY_ELEM_EXP (ast)
        = pkl_do_pass_1 (PKL_AST_ARRAY_ELEM_EXP (ast), data,
                         transforms);
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_ARRAY_REF:
      PKL_AST_ARRAY_REF_ARRAY (ast)
        = pkl_do_pass_1 (PKL_AST_ARRAY_REF_ARRAY (ast), data,
                         transforms);
      PKL_AST_ARRAY_REF_INDEX (ast)
        = pkl_do_pass_1 (PKL_AST_ARRAY_REF_INDEX (ast), data,
                         transforms);
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_STRUCT:
      PKL_PASS_CHAIN (PKL_AST_STRUCT_ELEMS (ast));
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_STRUCT_ELEM:
      PKL_AST_STRUCT_ELEM_NAME (ast)
        = pkl_do_pass_1 (PKL_AST_STRUCT_ELEM_NAME (ast), data,
                         transforms);
      PKL_AST_STRUCT_ELEM_EXP (ast)
        = pkl_do_pass_1 (PKL_AST_STRUCT_ELEM_EXP (ast), data,
                         transforms);
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_STRUCT_REF:
      PKL_AST_STRUCT_REF_STRUCT (ast)
        = pkl_do_pass_1 (PKL_AST_STRUCT_REF_STRUCT (ast), data,
                         transforms);
      PKL_AST_STRUCT_REF_IDENTIFIER (ast)
        = pkl_do_pass_1 (PKL_AST_STRUCT_REF_IDENTIFIER (ast), data,
                         transforms);
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_OFFSET:
      PKL_AST_OFFSET_MAGNITUDE (ast)
        = pkl_do_pass_1 (PKL_AST_OFFSET_MAGNITUDE (ast), data,
                         transforms);
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_TYPE:
      {
        switch (PKL_AST_TYPE_CODE (ast))
          {
          case PKL_TYPE_ARRAY:
            PKL_AST_TYPE_A_NELEM (ast)
              = pkl_do_pass_1 (PKL_AST_TYPE_A_NELEM (ast), data,
                               transforms);
            PKL_AST_TYPE_A_ETYPE (ast)
              = pkl_do_pass_1 (PKL_AST_TYPE_A_ETYPE (ast), data,
                               transforms);
            /* CALL TRANSFORMS */
            break;
          case PKL_TYPE_STRUCT:
            PKL_AST_TYPE_S_ELEMS (ast)
              = pkl_do_pass_1 (PKL_AST_TYPE_S_ELEMS (ast), data,
                               transforms);
            /* CALL TRANSFORMS */
            break;
          case PKL_TYPE_OFFSET:
            PKL_AST_TYPE_O_BASE_TYPE (ast)
              = pkl_do_pass_1 (PKL_AST_TYPE_O_BASE_TYPE (ast), data,
                               transforms);
            /* CALL TRANSFORMS */
            break;
          case PKL_TYPE_INTEGRAL:
            /* CALL TRANSFORMS */
            break;
          case PKL_TYPE_STRING:
            /* CALL TRANSFORMS */
            break;
          case PKL_TYPE_NOTYPE:
          default:
            assert (0);
          }
        break;
      }
    case PKL_AST_STRUCT_TYPE_ELEM:
      PKL_AST_STRUCT_TYPE_ELEM_NAME (ast)
        = pkl_do_pass_1 (PKL_AST_STRUCT_TYPE_ELEM_NAME (ast), data,
                         transforms);
      PKL_AST_STRUCT_TYPE_ELEM_TYPE (ast)
        = pkl_do_pass_1 (PKL_AST_STRUCT_TYPE_ELEM_TYPE (ast), data,
                         transforms);
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_INTEGER:
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_STRING:
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_IDENTIFIER:
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_DECL:
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_ENUM:
      /* CALL TRANSFORMS */
      break;
    case PKL_AST_ENUMERATOR:
      /* CALL TRANSFORMS */
      break;
    default:
      break;
    }

  va_end (transforms);

  /* If a new node was created to replace the incoming node, increase
     its reference counter.  This assumes that the node returned by
     this function will be stored in some other node (or the top-level
     AST structure).  */
  if (ast != ast_orig)
    ASTREF (ast);
  
  return ast;
}

pkl_ast
pkl_do_pass (pkl_ast ast, void *data, ...)
{
  va_list transforms;

  va_start (transforms, data);
  ast->ast = pkl_do_pass_1 (ast->ast, data, transforms);
  va_end (transforms);

  return ast;
}
