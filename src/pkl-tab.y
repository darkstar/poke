/* pkl-tab.y - LARL(1) parser for Poke.  */

/* Copyright (C) 2017 Jose E. Marchesi.  */

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

%define api.pure full
%define parse.lac full
%define parse.error verbose
%locations
%name-prefix "pkl_tab_"

%lex-param {void *scanner}
%parse-param {struct pkl_parser *pkl_parser}


%{
#include <config.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <xalloc.h>
#include <assert.h>

#include "pkl-ast.h"
#include "pkl-parser.h" /* For struct pkl_parser.  */
#define YYDEBUG 1
#include "pkl-tab.h"
#include "pkl-lex.h"

#ifdef PKL_DEBUG
# include "pkl-gen.h"
#endif

#define scanner (pkl_parser->scanner)
  
/* YYLLOC_DEFAULT -> default code for computing locations.  */
  
#define PKL_AST_CHILDREN_STEP 12

/* Error reporting function for pkl_tab_parse.

   When this function returns, the parser tries to recover the error.
   If it is unable to recover, then it returns with 1 (syntax error)
   immediately.  */
  
void
pkl_tab_error (YYLTYPE *llocp,
               struct pkl_parser *pkl_parser,
               char const *err)
{
  // XXX
  //  if (YYRECOVERING ())
  //    return;
  // XXX: store the line read and other info for pretty
  //      error printing in EXTRA.
  //  if (!pkl_parser->interactive)
    fprintf (stderr, "%s: %d: %s\n", pkl_parser->filename,
             llocp->first_line, err);
}

#if 0

/* The following functions are used in the actions in several grammar
   rules below.  This is to avoid replicating code in situations where
   the difference between rules are just a different permutation of
   its elements.

   All these functions return 1 if the action is executed
   successfully, or 0 if a syntax error should be raised at the
   grammar rule invoking the function.  */

static int
enum_specifier_action (struct pkl_parser *pkl_parser,
                       pkl_ast_node *enumeration,
                       pkl_ast_node tag, YYLTYPE *loc_tag,
                       pkl_ast_node enumerators, YYLTYPE *loc_enumerators,
                       pkl_ast_node docstr, YYLTYPE *loc_docstr)
{
  *enumeration = pkl_ast_make_enum (tag, enumerators, docstr);

  if (pkl_ast_register (pkl_parser->ast,
                        PKL_AST_IDENTIFIER_POINTER (tag),
                        *enumeration) == NULL)
    {
      pkl_tab_error (loc_tag, pkl_parser, "enum already defined");
      return 0;
    }

  return 1;
}

static int
struct_specifier_action (struct pkl_parser *pkl_parser,
                         pkl_ast_node *strct,
                         pkl_ast_node tag, YYLTYPE *loc_tag,
                         pkl_ast_node docstr, YYLTYPE *loc_docstr,
                         pkl_ast_node mem, YYLTYPE *loc_mem)
{
  *strct = pkl_ast_make_struct (tag, docstr, mem);

  if (pkl_ast_register (pkl_parser->ast,
                        PKL_AST_IDENTIFIER_POINTER (tag),
                        *strct) == NULL)
    {
      pkl_tab_error (loc_tag, pkl_parser, "struct already defined");
      return 0;
    }

  return 1;
}
#endif

static int
check_operand_unary (pkl_ast ast,
                     enum pkl_ast_op opcode,
                     pkl_ast_node *a)
{
  /* All unary operators require an integer as an argument.  */
  if (PKL_AST_TYPE_CODE (PKL_AST_TYPE (*a)) != PKL_TYPE_INTEGRAL)
    return 0;

  return 1;
}

static int
promote_to_integral (size_t size, int sign,
                     pkl_ast ast, pkl_ast_node *a)
{
  if (PKL_AST_TYPE_CODE (PKL_AST_TYPE (*a)) == PKL_TYPE_INTEGRAL)
    {
      if (!(PKL_AST_TYPE_I_SIZE (*a) == size
            && PKL_AST_TYPE_I_SIGNED (*a) == sign))
        {
          pkl_ast_node desired_type
            = pkl_ast_get_integral_type (ast, size, sign);
          *a = pkl_ast_make_unary_exp (PKL_AST_OP_CAST, *a);
          PKL_AST_TYPE (*a) = ASTREF (desired_type);
        }

      return 1;
    }

  return 0;

}

static int
promote_to_bool (pkl_ast ast, pkl_ast_node *a)
{
  return promote_to_integral (32, 1, ast, a);
}

static int
promote_to_ulong (pkl_ast ast, pkl_ast_node *a)
{
  return promote_to_integral (64, 0, ast, a);
}
 
static int
promote_operands_binary (pkl_ast ast,
                         pkl_ast_node *a,
                         pkl_ast_node *b,
                         int allow_strings,
                         int allow_arrays,
                         int allow_tuples)
{
  pkl_ast_node *to_promote_a = NULL;
  pkl_ast_node *to_promote_b = NULL;
  pkl_ast_node ta = PKL_AST_TYPE (*a);
  pkl_ast_node tb = PKL_AST_TYPE (*b);
  size_t size_a;
  size_t size_b;
  int sign_a;
  int sign_b;

  /* Both arguments should be either integrals, strings, arrays or
     tuples.  */

  if (PKL_AST_TYPE_CODE (ta) != PKL_AST_TYPE_CODE (tb))
    return 0;

  if ((!allow_strings && PKL_AST_TYPE_CODE (ta) == PKL_TYPE_STRING)
      || (!allow_arrays && PKL_AST_TYPE_CODE (ta) == PKL_TYPE_ARRAY)
      || (!allow_tuples && PKL_AST_TYPE_CODE (ta) == PKL_TYPE_TUPLE))
    return 0;


  if (!(PKL_AST_TYPE_CODE (ta) == PKL_TYPE_INTEGRAL))
    /* No need to promote non-integral types.  */
    return 1;

  /* Handle promotion of integral operands.  The rules are:

     - If one operand is narrower than the other, it is promoted to
       have the same width.  

     - If one operand is unsigned and the other signed, the signed
       operand is promoted to unsigned.  */

  size_a = PKL_AST_TYPE_I_SIZE (ta);
  size_b = PKL_AST_TYPE_I_SIZE (tb);
  sign_a = PKL_AST_TYPE_I_SIGNED (ta);
  sign_b = PKL_AST_TYPE_I_SIGNED (tb);

  if (size_a > size_b)
    {
      size_b = size_a;
      to_promote_b = b;
    }
  else if (size_a < size_b)
    {
      size_a = size_b;
      to_promote_a = a;
    }

  if (sign_a == 0 && sign_b == 1)
    {
      sign_b = 0;
      to_promote_b = b;
    }
  else if (sign_a == 1 && sign_b == 0)
    {
      sign_a = 0;
      to_promote_a = b;
    }

  if (to_promote_a != NULL)
    {
      pkl_ast_node t
        = pkl_ast_get_integral_type (ast, size_a, sign_a);
      *a = pkl_ast_make_unary_exp (PKL_AST_OP_CAST, *a);
      PKL_AST_TYPE (*a) = ASTREF (t);
    }

  if (to_promote_b != NULL)
    {
      pkl_ast_node t
        = pkl_ast_get_integral_type (ast, size_b, sign_b);
      *b = pkl_ast_make_unary_exp (PKL_AST_OP_CAST, *b);
      PKL_AST_TYPE (*b) = ASTREF (t);
    }

  return 1;
}

static int
check_tuple (struct pkl_parser *parser,
             YYLTYPE *llocp,
             pkl_ast_node elems,
             size_t *nelem,
             pkl_ast_node *type)
{
  pkl_ast_node t, u;
  pkl_ast_node tuple_type_elems;

  tuple_type_elems = NULL;
  *nelem = 0;
  for (t = elems; t; t = PKL_AST_CHAIN (t))
    {
      pkl_ast_node name;
      pkl_ast_node tuple_type_elem;
      pkl_ast_node ename;
      pkl_ast_node type;

      /* Add the name for this tuple element.  */
      name = PKL_AST_TUPLE_ELEM_NAME (t);
      if (name)
        {
          assert (PKL_AST_CODE (name) == PKL_AST_IDENTIFIER);
          for (u = elems; u != t; u = PKL_AST_CHAIN (u))
            {
              pkl_ast_node uname
                = PKL_AST_TUPLE_ELEM_NAME (u);

              if (uname == NULL)
                continue;
              
              if (strcmp (PKL_AST_IDENTIFIER_POINTER (name),
                          PKL_AST_IDENTIFIER_POINTER (uname)) == 0)
                {
                  pkl_tab_error (llocp, parser,
                                 "duplicated element name in tuple.");
                  return 0;
                }
            }

          ename = pkl_ast_make_identifier (PKL_AST_IDENTIFIER_POINTER (name));
        }
      else
        ename = pkl_ast_make_identifier ("");

      type = PKL_AST_TYPE (PKL_AST_TUPLE_ELEM_EXP (t));

      tuple_type_elem = pkl_ast_make_tuple_type_elem (ename, pkl_ast_dup_type (type));
      tuple_type_elems = pkl_ast_chainon (tuple_type_elems,
                                          tuple_type_elem);
      *nelem += 1;
    }

  /* Now build the type for the tuple.  */
  *type = pkl_ast_make_tuple_type (*nelem, tuple_type_elems);

  return 1;
}

static int
check_tuple_type (struct pkl_parser *parser,
                  YYLTYPE *llocp,
                  pkl_ast_node tuple_type_elems,
                  size_t *nelem)
{
  pkl_ast_node t, u;

  *nelem = 0;
  for (t = tuple_type_elems; t; t = PKL_AST_CHAIN (t))
    {
      for (u = tuple_type_elems; u != t; u = PKL_AST_CHAIN (u))
        {
          pkl_ast_node tname = PKL_AST_TUPLE_TYPE_ELEM_NAME (u);
          pkl_ast_node uname = PKL_AST_TUPLE_TYPE_ELEM_NAME (t);

          if (uname
              && tname
              && strcmp (PKL_AST_IDENTIFIER_POINTER (uname),
                         PKL_AST_IDENTIFIER_POINTER (tname)) == 0)
            {
              pkl_tab_error (llocp, parser,
                             "duplicated element name in tuple type spec.");
              return 0;
            }
        }
      
      *nelem += 1;
    }

  return 1;
}

static int
check_tuple_ref (struct pkl_parser *parser,
                 YYLTYPE *llocp,
                 pkl_ast_node ttype,
                 pkl_ast_node identifier,
                 pkl_ast_node *type)
{
  pkl_ast_node e;

  assert (PKL_AST_TYPE_CODE (ttype) == PKL_TYPE_TUPLE);

  *type = NULL;
  for (e = PKL_AST_TYPE_T_ELEMS (ttype); e; e = PKL_AST_CHAIN (e))
    {
      if (strcmp (PKL_AST_IDENTIFIER_POINTER (PKL_AST_TUPLE_TYPE_ELEM_NAME (e)),
                  PKL_AST_IDENTIFIER_POINTER (identifier)) == 0)
        {
          *type = PKL_AST_TUPLE_TYPE_ELEM_TYPE (e);
          break;
        }
    }

  if (*type == NULL)
    {
      pkl_tab_error (llocp, parser,
                     "invalid tuple member");
      return 0;
    }
  
  return 1;
}


static int
check_array (struct pkl_parser *parser,
             YYLTYPE *llocp,
             pkl_ast_node elems,
             pkl_ast_node *type,
             size_t *nelem)
{
  pkl_ast_node t, array_nelem;
  size_t index;

  *type = NULL;
  *nelem = 0;

  for (index = 0, t = elems; t; t = PKL_AST_CHAIN (t))
    {
      pkl_ast_node elem = PKL_AST_ARRAY_ELEM_EXP (t);
      size_t elem_index = PKL_AST_ARRAY_ELEM_INDEX (t);
      size_t elems_appended, effective_index;
      
      /* First check the type of the element.  */
      assert (PKL_AST_TYPE (elem));
      if (*type == NULL)
        *type = PKL_AST_TYPE (elem);
      else if (!pkl_ast_type_equal (PKL_AST_TYPE (elem), *type))
        {
          pkl_tab_error (llocp, parser,
                         "array element is of the wrong type.");
          return 0;
        }        
      
      /* Then set the index of the element.  */
      if (elem_index == PKL_AST_ARRAY_NOINDEX)
        {
          effective_index = index;
          elems_appended = 1;
        }
      else
        {
          if (elem_index < index)
            elems_appended = 0;
          else
            elems_appended = elem_index - index + 1;
          effective_index = elem_index;
        }
      
      PKL_AST_ARRAY_ELEM_INDEX (t) = effective_index;
      index += elems_appended;
      *nelem += elems_appended;
    }

  /* Finally, set the type of the array itself.  */
  array_nelem = pkl_ast_make_integer (*nelem);
  PKL_AST_TYPE (array_nelem) = pkl_ast_get_integral_type (parser->ast,
                                                          64, 0);
  *type = pkl_ast_make_array_type (array_nelem, *type);

  return 1;
}

%}

