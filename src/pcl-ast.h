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

/* The following enumeration defines the codes characterizing the
   several types of nodes supported in the PCL abstract syntax
   trees.  */

enum pcl_ast_code
{
  PCL_AST_PROGRAM,
  PCL_AST_EXP,
  PCL_AST_COND_EXP,
  PCL_AST_ENUM,
  PCL_AST_ENUMERATOR,
  PCL_AST_STRUCT,
  PCL_AST_MEM,
  PCL_AST_FIELD,
  PCL_AST_COND,
  PCL_AST_LOOP,
  PCL_AST_ASSERTION,
  PCL_AST_TYPE,
  PCL_AST_ARRAY_REF,
  PCL_AST_STRUCT_REF,
  PCL_AST_INTEGER,
  PCL_AST_STRING,
  PCL_AST_IDENTIFIER,
  PCL_AST_DOC_STRING,
  PCL_AST_LOC
};

/* The AST nodes representing expressions are characterized by
   operators (see below in this file for more details on this.)  The
   following enumeration defines the operator codes.

   The definitions of the operators are in pdl-ops.def.  */

#define PCL_DEF_OP(SYM, STRING) SYM,
enum pcl_ast_op
{
#include "pcl-ops.def"
};
#undef PCL_DEF_OP

/* Certain AST nodes can be characterized of featuring a byte
   endianness.  The following values are supported:

   MSB means that the most significative bytes come first.  This is
   what is popularly known as big-endian.

   LSB means that the least significative bytes come first.  This is
   what is known as little-endian.

   In both endiannesses the bits inside the bytes are ordered from
   most significative to least significative.

   The function `pcl_ast-default_endian' returns the endianness used
   in the system running poke.  */

enum pcl_ast_endian
{
  PCL_AST_MSB, /* Big-endian.  */
  PCL_AST_LSB  /* Little-endian.  */
};

enum pcl_ast_endian pcl_ast_default_endian (void);

/* The AST nodes representing types are characterized by type codes
   (see below in this file for more details on this.)  The following
   enumeration defines the type codes.

   The definitions of the supported types are in pdl-types.def.  */

#define PCL_DEF_TYPE(CODE,ID,SIZE) CODE,
enum pcl_ast_type_code
{
  PCL_TYPE_NOTYPE,
#include "pcl-types.def"
  PCL_TYPE_ENUM,
  PCL_TYPE_STRUCT
};
#undef PCL_DEF_TYPE

/* Next we define the several supported types of nodes in the abstract
   syntax tree, which are discriminated using the codes defined in the
   `pcl_ast_code' enumeration above.

   Accessor macros are defined to access the attributes of the
   different nodes, and should be used as both l-values and r-values
   to inspect and modify nodes, respectively.

   Declarations for constructor functions are also provided, that can
   be used to create new instances of nodes.  */

typedef union pcl_ast_node *pcl_ast_node;

/* The `pcl_ast_common' struct defines fields which are common to
   every node in the AST, regardless of their type.

   CHAIN and CHAIN2 are used to chain AST nodes together.  This serves
   several purposes in the compiler:

   CHAIN is used to form sibling relationships in the tree.

   CHAIN2 is used to link nodes together in containers, such as hash
   table buckets.

   The `pcl_ast_chainon' utility function is provided in order to
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

#define PCL_AST_CHAIN(AST) ((AST)->common.chain)
#define PCL_AST_CHAIN2(AST) ((AST)->common.chain2)
#define PCL_AST_CODE(AST) ((AST)->common.code)
#define PCL_AST_LITERAL_P(AST) ((AST)->common.literal_p)
#define PCL_AST_REGISTERED_P(AST) ((AST)->common.registered_p)
#define PCL_AST_REFCOUNT(AST) ((AST)->common.refcount)

#define ASTREF(AST) ((AST) ? (++((AST)->common.refcount), (AST)) \
                     : NULL)

struct pcl_ast_common
{
  union pcl_ast_node *chain;
  union pcl_ast_node *chain2;
  enum pcl_ast_code code : 8;
  int refcount;

  unsigned literal_p : 1;
};

pcl_ast_node pcl_ast_chainon (pcl_ast_node ast1,
                              pcl_ast_node ast2);

/* PCL_AST_PROGRAM nodes represent PCL programs.

   DECLARATIONS points to a possibly empty list of struct definitions
   and enumerations, i.e. to nodes of types PCL_AST_STRUCT and
   PCL_AST_ENUM respectively.  */

