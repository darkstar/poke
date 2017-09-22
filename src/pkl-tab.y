/* pkl-tab.y - LARL(1) parser for the Poke Command Language.  */

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

#include "pkl-ast.h"
#include "pkl-parser.h" /* For struct pkl_parser.  */
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
  fprintf (stderr, "stdin: %d: %s\n", llocp->first_line, err);
}

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
 
%}

%union {
  pkl_ast_node ast;
  enum pkl_ast_op opcode;
  int integer;
}

%destructor { /* pkl_ast_free ($$);*/ } <ast>

%token <ast> INTEGER
%token <ast> STR
%token <ast> IDENTIFIER
%token <ast> TYPENAME
%token <ast> DOCSTR

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

%token AND
%token OR
%token LE
%token GE
%token INC
%token DEC
%token SL
%token SR
%token EQ
%token NE

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

%type <opcode> unary_operator assignment_operator

%type <integer> mem_endianness sign_qualifier

%type <ast> enumerator_list enumerator constant_expression
%type <ast> conditional_expression logical_or_expression
%type <ast> logical_and_expression inclusive_or_expression
%type <ast> exclusive_or_expression and_expression
%type <ast> equality_expression relational_expression
%type <ast> shift_expression additive_expression
%type <ast> multiplicative_expression unary_expression
%type <ast> postfix_expression primary_expression
%type <ast> expression assignment_expression
%type <ast> type_specifier declaration declaration_specifiers
%type <ast> typedef_specifier enum_specifier struct_specifier 
%type <ast> program program_elem_list program_elem
%type <ast> mem_layout mem_declarator_list
%type <ast> mem_declarator mem_field_with_docstr mem_field_with_endian
%type <ast> mem_field_with_size mem_field mem_cond mem_loop
%type <ast> assert

%start program

%% /* The grammar follows.  */

program: program_elem_list
          	{
                  if (yynerrs > 0)
                    {
                      pkl_ast_node_free ($1);
                      YYERROR;
                    }
                  
                  $$ = pkl_ast_make_program ($1);
                  pkl_parser->ast->ast = ASTREF ($$);
                }
        ;

program_elem_list:
          %empty
		{ $$ = NULL; }
	| program_elem
        	{
                  pkl_parser->at_start = 1;
                  pkl_parser->at_end = 1;
                  $$ = $1;
                }
        | program_elem_list program_elem
        	{
                  pkl_parser->at_start = 1;
                  pkl_parser->at_end = 1;
                  $$ = pkl_ast_chainon ($1, $2);
                }
	;

program_elem:
	  declaration
        | expression ';'
        ;

/*
 * Expressions.
 */

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

expression:
	  assignment_expression
        | expression ',' assignment_expression
		{ $$ = pkl_ast_chainon ($1, $3); }
        ;

assignment_expression:
	  conditional_expression
        | unary_expression assignment_operator assignment_expression
		{ $$ = pkl_ast_make_binary_exp ($2, $1, $3); }
        ;

assignment_operator:
	'='		{ $$ = PKL_AST_OP_ASSIGN; }
	| MULA	{ $$ = PKL_AST_OP_MULA; }
	| DIVA	{ $$ = PKL_AST_OP_DIVA; }
	| MODA	{ $$ = PKL_AST_OP_MODA; }
	| ADDA	{ $$ = PKL_AST_OP_ADDA; }
	| SUBA	{ $$ = PKL_AST_OP_SUBA; }
	| SLA	{ $$ = PKL_AST_OP_SLA; }
	| SRA	{ $$ = PKL_AST_OP_SRA; }
	| BANDA	{ $$ = PKL_AST_OP_BANDA; }
	| XORA	{ $$ = PKL_AST_OP_XORA; }
	| IORA	{ $$ = PKL_AST_OP_IORA; }
        ;

conditional_expression:
	  logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
          	{ $$ = pkl_ast_make_cond_exp ($1, $3, $5); }
	;

logical_or_expression:
	  logical_and_expression
        | logical_or_expression OR logical_and_expression
          	{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_OR, $1, $3); }
        ;

logical_and_expression:
	  inclusive_or_expression
	| logical_and_expression AND inclusive_or_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_AND, $1, $3); }
	;

inclusive_or_expression:
	  exclusive_or_expression
        | inclusive_or_expression '|' exclusive_or_expression
	        { $$ = pkl_ast_make_binary_exp (PKL_AST_OP_IOR, $1, $3); }
	;

