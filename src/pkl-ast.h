/* pkl-ast.h - Abstract Syntax Tree for Poke.  */

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

#ifndef PKL_AST_H
#define PKL_AST_H

#include <config.h>

#ifdef PKL_DEBUG
# include <stdio.h>
#endif

#include <stdint.h>
#include "pkl-ast.h"

/* The following enumeration defines the codes characterizing the
   several types of nodes supported in the PKL abstract syntax
   trees.  */

enum pkl_ast_code
{
  PKL_AST_PROGRAM,
  /* Expressions.  */
  PKL_AST_EXP,
  PKL_AST_COND_EXP,
  PKL_AST_INTEGER,
  PKL_AST_STRING,
  PKL_AST_IDENTIFIER,
  PKL_AST_ARRAY,
  PKL_AST_ARRAY_INITIALIZER,
  PKL_AST_ARRAY_REF,
  PKL_AST_STRUCT,
  PKL_AST_STRUCT_ELEM,
  PKL_AST_STRUCT_REF,
  PKL_AST_OFFSET,
  PKL_AST_CAST,
  PKL_AST_MAP,
  /* Types.  */
  PKL_AST_TYPE,
  PKL_AST_STRUCT_ELEM_TYPE,
  /* Declarations.  */
  PKL_AST_DECL,
  PKL_AST_ENUM,
  PKL_AST_ENUMERATOR,
  /* Statements.  */
  PKL_AST_COMP_STMT,
  PKL_AST_ASS_STMT,
  PKL_AST_IF_STMT,
  PKL_AST_RETURN_STMT,
  PKL_AST_EXP_STMT,
  PKL_AST_LAST
};

/* The AST nodes representing expressions are characterized by
   operators (see below in this file for more details on this.)  The
   following enumeration defines the operator codes.

   The definitions of the operators are in pkl-ops.def.  */

#define PKL_DEF_OP(SYM, STRING) SYM,
enum pkl_ast_op
{
#include "pkl-ops.def"
 PKL_AST_OP_LAST
};
#undef PKL_DEF_OP

/* Certain AST nodes can be characterized of featuring a byte
   endianness.  The following values are supported:

   MSB means that the most significative bytes come first.  This is
   what is popularly known as big-endian.

   LSB means that the least significative bytes come first.  This is
   what is known as little-endian.

   In both endiannesses the bits inside the bytes are ordered from
   most significative to least significative.

   The function `pkl_ast_default_endian' returns the endianness used
   in the system running poke.  */

enum pkl_ast_endian
{
  PKL_AST_MSB, /* Big-endian.  */
  PKL_AST_LSB  /* Little-endian.  */
};

enum pkl_ast_endian pkl_ast_default_endian (void);

/* The AST nodes representing types are characterized by type codes
   (see below in this file for more details on this.)  The following
   enumeration defines the type codes.

   The definitions of the supported integral types are in
   pdl-types.def.  */

#define PKL_DEF_TYPE(CODE,NAME,SIZE,SIGNED) CODE,
enum pkl_ast_integral_type_code
{
# include "pkl-types.def"
  PKL_TYPE_LAST_INTEGRAL
};
#undef PKL_DEF_TYPE

enum pkl_ast_type_code
{
  PKL_TYPE_INTEGRAL = PKL_TYPE_LAST_INTEGRAL,
  PKL_TYPE_STRING,
  PKL_TYPE_ARRAY,
  PKL_TYPE_STRUCT,
  PKL_TYPE_OFFSET,
  PKL_TYPE_NOTYPE,
};

/* Next we define the several supported types of nodes in the abstract
   syntax tree, which are discriminated using the codes defined in the
   `pkl_ast_code' enumeration above.

   Accessor macros are defined to access the attributes of the
   different nodes, and should be used as both l-values and r-values
   to inspect and modify nodes, respectively.

   Declarations for constructor functions are also provided, that can
   be used to create new instances of nodes.  */

typedef union pkl_ast_node *pkl_ast_node;