#define PCL_AST_PROGRAM_DECLARATIONS(AST) ((AST)->program.declarations)

struct pcl_ast_program
{
  struct pcl_ast_common common;
  union pcl_ast_node *declarations;
};

pcl_ast_node pcl_ast_make_program (pcl_ast_node declarations);

/* PCL_AST_IDENTIFIER nodes represent identifiers in PCL programs.
   
   POINTER must point to a NULL-terminated string.
   LENGTH contains the size in bytes of the identifier.  */

#define PCL_AST_IDENTIFIER_LENGTH(AST) ((AST)->identifier.length)
#define PCL_AST_IDENTIFIER_POINTER(AST) ((AST)->identifier.pointer)

struct pcl_ast_identifier
{
  struct pcl_ast_common common;
  size_t length;
  char *pointer;
};

pcl_ast_node pcl_ast_make_identifier (const char *str);

/* PCL_AST_INTEGER nodes represent integer numeric literals in PCL
   programs.

   VALUE contains a 64-bit unsigned integer.  */

#define PCL_AST_INTEGER_VALUE(AST) ((AST)->integer.value)

struct pcl_ast_integer
{
  struct pcl_ast_common common;
  uint64_t value;
};

pcl_ast_node pcl_ast_make_integer (uint64_t value);

/* PCL_AST_STRING nodes represent string literals in PCL programs.

   POINTER must point to a NULL-terminated string.
   LENGTH contains the size in bytes of the string.  */

#define PCL_AST_STRING_LENGTH(AST) ((AST)->string.length)
#define PCL_AST_STRING_POINTER(AST) ((AST)->string.pointer)

struct pcl_ast_string
{
  struct pcl_ast_common common;
  size_t length;
  char *pointer;
};

pcl_ast_node pcl_ast_make_string (const char *str);

/* PCL_AST_DOC_STRING nodes represent text that explains the meaning
   or the contents of other nodes/entities.

   POINTER must point to a NULL-terminated string.
   LENGTH contains the size in bytes of the doc string.  */

#define PCL_AST_DOC_STRING_LENGTH(AST) ((AST)->doc_string.length)
#define PCL_AST_DOC_STRING_POINTER(AST) ((AST)->doc_string.pointer)

struct pcl_ast_doc_string
{
  struct pcl_ast_common common;
  size_t length;
  char *pointer;
};

pcl_ast_node pcl_ast_make_doc_string (const char *str,
                                      pcl_ast_node entity);

/* PCL_AST_EXP nodes represent unary and binary expressions,
   consisting on an operator and one or two operators, respectively.

   The supported operators are specified in pcl-ops.def.

   We defined two constructors for this node type: one for unary
   expressions and another for binary expressions.  */

#define PCL_AST_EXP_CODE(AST) ((AST)->exp.code)
#define PCL_AST_EXP_NUMOPS(AST) ((AST)->exp.numops)
#define PCL_AST_EXP_OPERAND(AST, I) ((AST)->exp.operands[(I)])

struct pcl_ast_exp
{
  struct pcl_ast_common common;
  enum pcl_ast_op code;
  uint8_t numops : 8;
  union pcl_ast_node *operands[2];
};

pcl_ast_node pcl_ast_make_unary_exp (enum pcl_ast_op code,
                                     pcl_ast_node op);
pcl_ast_node pcl_ast_make_binary_exp (enum pcl_ast_op code,
                                      pcl_ast_node op1,
                                      pcl_ast_node op2);

/* PCL_AST_COND_EXP nodes represent conditional expressions, having
   exactly the same semantics than the C tertiary operator:

                   exp1 ? exp2 : exp3

   where exp1 is evaluated and then, depending on its value, either
   exp2 (if exp1 is not 0) or exp3 (if exp1 is 0) are executed.

   COND, THENEXP and ELSEEXP must point to expressions, i.e. to nodes
   node of type  PCL_AST_EXP or type PCL_AST_COND_EXP.  */

#define PCL_AST_COND_EXP_COND(AST) ((AST)->cond_exp.cond)
#define PCL_AST_COND_EXP_THENEXP(AST) ((AST)->cond_exp.thenexp)
#define PCL_AST_COND_EXP_ELSEEXP(AST) ((AST)->cond_exp.elseexp)

struct pcl_ast_cond_exp
{
  struct pcl_ast_common common;
  union pcl_ast_node *cond;
  union pcl_ast_node *thenexp;
  union pcl_ast_node *elseexp;
};