exclusive_or_expression:
	  and_expression
        | exclusive_or_expression '^' and_expression
          	{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_XOR, $1, $3); }
	;

and_expression:
	  equality_expression
        | and_expression '&' equality_expression
          { $$ = pkl_ast_make_binary_exp (PKL_AST_OP_BAND, $1, $3); }
	;

equality_expression:
          relational_expression
	| equality_expression EQ relational_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_EQ, $1, $3); }
	| equality_expression NE relational_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_NE, $1, $3); }
	;

relational_expression:
	  shift_expression
        | relational_expression '<' shift_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_LT, $1, $3); }
        | relational_expression '>' shift_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_GT, $1, $3); }
        | relational_expression LE shift_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_LE, $1, $3); }
        | relational_expression GE shift_expression
        	{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_GE, $1, $3); }
        ;

shift_expression:
	  additive_expression
        | shift_expression SL additive_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_SL, $1, $3); }
        | shift_expression SR additive_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_SR, $1, $3); }
        ;

additive_expression:
	  multiplicative_expression
        | additive_expression '+' multiplicative_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_ADD, $1, $3); }
        | additive_expression '-' multiplicative_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_SUB, $1, $3); }
        ;

multiplicative_expression:
	  unary_expression
        | multiplicative_expression '*' unary_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_MUL, $1, $3); }
        | multiplicative_expression '/' unary_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_DIV, $1, $3); }
        | multiplicative_expression '%' unary_expression
		{ $$ = pkl_ast_make_binary_exp (PKL_AST_OP_MOD, $1, $3); }
        ;

unary_expression:
	  postfix_expression
        | INC unary_expression
		{ $$ = pkl_ast_make_unary_exp (PKL_AST_OP_INC, $2); }
        | DEC unary_expression
		{ $$ = pkl_ast_make_unary_exp (PKL_AST_OP_DEC, $2); }
        | unary_operator multiplicative_expression
		{ $$ = pkl_ast_make_unary_exp ($1, $2); }
        | SIZEOF unary_expression
		{ $$ = pkl_ast_make_unary_exp (PKL_AST_OP_SIZEOF, $2); }
        | SIZEOF '(' TYPENAME ')'
		{ $$ = pkl_ast_make_unary_exp (PKL_AST_OP_SIZEOF, $3); }
        ;

unary_operator:
	  '&' 	{ $$ = PKL_AST_OP_ADDRESS; }
	| '+'	{ $$ = PKL_AST_OP_POS; }
	| '-'	{ $$ = PKL_AST_OP_NEG; }
	| '~'	{ $$ = PKL_AST_OP_BNOT; }
	| '!'	{ $$ = PKL_AST_OP_NOT; }
	;

postfix_expression:
	  primary_expression
        | postfix_expression '[' expression ']'
		{ $$ = pkl_ast_make_array_ref ($1, $3); }
        | postfix_expression '.' IDENTIFIER
		{ $$ = pkl_ast_make_struct_ref ($1, $3); }
        | postfix_expression INC
		{ $$ = pkl_ast_make_unary_exp (PKL_AST_OP_INC, $1); }
	| postfix_expression DEC
		{ $$ = pkl_ast_make_unary_exp (PKL_AST_OP_DEC, $1); }
/*
        | '(' type_name ')' '{' initializer_list '}'
		{ $$ = pkl_ast_make_initializer ($2, $5); }
        | '(' type_name ')' '{' initializer_list ',' '}'
		{ $$ = pkl_ast_make_initializer ($2, $5); }
 */
	;

primary_expression:
	  IDENTIFIER
        | INTEGER
        | STR
        | '.'
          	{ $$ = pkl_ast_make_loc (); }
        | '(' expression ')'
		{ $$ = $2; }
	;

/*
 * Declarations.
 */

declaration:
	  declaration_specifiers ';'
        ;

declaration_specifiers:
          typedef_specifier
	| struct_specifier
        | enum_specifier
        ;

/*
 * Typedefs
 */

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

type_specifier:
	  sign_qualifier TYPENAME
        	{ PKL_AST_TYPE_SIGNED ($2) = $1; $$ = $2; }
	| TYPENAME
	| STRUCT IDENTIFIER
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

sign_qualifier:
	  SIGNED	{ $$ = 1; }
	| UNSIGNED	{ $$ = 0; }
        ;

/*
 * Enumerations.
 */

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

/*
 * Structs.
 */

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

/*
 * Memory layouts.
 */

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

                          /* Discard the size inferred from the field
                             type and replace it with the
                             field width expression.  */
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

%%
