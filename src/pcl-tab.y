/* pcl-tab.y - LARL(1) parser for the Poke Command Language.  */

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
%name-prefix "pcl_tab_"

%lex-param {void *scanner}
%parse-param {void *scanner}


%{
#include <config.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <xalloc.h>

#include <pcl-ast.h>
#include "pcl-tab.h"
#include "pcl-lex.h"

/* YYLLOC_DEFAULT -> default code for computing locations.  */
  
#define PCL_AST_CHILDREN_STEP 12

/* Error reporting function for pcl_tab_parse.

   When this function returns, the parser tries to recover the error.
   If it is unable to recover, then it returns with 1 (syntax error)
   immediately.

   We use the default structure for YYLTYPE:

     typedef struct YYLTYPE
     {
       int first_line;
       int first_column;
       int last_line;
       int last_column;
     } YYLTYPE;
 */
  
void
pcl_tab_error (YYLTYPE *llocp, void *extra, char const *err)
{
  /* yynerrs is a local variable of yyparse that contains the number of
     syntax errors reported so far.  */
  fprintf (stderr, "stdin: %d: %s\n", llocp->first_line, err);
}
 
%}

%union {
  pcl_ast ast;
  enum pcl_ast_op opcode;
}

%token <ast> PCL_TOK_INT
%token <ast> PCL_TOK_STR
%token <ast> PCL_TOK_ID
%token <ast> PCL_TOK_TYPENAME
%token <ast> PCL_TOK_DOCSTR

%token PCL_TOK_ENUM
%token PCL_TOK_STRUCT
%token PCL_TOK_TYPEDEF
%token PCL_TOK_BREAK
%token PCL_TOK_CONST
%token PCL_TOK_CONTINUE
%token PCL_TOK_ELSE
%token PCL_TOK_FOR
%token PCL_TOK_IF
%token PCL_TOK_SIZEOF
%token PCL_TOK_ERR

%token PCL_TOK_AND
%token PCL_TOK_OR
%token PCL_TOK_LE
%token PCL_TOK_GE
%token PCL_TOK_INC
%token PCL_TOK_DEC
%token PCL_TOK_SL
%token PCL_TOK_SR
%token PCL_TOK_EQ
%token PCL_TOK_NE

%token <opcode> PCL_TOK_MULA
%token <opcode> PCL_TOK_DIVA
%token <opcode> PCL_TOK_MODA
%token <opcode> PCL_TOK_ADDA
%token <opcode> PCL_TOK_SUBA
%token <opcode> PCL_TOK_SLA
%token <opcode> PCL_TOK_SRA
%token <opcode> PCL_TOK_BANDA
%token <opcode> PCL_TOK_XORA
%token <opcode> PCL_TOK_IORA

%token PCL_TOK_MSB PCL_TOK_LSB

%type <opcode> unary_operator assignment_operator

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
%type <ast> typedef_specifier struct_specifier enum_specifier
%type <ast> struct_declaration_list struct_declaration_with_endian
%type <ast> struct_declaration struct_field declaration_list program

%start program

%% /* The grammar follows.  */

program: declaration_list
          	{
                  $$ = pcl_ast_make_program ();
                  PCL_AST_PROGRAM_DECLARATIONS ($$) = $1;

#ifdef PCL_DEBUG
                  pcl_ast_print ($$, 0);
#endif                  
                }
        ;

declaration_list:
	  declaration
        | declaration_list declaration
		{
                  $$ = pcl_ast_chainon ($1, $2);
                }
	;

/*
 * Expressions.
 */

constant_expression:
	  conditional_expression
          	{
                  if (!PCL_AST_LITERAL_P ($1))
                    {
                      pcl_tab_error (&@1, NULL, "expected constant expression");
                      YYERROR;
                    }

                  $$ = $1;
                }
        ;

expression:
	  assignment_expression
        | expression ',' assignment_expression
		{ $$ = pcl_ast_chainon ($1, $3); }
        ;

assignment_expression:
	  conditional_expression
        | unary_expression assignment_operator assignment_expression
		{ $$ = pcl_ast_make_binary_exp ($2, $1, $3); }
        ;

