/* pkl-tab.y - LARL(1) parser for Poke.  */

/* Copyright (C) 2018 Jose E. Marchesi.  */

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

/* Error reporting function.  When this function returns, the parser
   may try to recover the error.  If it is unable to recover, then it
   returns with 1 (syntax error) immediately.  */
  
void
pkl_tab_error (YYLTYPE *llocp,
               struct pkl_parser *pkl_parser,
               char const *err)
{
  /* XXX if (!pkl_parser->interactive) */
  if (pkl_parser->filename != NULL)
    fprintf (stderr, "%s: %d: %s\n", pkl_parser->filename,
             llocp->first_line, err);
  else
    fprintf (stderr, "%s\n", err);
}

/* Forward declarations for functions defined below in this file,
   after the rules section.  See the comments at the definition of the
   functions for information about what they do.  */

static pkl_ast_node finish_array (struct pkl_parser *parser,
                                  YYLTYPE *llocp,
                                  pkl_ast_node elems);

static pkl_ast_node finish_struct (struct pkl_parser *parser,
                                   YYLTYPE *llocp,
                                   pkl_ast_node elems);

static pkl_ast_node finish_struct_ref (struct pkl_parser *parser,
                                       YYLTYPE *loc_sct,
                                       YYLTYPE *loc_identifier,
                                       pkl_ast_node sct,
                                       pkl_ast_node identifier);

static pkl_ast_node finish_struct_type (struct pkl_parser *parser,
                                        YYLTYPE *llocp,
                                        pkl_ast_node stype_elems);

static pkl_ast_node finish_sizeof_type (struct pkl_parser *parser,
                                        YYLTYPE *llocp,
                                        pkl_ast_node type);
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
%type <ast> array_initializer_list array_initializer
%type <ast> struct_elem_list struct_elem
%type <ast> type_specifier
%type <ast> struct_type_specifier struct_elem_type_list struct_elem_type
/*                      %type <ast> stmt_list stmt pushlevel compstmt*/

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
                  $$ = pkl_ast_make_unary_exp ($1, $2);
                }
        | '(' type_specifier ')' expression %prec UNARY
        	{
                  $$ = pkl_ast_make_cast ($2, $4);
                }
        | SIZEOF expression %prec UNARY
        	{
                  $$ = pkl_ast_make_unary_exp (PKL_AST_OP_SIZEOF, $2);
                }
        | SIZEOF type_specifier %prec UNARY
        	{
                  $$ = finish_sizeof_type (pkl_parser,
                                           &@2, $2);
                  if ($$ == NULL)
                    YYERROR;
                }
	| SIZEOF '(' type_specifier ')' %prec UNARY
        	{
                  $$ = finish_sizeof_type (pkl_parser,
                                           &@3, $3);
                  if ($$ == NULL)
                    YYERROR;
                }
        | expression '+' expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_ADD,
                                                $1, $3);
                }
        | expression '-' expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_SUB,
                                                $1, $3);
                }
        | expression '*' expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_MUL,
                                                $1, $3);
                }
        | expression '/' expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_DIV,
                                                $1, $3);
                }
        | expression '%' expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_MOD,
                                                $1, $3);
                }
        | expression SL expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_SL,
                                                $1, $3);
                }
        | expression SR expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_SR,
                                                $1, $3);
                }
        | expression EQ expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_EQ,
                                                $1, $3);
                }
	| expression NE expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_NE,
                                                $1, $3);
                }
        | expression '<' expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_LT,
                                                $1, $3);
                }
        | expression '>' expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_GT,
                                                $1, $3);
                }
        | expression LE expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_LE,
                                                $1, $3);
                }
	| expression GE expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_GE,
                                                $1, $3);
                }
        | expression '|' expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_IOR,
                                                $1, $3);
                }
        | expression '^' expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_XOR,
                                                $1, $3);
                }
	| expression '&' expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_BAND,
                                                $1, $3);
                }
        | expression AND expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_AND,
                                                $1, $3);
                }
	| expression OR expression
        	{
                  $$ = pkl_ast_make_binary_exp (PKL_AST_OP_OR,
                                                $1, $3);
                }