/* The `pkl_ast_common' struct defines fields which are common to
   every node in the AST, regardless of their type.

   AST is a pointer to the `pkl_ast' structure containing this node.

   UID is an unique identifier that characterizes the node.  It is
   allocated when the node is created, and then never reused again.

   CHAIN and CHAIN2 are used to chain AST nodes together.  This serves
   several purposes in the compiler:

   CHAIN is used to form sibling relationships in the tree.

   CHAIN2 is used to link nodes together in containers, such as hash
   table buckets, and frames.

   The `pkl_ast_chainon' utility function is provided in order to
   confortably add elements to a list of nodes.  It operates on CHAIN,
   not CHAIN2.

   CODE identifies the type of node.

   LOC is the location in the source program of the entity represented
   by the node.  It is the task of the parser to fill in this
   information, which is used in error reporting.

   The LITERAL_P flag is used in expression nodes, and tells whether
   the expression is constant, i.e. whether the value of the
   expression can be calculated at compile time.  This is used to
   implement some optimizations.

   It is possible for a node to be referred from more than one place.
   To manage memory, we use a REFCOUNT that is initially 0.  The
   ASTREF macro defined below tells the node a new reference is being
   made.

   There is no constructor defined for common nodes.  */

#define PKL_AST_AST(AST) ((AST)->common.ast)
#define PKL_AST_UID(AST) ((AST)->common.uid)
#define PKL_AST_CHAIN(AST) ((AST)->common.chain)
#define PKL_AST_TYPE(AST) ((AST)->common.type)
#define PKL_AST_CHAIN2(AST) ((AST)->common.chain2)
#define PKL_AST_CODE(AST) ((AST)->common.code)
#define PKL_AST_LOC(AST) ((AST)->common.loc)
#define PKL_AST_LITERAL_P(AST) ((AST)->common.literal_p)
#define PKL_AST_REGISTERED_P(AST) ((AST)->common.registered_p)
#define PKL_AST_REFCOUNT(AST) ((AST)->common.refcount)

/* NOTE: ASTREF needs an l-value!  */
#define ASTREF(AST) ((AST) ? (++((AST)->common.refcount), (AST)) \
                     : NULL)

struct pkl_ast_loc
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};

typedef struct pkl_ast_loc pkl_ast_loc;

static struct pkl_ast_loc PKL_AST_NOLOC __attribute__((unused))
   = { 0, 0, 0, 0 };

#define PKL_AST_LOC_VALID(L)                    \
  (!((L).first_line == 0                        \
     && (L).first_column == 0                   \
     && (L).last_line == 0                      \
     && (L).last_column == 0))

struct pkl_ast_common
{
  struct pkl_ast *ast;
  uint64_t uid;
  union pkl_ast_node *chain;
  union pkl_ast_node *type;
  union pkl_ast_node *chain2;
  enum pkl_ast_code code : 8;
  struct pkl_ast_loc loc;
  int refcount;

  unsigned literal_p : 1;
};

pkl_ast_node pkl_ast_chainon (pkl_ast_node ast1,
                              pkl_ast_node ast2);


typedef struct pkl_ast *pkl_ast; /* Forward declaration. */

/* PKL_AST_PROGRAM nodes represent PKL programs.

   ELEMS points to a possibly empty list of struct definitions,
   enumerations, and expressions i.e. to nodes of types
   PKL_AST_STRUCT, PKL_AST_ENUM and PKL_AST_EXP respectively.  */

#define PKL_AST_PROGRAM_ELEMS(AST) ((AST)->program.elems)

struct pkl_ast_program
{
  struct pkl_ast_common common;
  union pkl_ast_node *elems;
};

pkl_ast_node pkl_ast_make_program (pkl_ast ast,
                                   pkl_ast_node declarations);

/* PKL_AST_IDENTIFIER nodes represent identifiers in PKL programs.
   
   POINTER must point to a NULL-terminated string.

   LENGTH contains the size in bytes of the identifier

   BACK and OVER conform the lexical address to find the storage of
   the entity represented by the identifier.  See pkl-env.h for a
   description of these.  */