assignment_operator:
	'='		{ $$ = PCL_AST_OP_ASSIGN; }
	| PCL_TOK_MULA	{ $$ = PCL_AST_OP_MULA; }
	| PCL_TOK_DIVA	{ $$ = PCL_AST_OP_DIVA; }
	| PCL_TOK_MODA	{ $$ = PCL_AST_OP_MODA; }
	| PCL_TOK_ADDA	{ $$ = PCL_AST_OP_ADDA; }
	| PCL_TOK_SUBA	{ $$ = PCL_AST_OP_SUBA; }
	| PCL_TOK_SLA	{ $$ = PCL_AST_OP_SLA; }
	| PCL_TOK_SRA	{ $$ = PCL_AST_OP_SRA; }
	| PCL_TOK_BANDA	{ $$ = PCL_AST_OP_BANDA; }
	| PCL_TOK_XORA	{ $$ = PCL_AST_OP_XORA; }
	| PCL_TOK_IORA	{ $$ = PCL_AST_OP_IORA; }
        ;

conditional_expression:
	  logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
          	{ $$ = pcl_ast_make_cond_exp ($1, $3, $5); }
	;

logical_or_expression:
	  logical_and_expression
        | logical_or_expression PCL_TOK_OR logical_and_expression
          	{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_OR, $1, $3); }
        ;

logical_and_expression:
	  inclusive_or_expression
	| logical_and_expression PCL_TOK_AND inclusive_or_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_AND, $1, $3); }
	;

inclusive_or_expression:
	  exclusive_or_expression
        | inclusive_or_expression '|' exclusive_or_expression
	        { $$ = pcl_ast_make_binary_exp (PCL_AST_OP_IOR, $1, $3); }
	;

exclusive_or_expression:
	  and_expression
        | exclusive_or_expression '^' and_expression
          	{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_XOR, $1, $3); }
	;

and_expression:
	  equality_expression
        | and_expression '&' equality_expression
          { $$ = pcl_ast_make_binary_exp (PCL_AST_OP_BAND, $1, $3); }
	;

equality_expression:
          relational_expression
	| equality_expression PCL_TOK_EQ relational_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_EQ, $1, $3); }
	| equality_expression PCL_TOK_NE relational_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_NE, $1, $3); }
	;

relational_expression:
	  shift_expression
        | relational_expression '<' shift_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_LT, $1, $3); }
        | relational_expression '>' shift_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_GT, $1, $3); }
        | relational_expression PCL_TOK_LE shift_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_LE, $1, $3); }
        | relational_expression PCL_TOK_GE shift_expression
        	{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_GE, $1, $3); }
        ;

shift_expression:
	  additive_expression
        | shift_expression PCL_TOK_SL additive_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_SL, $1, $3); }
        | shift_expression PCL_TOK_SR additive_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_SR, $1, $3); }
        ;

additive_expression:
	  multiplicative_expression
        | additive_expression '+' multiplicative_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_ADD, $1, $3); }
        | additive_expression '-' multiplicative_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_SUB, $1, $3); }
        ;

multiplicative_expression:
	  unary_expression
        | multiplicative_expression '*' unary_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_MUL, $1, $3); }
        | multiplicative_expression '/' unary_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_DIV, $1, $3); }
        | multiplicative_expression '%' unary_expression
		{ $$ = pcl_ast_make_binary_exp (PCL_AST_OP_MOD, $1, $3); }
        ;

unary_expression:
	  postfix_expression
        | PCL_TOK_INC unary_expression
		{ $$ = pcl_ast_make_unary_exp (PCL_AST_OP_INC, $2); }
        | PCL_TOK_DEC unary_expression
		{ $$ = pcl_ast_make_unary_exp (PCL_AST_OP_DEC, $2); }
        | unary_operator multiplicative_expression
		{ $$ = pcl_ast_make_unary_exp ($1, $2); }
        | PCL_TOK_SIZEOF unary_expression
		{ $$ = pcl_ast_make_unary_exp (PCL_AST_OP_SIZEOF, $2); }
        | PCL_TOK_SIZEOF '(' PCL_TOK_TYPENAME ')'
		{ $$ = pcl_ast_make_unary_exp (PCL_AST_OP_SIZEOF, $3); }
        ;