%union {
  pkl_ast_node ast;
  enum pkl_ast_op opcode;
  int integer;
}

%destructor { $$ = ASTREF ($$); pkl_ast_node_free ($$); } <ast>

/* Primaries.  */

%token <ast> INTEGER
%token <ast> CHAR
%token <ast> STR
%token <ast> IDENTIFIER
%token <ast> TYPENAME

/* Reserved words.  */

%token ENUM
%token STRUCT
%token TYPEDEF
%token BREAK
%token CONST
%token CONTINUE
%token ELSE
%token FOR
%token WHILE
%token IF
%token SIZEOF
%token TYPEOF
%token ELEMSOF
%token ASSERT
%token ERR

/* Operator tokens and their precedences, in ascending order.  */

%right '?' ':'
%left OR
%left AND
%left '|'
%left '^'
%left '&'
%left EQ NE
%left LE GE
%left SL SR
%left '+' '-'
%left '*' '/' '%'
%right UNARY INC DEC
%left '@'
%left HYPERUNARY
%left '.'

%token <opcode> MULA
%token <opcode> DIVA
%token <opcode> MODA
%token <opcode> ADDA
%token <opcode> SUBA
%token <opcode> SLA
%token <opcode> SRA
%token <opcode> BANDA
%token <opcode> XORA
%token <opcode> IORA