#define PKL_AST_IDENTIFIER_LENGTH(AST) ((AST)->identifier.length)
#define PKL_AST_IDENTIFIER_POINTER(AST) ((AST)->identifier.pointer)
#define PKL_AST_IDENTIFIER_BACK(AST) ((AST)->identifier.back)
#define PKL_AST_IDENTIFIER_OVER(AST) ((AST)->identifier.over)

struct pkl_ast_identifier
{
  struct pkl_ast_common common;
  size_t length;
  char *pointer;
  int back;
  int over;
};

pkl_ast_node pkl_ast_make_identifier (pkl_ast ast,
                                      const char *str);

/* PKL_AST_INTEGER nodes represent integer constants in poke programs.

   VALUE contains a 64-bit unsigned integer.  */

#define PKL_AST_INTEGER_VALUE(AST) ((AST)->integer.value)

struct pkl_ast_integer
{
  struct pkl_ast_common common;
  uint64_t value;
};

pkl_ast_node pkl_ast_make_integer (pkl_ast ast,
                                   uint64_t value);

/* PKL_AST_STRING nodes represent string literals in PKL programs.

   POINTER must point to a NULL-terminated string.
   LENGTH contains the size in bytes of the string.  */

#define PKL_AST_STRING_LENGTH(AST) ((AST)->string.length)
#define PKL_AST_STRING_POINTER(AST) ((AST)->string.pointer)

struct pkl_ast_string
{
  struct pkl_ast_common common;
  size_t length;
  char *pointer;
};

pkl_ast_node pkl_ast_make_string (pkl_ast ast,
                                  const char *str);

/* PKL_AST_ARRAY nodes represent array literals.  Each array holds a
   sequence of elements, all of them having the same type.  There must
   be at least one element in the array, i.e. emtpy arrays are not
   allowed.  */

#define PKL_AST_ARRAY_NELEM(AST) ((AST)->array.nelem)
#define PKL_AST_ARRAY_NINITIALIZER(AST) ((AST)->array.ninitializer)
#define PKL_AST_ARRAY_INITIALIZERS(AST) ((AST)->array.initializers)

struct pkl_ast_array
{
  struct pkl_ast_common common;

  size_t nelem;
  size_t ninitializer;
  union pkl_ast_node *initializers;
};

pkl_ast_node pkl_ast_make_array (pkl_ast ast,
                                 size_t nelem,
                                 size_t ninitializer,
                                 pkl_ast_node initializers);


/* PKL_AST_ARRAY_INITIALIZER nodes represent initializers in array
   literals.  They are characterized by an index into the array and a
   contained expression.  */

#define PKL_AST_ARRAY_INITIALIZER_INDEX(AST) ((AST)->array_initializer.index)
#define PKL_AST_ARRAY_INITIALIZER_EXP(AST) ((AST)->array_initializer.exp)

struct pkl_ast_array_initializer
{
  struct pkl_ast_common common;

  union pkl_ast_node *index;
  union pkl_ast_node *exp;
};

pkl_ast_node pkl_ast_make_array_initializer (pkl_ast ast,
                                             pkl_ast_node index,
                                             pkl_ast_node exp);


/* PKL_AST_STRUCT nodes represent struct literals.  */

#define PKL_AST_STRUCT_NELEM(AST) ((AST)->sct.nelem)
#define PKL_AST_STRUCT_ELEMS(AST) ((AST)->sct.elems)

struct pkl_ast_struct
{
  struct pkl_ast_common common;

  size_t nelem;
  union pkl_ast_node *elems;
};

pkl_ast_node pkl_ast_make_struct (pkl_ast ast,
                                  size_t nelem,
                                  pkl_ast_node elems);

/* PKL_AST_STRUCT_ELEM nodes represent elements in struct
   literals.  */

#define PKL_AST_STRUCT_ELEM_NAME(AST) ((AST)->sct_elem.name)
#define PKL_AST_STRUCT_ELEM_EXP(AST) ((AST)->sct_elem.exp)

struct pkl_ast_struct_elem
{
  struct pkl_ast_common common;

  union pkl_ast_node *name;
  union pkl_ast_node *exp;
};