unary_operator:
	  '&' 	{ $$ = PCL_AST_OP_ADDRESS; }
	| '+'	{ $$ = PCL_AST_OP_POS; }
	| '-'	{ $$ = PCL_AST_OP_NEG; }
	| '~'	{ $$ = PCL_AST_OP_BNOT; }
	| '!'	{ $$ = PCL_AST_OP_NOT; }
	;

postfix_expression:
	  primary_expression
        | postfix_expression '[' expression ']'
		{ $$ = pcl_ast_make_array_ref ($1, $3); }
        | postfix_expression '.' PCL_TOK_ID
		{ $$ = pcl_ast_make_struct_ref ($1, $3); }
        | postfix_expression PCL_TOK_INC
		{ $$ = pcl_ast_make_unary_exp (PCL_AST_OP_INC, $1); }
	| postfix_expression PCL_TOK_DEC
		{ $$ = pcl_ast_make_unary_exp (PCL_AST_OP_DEC, $1); }
/*
        | '(' type_name ')' '{' initializer_list '}'
		{ $$ = pcl_ast_make_initializer ($2, $5); }
        | '(' type_name ')' '{' initializer_list ',' '}'
		{ $$ = pcl_ast_make_initializer ($2, $5); }
 */
	;

primary_expression:
	  PCL_TOK_ID
        | PCL_TOK_INT
        | PCL_TOK_STR
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
        | PCL_TOK_MSB struct_specifier
          	{ PCL_AST_STRUCT_ENDIAN ($2) = PCL_AST_MSB; $$ = $2; }
        | PCL_TOK_LSB struct_specifier
        	{ PCL_AST_STRUCT_ENDIAN ($2) = PCL_AST_LSB; $$ = $2; }
        | enum_specifier
        ;

/*
 * Typedefs
 */

typedef_specifier:
	  PCL_TOK_TYPEDEF type_specifier PCL_TOK_ID
          	{
                  if (pcl_ast_register_type (PCL_AST_IDENTIFIER_POINTER($3),
                                             $2) == NULL)
                    {
                      pcl_tab_error (&@2, "Duplicated type", NULL);
                      YYERROR;
                    }
                }
        ;

type_specifier:
	  's' ':' constant_expression
          	{ $$ = pcl_ast_make_type (1, $3, NULL, NULL); }
        | 'u' ':' constant_expression
        	{ $$ = pcl_ast_make_type (0, $3, NULL, NULL); }
        | PCL_TOK_TYPENAME
	| PCL_TOK_STRUCT PCL_TOK_ID
        	{
                  pcl_ast type
                    = pcl_ast_get_type (PCL_AST_IDENTIFIER_POINTER ($2));

                  if (!type)
                    {
                      pcl_tab_error (&@2, NULL, "Undefined struct.");
                      YYERROR;
                    }
                  
                  $$ = type;
                }
        | PCL_TOK_ENUM PCL_TOK_ID
        	{
                  pcl_ast type
                    = pcl_ast_get_type (PCL_AST_IDENTIFIER_POINTER ($2));
                  
                  if (!type)
                    {
                      pcl_tab_error (&@2, "Undefined enum.", NULL);
                      YYERROR;
                    }
                  
                  $$ = type;
                }
        ;

/*
 * Enumerations.
 */

