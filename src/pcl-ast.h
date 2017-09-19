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

#ifdef PCL_DEBUG
# include <stdio.h>
#endif

#include <stdint.h>
#include "pcl-ast.h"

/*
 * The PCL abstract syntax tree is heavily influenced by the `tree'
 * abstraction used in the GCC front-end, and it is implemented in a
 * very similar way.
 */

enum pcl_ast_code
{
  PCL_AST_PROGRAM,
  /* Expressions.  */
  PCL_AST_EXP,
  PCL_AST_COND_EXP,
  /* Enumerations.  */
  PCL_AST_ENUM,
  PCL_AST_ENUMERATOR,
  /* Structs and their components.  */
  PCL_AST_STRUCT,
  /* Memory layouts.  */
  PCL_AST_MEM,
  PCL_AST_FIELD,
  PCL_AST_COND,
  PCL_AST_LOOP,
  PCL_AST_ASSERTION,
  /* Types.  */
  PCL_AST_TYPE,
  /* References.  */
  PCL_AST_ARRAY_REF,
  PCL_AST_STRUCT_REF,
  /* PCL_AST_SYMBOL_REF, for symbols */
  /* Leafs.  */
  PCL_AST_INTEGER,
  PCL_AST_STRING,
  PCL_AST_IDENTIFIER,
  PCL_AST_DOC_STRING,
  PCL_AST_LOC
};

#define PCL_DEF_OP(SYM, STRING) SYM,
enum pcl_ast_op
{
#include "pcl-ops.def"
};
#undef PCL_DEF_OP

enum pcl_ast_endian
{
  PCL_AST_MSB, /* Big-endian.  */
  PCL_AST_LSB  /* Little-endian.  */
};

#define PCL_DEF_TYPE(CODE,ID,SIZE) CODE,

enum pcl_ast_type_code
{
  PCL_TYPE_NOTYPE,
#include "pcl-types.def"
  PCL_TYPE_ENUM,
  PCL_TYPE_STRUCT
};

#undef PCL_DEF_TYPE

/* The following structs define the several supported types of nodes
   in the abstract syntax tree, which are discriminated using the
   codes defined in the `pcl_ast_code' enumeration above.

   Accessor macros are defined, and should be used as both l-values
   and r-values.

   Note that the `pcl_ast_common' struct defines fields which are
   common to every node in the AST, regardless of their type.  */

#define PCL_AST_CHAIN(AST) ((AST)->common.chain)
#define PCL_AST_CODE(AST) ((AST)->common.code)
#define PCL_AST_LITERAL_P(AST) ((AST)->common.literal_p)
#define PCL_AST_REGISTERED_P(AST) ((AST)->common.registered_p)

struct pcl_ast_common
{
  union pcl_ast_s *chain;
  enum pcl_ast_code code : 8;

  unsigned literal_p : 1;
  unsigned registered_p :1;
};

#define PCL_AST_PROGRAM_DECLARATIONS(AST) ((AST)->program.declarations)