/*                      | expression '?' expression ':' expression
        	{ $$ = pkl_ast_make_cond_exp ($1, $3, $5); }*/
	| '[' expression IDENTIFIER ']'
        	{
                  int units;
                  
                  if (strcmp (PKL_AST_IDENTIFIER_POINTER ($3), "b") == 0)
                    units = PKL_AST_OFFSET_UNIT_BITS;
                  else if (strcmp (PKL_AST_IDENTIFIER_POINTER ($3), "B") == 0)
                    units = PKL_AST_OFFSET_UNIT_BYTES;
                  else
                    {
                      pkl_tab_error (&@3, pkl_parser,
                                     "expected `b' or `B'");
                      YYERROR;
                    }

                  $$ = pkl_ast_make_offset ($2, units);
                }
        ;

unary_operator:
	  '-'		{ $$ = PKL_AST_OP_NEG; }
	| '+'		{ $$ = PKL_AST_OP_POS; }
	| '~'		{ $$ = PKL_AST_OP_BNOT; }
	| '!'		{ $$ = PKL_AST_OP_NOT; }
	;

primary:
	  INTEGER
        | CHAR
        | STR
        | '(' expression ')'
        	{ $$ = $2; }
        | primary '[' expression ']' %prec '.'
        	{
                  if (PKL_AST_TYPE_CODE (PKL_AST_TYPE ($1))
                      != PKL_TYPE_ARRAY)
                    {
                      pkl_tab_error (&@1, pkl_parser,
                                     "operator to [] must be an array.");
                      YYERROR;
                    }

                  $$ = pkl_ast_make_array_ref ($1, $3);
                  PKL_AST_TYPE ($$) =
                    ASTREF (PKL_AST_TYPE_A_ETYPE (PKL_AST_TYPE ($1)));
                }
        | '[' array_initializer_list ']'
        	{
                  $$ = finish_array (pkl_parser, &@2, $2);
                  if ($$ == NULL)
                    YYERROR;
                }
	| '{' struct_elem_list '}'
        	{
                  $$ = finish_struct (pkl_parser, &@2, $2);
                  if ($$ == NULL)
                    YYERROR;
                }
        | primary '.' IDENTIFIER
        	{
                  $$ = finish_struct_ref (pkl_parser,
                                          &@1, &@3,
                                          $1, $3);
                  if ($$ == NULL)
                    YYERROR;
                }
	;

struct_elem_list:
	  %empty
		{ $$ = NULL; }
        | struct_elem
        | struct_elem_list ',' struct_elem
		{                  
                  $$ = pkl_ast_chainon ($3, $1);
                }
        ;

struct_elem:
	  expression
          	{
                  $$ = pkl_ast_make_struct_elem (NULL, $1);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($1));
                }
        | '.' IDENTIFIER '=' expression
	        {
                  $$ = pkl_ast_make_struct_elem ($2, $4);
                  PKL_AST_TYPE ($$) = ASTREF (PKL_AST_TYPE ($4));
                }
        ;

array_initializer_list:
	  array_initializer
        | array_initializer_list ',' array_initializer
          	{
                  $$ = pkl_ast_chainon ($1, $3);
                }
        ;

array_initializer:
	  expression
          	{
                  $$ = pkl_ast_make_array_initializer (PKL_AST_ARRAY_NOINDEX,
                                                       $1);
                }
        | '.' '[' INTEGER ']' '=' expression
        	{
                  $$ = pkl_ast_make_array_initializer (PKL_AST_INTEGER_VALUE ($3),
                                                       $6);
                }
        ;


type_specifier:
	  TYPENAME
        | type_specifier '[' expression ']'
          	{
                  $$ = pkl_ast_make_array_type ($3, $1);
                }
	| type_specifier '[' ']'
        	{
                  $$ = pkl_ast_make_array_type (NULL, $1);
                }
        | struct_type_specifier
        ;


struct_type_specifier:
	  STRUCT '{' '}'
          	{
                  $$ = pkl_ast_make_struct_type (0, NULL);
                }
        | STRUCT '{' struct_elem_type_list '}'
        	{
                  $$ = finish_struct_type (pkl_parser, &@3, $3);
                  if ($$ == NULL)
                    YYERROR;
                }
        ;

struct_elem_type_list:
	  struct_elem_type
        | struct_elem_type_list struct_elem_type
        	{ $$ = pkl_ast_chainon ($2, $1); }
        ;

struct_elem_type:
	  type_specifier IDENTIFIER ';'
          	{
                  $$ = pkl_ast_make_struct_type_elem ($2, $1);
                }
        | type_specifier ';'
        	{
                  $$ = pkl_ast_make_struct_type_elem (NULL, $1);
                }
        ;

