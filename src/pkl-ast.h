/* pkl-ast.h - Abstract Syntax Tree for Poke.  */

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
  PKL_AST_EXP,
  PKL_AST_COND_EXP,
  PKL_AST_ENUM,
  PKL_AST_ENUMERATOR,
  PKL_AST_STRUCT,
  PKL_AST_MEM,
  PKL_AST_FIELD,
  PKL_AST_COND,
  PKL_AST_LOOP,
  PKL_AST_ASSERTION,
  PKL_AST_TYPE,
  PKL_AST_ARRAY_REF,
  PKL_AST_STRUCT_REF,
  PKL_AST_INTEGER,
  PKL_AST_STRING,
  PKL_AST_IDENTIFIER,
  PKL_AST_DOC_STRING,
  PKL_AST_LOC,
  PKL_AST_CAST,
  PKL_AST_ARRAY,
  PKL_AST_ARRAY_ELEM,
  PKL_AST_TUPLE,
  PKL_AST_TUPLE_ELEM
};

/* The AST nodes representing expressions are characterized by
   operators (see below in this file for more details on this.)  The
   following enumeration defines the operator codes.

   The definitions of the operators are in pkl-ops.def.  */

#define PKL_DEF_OP(SYM, STRING) SYM,
enum pkl_ast_op
{
#include "pkl-ops.def"
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

   The function `pkl_ast-default_endian' returns the endianness used
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

   The definitions of the supported types are in pdl-types.def.  */

#define PKL_DEF_TYPE(CODE,ID,SIZE,SIGNED) CODE,
enum pkl_ast_type_code
{
#include "pkl-types.def"
  PKL_TYPE_STRING,
  PKL_TYPE_TUPLE,
  PKL_TYPE_ENUM,
  PKL_TYPE_STRUCT,
  PKL_TYPE_NOTYPE,
};
#undef PKL_DEF_TYPE

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

   CHAIN and CHAIN2 are used to chain AST nodes together.  This serves
   several purposes in the compiler:

   CHAIN is used to form sibling relationships in the tree.

   CHAIN2 is used to link nodes together in containers, such as hash
   table buckets.

   The `pkl_ast_chainon' utility function is provided in order to
   confortably add elements to a list of nodes.  It operates on CHAIN,
   not CHAIN2.

   CODE identifies the type of node.

   The LITERAL_P flag is used in expression nodes, and tells whether
   the expression is constant, i.e. whether the value of the
   expression can be calculated at compile time.  This is used to
   implement some optimizations.

   It is possible for a node to be referred from more than one place.
   To manage memory, we use a REFCOUNT that is initially 0.  The
   ASTREF macro defined below tells the node a new reference is being
   made.

   There is no constructor defined for common nodes.  */

#define PKL_AST_CHAIN(AST) ((AST)->common.chain)
#define PKL_AST_TYPE(AST) ((AST)->common.type)
#define PKL_AST_CHAIN2(AST) ((AST)->common.chain2)
#define PKL_AST_CODE(AST) ((AST)->common.code)
#define PKL_AST_LITERAL_P(AST) ((AST)->common.literal_p)
#define PKL_AST_REGISTERED_P(AST) ((AST)->common.registered_p)
#define PKL_AST_REFCOUNT(AST) ((AST)->common.refcount)

#define ASTREF(AST) ((AST) ? (++((AST)->common.refcount), (AST)) \
                     : NULL)

struct pkl_ast_common
{
  union pkl_ast_node *chain;
  union pkl_ast_node *type;
  union pkl_ast_node *chain2;
  enum pkl_ast_code code : 8;
  int refcount;

  unsigned literal_p : 1;
};

pkl_ast_node pkl_ast_chainon (pkl_ast_node ast1,
                              pkl_ast_node ast2);

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

pkl_ast_node pkl_ast_make_program (pkl_ast_node declarations);

/* PKL_AST_IDENTIFIER nodes represent identifiers in PKL programs.
   
   POINTER must point to a NULL-terminated string.
   LENGTH contains the size in bytes of the identifier.  */

#define PKL_AST_IDENTIFIER_LENGTH(AST) ((AST)->identifier.length)
#define PKL_AST_IDENTIFIER_POINTER(AST) ((AST)->identifier.pointer)

struct pkl_ast_identifier
{
  struct pkl_ast_common common;
  size_t length;
  char *pointer;
};

pkl_ast_node pkl_ast_make_identifier (const char *str);

/* PKL_AST_INTEGER nodes represent integer constants in poke programs.

   VALUE contains a 64-bit unsigned integer.  */