pkl_ast_node pkl_ast_make_struct_elem (pkl_ast ast,
                                       pkl_ast_node name,
                                       pkl_ast_node exp);

/* PKL_AST_EXP nodes represent unary and binary expressions,
   consisting on an operator and one or two operators, respectively.

   The supported operators are specified in pkl-ops.def.

   There are two constructors for this node type: one for unary
   expressions and another for binary expressions.  */

#define PKL_AST_EXP_CODE(AST) ((AST)->exp.code)
#define PKL_AST_EXP_NUMOPS(AST) ((AST)->exp.numops)
#define PKL_AST_EXP_OPERAND(AST, I) ((AST)->exp.operands[(I)])

struct pkl_ast_exp
{
  struct pkl_ast_common common;
  enum pkl_ast_op code;
  uint8_t numops : 8;
  union pkl_ast_node *operands[2];
};

pkl_ast_node pkl_ast_make_unary_exp (pkl_ast ast,
                                     enum pkl_ast_op code,
                                     pkl_ast_node op);
pkl_ast_node pkl_ast_make_binary_exp (pkl_ast ast,
                                      enum pkl_ast_op code,
                                      pkl_ast_node op1,
                                      pkl_ast_node op2);

/* PKL_AST_COND_EXP nodes represent conditional expressions, having
   exactly the same semantics than the C tertiary operator:

                   exp1 ? exp2 : exp3

   where exp1 is evaluated and then, depending on its value, either
   exp2 (if exp1 is not 0) or exp3 (if exp1 is 0) are executed.

   COND, THENEXP and ELSEEXP must point to expressions, i.e. to nodes
   node of type  PKL_AST_EXP or type PKL_AST_COND_EXP.  */

#define PKL_AST_COND_EXP_COND(AST) ((AST)->cond_exp.cond)
#define PKL_AST_COND_EXP_THENEXP(AST) ((AST)->cond_exp.thenexp)
#define PKL_AST_COND_EXP_ELSEEXP(AST) ((AST)->cond_exp.elseexp)

struct pkl_ast_cond_exp
{
  struct pkl_ast_common common;
  union pkl_ast_node *cond;
  union pkl_ast_node *thenexp;
  union pkl_ast_node *elseexp;
};

pkl_ast_node pkl_ast_make_cond_exp (pkl_ast ast,
                                    pkl_ast_node cond,
                                    pkl_ast_node thenexp,
                                    pkl_ast_node elseexp);

/* PKL_AST_ENUMERATOR nodes represent the definition of a constant
   into an enumeration.

   Each constant is characterized with an IDENTIFIER that identifies
   it globally (meaning enumerator identifiers must be unique), an
   optional VALUE, which must be a constant expression, and an
   optional doc-string.

   If the value is not explicitly provided, it must be calculated
   considering the rest of the enumerators in the enumeration, exactly
   like in C enums.  */

#define PKL_AST_ENUMERATOR_IDENTIFIER(AST) ((AST)->enumerator.identifier)
#define PKL_AST_ENUMERATOR_VALUE(AST) ((AST)->enumerator.value)

struct pkl_ast_enumerator
{
  struct pkl_ast_common common;
  union pkl_ast_node *identifier;
  union pkl_ast_node *value;
};

pkl_ast_node pkl_ast_make_enumerator (pkl_ast ast,
                                      pkl_ast_node identifier,
                                      pkl_ast_node value);

/* PKL_AST_ENUM nodes represent enumerations, having semantics much
   like the C enums.

   TAG is mandatory and must point to a PKL_AST_IDENTIFIER.  This
   identifier characterizes the enumeration globally in the enums
   namespace.

   VALUES must point to a chain of PKL_AST_ENUMERATOR nodes containing
   at least one node.  This means empty enumerations are not allowed.  */

#define PKL_AST_ENUM_TAG(AST) ((AST)->enumeration.tag)
#define PKL_AST_ENUM_VALUES(AST) ((AST)->enumeration.values)

struct pkl_ast_enum
{
  struct pkl_ast_common common;
  union pkl_ast_node *tag;
  union pkl_ast_node *values;
};