pcl_ast_node pcl_ast_make_cond_exp (pcl_ast_node cond,
                                    pcl_ast_node thenexp,
                                    pcl_ast_node elseexp);

/* PCL_AST_ENUMERATOR nodes represent the definition of a constant
   into an enumeration.

   Each constant is characterized with an IDENTIFIER that identifies
   it globally (meaning enumerator identifiers must be unique), an
   optional VALUE, which must be a constant expression, and an
   optional doc-string.

   If the value is not explicitly provided, it must be calculated
   considering the rest of the enumerators in the enumeration, exactly
   like in C enums.  */

#define PCL_AST_ENUMERATOR_IDENTIFIER(AST) ((AST)->enumerator.identifier)
#define PCL_AST_ENUMERATOR_VALUE(AST) ((AST)->enumerator.value)
#define PCL_AST_ENUMERATOR_DOCSTR(AST) ((AST)->enumerator.docstr)

struct pcl_ast_enumerator
{
  struct pcl_ast_common common;
  union pcl_ast_node *identifier;
  union pcl_ast_node *value;
  union pcl_ast_node *docstr;
};

pcl_ast_node pcl_ast_make_enumerator (pcl_ast_node identifier,
                                      pcl_ast_node value,
                                      pcl_ast_node docstr);

/* PCL_AST_ENUM nodes represent enumerations, having semantics much
   like the C enums.

   TAG is mandatory and must point to a PCL_AST_IDENTIFIER.  This
   identifier characterizes the enumeration globally in the enums
   namespace.

   VALUES must point to a chain of PCL_AST_ENUMERATOR nodes containing
   at least one node.  This means empty enumerations are not allowed.

   DOCSTR optionally points to a PCL_AST_DOCSTR.  If it exists, it
   contains text explaining the meaning of the collection of constants
   defined in the enumeration.  */

#define PCL_AST_ENUM_TAG(AST) ((AST)->enumeration.tag)
#define PCL_AST_ENUM_VALUES(AST) ((AST)->enumeration.values)
#define PCL_AST_ENUM_DOCSTR(AST) ((AST)->enumeration.docstr)

struct pcl_ast_enum
{
  struct pcl_ast_common common;
  union pcl_ast_node *tag;
  union pcl_ast_node *values;
  union pcl_ast_node *docstr;
};

pcl_ast_node pcl_ast_make_enum (pcl_ast_node tag,
                                pcl_ast_node values,
                                pcl_ast_node docstr);


/* PCL_AST_STRUCT nodes represent PCL structs, which are similar to C
   structs... but not quite the same thing!

   In PCL a struct is basically the declaration of a named memory
   layout (see below for more info on memory layouts.)  Structs can
   only be declared at the top-level.
   
   TAG must point to a node of type PCL_AST_IDENTIFIER, and globally
   identifies the struct in the strucs namespace.

   MEM must point to a node of type PCL_AST_MEM, that contains the
   memory layout named in this struct.

   DOCSTR optionally points to a docstring that documents the
   meaning/contents of the struct.  */


#define PCL_AST_STRUCT_TAG(AST) ((AST)->strct.tag)
#define PCL_AST_STRUCT_DOCSTR(AST) ((AST)->strct.docstr)
#define PCL_AST_STRUCT_MEM(AST) ((AST)->strct.mem)

struct pcl_ast_struct
{
  struct pcl_ast_common common;
  union pcl_ast_node *tag;
  union pcl_ast_node *docstr;
  union pcl_ast_node *mem;
};

pcl_ast_node pcl_ast_make_struct (pcl_ast_node tag,
                                  pcl_ast_node docstr,
                                  pcl_ast_node mem);

/* PCL_AST_MEM nodes represent memory layouts.  The layouts are
   described by a program that, once executed, describes a collection
   of subareas over a possibly non-contiguous memory area.

   ENDIAN is the endianness used by the components (filds) of the
   memory layout.

   COMPONENTS points to a possibly empty list of other layouts,
   fields, conditionals, loops and assertions, i.e. of nodes of types
   PCL_AST_MEM, PCL_AST_FIELD, PCL_AST_COND, PCL_AST_LOOP and
   PCL_AST_ASSERTION respectively.  */

#define PCL_AST_MEM_ENDIAN(AST) ((AST)->mem.endian)
#define PCL_AST_MEM_COMPONENTS(AST) ((AST)->mem.components)

struct pcl_ast_mem
{
  struct pcl_ast_common common;
  enum pcl_ast_endian endian;
  union pcl_ast_node *components;
};