#define PKL_AST_INTEGER_VALUE(AST) ((AST)->integer.value)

struct pkl_ast_integer
{
  struct pkl_ast_common common;
  uint64_t value;
};

pkl_ast_node pkl_ast_make_integer (uint64_t value);

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

pkl_ast_node pkl_ast_make_string (const char *str);

/* PKL_AST_DOC_STRING nodes represent text that explains the meaning
   or the contents of other nodes/entities.

   POINTER must point to a NULL-terminated string.
   LENGTH contains the size in bytes of the doc string.  */

#define PKL_AST_DOC_STRING_LENGTH(AST) ((AST)->doc_string.length)
#define PKL_AST_DOC_STRING_POINTER(AST) ((AST)->doc_string.pointer)

struct pkl_ast_doc_string
{
  struct pkl_ast_common common;
  size_t length;
  char *pointer;
};

pkl_ast_node pkl_ast_make_doc_string (const char *str,
                                      pkl_ast_node entity);

/* PKL_AST_CAST nodes represent cast constructions, which are
   applications of types to IO space.  XXX: explain.  */

#define PKL_AST_CAST_EXP(AST) ((AST)->cast.exp)

struct pkl_ast_cast
{
  struct pkl_ast_common common;
  union pkl_ast_node *exp;
};

pkl_ast_node pkl_ast_make_cast (pkl_ast_node type,
                                pkl_ast_node exp);


/* PKL_AST_ARRAY nodes represent array literals.  Each array holds a
   sequence of elements, all of them having the same type.  There must
   be at least one element in the array, i.e. emtpy arrays are not
   allowed.  */

#define PKL_AST_ARRAY_NELEM(AST) ((AST)->array.nelem)
#define PKL_AST_ARRAY_ELEMS(AST) ((AST)->array.elems)

struct pkl_ast_array
{
  struct pkl_ast_common common;

  size_t nelem;
  union pkl_ast_node *elems;
};

pkl_ast_node pkl_ast_make_array (pkl_ast_node etype,
                                 size_t nelem,
                                 pkl_ast_node elems);


/* PKL_AST_ARRAY_ELEM nodes represent nodes in array literals.  They
   are characterized by an index into the array and a contained
   expression.  */

#define PKL_AST_ARRAY_ELEM_INDEX(AST) ((AST)->array_elem.index)
#define PKL_AST_ARRAY_ELEM_EXP(AST) ((AST)->array_elem.exp)

struct pkl_ast_array_elem
{
  struct pkl_ast_common common;

#define PKL_AST_ARRAY_NOINDEX ((uint64_t)-1)
  size_t index;
  union pkl_ast_node *exp;
};

pkl_ast_node pkl_ast_make_array_elem (size_t index,
                                      pkl_ast_node exp);


/* PKL_AST_TUPLE nodes represent tuples.  */

#define PKL_AST_TUPLE_NELEM(AST) ((AST)->tuple.nelem)
#define PKL_AST_TUPLE_ELEMS(AST) ((AST)->tuple.elems)

struct pkl_ast_tuple
{
  struct pkl_ast_common common;

  size_t nelem;
  union pkl_ast_node *elems;
};

pkl_ast_node pkl_ast_make_tuple (size_t nelem,
                                 pkl_ast_node elems);

/* PKL_AST_TUPLE_ELEM nodes represent elements in tuples.  */

#define PKL_AST_TUPLE_ELEM_NAME(AST) ((AST)->tuple_elem.name)
#define PKL_AST_TUPLE_ELEM_OFFSET(AST) ((AST)->tuple_elem.offset)
#define PKL_AST_TUPLE_ELEM_EXP(AST) ((AST)->tuple_elem.exp)

struct pkl_ast_tuple_elem
{
  struct pkl_ast_common common;

  char *name;
  union pkl_ast_node *offset;
  union pkl_ast_node *exp;
};