pkl_ast_node pkl_ast_make_enum (pkl_ast ast,
                                pkl_ast_node tag,
                                pkl_ast_node values);

/* PKL_AST_ARRAY_REF nodes represent references to an array element.

   BASE must point to a PKL_AST_ARRAY node.

   INDEX must point to an expression whose evaluation is the offset of
   the element into the field, in units of the field's SIZE.  */

#define PKL_AST_ARRAY_REF_ARRAY(AST) ((AST)->aref.array)
#define PKL_AST_ARRAY_REF_INDEX(AST) ((AST)->aref.index)

struct pkl_ast_array_ref
{
  struct pkl_ast_common common;
  union pkl_ast_node *array;
  union pkl_ast_node *index;
};

pkl_ast_node pkl_ast_make_array_ref (pkl_ast ast,
                                     pkl_ast_node array,
                                     pkl_ast_node index);

/* PKL_AST_STRUCT_REF nodes represent references to a struct
   element.  */

#define PKL_AST_STRUCT_REF_STRUCT(AST) ((AST)->sref.sct)
#define PKL_AST_STRUCT_REF_IDENTIFIER(AST) ((AST)->sref.identifier)

struct pkl_ast_struct_ref
{
  struct pkl_ast_common common;

  union pkl_ast_node *sct;
  union pkl_ast_node *identifier;
};

pkl_ast_node pkl_ast_make_struct_ref (pkl_ast ast,
                                      pkl_ast_node sct,
                                      pkl_ast_node identifier);

/* PKL_AST_STRUCT_ELEM_TYPE nodes represent the element part of a
   struct type.

   NAME is a PKL_AST_IDENTIFIER node, or NULL if the struct type
   element has no name.

   TYPE is a PKL_AST_TYPE node.  */

#define PKL_AST_STRUCT_ELEM_TYPE_NAME(AST) ((AST)->sct_type_elem.name)
#define PKL_AST_STRUCT_ELEM_TYPE_TYPE(AST) ((AST)->sct_type_elem.type)

struct pkl_ast_struct_elem_type
{
  struct pkl_ast_common common;

  union pkl_ast_node *name;
  union pkl_ast_node *type;
};

pkl_ast_node pkl_ast_make_struct_elem_type (pkl_ast ast,
                                            pkl_ast_node name,
                                            pkl_ast_node type);

/* PKL_AST_TYPE nodes represent types.
   
   CODE contains the kind of type, as defined in the pkl_ast_type_code
   enumeration above.

   In integral types, SIGNED is 1 if the type denotes a signed numeric
   type.  In non-integral types SIGNED is 0.  SIZE is the size in bits
   of type.

   In array types, ETYPE is a PKL_AST_TYPE node.  If NELEM is present
   then it is the number of elements in the array.

   In struct types, NELEM is the number of elements in the struct type.
   ELEMS is a chain of PKL_AST_STRUCT_ELEM_TYPE nodes.

   When the size of a value of a given type can be determined at
   compile time, we say that such type is "complete".  Otherwise, we
   say that the type is "incomplete" and should be completed at
   run-time.  */

#define PKL_AST_TYPE_CODE(AST) ((AST)->type.code)
#define PKL_AST_TYPE_NAME(AST) ((AST)->type.name)
#define PKL_AST_TYPE_COMPLETE(AST) ((AST)->type.complete)
#define PKL_AST_TYPE_I_SIZE(AST) ((AST)->type.val.integral.size)
#define PKL_AST_TYPE_I_SIGNED(AST) ((AST)->type.val.integral.signed_p)
#define PKL_AST_TYPE_A_NELEM(AST) ((AST)->type.val.array.nelem)
#define PKL_AST_TYPE_A_ETYPE(AST) ((AST)->type.val.array.etype)
#define PKL_AST_TYPE_S_NELEM(AST) ((AST)->type.val.sct.nelem)
#define PKL_AST_TYPE_S_ELEMS(AST) ((AST)->type.val.sct.elems)
#define PKL_AST_TYPE_O_UNIT(AST) ((AST)->type.val.off.unit)
#define PKL_AST_TYPE_O_BASE_TYPE(AST) ((AST)->type.val.off.base_type)