/*
 * Statements.
 */

/*
pushlevel:
         * This rule is used below in order to set a new current
            binding level that will be in effect for subsequent
            reductions of declarations.  *
	  %empty
		{
                  push_level ();
                  $$ = pkl_parser->current_block;
                  pkl_parser->current_block
                    = pkl_ast_make_let (* supercontext * $$, * body * NULL);
                }
	;

compstmt:
	  '{' '}'
          	{ $$ = pkl_ast_make_compound_stmt (NULL); }
        | '{' pushlevel stmt_list '}'
        	{
                  $$ = pkl_parser->current_block; 
                  PKL_AST_LET_BODY ($$, $3);
                  pop_level ();
                  * XXX: build a compound instead of a let if
                     stmt_list doesn't contain any declaration.  *
                }
        ;

stmt_list:
	  stmt
        | stmt_list stmt
          	{ $$ = pkl_ast_chainon ($1, $2); }
	;

stmt:
	  compstmt
        | IDENTIFIER '=' expression ';'
          	{ $$ = pkl_ast_make_assign_stmt ($1, $3); }
        ;
*/
          
/*
 * Declarations.
 */

/* decl.c:  start_decl, finish_decl
  
   Identifiers have a local value and a global value.  */

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
        ;

enumerator_list:
	  enumerator
	| enumerator_list ',' enumerator
          	{ $$ = pkl_ast_chainon ($1, $3); }
	;

enumerator:
	  IDENTIFIER
                { $$ = pkl_ast_make_enumerator ($1, NULL, NULL); }
        | IDENTIFIER '=' constant_expression
                { $$ = pkl_ast_make_enumerator ($1, $3, NULL); }
	;
*/

%%

/* Finish an array and return it.  Compute and set the indexes of all
   the elements and set the size of the array consequently.

   In case of a syntax error, return NULL.  */

static pkl_ast_node
finish_array (struct pkl_parser *parser,
              YYLTYPE *llocp,
              pkl_ast_node initializers)
{
  pkl_ast_node array, tmp;
  size_t index, nelem, ninitializer;

  nelem = 0;
  for (index = 0, tmp = initializers, ninitializer = 0;
       tmp;
       tmp = PKL_AST_CHAIN (tmp), ++ninitializer)
    {
      size_t initializer_index = PKL_AST_ARRAY_INITIALIZER_INDEX (tmp);
      size_t elems_appended, effective_index;
      
      /* Set the index of the initializer.  */
      if (initializer_index == PKL_AST_ARRAY_NOINDEX)
        {
          effective_index = index;
          elems_appended = 1;
        }
      else
        {
          if (initializer_index < index)
            elems_appended = 0;
          else
            elems_appended = initializer_index - index + 1;
          effective_index = initializer_index;
        }
      
      PKL_AST_ARRAY_INITIALIZER_INDEX (tmp) = effective_index;
      index += elems_appended;
      nelem += elems_appended;
    }

  array = pkl_ast_make_array (nelem, ninitializer,
                              pkl_ast_reverse (initializers));
  return array;
}

/* Finish a struct and return it.  Derive the type of the struct after
   the types of its elements.

   In case of a syntax error, return NULL.  */

static pkl_ast_node
finish_struct (struct pkl_parser *parser,
               YYLTYPE *llocp,
               pkl_ast_node elems)
{
  pkl_ast_node t;
  pkl_ast_node struct_type_elems, type, sct;
  size_t nelem;

  struct_type_elems = NULL;
  nelem = 0;
  for (t = elems; t; t = PKL_AST_CHAIN (t))
    {
      pkl_ast_node name;
      pkl_ast_node struct_type_elem;
      pkl_ast_node ename;
      pkl_ast_node type;

      /* Add the name for this struct element.  */
      name = PKL_AST_STRUCT_ELEM_NAME (t);
      if (name)
          ename = pkl_ast_make_identifier (PKL_AST_IDENTIFIER_POINTER (name));
      else
        ename = pkl_ast_make_identifier ("");

      type = PKL_AST_TYPE (PKL_AST_STRUCT_ELEM_EXP (t));

      struct_type_elem = pkl_ast_make_struct_type_elem (ename,
                                                        pkl_ast_dup_type (type));
      struct_type_elems = pkl_ast_chainon (struct_type_elems,
                                           struct_type_elem);
      nelem += 1;
    }

  /* Now build the type for the struct.  */
  type = pkl_ast_make_struct_type (nelem, struct_type_elems);

  sct = pkl_ast_make_struct (nelem, elems);
  PKL_AST_TYPE (sct) = ASTREF (type);
  
  return sct;
}