pkl_ast_node pkl_ast_make_tuple_elem (const char *name,
                                      pkl_ast_node offset,
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

pkl_ast_node pkl_ast_make_unary_exp (enum pkl_ast_op code,
                                     pkl_ast_node type,
                                     pkl_ast_node op);
pkl_ast_node pkl_ast_make_binary_exp (enum pkl_ast_op code,
                                      pkl_ast_node type,
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

pkl_ast_node pkl_ast_make_cond_exp (pkl_ast_node cond,
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
#define PKL_AST_ENUMERATOR_DOCSTR(AST) ((AST)->enumerator.docstr)

struct pkl_ast_enumerator
{
  struct pkl_ast_common common;
  union pkl_ast_node *identifier;
  union pkl_ast_node *value;
  union pkl_ast_node *docstr;
};

pkl_ast_node pkl_ast_make_enumerator (pkl_ast_node identifier,
                                      pkl_ast_node value,
                                      pkl_ast_node docstr);

/* PKL_AST_ENUM nodes represent enumerations, having semantics much
   like the C enums.

   TAG is mandatory and must point to a PKL_AST_IDENTIFIER.  This
   identifier characterizes the enumeration globally in the enums
   namespace.

   VALUES must point to a chain of PKL_AST_ENUMERATOR nodes containing
   at least one node.  This means empty enumerations are not allowed.

   DOCSTR optionally points to a PKL_AST_DOCSTR.  If it exists, it
   contains text explaining the meaning of the collection of constants
   defined in the enumeration.  */

#define PKL_AST_ENUM_TAG(AST) ((AST)->enumeration.tag)
#define PKL_AST_ENUM_VALUES(AST) ((AST)->enumeration.values)
#define PKL_AST_ENUM_DOCSTR(AST) ((AST)->enumeration.docstr)

struct pkl_ast_enum
{
  struct pkl_ast_common common;
  union pkl_ast_node *tag;
  union pkl_ast_node *values;
  union pkl_ast_node *docstr;
};

pkl_ast_node pkl_ast_make_enum (pkl_ast_node tag,
                                pkl_ast_node values,
                                pkl_ast_node docstr);


/* PKL_AST_STRUCT nodes represent PKL structs, which are similar to C
   structs... but not quite the same thing!

   In PKL a struct is basically the declaration of a named memory
   layout (see below for more info on memory layouts.)  Structs can
   only be declared at the top-level.
   
   TAG must point to a node of type PKL_AST_IDENTIFIER, and globally
   identifies the struct in the strucs namespace.

   MEM must point to a node of type PKL_AST_MEM, that contains the
   memory layout named in this struct.

   DOCSTR optionally points to a docstring that documents the
   meaning/contents of the struct.  */


#define PKL_AST_STRUCT_TAG(AST) ((AST)->strct.tag)
#define PKL_AST_STRUCT_DOCSTR(AST) ((AST)->strct.docstr)
#define PKL_AST_STRUCT_MEM(AST) ((AST)->strct.mem)

struct pkl_ast_struct
{
  struct pkl_ast_common common;
  union pkl_ast_node *tag;
  union pkl_ast_node *docstr;
  union pkl_ast_node *mem;
};

pkl_ast_node pkl_ast_make_struct (pkl_ast_node tag,
                                  pkl_ast_node docstr,
                                  pkl_ast_node mem);

/* PKL_AST_MEM nodes represent memory layouts.  The layouts are
   described by a program that, once executed, describes a collection
   of subareas over a possibly non-contiguous memory area.

   ENDIAN is the endianness used by the components (filds) of the
   memory layout.

   COMPONENTS points to a possibly empty list of other layouts,
   fields, conditionals, loops and assertions, i.e. of nodes of types
   PKL_AST_MEM, PKL_AST_FIELD, PKL_AST_COND, PKL_AST_LOOP and
   PKL_AST_ASSERTION respectively.  */

#define PKL_AST_MEM_ENDIAN(AST) ((AST)->mem.endian)
#define PKL_AST_MEM_COMPONENTS(AST) ((AST)->mem.components)

struct pkl_ast_mem
{
  struct pkl_ast_common common;
  enum pkl_ast_endian endian;
  union pkl_ast_node *components;
};

pkl_ast_node pkl_ast_make_mem (enum pkl_ast_endian endian,
                               pkl_ast_node components);

/* PKL_AST_FIELD nodes represent fields in memory layouts.

   ENDIAN is the endianness in which the data in the field is stored.

   NAME must point to an identifier which should be unique within the
   containing struct (i.e. within the containing top-level memory
   layout.)
   
   TYPE must point to a PKL_AST_TYPE node that describes the value
   stored in the field.

   NUM_ENTS must point to an expression specifiying how many entities
   of size SIZE are stored in the field.

   SIZE is the size in bits of each element stored in the field.

   DOCSTR optionally points to a PKL_AST_DOC_STRING node that
   documents the purpose/meaning of the field contents.  */

#define PKL_AST_FIELD_NAME(AST) ((AST)->field.name)
#define PKL_AST_FIELD_ENDIAN(AST) ((AST)->field.endian)
#define PKL_AST_FIELD_TYPE(AST) ((AST)->field.type)
#define PKL_AST_FIELD_DOCSTR(AST) ((AST)->field.docstr)
#define PKL_AST_FIELD_NUM_ENTS(AST) ((AST)->field.num_ents)
#define PKL_AST_FIELD_SIZE(AST) ((AST)->field.size)

struct pkl_ast_field
{
  struct pkl_ast_common common;
  enum pkl_ast_endian endian;
  union pkl_ast_node *name;
  union pkl_ast_node *type;
  union pkl_ast_node *docstr;
  union pkl_ast_node *num_ents;
  union pkl_ast_node *size;
};

pkl_ast_node pkl_ast_make_field (pkl_ast_node name,
                                 pkl_ast_node type,
                                 pkl_ast_node docstr,
                                 enum pkl_ast_endian endian,
                                 pkl_ast_node num_ents,
                                 pkl_ast_node size);

/* PKL_AST_COND nodes represent conditionals.

   A conditional allows to conditionally define memory layouts, in a
   very similar way the if-then-else statements determine the control
   flow in conventional programming languages.

   EXP must point to an expression whose result is interpreted as a
   boolean in a C-style, 0 meaning false and any other value meaning
   true.

   THENPART must point to a PKL_AST_MEM node, which is the memory
   layout that gets defined should EXP evaluate to true.

   ELSEPART optionally points to a PKL_AST_MEM node, which is the
   memory layout that gets defined should EXP evaluate to false.  */

#define PKL_AST_COND_EXP(AST) ((AST)->cond.exp)
#define PKL_AST_COND_THENPART(AST) ((AST)->cond.thenpart)
#define PKL_AST_COND_ELSEPART(AST) ((AST)->cond.elsepart)

struct pkl_ast_cond
{
  struct pkl_ast_common common;
  union pkl_ast_node *exp;
  union pkl_ast_node *thenpart;
  union pkl_ast_node *elsepart;
};

pkl_ast_node pkl_ast_make_cond (pkl_ast_node exp,
                                pkl_ast_node thenpart,
                                pkl_ast_node elsepart);

/* PKL_AST_LOOP nodes represent loops.

   A loop allows to define multiple memory layouts, in a very similar
   way iterative statements work in conventional programming
   languages.

   PRE optionally points to an expression which is evaluated before
   entering the loop.

   COND optionally points to a condition that determines whether the
   loop is entered initially, and afer each iteration.

   BODY must point to a PKL_AST_MEM node, which is the memory layout
   that is defined in each loop iteration.
   
   POST optionally points to an expression which is evaluated after
   defining the memory layout in each iteration.

   Note that if COND is not defined, the effect is an infinite loop.  */

#define PKL_AST_LOOP_PRE(AST) ((AST)->loop.pre)
#define PKL_AST_LOOP_COND(AST) ((AST)->loop.cond)
#define PKL_AST_LOOP_POST(AST) ((AST)->loop.post)
#define PKL_AST_LOOP_BODY(AST) ((AST)->loop.body)

struct pkl_ast_loop
{
  struct pkl_ast_common common;
  union pkl_ast_node *pre;
  union pkl_ast_node *cond;
  union pkl_ast_node *post;
  union pkl_ast_node *body;
};

pkl_ast_node pkl_ast_make_loop (pkl_ast_node pre,
                                pkl_ast_node cond,
                                pkl_ast_node post,
                                pkl_ast_node body);

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

pkl_ast_node pkl_ast_make_array_ref (pkl_ast_node array,
                                     pkl_ast_node index);

/* PKL_AST_STRUCT_REF nodes represent references to fields within a
   struct.

   BASE must point to a PKL_AST_IDENTIFIER node, which in turn should
   identify a field whose type is a struct.

   IDENTIFIER must point to a PKL_AST_IDENTIFIER node, which in turn
   should identify a field defined in the struct characterized by
   BASE.  */

#define PKL_AST_STRUCT_REF_BASE(AST) ((AST)->sref.base)
#define PKL_AST_STRUCT_REF_IDENTIFIER(AST) ((AST)->sref.identifier)

struct pkl_ast_struct_ref
{
  struct pkl_ast_common common;
  union pkl_ast_node *base;
  union pkl_ast_node *identifier;
};

pkl_ast_node pkl_ast_make_struct_ref (pkl_ast_node base,
                                      pkl_ast_node identifier);

/* PKL_AST_TYPE nodes represent field types.
   
   CODE contains the kind of type, as defined in the pkl_ast_type_code
   enumeration above.

   SIGNED is 1 if the type denotes a signed numeric type.

   SIZE is the witdh in bits of type.

   ENUMERATION must point to a PKL_AST_ENUM node if the type code is
   PKL_TYPE_ENUM.

   STRUCT must point to a PKL_AST_STRUCT node if the type code is
   PKL_TYPE_STRUCT.  */

#define PKL_AST_TYPE_NAME(AST) ((AST)->type.name)
#define PKL_AST_TYPE_CODE(AST) ((AST)->type.code)
#define PKL_AST_TYPE_SIGNED(AST) ((AST)->type.signed_p)
#define PKL_AST_TYPE_ARRAYOF(AST) ((AST)->type.arrayof)
#define PKL_AST_TYPE_SIZE(AST) ((AST)->type.size)
#define PKL_AST_TYPE_ENUMERATION(AST) ((AST)->type.enumeration)
#define PKL_AST_TYPE_STRUCT(AST) ((AST)->type.strt)
#define PKL_AST_TYPE_INTEGRAL(AST) ((AST)->type.size > 0)

struct pkl_ast_type
{
  struct pkl_ast_common common;
  char *name;

  enum pkl_ast_type_code code;
  int signed_p;
  int arrayof;
  size_t size;
  union pkl_ast_node *enumeration;
  union pkl_ast_node *strt;
};

pkl_ast_node pkl_ast_make_type (enum pkl_ast_type_code code,
                                int signed_p,
                                size_t size,
                                pkl_ast_node enumeration,
                                pkl_ast_node strct);

pkl_ast_node pkl_ast_type_dup (pkl_ast_node type);
int pkl_ast_type_equal (pkl_ast_node t1, pkl_ast_node t2);

/* PKL_AST_LOC nodes represent the current struct's location
   counter.

   This node can occur anywhere in an expression, inluding both sides
   of assignment operators.  */

struct pkl_ast_loc
{
  struct pkl_ast_common common;
};

pkl_ast_node pkl_ast_make_loc (void);

/* PKL_AST_ASSERTION nodes represent checks that can occur anywhere
   within a memory layout.

   EXP must point to an expression.  If the expression evaluates to 1
   a fatal error is raised at execution time.  */

#define PKL_AST_ASSERTION_EXP(AST) ((AST)->assertion.exp)

struct pkl_ast_assertion
{
  struct pkl_ast_common common;
  union pkl_ast_node *exp;
};

pkl_ast_node pkl_ast_make_assertion (pkl_ast_node exp);

/* Finally, the `pkl_ast_node' type, which represents an AST node of
   any type.  */

union pkl_ast_node
{
  struct pkl_ast_common common; /* This field _must_ appear first.  */
  struct pkl_ast_program program;
  struct pkl_ast_struct strct;
  struct pkl_ast_mem mem;
  struct pkl_ast_field field;
  struct pkl_ast_cond cond;
  struct pkl_ast_loop loop;
  struct pkl_ast_identifier identifier;
  struct pkl_ast_integer integer;
  struct pkl_ast_string string;
  struct pkl_ast_doc_string doc_string;
  struct pkl_ast_exp exp;
  struct pkl_ast_cond_exp cond_exp;
  struct pkl_ast_array_ref aref;
  struct pkl_ast_struct_ref sref;
  struct pkl_ast_enumerator enumerator;
  struct pkl_ast_enum enumeration;
  struct pkl_ast_type type;
  struct pkl_ast_assertion assertion;
  struct pkl_ast_loc loc;
  struct pkl_ast_cast cast;
  struct pkl_ast_array array;
  struct pkl_ast_array_elem array_elem;
  struct pkl_ast_tuple tuple;
  struct pkl_ast_tuple_elem tuple_elem;
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
  pkl_ast_node ast;

  pkl_hash ids_hash_table;
  pkl_hash types_hash_table;
  pkl_hash enums_hash_table;
  pkl_hash structs_hash_table;

  pkl_ast_node *stdtypes;
};

typedef struct pkl_ast *pkl_ast;

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

pkl_ast_node pkl_ast_get_std_type (pkl_ast ast,
                                   enum pkl_ast_type_code code);

pkl_ast_node pkl_ast_search_std_type (pkl_ast ast,
                                      size_t size, int signed_p);

#ifdef PKL_DEBUG

/* The following function dumps a human-readable description of the
   tree headed by the node AST.  It is used for debugging
   purposes.  */

void pkl_ast_print (FILE *fd, pkl_ast_node ast);

#endif

#endif /* ! PKL_AST_H */