#define PKL_AST_TYPE_COMPLETE_UNKNOWN 0
#define PKL_AST_TYPE_COMPLETE_YES 1
#define PKL_AST_TYPE_COMPLETE_NO 2
  
struct pkl_ast_type
{
  struct pkl_ast_common common;

  enum pkl_ast_type_code code;
  int complete;
  char *name;
  
  union
  {
    struct
    {
      size_t size;
      int signed_p;
    } integral;

    struct
    {
      union pkl_ast_node *nelem;
      union pkl_ast_node *etype;
    } array;

    struct
    {
      size_t nelem;
      union pkl_ast_node *elems;
    } sct;

    struct
    {
      union pkl_ast_node *unit;
      union pkl_ast_node *base_type;
    } off;
    
  } val;
};

pkl_ast_node pkl_ast_make_integral_type (pkl_ast ast, size_t size, int signed_p);
pkl_ast_node pkl_ast_make_string_type (pkl_ast ast);
pkl_ast_node pkl_ast_make_array_type (pkl_ast ast, pkl_ast_node nelem, pkl_ast_node etype);
pkl_ast_node pkl_ast_make_struct_type (pkl_ast ast, size_t nelem, pkl_ast_node elems);
pkl_ast_node pkl_ast_make_offset_type (pkl_ast ast, pkl_ast_node base_type, pkl_ast_node unit);

pkl_ast_node pkl_ast_dup_type (pkl_ast_node type);
int pkl_ast_type_equal (pkl_ast_node t1, pkl_ast_node t2);
pkl_ast_node pkl_ast_sizeof_type (pkl_ast ast, pkl_ast_node type);
int pkl_ast_type_is_complete (pkl_ast_node type);

/* PKL_AST_DECL nodes represent the declaration of a named entity:
   function, type, variable....

   NAME is PKL_AST_IDENTIFIER node containing the name in the
   association.

   TYPE is the type of the entity referred by the name.

   INITIAL is the initial value of the entity, if any.  The initial
   value is optional for variables and constants if an explicit type
   was used in the declaration.  Initial values are mandatory for
   functions and type declarations.  */

#define PKL_AST_DECL_NAME(AST) ((AST)->decl.name)
#define PKL_AST_DECL_TYPE(AST) ((AST)->decl.type)
#define PKL_AST_DECL_INITIAL(AST) ((AST)->decl.initial)

struct pkl_ast_decl
{
  struct pkl_ast_common common;

  union pkl_ast_node *name;
  union pkl_ast_node *type;
  union pkl_ast_node *initial;
};

/* PKL_AST_OFFSET nodes represent poke object constructions.

   MAGNITUDE is an integer expression.
   UNIT is either PKL_AST_OFFSET_UNIT_BITS or
   PKL_AST_AST_OFFSET_UNITS_BYTES.  */

#define PKL_AST_OFFSET_MAGNITUDE(AST) ((AST)->offset.magnitude)
#define PKL_AST_OFFSET_UNIT(AST) ((AST)->offset.unit)

#define PKL_AST_OFFSET_UNIT_BITS 1
#define PKL_AST_OFFSET_UNIT_NIBBLES 4
#define PKL_AST_OFFSET_UNIT_BYTES (2 * PKL_AST_OFFSET_UNIT_NIBBLES)
#define PKL_AST_OFFSET_UNIT_KILOBITS (1024 * PKL_AST_OFFSET_UNIT_BITS)
#define PKL_AST_OFFSET_UNIT_KILOBYTES (1024 * PKL_AST_OFFSET_UNIT_BYTES)
#define PKL_AST_OFFSET_UNIT_MEGABITS (1024 * PKL_AST_OFFSET_UNIT_KILOBITS)
#define PKL_AST_OFFSET_UNIT_MEGABYTES (1024 * PKL_AST_OFFSET_UNIT_KILOBYTES)
#define PKL_AST_OFFSET_UNIT_GIGABITS (1024 * PKL_AST_OFFSET_UNIT_MEGABITS)