enum_specifier:
	  PCL_TOK_ENUM PCL_TOK_ID '{' enumerator_list '}'
		{
                  $$ = pcl_ast_make_enum ($2, $4, NULL);
                  if (pcl_ast_register_type (PCL_AST_IDENTIFIER_POINTER ($2),
                                             $$) == NULL)
                    {
                      pcl_tab_error (&@2, "Duplicated type.", NULL);
                      YYERROR;
                    }
                }
	| PCL_TOK_ENUM PCL_TOK_ID PCL_TOK_DOCSTR '{' enumerator_list '}'
		{
                  $$ = pcl_ast_make_enum ($2, $5, $3);
                  PCL_AST_DOC_STRING_ENTITY ($3) = $$;
                  if (pcl_ast_register_type (PCL_AST_IDENTIFIER_POINTER ($2),
                                             $$) == NULL)
                    {
                      pcl_tab_error (&@2, "Duplicated type.", NULL);
                      YYERROR;
                    }
                }
        | PCL_TOK_ENUM PCL_TOK_DOCSTR PCL_TOK_ID '{' enumerator_list '}'
		{
                  $$ = pcl_ast_make_enum ($3, $5, $2);
                  PCL_AST_DOC_STRING_ENTITY ($2) = $$;
                  if (pcl_ast_register_type (PCL_AST_IDENTIFIER_POINTER ($3),
                                             $$) == NULL)
                    {
                      pcl_tab_error (&@3, "Duplicated type.", NULL);
                      YYERROR;
                    }
                }
        | PCL_TOK_ENUM PCL_TOK_ID '{' enumerator_list '}' PCL_TOK_DOCSTR
		{
                  $$ = pcl_ast_make_enum ($2, $4, $6);
                  PCL_AST_DOC_STRING_ENTITY ($6) = $$;
                  if (pcl_ast_register_type (PCL_AST_IDENTIFIER_POINTER ($2),
                                             $$) == NULL)
                    {
                      pcl_tab_error (&@2, "Duplicated type.", NULL);
                      YYERROR;
                    }
                }
        | PCL_TOK_DOCSTR PCL_TOK_ENUM PCL_TOK_ID '{' enumerator_list '}'
		{
                  $$ = pcl_ast_make_enum ($3, $5, $1);
                  PCL_AST_DOC_STRING_ENTITY ($1) = $$;
                  if (pcl_ast_register_type (PCL_AST_IDENTIFIER_POINTER ($3),
                                             $$) == NULL)
                    {
                      pcl_tab_error (&@3, "Duplicated type.", NULL);
                      YYERROR;
                    }
                }        
        ;

enumerator_list:
	  enumerator
	| enumerator_list ',' enumerator
		{ $$ = pcl_ast_chainon ($1, $3); }
	;

enumerator:
	  PCL_TOK_ID
                { $$ = pcl_ast_make_enumerator ($1, NULL, NULL); }
	| PCL_TOK_ID PCL_TOK_DOCSTR
                {
                  $$ = pcl_ast_make_enumerator ($1, NULL, $2);
                  PCL_AST_DOC_STRING_ENTITY ($2) = $$;
                }
        | PCL_TOK_ID '=' constant_expression
                { $$ = pcl_ast_make_enumerator ($1, $3, NULL); }
        | PCL_TOK_ID '=' PCL_TOK_DOCSTR constant_expression
        	{
                  $$ = pcl_ast_make_enumerator ($1, $4, $3);
                  PCL_AST_DOC_STRING_ENTITY ($3) = $$;
                }
        | PCL_TOK_ID '=' constant_expression PCL_TOK_DOCSTR
	        {
                  $$ = pcl_ast_make_enumerator ($1, $3, $4);
                  PCL_AST_DOC_STRING_ENTITY ($4) = $$;
                }
        | PCL_TOK_DOCSTR PCL_TOK_ID '=' constant_expression
        	{
                  $$ = pcl_ast_make_enumerator ($2, $4, $1);
                  PCL_AST_DOC_STRING_ENTITY ($1) = $$;
                }
	;

/*
 * Structs.
 */

