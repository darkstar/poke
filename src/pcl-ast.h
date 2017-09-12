/* pcl-ast.h - Abstract Syntax Tree for PCL.  */

/* Copyright (C) 2017 Jose E. Marchesi */

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

#ifndef PCL_AST_H
#define PCL_AST_H

#include <config.h>

#include <stdint.h>
#include "pcl-ast.h"

/*
 * The PCL abstract syntax tree is heavily influenced by the `tree'
 * abstraction used in the GCC front-end, and it is implemented in a
 * very similar way.
 */

enum pcl_ast_code
{
  PCL_AST_ENUMERATOR,
  PCL_AST_EXP,
  PCL_AST_COND_EXP,
  PCL_AST_INTEGER,
  PCL_AST_STRING,
  PCL_AST_STRUCT,
  PCL_AST_FIELD,
  PCL_AST_COMPOUND,
  PCL_AST_STMT,
  PCL_AST_IF_STMT,
  PCL_AST_LOOP,
  PCL_AST_IDENTIFIER,
  PCL_AST_ARRAY_REF,
  PCL_AST_STRUCT_REF
};

enum pcl_ast_op
{
  /* Binary operators.  */
  PCL_AST_OP_OR,
  PCL_AST_OP_IOR,
  PCL_AST_OP_XOR,
  PCL_AST_OP_AND,
  PCL_AST_OP_BAND,
  PCL_AST_OP_EQ,
  PCL_AST_OP_NE,
  PCL_AST_OP_SL,
  PCL_AST_OP_SR,
  PCL_AST_OP_ADD,
  PCL_AST_OP_SUB,
  PCL_AST_OP_MUL,
  PCL_AST_OP_DIV,
  PCL_AST_OP_MOD,
  /* Unary operators.  */
  PCL_AST_OP_INC,
  PCL_AST_OP_DEC,
  PCL_AST_OP_SIZEOF,
  PCL_AST_OP_ADDRESS,
  PCL_AST_OP_POS,
  PCL_AST_OP_NEG,
  PCL_AST_OP_BNOT,
  PCL_AST_OP_NOT
};

/* The following structs define the several supported types of nodes
   in the abstract syntax tree, which are discriminated using the
   codes defined in the `pcl_ast_code' enumeration above.

   Accessor macros are defined, and should be used as both l-values
   and r-values.

   Note that the `pcl_ast_common' struct defines fields which are
   common to every node in the AST, regardless of their type.  */

#define PCL_AST_CHAIN(AST) ((AST)->common.chain)
#define PCL_AST_CODE(AST) ((AST)->common.code)
#define PCL_AST_LITERAL_P(AST) ((AST)->common.literal)

struct pcl_ast_common
{
  union pcl_ast *chain;
  enum pcl_ast_code code : 8;

  unsigned literal_p : 1;
};

#define PCL_AST_IDENTIFIER_LENGTH(AST) ((AST)->identifier.length)
#define PCL_AST_IDENTIFIER_POINTER(AST) ((AST)->identifier.pointer)

struct pcl_ast_identifier
{
  struct pcl_ast_common common;
  size_t length;
  char *pointer;
};

#define PCL_AST_INTEGER_VALUE(AST) ((AST)->integer.value)

struct pcl_ast_integer
{
  struct pcl_ast_common common;
  uint64_t value;
};

#define PCL_AST_STRING_LENGTH(AST) ((AST)->string.length)
#define PCL_AST_STRING_POINTER(AST) ((AST)->string.pointer)

struct pcl_ast_string
{
  struct pcl_ast_common common;
  size_t length;
  char *pointer;
};

#define PCL_AST_EXP_CODE(AST) ((AST)->exp.code)
#define PCL_AST_EXP_NUMOPS(AST) ((AST)->exp.numops)
#define PCL_AST_EXP_OPERAND(AST, I) ((AST)->exp.operands[(I)])

struct pcl_ast_exp
{
  struct pcl_ast_common common;
  enum pcl_ast_op code;
  uint8_t numops : 8;
  union pcl_ast *operands[2];
};

#define PCL_AST_COND_EXP_COND(AST) ((AST)->cond_exp.cond)
#define PCL_AST_COND_EXP_THENEXP(AST) ((AST)->cond_exp.thenexp)
#define PCL_AST_COND_EXP_ELSEEXP(AST) ((AST)->cond_exp.elseexp)

struct pcl_ast_cond_exp
{
  struct pcl_ast_common common;
  union pcl_ast *cond;
  union pcl_ast *thenexp;
  union pcl_ast *elseexp;
}

#define PCL_AST_ENUMERATOR_IDENTIFIER(AST) ((AST)->enumerator.identifier)
#define PCL_AST_ENUMERATOR_VALUE(AST) ((AST)->enumerator.value)
#define PCL_AST_ENUMERATOR_DOCSTR(AST) ((AST)->enumerator.docstr)

struct pcl_ast_enumerator
{
  struct pcl_ast_common common;
  union pcl_ast *identifier;
  union pcl_ast *value;
  union pcl_ast *docstr;
};