%token MSB LSB
%token SIGNED UNSIGNED

%type <opcode> unary_operator

%type <ast> program program_elem_list program_elem
%type <ast> expression primary
%type <ast> array_elem_list array_elem
%type <ast> tuple_elem_list tuple_elem
%type <ast> type_specifier
%type <ast> tuple_type_specifier tuple_elem_type_list tuple_elem_type

%start program

%% /* The grammar follows.  */

program: program_elem_list
          	{
                  $$ = pkl_ast_make_program ($1);
                  pkl_parser->ast->ast = ASTREF ($$);
                }
        ;

program_elem_list:
          %empty
		{
                  if (pkl_parser->what == PKL_PARSE_EXPRESSION)
                    /* We should parse exactly one expression.  */
                    YYERROR;
                  $$ = NULL;
                }
	| program_elem
        | program_elem_list program_elem
        	{
                  if (pkl_parser->what == PKL_PARSE_EXPRESSION)
                    /* We should parse exactly one expression.  */
                    YYERROR;
                  $$ = pkl_ast_chainon ($1, $2);
                }
	;

program_elem:
          expression
        	{
                  if (pkl_parser->what != PKL_PARSE_EXPRESSION)
                    /* Expressions are not valid top-level structures
                       in full poke programs.  */
                    YYERROR;
                  $$ = $1;
                }
	| expression ','
        	{
                  if (pkl_parser->what != PKL_PARSE_EXPRESSION)
                    /* Expressions are not valid top-level structures
                       in full poke programs.  */
                    YYERROR;
                  $$ = pkl_ast_make_program ($1);
                  pkl_parser->ast->ast = ASTREF ($$);
                  YYACCEPT;
                }