struct pcl_ast_program
{
  struct pcl_ast_common common;
  union pcl_ast_s *declarations;
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

#define PCL_AST_DOC_STRING_LENGTH(AST) ((AST)->doc_string.length)
#define PCL_AST_DOC_STRING_POINTER(AST) ((AST)->doc_string.pointer)
#define PCL_AST_DOC_STRING_ENTITY(AST) ((AST)->doc_string.entity)

struct pcl_ast_doc_string
{
  struct pcl_ast_common common;
  size_t length;
  char *pointer;
  union pcl_ast_s *entity;
};

#define PCL_AST_EXP_CODE(AST) ((AST)->exp.code)
#define PCL_AST_EXP_NUMOPS(AST) ((AST)->exp.numops)
#define PCL_AST_EXP_OPERAND(AST, I) ((AST)->exp.operands[(I)])

struct pcl_ast_exp
{
  struct pcl_ast_common common;
  enum pcl_ast_op code;
  uint8_t numops : 8;
  union pcl_ast_s *operands[2];
};

#define PCL_AST_COND_EXP_COND(AST) ((AST)->cond_exp.cond)
#define PCL_AST_COND_EXP_THENEXP(AST) ((AST)->cond_exp.thenexp)
#define PCL_AST_COND_EXP_ELSEEXP(AST) ((AST)->cond_exp.elseexp)

struct pcl_ast_cond_exp
{
  struct pcl_ast_common common;
  union pcl_ast_s *cond;
  union pcl_ast_s *thenexp;
  union pcl_ast_s *elseexp;
};

#define PCL_AST_ENUMERATOR_IDENTIFIER(AST) ((AST)->enumerator.identifier)
#define PCL_AST_ENUMERATOR_VALUE(AST) ((AST)->enumerator.value)
#define PCL_AST_ENUMERATOR_DOCSTR(AST) ((AST)->enumerator.docstr)

struct pcl_ast_enumerator
{
  struct pcl_ast_common common;
  union pcl_ast_s *identifier;
  union pcl_ast_s *value;
  union pcl_ast_s *docstr;
};

#define PCL_AST_ENUM_TAG(AST) ((AST)->enumeration.tag)
#define PCL_AST_ENUM_VALUES(AST) ((AST)->enumeration.values)
#define PCL_AST_ENUM_DOCSTR(AST) ((AST)->enumeration.docstr)

struct pcl_ast_enum
{
  struct pcl_ast_common common;
  union pcl_ast_s *tag;
  union pcl_ast_s *values;
  union pcl_ast_s *docstr;
};

#define PCL_AST_MEM_ENDIAN(AST) ((AST)->mem.endian)
#define PCL_AST_MEM_COMPONENTS(AST) ((AST)->mem.components)

struct pcl_ast_mem
{
  struct pcl_ast_common common;
  enum pcl_ast_endian endian;
  union pcl_ast_s *components;
};

#define PCL_AST_STRUCT_TAG(AST) ((AST)->strct.tag)
#define PCL_AST_STRUCT_DOCSTR(AST) ((AST)->strct.docstr)
#define PCL_AST_STRUCT_MEM(AST) ((AST)->strct.mem)

struct pcl_ast_struct
{
  struct pcl_ast_common common;
  union pcl_ast_s *tag;
  union pcl_ast_s *docstr;
  union pcl_ast_s *mem;
};

#define PCL_AST_FIELD_NAME(AST) ((AST)->field.name)
#define PCL_AST_FIELD_ENDIAN(AST) ((AST)->field.endian)
#define PCL_AST_FIELD_TYPE(AST) ((AST)->field.type)
#define PCL_AST_FIELD_DOCSTR(AST) ((AST)->field.docstr)
#define PCL_AST_FIELD_NUM_ENTS(AST) ((AST)->field.num_ents)
#define PCL_AST_FIELD_SIZE(AST) ((AST)->field.size)

struct pcl_ast_field
{
  struct pcl_ast_common common;
  enum pcl_ast_endian endian;
  union pcl_ast_s *name;
  union pcl_ast_s *type;
  union pcl_ast_s *docstr;
  union pcl_ast_s *num_ents;
  union pcl_ast_s *size;
};

#define PCL_AST_COND_EXP(AST) ((AST)->cond.exp)
#define PCL_AST_COND_THENPART(AST) ((AST)->cond.thenpart)
#define PCL_AST_COND_ELSEPART(AST) ((AST)->cond.elsepart)

struct pcl_ast_cond
{
  struct pcl_ast_common common;
  union pcl_ast_s *exp;
  union pcl_ast_s *thenpart;
  union pcl_ast_s *elsepart;
};

#define PCL_AST_LOOP_PRE(AST) ((AST)->loop.pre)
#define PCL_AST_LOOP_COND(AST) ((AST)->loop.cond)
#define PCL_AST_LOOP_POST(AST) ((AST)->loop.post)
#define PCL_AST_LOOP_BODY(AST) ((AST)->loop.body)

struct pcl_ast_loop
{
  struct pcl_ast_common common;
  union pcl_ast_s *pre;
  union pcl_ast_s *cond;
  union pcl_ast_s *post;
  union pcl_ast_s *body;
};

#define PCL_AST_ARRAY_REF_BASE(AST) ((AST)->aref.base)
#define PCL_AST_ARRAY_REF_INDEX(AST) ((AST)->aref.index)

struct pcl_ast_array_ref
{
  struct pcl_ast_common common;
  union pcl_ast_s *base;
  union pcl_ast_s *index;
};

#define PCL_AST_STRUCT_REF_BASE(AST) ((AST)->sref.base)
#define PCL_AST_STRUCT_REF_IDENTIFIER(AST) ((AST)->sref.identifier)

struct pcl_ast_struct_ref
{
  struct pcl_ast_common common;
  union pcl_ast_s *base;
  union pcl_ast_s *identifier;
};

#define PCL_AST_TYPE_NAME(AST) ((AST)->type.name)
#define PCL_AST_TYPE_CODE(AST) ((AST)->type.code)
#define PCL_AST_TYPE_SIGNED(AST) ((AST)->type.signed_p)
#define PCL_AST_TYPE_SIZE(AST) ((AST)->type.size)
#define PCL_AST_TYPE_ENUMERATION(AST) ((AST)->type.enumeration)
#define PCL_AST_TYPE_STRUCT(AST) ((AST)->type.strt)

struct pcl_ast_type
{
  struct pcl_ast_common common;
  char *name;