struct pkl_ast_offset
{
  struct pkl_ast_common common;

  union pkl_ast_node *magnitude;
  union pkl_ast_node *unit;
};

pkl_ast_node pkl_ast_make_offset (pkl_ast ast,
                                  pkl_ast_node magnitude,
                                  pkl_ast_node unit);

pkl_ast_node pkl_ast_id_to_offset_unit (pkl_ast ast,
                                        pkl_ast_node id);

/* PKL_AST_CAST nodes represent casts at the language level.
   
   TYPE is the target type in the case.
   EXP is the expression whole value should be casted to the targe
   type.  */

#define PKL_AST_CAST_TYPE(AST) ((AST)->cast.type)
#define PKL_AST_CAST_EXP(AST) ((AST)->cast.exp)

struct pkl_ast_cast
{
  struct pkl_ast_common common;

  union pkl_ast_node *type;
  union pkl_ast_node *exp;
};

pkl_ast_node pkl_ast_make_cast (pkl_ast ast,
                                pkl_ast_node type,
                                pkl_ast_node exp);

/* PKL_AST_MAP nodes represent maps, i.e. the mapping of some type at
   some offset in IO space.

   TYPE is the mapped type.
   OFFSET is the offset in IO space where the TYPE is mapped.  */

#define PKL_AST_MAP_TYPE(AST) ((AST)->map.type)
#define PKL_AST_MAP_OFFSET(AST) ((AST)->map.offset)

struct pkl_ast_map
{
  struct pkl_ast_common common;

  union pkl_ast_node *type;
  union pkl_ast_node *offset;
};

pkl_ast_node pkl_ast_make_map (pkl_ast ast,
                               pkl_ast_node type,
                               pkl_ast_node offset);

/* PKL_AST_COMPOUND_STMT nodes represent compound statements in the
   language.

   STMTS is a possibly empty chain of statements.  */

#define PKL_AST_COMP_STMT_STMTS(AST) ((AST)->comp_stmt.stmts)

struct pkl_ast_comp_stmt
{
  struct pkl_ast_common common;

  union pkl_ast_node *stmts;
};

pkl_ast_node pkl_ast_make_comp_stmt (pkl_ast ast, pkl_ast_node stmts);

/* PKL_AST_ASS_STMT nodes represent assignment statements in the
   language.

   LVALUE is the l-value of the assignment.
   EXP is the r-value of the assignment.  */

#define PKL_AST_ASS_STMT_LVALUE(AST) ((AST)->ass_stmt.lvalue)
#define PKL_AST_ASS_STMT_EXP(AST) ((AST)->ass_stmt.exp)

struct pkl_ast_ass_stmt
{
  struct pkl_ast_common common;

  union pkl_ast_node *lvalue;
  union pkl_ast_node *exp;
};

pkl_ast_node pkl_ast_make_ass_stmt (pkl_ast ast,
                                    pkl_ast_node lvalue, pkl_ast_node exp);

/* PKL_AST_IF_STMT nodes represent conditional statements, with an
   optional `else' part.

   EXP is the expression that is to be evaluated to decide what branch
   to take.

   THEN_STMT is the statement to be executed when EXP holds true.
   ELSE_STMT is the statement to be executed when EXP holds false.  */

#define PKL_AST_IF_STMT_EXP(AST) ((AST)->if_stmt.exp)
#define PKL_AST_IF_STMT_THEN_STMT(AST) ((AST)->if_stmt.then_stmt)
#define PKL_AST_IF_STMT_ELSE_STMT(AST) ((AST)->if_stmt.else_stmt)

struct pkl_ast_if_stmt
{
  struct pkl_ast_common common;

  union pkl_ast_node *exp;
  union pkl_ast_node *then_stmt;
  union pkl_ast_node *else_stmt;
};

pkl_ast_node pkl_ast_make_if_stmt (pkl_ast ast,
                                   pkl_ast_node exp,
                                   pkl_ast_node then_stmt,
                                   pkl_ast_node else_stmt);

/* PKL_AST_RETURN_STMT nodes represent return statements.

   EXP is the expression to return to the caller.  */