/* Finish a reference to a struct and return it.  Check whether SCT is
   indeed a struct, and also that it contains an element named after
   IDENTIFIER.  Also set the type of the struct ref after the type of
   the referred element.

   In case of a syntax error, return NULL.  */

static pkl_ast_node
finish_struct_ref (struct pkl_parser *parser,
                   YYLTYPE *loc_sct,
                   YYLTYPE *loc_identifier,
                   pkl_ast_node sct,
                   pkl_ast_node identifier)
{
  pkl_ast_node e, type, sref, stype;

  stype = PKL_AST_TYPE (sct);
  if (PKL_AST_TYPE_CODE (stype) != PKL_TYPE_STRUCT)
    {
      pkl_tab_error (loc_sct, parser,
                     "expected struct.");
      return NULL;
    }

  type = NULL;
  for (e = PKL_AST_TYPE_S_ELEMS (stype); e; e = PKL_AST_CHAIN (e))
    {
      if (strcmp (PKL_AST_IDENTIFIER_POINTER (PKL_AST_STRUCT_TYPE_ELEM_NAME (e)),
                  PKL_AST_IDENTIFIER_POINTER (identifier)) == 0)
        {
          type = PKL_AST_STRUCT_TYPE_ELEM_TYPE (e);
          break;
        }
    }

  if (type == NULL)
    {
      pkl_tab_error (loc_identifier, parser,
                     "invalid struct member");
      return NULL;
    }

  sref = pkl_ast_make_struct_ref (sct, identifier);
  PKL_AST_TYPE (sref) = ASTREF (type);
  
  return sref;
}

/* Finish a struct type and return it.  Check that no duplicated named
   elements are declared in the type.

   In case of a syntax error, return NULL.  */

static pkl_ast_node
finish_struct_type (struct pkl_parser *parser,
                    YYLTYPE *llocp,
                    pkl_ast_node stype_elems)
{
  pkl_ast_node t, u, stype;
  size_t nelem;

  nelem = 0;
  for (t = stype_elems; t; t = PKL_AST_CHAIN (t))
    {
      for (u = stype_elems; u != t; u = PKL_AST_CHAIN (u))
        {
          pkl_ast_node tname = PKL_AST_STRUCT_TYPE_ELEM_NAME (u);
          pkl_ast_node uname = PKL_AST_STRUCT_TYPE_ELEM_NAME (t);

          if (uname
              && tname
              && strcmp (PKL_AST_IDENTIFIER_POINTER (uname),
                         PKL_AST_IDENTIFIER_POINTER (tname)) == 0)
            {
              pkl_tab_error (llocp, parser,
                             "duplicated element name in struct type spec.");
              return NULL;
            }
        }
      
      nelem += 1;
    }

  stype = pkl_ast_make_struct_type (nelem, stype_elems);
  return stype;
}

static pkl_ast_node
finish_sizeof_type (struct pkl_parser *parser,
                    YYLTYPE *llocp,
                    pkl_ast_node type)
{
  pkl_ast_node offset;
  pkl_ast_node magnitude_type, magnitude, offset_type;

  if (!PKL_AST_TYPE_COMPLETE_P (type))
    {
      pkl_tab_error (llocp, parser,
                     "sizeof only works on complete types");
      return NULL;
    }
  
  /* Calculate the size of the complete type in bytes and put it in an
     integer node.  */
  magnitude_type = pkl_ast_get_integral_type (parser->ast,
                                              64, 0);
  magnitude = pkl_ast_make_integer (pkl_ast_sizeof_type (type));
  PKL_AST_TYPE (magnitude) = ASTREF (magnitude_type);
  
  /* Build an offset with that magnitude, and unit bits.  */
  offset = pkl_ast_make_offset (magnitude,
                                PKL_AST_OFFSET_UNIT_BITS);
  offset_type = pkl_ast_make_offset_type (magnitude_type,
                                          PKL_AST_OFFSET_UNIT_BITS);
  PKL_AST_TYPE (offset) = ASTREF (offset_type);

  return offset;
}