/*	  declaration
          	{
                  if (pkl_parser->what == PKL_PARSE_EXPRESSION)
                    YYERROR;
                  $$ = $1;
                  }*/
        ;

/*
 * Expressions.
 */

expression:
	  primary
        | unary_operator expression %prec UNARY
          	{
                  if (($1 == PKL_AST_OP_NOT
                       && !promote_to_bool (pkl_parser->ast, &$2))
                      || (!check_operand_unary (pkl_parser->ast, $1, &$2)))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operand to unary operator.");
                        YYERROR;
                    }
                  $$ = pkl_ast_make_unary_exp ($1, $2);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($2));
                }
        | '(' expression ')' expression %prec UNARY
        	{
                  if (PKL_AST_CODE ($2) != PKL_AST_TYPE)
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "expected type in cast.");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_unary_exp (PKL_AST_OP_CAST, $4);
                  PKL_AST_TYPE ($$) = ASTREF ($2);
                }
        | tuple_type_specifier expression %prec UNARY
        	{
                  $$ = pkl_ast_make_unary_exp (PKL_AST_OP_CAST, $2);
                  PKL_AST_TYPE ($$) = ASTREF ($1);
                }
        | TYPEOF expression %prec UNARY
        	{
                  pkl_ast_node metatype;
                  
                  if (PKL_AST_TYPE_TYPEOF (PKL_AST_TYPE ($2)) > 0)
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "operand to typeof can't be a type.");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_unary_exp (PKL_AST_OP_TYPEOF, $2);
                  metatype = pkl_ast_make_metatype (PKL_AST_TYPE ($2));
                  PKL_AST_TYPE ($$) = ASTREF (metatype);
                }
        | SIZEOF expression %prec UNARY
        	{
                  if (PKL_AST_TYPE_TYPEOF (PKL_AST_TYPE ($2)) > 0)
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "operand to sizeof can't be a type.");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_unary_exp (PKL_AST_OP_SIZEOF, $2);
                  PKL_AST_TYPE ($$)
                    = pkl_ast_get_integral_type (pkl_parser->ast,
                                                 64, 0);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($$));
                }
        | ELEMSOF expression %prec UNARY
        	{
                  if (PKL_AST_TYPE_TYPEOF (PKL_AST_TYPE ($2)) > 0)
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "operand to elemsof can't be a type.");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_unary_exp (PKL_AST_OP_ELEMSOF, $2);
                  PKL_AST_TYPE ($$)
                    = pkl_ast_get_integral_type (pkl_parser->ast,
                                                 64, 0);
                }
        | expression '+' expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                1 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to '+'.");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_ADD,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression '-' expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                0 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to '-'.");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_SUB,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression '*' expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                0 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to '*'.");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_MUL,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression '/' expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                0 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to '/'.");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_DIV,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression '%' expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                0 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to '%'.");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_MOD,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression SL expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                0 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to <<");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_SL,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression SR expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                0 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to >>");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_SR,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression EQ expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                1 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to ==");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_EQ,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
	| expression NE expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                1 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to !=");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_NE,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression '<' expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                1 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to <");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_LT,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression '>' expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                1 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to >");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_GT,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression LE expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                1 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to <=");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_LE,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
	| expression GE expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                1 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to >=");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_GE,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression '|' expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                0 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to |");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_IOR,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression '^' expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                0 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to ^");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_XOR,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
	| expression '&' expression
        	{
                  if (!promote_operands_binary (pkl_parser->ast,
                                                &$1, &$3,
                                                0 /* allow_strings */,
                                                0 /* allow_arrays */,
                                                0 /* allow_tuples */))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to &");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_BAND,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression AND expression
        	{
                  if (!promote_to_bool (pkl_parser->ast, &$1)
                      || !promote_to_bool (pkl_parser->ast, &$3))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to &&");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_AND,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
	| expression OR expression
        	{
                  if (!promote_to_bool (pkl_parser->ast, &$1)
                      || !promote_to_bool (pkl_parser->ast, &$3))
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "invalid operators to ||");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_OR,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | expression '@' expression
        	{
                  if (PKL_AST_TYPE_TYPEOF (PKL_AST_TYPE ($1)) == 0)
                    {
                      pkl_tab_error (&@1, pkl_parser,
                                     "expected type in mapping.");
                      YYERROR;
                    }
                  if (!promote_to_ulong (pkl_parser->ast, &$3))
                    {
                      pkl_tab_error (&@3, pkl_parser,
                                     "invalid IO offset in mapping.");
                      YYERROR;
                    }
		  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_MAP,
                                                $1, $3);
                  PKL_AST_TYPE ($$) = ASTREF ($1);
                }
        | expression '?' expression ':' expression
        	{ $$ = pkl_ast_make_cond_exp ($1, $3, $5); }
        ;

unary_operator:
	  '-'		{ $$ = PKL_AST_OP_NEG; }
	| '+'		{ $$ = PKL_AST_OP_POS; }
	| INC		{ $$ = PKL_AST_OP_PREINC; }
	| DEC		{ $$ = PKL_AST_OP_PREDEC; }
	| '~'		{ $$ = PKL_AST_OP_BNOT; }
	| '!'		{ $$ = PKL_AST_OP_NOT; }
	;

primary:
	  INTEGER
        | CHAR
        | STR
        | type_specifier
          	{
                  pkl_ast_node metatype;
                  
                  $$ = $1;
                  metatype = pkl_ast_make_metatype ($1);
                  PKL_AST_TYPE ($$) = ASTREF (metatype);
                }
          /*        | IDENTIFIER */
        | '(' expression ')'
        	{ $$ = $2; }
	| primary INC
        	{
                  $$ = pkl_ast_make_unary_exp (PKL_AST_OP_POSTINC,
                                               $1);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | primary DEC
        	{
                  $$ = pkl_ast_make_unary_exp (PKL_AST_OP_POSTDEC,
                                               $1);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | primary '[' expression ']' %prec '.'
        	{
                  if (PKL_AST_TYPE_CODE (PKL_AST_TYPE ($1))
                      != PKL_TYPE_ARRAY)
                    {
                      pkl_tab_error (&@1, pkl_parser,
                                     "operator to [] must be an array.");
                      YYERROR;
                    }
                  if (!promote_to_ulong (pkl_parser->ast, &$3))
                    {
                      pkl_tab_error (&@1, pkl_parser,
                                     "invalid index in array reference.");
                    }
                  $$ = pkl_ast_make_array_ref ($1, $3);
                  PKL_AST_TYPE ($$) =
                    ASTREF (PKL_AST_TYPE_A_ETYPE (PKL_AST_TYPE ($1)));
                }
        | '[' array_elem_list ']'
        	{
                  pkl_ast_node type;
                  size_t nelem;

                  if (!check_array (pkl_parser,
                                    &@2, $2,
                                    &type, &nelem))
                    YYERROR;
                  
                  $$ = pkl_ast_make_array (nelem, $2);
                  PKL_AST_TYPE ($$) = ASTREF (type);
                }
	| '{' tuple_elem_list '}'
        	{
                  size_t nelem;
                  pkl_ast_node type;

                  if (!check_tuple (pkl_parser,
                                    &@2, $2,
                                    &nelem, &type))
                    YYERROR;

                  $$ = pkl_ast_make_tuple (nelem, $2);
                  PKL_AST_TYPE ($$) = ASTREF (type);
                }
        | primary '.' IDENTIFIER
        	{
                  pkl_ast_node type;
                  
                  if (PKL_AST_TYPE_CODE (PKL_AST_TYPE ($1)) != PKL_TYPE_TUPLE)
                    {
                      pkl_tab_error (&@1, pkl_parser,
                                     "operator to . must be a tuple.");
                      YYERROR;
                    }

                  if (!check_tuple_ref (pkl_parser, &@3,
                                        PKL_AST_TYPE ($1), $3,
                                        &type))
                      YYERROR;
                  
                  $$ = pkl_ast_make_tuple_ref ($1, $3);
                  PKL_AST_TYPE ($$) = ASTREF (type);
                }
	;

tuple_elem_list:
	  %empty
		{ $$ = NULL; }
        | tuple_elem
        | tuple_elem_list ',' tuple_elem
		{                  
                  $$ = pkl_ast_chainon ($1, $3);
                }
        ;

tuple_elem:
	  expression
          	{
                  $$ = pkl_ast_make_tuple_elem (NULL, $1);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | '.' IDENTIFIER '=' expression
	        {
                  $$ = pkl_ast_make_tuple_elem ($2, $4);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($4));
                }
        ;

array_elem_list:
	  array_elem
        | array_elem_list ',' array_elem
          	{
                  $$ = pkl_ast_chainon ($1, $3);
                }
        ;

array_elem:
	  expression
          	{
                  $$ = pkl_ast_make_array_elem (PKL_AST_ARRAY_NOINDEX,
                                                $1);
                  /* Note how array elems do not have a type.  See
                     `check_array' above.  */
                }
        | '.' '[' INTEGER ']' '=' expression
        	{
                  $$ = pkl_ast_make_array_elem (PKL_AST_INTEGER_VALUE ($3),
                                                $6);
                  /* Note how array elems do not have a type.  See
                     `check_array' above.  */
                }
        ;

/*
 * Declarations.
 */

/*
declaration:
	  declaration_specifiers ';'
        ;

declaration_specifiers:
          typedef_specifier
          	| struct_specifier
                | enum_specifier 
        ;

*/

/*
 * Typedefs
 */

/*
typedef_specifier:
	  TYPEDEF type_specifier IDENTIFIER
          	{
                  const char *id = PKL_AST_IDENTIFIER_POINTER ($3);
                  pkl_ast_node type = pkl_ast_make_type (PKL_AST_TYPE_CODE ($2),
                                                         PKL_AST_TYPE_SIGNED ($2),
                                                         PKL_AST_TYPE_SIZE ($2),
                                                         PKL_AST_TYPE_ENUMERATION ($2),
                                                         PKL_AST_TYPE_STRUCT ($2));

                  if (pkl_ast_register (pkl_parser->ast, id, type) == NULL)
                    {
                      pkl_tab_error (&@2, pkl_parser, "type already defined");
                      YYERROR;
                    }

                  $$ = NULL;
                }
        ;
*/

type_specifier:
	  TYPENAME
        | type_specifier '[' expression ']'
          	{
                  if (!promote_to_ulong (pkl_parser->ast, &$3))
                    {
                      pkl_tab_error (&@3, pkl_parser,
                                     "invalid size in array type literal.");
                      YYERROR;
                    }
                  $$ = pkl_ast_make_array_type ($3, $1);
                }
        | tuple_type_specifier
        ;


tuple_type_specifier:
	  '(' tuple_elem_type ',' ')'
          	{
                  $$ = pkl_ast_make_tuple_type (1, $2);
                }
        | '(' tuple_elem_type ',' tuple_elem_type_list ')'
        	{
                  size_t nelem;
                  pkl_ast_node elem_list = pkl_ast_chainon ($4, $2);
                  if (!check_tuple_type (pkl_parser, &@2,
                                         elem_list, &nelem))
                    YYERROR;
                  $$ = pkl_ast_make_tuple_type (nelem, $4);
                }
        ;

tuple_elem_type_list:
	  tuple_elem_type
        | tuple_elem_type_list ',' tuple_elem_type
        	{ $$ = pkl_ast_chainon ($3, $1); }
        ;

tuple_elem_type:
	  type_specifier IDENTIFIER
          	{
                  $$ = pkl_ast_make_tuple_type_elem ($2, $1);
                }
        | type_specifier
        	{
                  $$ = pkl_ast_make_tuple_type_elem (NULL, $1);
                }
        ;

          /*          	| STRUCT IDENTIFIER
        	{
                  pkl_ast_node strct
                    = pkl_ast_get_registered (pkl_parser->ast,
                                              PKL_AST_IDENTIFIER_POINTER ($2),
                                              PKL_AST_STRUCT);

                  if (!strct)
                    {
                      pkl_tab_error (&@2, pkl_parser,
                                     "expected struct tag");
                      YYERROR;
                    }
                  else
                    $$ = pkl_ast_make_type (PKL_TYPE_STRUCT, 0, 0, NULL, strct);
                }
        | ENUM IDENTIFIER
        	{
                  pkl_ast_node enumeration
                    = pkl_ast_get_registered (pkl_parser->ast,
                                              PKL_AST_IDENTIFIER_POINTER ($2),
                                              PKL_AST_ENUM);

                  if (!enumeration)
                    {
                      pkl_tab_error (&@2, pkl_parser, "expected enumeration tag");
                      YYERROR;
                    }
                  else
                    $$ = pkl_ast_make_type (PKL_TYPE_ENUM, 0, 32, enumeration, NULL);
                    }
        ;
*/

/*
 * Enumerations.
 */

/*
enum_specifier:
	  ENUM IDENTIFIER '{' enumerator_list '}'
          	{
                  if (! enum_specifier_action (pkl_parser,
                                               &$$,
                                               $IDENTIFIER, &@IDENTIFIER,
                                               $enumerator_list, &@enumerator_list,
                                               NULL, NULL))
                    YYERROR;
                }
	| ENUM IDENTIFIER DOCSTR '{' enumerator_list '}'
          	{
                  if (! enum_specifier_action (pkl_parser,
                                               &$$,
                                               $IDENTIFIER, &@IDENTIFIER,
                                               $enumerator_list, &@enumerator_list,
                                               $DOCSTR, &@DOCSTR))
                    YYERROR;
                }
        | ENUM DOCSTR IDENTIFIER '{' enumerator_list '}'
          	{
                  if (! enum_specifier_action (pkl_parser,
                                               &$$,
                                               $IDENTIFIER, &@IDENTIFIER,
                                               $enumerator_list, &@enumerator_list,
                                               $DOCSTR, &@DOCSTR))
                    YYERROR;
                }
        | ENUM IDENTIFIER '{' enumerator_list '}' DOCSTR
          	{
                  if (! enum_specifier_action (pkl_parser,
                                               &$$,
                                               $IDENTIFIER, &@IDENTIFIER,
                                               $enumerator_list, &@enumerator_list,
                                               $DOCSTR, &@DOCSTR))
                    YYERROR;
                }
        | DOCSTR ENUM IDENTIFIER '{' enumerator_list '}'
          	{
                  if (! enum_specifier_action (pkl_parser,
                                               &$$,
                                               $IDENTIFIER, &@IDENTIFIER,
                                               $enumerator_list, &@enumerator_list,
                                               $DOCSTR, &@DOCSTR))
                    YYERROR;
                }
        ;

enumerator_list:
	  enumerator
	| enumerator_list ',' enumerator
          	{ $$ = pkl_ast_chainon ($1, $3); }
	;

enumerator:
	  IDENTIFIER
                { $$ = pkl_ast_make_enumerator ($1, NULL, NULL); }
	| IDENTIFIER DOCSTR
                {
                  $$ = pkl_ast_make_enumerator ($1, NULL, $2);
                }
        | IDENTIFIER '=' constant_expression
                { $$ = pkl_ast_make_enumerator ($1, $3, NULL); }
        | IDENTIFIER '=' DOCSTR constant_expression
        	{
                  $$ = pkl_ast_make_enumerator ($1, $4, $3);
                }
        | IDENTIFIER '=' constant_expression DOCSTR
	        {
                  $$ = pkl_ast_make_enumerator ($1, $3, $4);
                }
        | DOCSTR IDENTIFIER '=' constant_expression
        	{
                  $$ = pkl_ast_make_enumerator ($2, $4, $1);
                }
	;

constant_expression:
	  conditional_expression
          	{
                  if (!PKL_AST_LITERAL_P ($1))
                    {
                      pkl_tab_error (&@1, pkl_parser,
                                     "expected constant expression");
                      YYERROR;
                    }

                  $$ = $1;
                }
        ;
*/
/*
 * Structs.
 */

/*
struct_specifier:
	  STRUCT IDENTIFIER mem_layout
			{
                          if (!struct_specifier_action (pkl_parser,
                                                        &$$,
                                                        $IDENTIFIER, &@IDENTIFIER,
                                                        NULL, NULL,
                                                        $mem_layout, &@mem_layout))
                            YYERROR;
                        }
	| DOCSTR STRUCT IDENTIFIER mem_layout
			{
                          if (!struct_specifier_action (pkl_parser,
                                                        &$$,
                                                        $IDENTIFIER, &@IDENTIFIER,
                                                        $DOCSTR, &@DOCSTR,
                                                        $mem_layout, &@mem_layout))
                            YYERROR;
                        }
        | STRUCT IDENTIFIER DOCSTR mem_layout
        	{
                  if (!struct_specifier_action (pkl_parser,
                                                &$$,
                                                $IDENTIFIER, &@IDENTIFIER,
                                                $DOCSTR, &@DOCSTR,
                                                $mem_layout, &@mem_layout))
                    YYERROR;
                }
        | STRUCT IDENTIFIER mem_layout DOCSTR
        		{
                          if (!struct_specifier_action (pkl_parser,
                                                        &$$,
                                                        $IDENTIFIER, &@IDENTIFIER,
                                                        $DOCSTR, &@DOCSTR,
                                                        $mem_layout, &@mem_layout))
                            YYERROR;
                        }
	;
*/
/*
 * Memory layouts.
 */

/*
mem_layout:
	  mem_endianness '{' mem_declarator_list '}'
          			{ $$ = pkl_ast_make_mem ($1, $3); }
	| '{' mem_declarator_list '}'
        			{ $$ = pkl_ast_make_mem (pkl_ast_default_endian (),
                                                         $2); }
        ;

mem_endianness:
	  MSB		{ $$ = PKL_AST_MSB; }
	| LSB		{ $$ = PKL_AST_LSB; }
	;

mem_declarator_list:
	  %empty
			{ $$ = NULL; }
	| mem_declarator ';'
        | mem_declarator_list  mem_declarator ';'
		        { $$ = pkl_ast_chainon ($1, $2); }
	;

mem_declarator:
	  mem_layout
        | mem_field_with_docstr
        | mem_cond
        | mem_loop
        | assignment_expression
        | assert
        ;

mem_field_with_docstr:
	  mem_field_with_endian
	| DOCSTR mem_field_with_endian
          		{
                          PKL_AST_FIELD_DOCSTR ($2) = ASTREF ($1);
                          $$ = $2;
                        }
        | mem_field_with_endian DOCSTR
		        {
                          PKL_AST_FIELD_DOCSTR ($1) = ASTREF ($2);
                          $$ = $1;
                        }
        ;

mem_field_with_endian:
	  mem_endianness mem_field_with_size
          		{
                          PKL_AST_FIELD_ENDIAN ($2) = $1;
                          $$ = $2;
                        }
        | mem_field_with_size
	;

mem_field_with_size:
	  mem_field
        | mem_field ':' expression
          		{
                          if (PKL_AST_TYPE_CODE (PKL_AST_FIELD_TYPE ($1))
                              == PKL_TYPE_STRUCT)
                            {
                              pkl_tab_error (&@1, pkl_parser,
                                             "fields of type struct can't have an\
 explicit size");
                              YYERROR;
                            }

                          * Discard the size inferred from the field
                             type and replace it with the
                             field width expression.  *
                          pkl_ast_node_free (PKL_AST_FIELD_SIZE ($1));
                          PKL_AST_FIELD_SIZE ($1) = ASTREF ($3);
                          $$ = $1;
                        }
        ;

mem_field:
	  type_specifier IDENTIFIER
          		{
                          pkl_ast_node one = pkl_ast_make_integer (1);
                          pkl_ast_node size = pkl_ast_make_integer (PKL_AST_TYPE_SIZE ($1));

                          $$ = pkl_ast_make_field ($2, $1, NULL,
                                                   pkl_ast_default_endian (),
                                                   one, size);
                        }
        | type_specifier IDENTIFIER '[' assignment_expression ']'
		        {
                          pkl_ast_node size = pkl_ast_make_integer (PKL_AST_TYPE_SIZE ($1));

                          $$ = pkl_ast_make_field ($2, $1, NULL,
                                                   pkl_ast_default_endian (),
                                                   $4, size);
                        }
	;

mem_cond:
	  IF '(' expression ')' mem_layout
          		{ $$ = pkl_ast_make_cond ($3, $5, NULL); }
        | IF '(' expression ')' mem_layout ELSE mem_layout
        		{ $$ = pkl_ast_make_cond ($3, $5, $7); }
        ;

mem_loop:
	  FOR '(' expression ';' expression ';' expression ')' mem_layout
          		{ $$ = pkl_ast_make_loop ($3, $5, $7, $9); }
        | WHILE '(' expression ')' mem_layout
        		{ $$ = pkl_ast_make_loop (NULL, $3, NULL, $5); }
	;

assert:
	  ASSERT expression
          		{ $$ = pkl_ast_make_assertion ($2); }
	;
*/

%%