struct_specifier:
	  PCL_TOK_STRUCT PCL_TOK_ID '{' struct_declaration_list '}'
		{
                  $$ = pcl_ast_make_struct ($2, $4, /* docstr */ NULL,
                                            pcl_ast_default_endian ());
                  if (pcl_ast_register_type (PCL_AST_IDENTIFIER_POINTER ($2),
                                             $$) == NULL)
                    {
                      pcl_tab_error (&@2, "Duplicated type.", NULL);
                      YYERROR;
                    }
                }
	| PCL_TOK_STRUCT PCL_TOK_ID PCL_TOK_DOCSTR '{' struct_declaration_list '}'
          	{
                  $$ = pcl_ast_make_struct ($2, $5, $3, pcl_ast_default_endian ());
                  PCL_AST_DOC_STRING_ENTITY ($3) = $$;
                  if (pcl_ast_register_type (PCL_AST_IDENTIFIER_POINTER ($2),
                                             $$) == NULL)
                    {
                      pcl_tab_error (&@2, "Duplicated type.", NULL);
                      YYERROR;
                    }
                        
                }
        | PCL_TOK_STRUCT PCL_TOK_DOCSTR PCL_TOK_ID '{' struct_declaration_list '}'
          	{
                  $$ = pcl_ast_make_struct ($3, $5, $2, pcl_ast_default_endian ());
                  PCL_AST_DOC_STRING_ENTITY ($2) = $$;
                  if (pcl_ast_register_type (PCL_AST_IDENTIFIER_POINTER ($3),
                                             $$) == NULL)
                    {
                      pcl_tab_error (&@3, "Duplicated type.", NULL);
                      YYERROR;
                    }
                }
        | PCL_TOK_STRUCT PCL_TOK_ID '{' struct_declaration_list '}' PCL_TOK_DOCSTR
          	{
                  $$ = pcl_ast_make_struct ($2, $4, $6, pcl_ast_default_endian ());
                  PCL_AST_DOC_STRING_ENTITY ($6) = $$;
                  if (pcl_ast_register_type (PCL_AST_IDENTIFIER_POINTER ($2),
                                             $$) == NULL)
                    {
                      pcl_tab_error (&@2, "Duplicated type.", NULL);
                      YYERROR;
                    }
                }
        ;

struct_declaration_list:
	  struct_declaration_with_endian
        | struct_declaration_list struct_declaration_with_endian
		{ $$ = pcl_ast_chainon ($1, $2); }
/* XXX: add conditionals and loops.  */
	;

struct_declaration_with_endian:
	  struct_declaration
        | PCL_TOK_MSB struct_declaration
		{
                  PCL_AST_FIELD_ENDIAN ($2) = PCL_AST_MSB;
                  $$ = $2;
                }
        | PCL_TOK_LSB struct_declaration
        	{
                  PCL_AST_FIELD_ENDIAN ($2) = PCL_AST_LSB;
                  $$ = $2;
                }
        ;

struct_declaration:
	  type_specifier struct_field PCL_TOK_DOCSTR ';'
          	{
                  PCL_AST_FIELD_TYPE ($2) = $1;
                  PCL_AST_FIELD_DOCSTR ($2) = $3;
                  $$ = $2;
                  PCL_AST_DOC_STRING_ENTITY ($3) = $$;
                }
        | type_specifier PCL_TOK_DOCSTR struct_field ';'
        	{
                  PCL_AST_FIELD_TYPE ($3) = $1;
                  PCL_AST_FIELD_DOCSTR ($3) = $2;
                  $$ = $3;
                  PCL_AST_DOC_STRING_ENTITY ($2) = $$;
                }
        | PCL_TOK_DOCSTR type_specifier struct_field ';'
        	{
                  PCL_AST_FIELD_TYPE ($3) = $2;
                  PCL_AST_FIELD_DOCSTR ($3) = $1;
                  $$ = $3;
                  PCL_AST_DOC_STRING_ENTITY ($1) = $$;
                }
        ;

struct_field:
	  PCL_TOK_ID
          	{
                  $$ = pcl_ast_make_field ($1,
                                           /* type */ NULL,
                                           /* docstr */ NULL,
                                           pcl_ast_default_endian (),
                                           /* size_exp */ NULL);
                }
        | PCL_TOK_ID '[' assignment_expression ']'
        	{
                  $$ = pcl_ast_make_field ($1,
                                           /* type */ NULL,
                                           /* docstr */ NULL,
                                           pcl_ast_default_endian (),
                                           $3);
                }
        ;

%%