  enum pcl_ast_type_code code;
  int signed_p;
  size_t size;
  union pcl_ast_s *enumeration;
  union pcl_ast_s *strt;
};

#define PCL_AST_ASSERTION_EXP(AST) ((AST)->assertion.exp)

struct pcl_ast_assertion
{
  struct pcl_ast_common common;
  union pcl_ast_s *exp;
};
  

/* Finally, the `pcl_ast' type, which represents both an AST tree and
   a node.  */

union pcl_ast_s
{
  struct pcl_ast_common common; /* This field _must_ appear first.  */
  struct pcl_ast_program program;
  struct pcl_ast_struct strct;
  struct pcl_ast_mem mem;
  struct pcl_ast_field field;
  struct pcl_ast_cond cond;
  struct pcl_ast_loop loop;
  struct pcl_ast_identifier identifier;
  struct pcl_ast_integer integer;
  struct pcl_ast_string string;
  struct pcl_ast_doc_string doc_string;
  struct pcl_ast_exp exp;
  struct pcl_ast_cond_exp cond_exp;
  struct pcl_ast_array_ref aref;
  struct pcl_ast_struct_ref sref;
  struct pcl_ast_enumerator enumerator;
  struct pcl_ast_enum enumeration;
  struct pcl_ast_type type;
  struct pcl_ast_assertion assertion;
};

typedef union pcl_ast_s *pcl_ast;

/*  Prototypes for the non-static functions implemented in pcl-ast.c.
    See the .c file for information on how to use them functions.  */

pcl_ast pcl_ast_chainon (pcl_ast ast1, pcl_ast ast2);

enum pcl_ast_endian pcl_ast_default_endian (void);

pcl_ast pcl_ast_make_integer (uint64_t value);
pcl_ast pcl_ast_make_string (const char *str);
pcl_ast pcl_ast_make_doc_string (const char *str, pcl_ast entity);
pcl_ast pcl_ast_make_enumerator (pcl_ast identifier, pcl_ast value,
                                 pcl_ast docstr);
pcl_ast pcl_ast_make_identifier (const char *str);
pcl_ast pcl_ast_make_cond_exp (pcl_ast cond, pcl_ast thenexp,
                               pcl_ast elseexp);
pcl_ast pcl_ast_make_binary_exp (enum pcl_ast_op code, pcl_ast op1,
                                 pcl_ast op2);
pcl_ast pcl_ast_make_unary_exp (enum pcl_ast_op code, pcl_ast op);
pcl_ast pcl_ast_make_array_ref (pcl_ast base, pcl_ast index);
pcl_ast pcl_ast_make_struct_ref (pcl_ast base, pcl_ast identifier);
pcl_ast pcl_ast_make_type (enum pcl_ast_type_code code, int signed_p, 
                           size_t size, pcl_ast enumeration, pcl_ast strct);
pcl_ast pcl_ast_make_struct (pcl_ast tag, pcl_ast docstr,
                             pcl_ast mem);
pcl_ast pcl_ast_make_mem (enum pcl_ast_endian endian,
                          pcl_ast components);

pcl_ast pcl_ast_make_field (pcl_ast name, pcl_ast type, pcl_ast docstr,
                            enum pcl_ast_endian endian, pcl_ast num_ents,
                            pcl_ast size);
pcl_ast pcl_ast_make_enum (pcl_ast tag, pcl_ast values, pcl_ast docstr);
pcl_ast pcl_ast_make_cond (pcl_ast exp, pcl_ast thenpart, pcl_ast elsepart);
pcl_ast pcl_ast_make_loop (pcl_ast pre, pcl_ast cond, pcl_ast post, pcl_ast body);
pcl_ast pcl_ast_make_assertion (pcl_ast exp);
pcl_ast pcl_ast_make_program (void);
pcl_ast pcl_ast_make_loc (void);

#ifdef PCL_DEBUG

void pcl_ast_print (FILE *fd, pcl_ast ast);

#endif /* PCL_DEBUG */

#endif /* ! PCL_AST_H */