#define PCL_AST_STRUCT_TAG(AST) ((AST)->strct.tag)
#define PCL_AST_STRUCT_FIELDS(AST) ((AST)->strct.fields)
#define PCL_AST_STRUCT_DOCSTR(AST) ((AST)->strct.docstr)

struct pcl_ast_struct
{
  struct pcl_ast_common common;
  union pcl_ast *tag;
  union pcl_ast *fields;
  union pcl_ast *docstr;
};

#define PCL_AST_FIELD_NAME(AST) ((AST)->field.name)
#define PCL_AST_FIELD_TYPE(AST) ((AST)->field.type)
#define PCL_AST_fIELD_DOCSTR(AST) ((AST)->field.docstr)

struct pcl_ast_field
{
  struct pcl_ast_common common;
  union pcl_ast *name;
  union pcl_ast *type;
  union pcl_ast *docstr;
};

#define PCL_AST_STMT_FILENAME(AST) ((AST)->stmt.filename)
#define PCL_AST_STMT_LINENUM(AST) ((AST)->stmt.linenum)
#define PCL_AST_STMT_BODY(AST) ((AST)->stmt.body)

struct pcl_ast_stmt
{
  struct pcl_ast_common common;
  char *filename;
  size_t linenum;
  union pcl_ast *body;
};

#define PCL_AST_IF_STMT_COND(AST) ((AST).if_stmt.cond)
#define PCL_AST_IF_STMT_THENPART(AST) ((AST).if_stmt.thenpart)
#define PCL_AST_IF_STMT_ELSEPART(AST) ((AST).if_stmt.elsepart)

struct pcl_ast_if_stmt
{
  struct pcl_ast_common common;
  union pcl_ast *cond;
  union pcl_ast *thenpart;
  union pcl_ast *elsepart;
};

#define PCL_AST_LOOP_PRE(AST) ((AST).loop.pre)
#define PCL_AST_LOOP_COND(AST) ((AST).loop.cond)
#define PCL_AST_LOOP_POST(AST) ((AST).loop.post)
#define PCL_AST_LOOP_BODY(AST) ((AST).loop.body)

struct pcl_ast_loop
{
  struct pcl_ast_common common;
  union pcl_ast *pre;
  union pcl_ast *cond;
  union pcl_ast *post;
  union pcl_ast *body;
};

#define PCL_AST_ARRAY_REF_BASE(AST) ((AST).aref.base)
#define PCL_AST_ARRAY_REF_INDEX(AST) ((AST).aref.index)

struct pcl_ast_array_ref
{
  struct pcl_ast_common common;
  union pcl_ast *base;
  union pcl_ast *index;
};

#define PCL_AST_STRUCT_REF_BASE(AST) ((AST).sref.base)
#define PCL_AST_STRUCT_REF_IDENTIFIER(AST) ((AST).sref.identifier)

struct pcl_ast_struct_ref
{
  struct pcl_ast_common common;
  union pcl_ast *base;
  union pcl_ast *identifier;
};

/* Finally, the `pcl_ast' type, which represents both an AST tree and
   a node.  */

union pcl_ast_s
{
  struct pcl_ast_struct strct;
  struct pcl_ast_field field;
  struct pcl_ast_stmt stmt;
  struct pcl_ast_if_stmt if_stmt;
  struct pcl_ast_loop loop;
  struct pcl_ast_identifier identifier;
  struct pcl_ast_integer integer;
  struct pcl_ast_string string;
  struct pcl_ast_exp exp;
  struct pcl_ast_cond_exp cond_exp;
  struct pcl_ast_array_ref aref;
  struct pcl_ast_struct_ref sref;
};

typedef union pcl_ast_s *pcl_ast;

/*  Prototypes for the non-static functions implemented in pcl-ast.c.
    See the .c file for information on how to use them functions.  */

pcl_ast pcl_ast_make_node (enum pcl_ast_code code);
pcl_ast pcl_ast_chainon (pcl_ast ast1, pcl_ast ast2);

pcl_ast pcl_ast_get_identifier (const char *str);
pcl_ast pcl_ast_make_integer (uint64_t value);
pcl_ast pcl_ast_make_string (const char *str);
pcl_ast pcl_ast_make_enumerator (pcl_ast identifier, pcl_ast value,
                                 pcl_ast docstr);
pcl_ast pcl_ast_make_cond_exp (pcl_ast cond, pcl_ast thenexp,
                               pcl_ast elseexp);
pcl_ast pcl_ast_make_binary_exp (enum pcl_ast_op code, pcl_ast op1,
                                 pcl_ast op2);
pcl_ast pcl_ast_make_unary_exp (enum pcl_ast_op code, pcl_ast op);
pcl_ast pcl_ast_make_array_ref (pcl_ast base, pcl_ast index);
pcl_ast pcl_ast_make_struct_ref (pcl_ast base, pcl_ast identifier);

#endif /* ! PCL_AST_H */