#define PKL_AST_RETURN_STMT_EXP(AST) ((AST)->return_stmt.exp)

struct pkl_ast_return_stmt
{
  struct pkl_ast_common common;

  union pkl_ast_node *exp;
};

pkl_ast_node pkl_ast_make_return_stmt (pkl_ast ast, pkl_ast_node exp);

/* Finally, the `pkl_ast_node' type, which represents an AST node of
   any type.  */

union pkl_ast_node
{
  struct pkl_ast_common common; /* This field _must_ appear first.  */
  struct pkl_ast_program program;
  /* Expressions.  */
  struct pkl_ast_exp exp;
  struct pkl_ast_cond_exp cond_exp;
  struct pkl_ast_integer integer;
  struct pkl_ast_string string;
  struct pkl_ast_identifier identifier;
  struct pkl_ast_array array;
  struct pkl_ast_array_initializer array_initializer;
  struct pkl_ast_array_ref aref;
  struct pkl_ast_struct sct;
  struct pkl_ast_struct_elem sct_elem;
  struct pkl_ast_struct_ref sref;
  struct pkl_ast_offset offset;
  struct pkl_ast_cast cast;
  struct pkl_ast_map map;
  /* Types.  */
  struct pkl_ast_type type;
  struct pkl_ast_struct_elem_type sct_type_elem;
  /* Declarations.  */
  struct pkl_ast_decl decl;
  struct pkl_ast_enum enumeration;
  struct pkl_ast_enumerator enumerator;
  /* Statements.  */
  struct pkl_ast_comp_stmt comp_stmt;
  struct pkl_ast_ass_stmt ass_stmt;
  struct pkl_ast_if_stmt if_stmt;
  struct pkl_ast_return_stmt return_stmt;
};

/* The `pkl_ast' struct defined below contains a PKL abstract syntax tree.

   AST contains the tree of linked nodes, starting with a
   PKL_AST_PROGRAM node.

   Some of the tree nodes can be stored in the several hash tables,
   which are created during parsing.

   `pkl_ast_init' allocates and initializes a new AST and returns a
   pointer to it.

   `pkl_ast_free' frees all the memory allocated to store the AST
   nodes and also the hash tables.

   `pkl_ast_node_free' frees the memory allocated to store a single
   node in the AST and its descendants.  This function is used by the
   bison parser.  */

#define HASH_TABLE_SIZE 1008
typedef pkl_ast_node pkl_hash[HASH_TABLE_SIZE];

struct pkl_ast
{
  size_t uid;
  pkl_ast_node ast;

  pkl_hash ids_hash_table;
  pkl_hash types_hash_table;
  pkl_hash enums_hash_table;
  pkl_hash structs_hash_table;

  pkl_ast_node *stdtypes;
  pkl_ast_node stringtype;

  char *buffer;
};

pkl_ast pkl_ast_init (void);
void pkl_ast_free (pkl_ast ast);
void pkl_ast_node_free (pkl_ast_node ast);

/* The following functions are used by the lexer and the parser in
   order to populate/inquiry the hash tables in the AST.  */

pkl_ast_node pkl_ast_get_identifier (pkl_ast ast,
                                     const char *str);

pkl_ast_node pkl_ast_get_registered (pkl_ast ast,
                                     const char *name,
                                     enum pkl_ast_code code);

pkl_ast_node pkl_ast_register (pkl_ast ast,
                               const char *name,
                               pkl_ast_node ast_node);

pkl_ast_node pkl_ast_get_string_type (pkl_ast ast);
pkl_ast_node pkl_ast_get_integral_type (pkl_ast ast,
                                        size_t size, int signed_p);

#ifdef PKL_DEBUG

/* The following function dumps a human-readable description of the
   tree headed by the node AST.  It is used for debugging
   purposes.  */

void pkl_ast_print (FILE *fd, pkl_ast_node ast);

/* Reverse the order of elements chained by CHAIN, and return the new
   head of the chain (old last element).  */

pkl_ast_node pkl_ast_reverse (pkl_ast_node ast);

#endif

#endif /* ! PKL_AST_H */