pcl_ast_node pcl_ast_make_mem (enum pcl_ast_endian endian,
                               pcl_ast_node components);

/* PCL_AST_FIELD nodes represent fields in memory layouts.

   ENDIAN is the endianness in which the data in the field is stored.

   NAME must point to an identifier which should be unique within the
   containing struct (i.e. within the containing top-level memory
   layout.)
   
   TYPE must point to a PCL_AST_TYPE node that describes the value
   stored in the field.

   NUM_ENTS must point to an expression specifiying how many entities
   of size SIZE are stored in the field.

   SIZE is the size in bits of each element stored in the field.

   DOCSTR optionally points to a PCL_AST_DOC_STRING node that
   documents the purpose/meaning of the field contents.  */

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
  union pcl_ast_node *name;
  union pcl_ast_node *type;
  union pcl_ast_node *docstr;
  union pcl_ast_node *num_ents;
  union pcl_ast_node *size;
};

pcl_ast_node pcl_ast_make_field (pcl_ast_node name,
                                 pcl_ast_node type,
                                 pcl_ast_node docstr,
                                 enum pcl_ast_endian endian,
                                 pcl_ast_node num_ents,
                                 pcl_ast_node size);

/* PCL_AST_COND nodes represent conditionals.

   A conditional allows to conditionally define memory layouts, in a
   very similar way the if-then-else statements determine the control
   flow in conventional programming languages.

   EXP must point to an expression whose result is interpreted as a
   boolean in a C-style, 0 meaning false and any other value meaning
   true.

   THENPART must point to a PCL_AST_MEM node, which is the memory
   layout that gets defined should EXP evaluate to true.

   ELSEPART optionally points to a PCL_AST_MEM node, which is the
   memory layout that gets defined should EXP evaluate to false.  */

#define PCL_AST_COND_EXP(AST) ((AST)->cond.exp)
#define PCL_AST_COND_THENPART(AST) ((AST)->cond.thenpart)
#define PCL_AST_COND_ELSEPART(AST) ((AST)->cond.elsepart)

struct pcl_ast_cond
{
  struct pcl_ast_common common;
  union pcl_ast_node *exp;
  union pcl_ast_node *thenpart;
  union pcl_ast_node *elsepart;
};

pcl_ast_node pcl_ast_make_cond (pcl_ast_node exp,
                                pcl_ast_node thenpart,
                                pcl_ast_node elsepart);

/* PCL_AST_LOOP nodes represent loops.

   A loop allows to define multiple memory layouts, in a very similar
   way iterative statements work in conventional programming
   languages.

   PRE optionally points to an expression which is evaluated before
   entering the loop.

   COND optionally points to a condition that determines whether the
   loop is entered initially, and afer each iteration.

   BODY must point to a PCL_AST_MEM node, which is the memory layout
   that is defined in each loop iteration.
   
   POST optionally points to an expression which is evaluated after
   defining the memory layout in each iteration.

   Note that if COND is not defined, the effect is an infinite loop.  */

#define PCL_AST_LOOP_PRE(AST) ((AST)->loop.pre)
#define PCL_AST_LOOP_COND(AST) ((AST)->loop.cond)
#define PCL_AST_LOOP_POST(AST) ((AST)->loop.post)
#define PCL_AST_LOOP_BODY(AST) ((AST)->loop.body)

struct pcl_ast_loop
{
  struct pcl_ast_common common;
  union pcl_ast_node *pre;
  union pcl_ast_node *cond;
  union pcl_ast_node *post;
  union pcl_ast_node *body;
};

pcl_ast_node pcl_ast_make_loop (pcl_ast_node pre,
                                pcl_ast_node cond,
                                pcl_ast_node post,
                                pcl_ast_node body);

/* PCL_AST_ARRAY_REF nodes represent references to elements stored in
   fields.

   BASE must point to a PCL_AST_IDENTIFIER node, which in turn should
   identify a field.

   INDEX must point to an expression whose evaluation is the offset of
   the element into the field, in units of the field's SIZE.  */

#define PCL_AST_ARRAY_REF_BASE(AST) ((AST)->aref.base)
#define PCL_AST_ARRAY_REF_INDEX(AST) ((AST)->aref.index)

struct pcl_ast_array_ref
{
  struct pcl_ast_common common;
  union pcl_ast_node *base;
  union pcl_ast_node *index;
};

pcl_ast_node pcl_ast_make_array_ref (pcl_ast_node base,
                                     pcl_ast_node index);

/* PCL_AST_STRUCT_REF nodes represent references to fields within a
   struct.

   BASE must point to a PCL_AST_IDENTIFIER node, which in turn should
   identify a field whose type is a struct.

   IDENTIFIER must point to a PCL_AST_IDENTIFIER node, which in turn
   should identify a field defined in the struct characterized by
   BASE.  */

#define PCL_AST_STRUCT_REF_BASE(AST) ((AST)->sref.base)
#define PCL_AST_STRUCT_REF_IDENTIFIER(AST) ((AST)->sref.identifier)

struct pcl_ast_struct_ref
{
  struct pcl_ast_common common;
  union pcl_ast_node *base;
  union pcl_ast_node *identifier;
};

pcl_ast_node pcl_ast_make_struct_ref (pcl_ast_node base,
                                      pcl_ast_node identifier);

/* PCL_AST_TYPE nodes represent field types.
   
   CODE contains the kind of type, as defined in the pcl_ast_type_code
   enumeration above.

   SIGNED is 1 if the type denotes a signed numeric type.

   SIZE is the witdh in bits of type.

   ENUMERATION must point to a PCL_AST_ENUM node if the type code is
   PCL_TYPE_ENUM.

   STRUCT must point to a PCL_AST_STRUCT node if the type code is
   PCL_TYPE_STRUCT.  */

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
  union pcl_ast_node *enumeration;
  union pcl_ast_node *strt;
};

pcl_ast_node pcl_ast_make_type (enum pcl_ast_type_code code,
                                int signed_p, size_t size,
                                pcl_ast_node enumeration,
                                pcl_ast_node strct);

/* PCL_AST_LOC nodes represent the current struct's location
   counter.

   This node can occur anywhere in an expression, inluding both sides
   of assignment operators.  */

struct pcl_ast_loc
{
  struct pcl_ast_common common;
};

pcl_ast_node pcl_ast_make_loc (void);

/* PCL_AST_ASSERTION nodes represent checks that can occur anywhere
   within a memory layout.

   EXP must point to an expression.  If the expression evaluates to 1
   a fatal error is raised at execution time.  */

#define PCL_AST_ASSERTION_EXP(AST) ((AST)->assertion.exp)

struct pcl_ast_assertion
{
  struct pcl_ast_common common;
  union pcl_ast_node *exp;
};

pcl_ast_node pcl_ast_make_assertion (pcl_ast_node exp);

/* Finally, the `pcl_ast_node' type, which represents an AST node of
   any type.  */

union pcl_ast_node
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
  struct pcl_ast_loc loc;
};

/* The `pcl_ast' struct defined below contains a PCL abstract syntax tree.

   AST contains the tree of linked nodes, starting with a
   PCL_AST_PROGRAM node.

   Some of the tree nodes can be stored in the several hash tables,
   which are created during parsing.

   `pcl_ast_init' allocates and initializes a new AST and returns a
   pointer to it.

   `pcl_ast_free' frees all the memory allocated to store the AST
   nodes and also the hash tables.

   `pcl_ast_node_free' frees the memory allocated to store a single
   node in the AST and its descendants.  This function is used by the
   bison parser.  */

#define HASH_TABLE_SIZE 1008
typedef pcl_ast_node pcl_hash[HASH_TABLE_SIZE];

struct pcl_ast
{
  pcl_ast_node ast;

  pcl_hash ids_hash_table;
  pcl_hash types_hash_table;
  pcl_hash enums_hash_table;
  pcl_hash structs_hash_table;
};

typedef struct pcl_ast *pcl_ast;

pcl_ast pcl_ast_init (void);
void pcl_ast_free (pcl_ast ast);
void pcl_ast_node_free (pcl_ast_node ast);

/* The following three functions are used by the lexer and the parser
   in order to populate/inquiry the hash tables in the AST.  */

pcl_ast_node pcl_ast_get_identifier (pcl_ast ast,
                                     const char *str);

pcl_ast_node pcl_ast_get_registered (pcl_ast ast,
                                     const char *name,
                                     enum pcl_ast_code code);

pcl_ast_node pcl_ast_register (pcl_ast ast,
                               const char *name,
                               pcl_ast_node ast_node);

#ifdef PCL_DEBUG

/* The following function dumps a human-readable description of the
   tree headed by the node AST.  It is used for debugging
   purposes.  */

void pcl_ast_print (FILE *fd, pcl_ast_node ast);

#endif

#endif /* ! PCL_AST_H */
