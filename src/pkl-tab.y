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

%initial-action
{
    @$.first_line = @$.last_line = 1;
    @$.first_column = @$.last_column = 1;
};

%{
#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <xalloc.h>
#include <assert.h>

#include "pkl.h"
#include "pkl-ast.h"
#include "pkl-parser.h" /* For struct pkl_parser.  */

#define YYLTYPE pkl_ast_loc
#define YYDEBUG 1
#include "pkl-tab.h"
#include "pkl-lex.h"

#ifdef PKL_DEBUG
# include "pkl-gen.h"
#endif

#define scanner (pkl_parser->scanner)
  
/* YYLLOC_DEFAULT -> default code for computing locations.  */
  
#define PKL_AST_CHILDREN_STEP 12

/* Convert a YYLTYPE value into an AST location and return it.  */

void
pkl_tab_error (YYLTYPE *llocp,
               struct pkl_parser *pkl_parser,
               char const *err)
{
    pkl_error (pkl_parser->ast, *llocp, "%s", err);
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
                  $$ = pkl_ast_make_program (pkl_parser->ast,$1);
                  PKL_AST_LOC ($$) = @$;
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
                  $$ = pkl_ast_make_program (pkl_parser->ast, $1);
                  PKL_AST_LOC ($$) = @$;
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
                  $$ = pkl_ast_make_unary_exp (pkl_parser->ast,
                                               $1, $2);
                  PKL_AST_LOC ($$) = @1;
                }
        | '(' type_specifier ')' expression %prec UNARY
        	{
                  $$ = pkl_ast_make_cast (pkl_parser->ast, $2, $4);
                  PKL_AST_LOC ($$).first_line = @1.first_line;
                  PKL_AST_LOC ($$).first_column = @1.first_column;
                  PKL_AST_LOC ($$).last_line = @3.last_line;
                  PKL_AST_LOC ($$).last_column = @3.last_column;
                }
        | SIZEOF expression %prec UNARY
        	{
                  $$ = pkl_ast_make_unary_exp (pkl_parser->ast,
                                               PKL_AST_OP_SIZEOF, $2);
                  PKL_AST_LOC ($$) = @1;
                }
        | SIZEOF type_specifier %prec UNARY
        	{
                  $$ = pkl_ast_make_unary_exp (pkl_parser->ast, PKL_AST_OP_SIZEOF,
                                               $2);
                  PKL_AST_LOC ($$) = @1;
                }
	| SIZEOF '(' type_specifier ')' %prec UNARY
        	{
                  $$ = pkl_ast_make_unary_exp (pkl_parser->ast, PKL_AST_OP_SIZEOF, $3);
                  PKL_AST_LOC ($$) = @1;
                }
        | expression '+' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_ADD,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression '-' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_SUB,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression '*' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_MUL,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression '/' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_DIV,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression '%' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_MOD,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression SL expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_SL,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression SR expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_SR,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression EQ expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_EQ,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
	| expression NE expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_NE,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression '<' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_LT,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression '>' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_GT,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression LE expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_LE,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
	| expression GE expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_GE,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression '|' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_IOR,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression '^' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_XOR,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
	| expression '&' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_BAND,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
        | expression AND expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_AND,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
                }
	| expression OR expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_OR,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @2;
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
                      pkl_error (pkl_parser->ast, @3, "expected `b' or `B'");
                      YYERROR;
                    }

                  $$ = pkl_ast_make_offset (pkl_parser->ast, $2, units);
                  PKL_AST_LOC ($$) = @$;
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
                {
                  $$ = $1;
                  PKL_AST_LOC ($$) = @$;
                }
        | CHAR
                {
                  $$ = $1;
                  PKL_AST_LOC ($$) = @$;
                }
        | STR
                {
                  $$ = $1;
                  PKL_AST_LOC ($$) = @$;
                } 
        | '(' expression ')'
        	{
                    $$ = $2;
                }
        | primary '[' expression ']' %prec '.'
        	{
                  $$ = pkl_ast_make_array_ref (pkl_parser->ast, $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | '[' array_initializer_list ']'
        	{
                    $$ = pkl_ast_make_array (pkl_parser->ast,
                                             0 /* nelem */,
                                             0 /* ninitializer */,
                                             $2);
                    PKL_AST_LOC ($$) = @$;
                }
	| '{' struct_elem_list '}'
        	{
                    $$ = pkl_ast_make_struct (pkl_parser->ast,
                                              0 /* nelem */, $2);
                    PKL_AST_LOC ($$) = @$;
                }
        | primary '.' IDENTIFIER
        	{
                    $$ = pkl_ast_make_struct_ref (pkl_parser->ast, $1, $3);
                    PKL_AST_LOC ($3) = @3;
                    PKL_AST_LOC ($$) = @$;
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
                    $$ = pkl_ast_make_struct_elem (pkl_parser->ast, NULL, $1);
                    PKL_AST_LOC ($$) = @$;
                }
        | '.' IDENTIFIER '=' expression
	        {
                    $$ = pkl_ast_make_struct_elem (pkl_parser->ast, $2, $4);
                    PKL_AST_LOC ($2) = @2;
                    PKL_AST_LOC ($$) = @$;
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
                    $$ = pkl_ast_make_array_initializer (pkl_parser->ast,
                                                         NULL, $1);
                    PKL_AST_LOC ($$) = @$;
                }
        | '.' '[' INTEGER ']' '=' expression
        	{
                    $$ = pkl_ast_make_array_initializer (pkl_parser->ast,
                                                         $3, $6);
                    PKL_AST_LOC ($3) = @3;
                    PKL_AST_LOC ($$) = @$;
                }
        ;


type_specifier:
	  TYPENAME
                {
                    $$ = $1;
                    PKL_AST_LOC ($$) = @$;
                }
        | type_specifier '[' expression ']'
          	{
                    $$ = pkl_ast_make_array_type (pkl_parser->ast, $3, $1);
                    PKL_AST_LOC ($$) = @$;
                }
	| type_specifier '[' ']'
        	{
                    $$ = pkl_ast_make_array_type (pkl_parser->ast, NULL, $1);
                    PKL_AST_LOC ($$) = @$;
                }
        | struct_type_specifier
        ;


struct_type_specifier:
	  STRUCT '{' '}'
          	{
                    $$ = pkl_ast_make_struct_type (pkl_parser->ast, 0, NULL);
                    PKL_AST_LOC ($$) = @$;
                }
        | STRUCT '{' struct_elem_type_list '}'
        	{
                    $$ = pkl_ast_make_struct_type (pkl_parser->ast, 0 /* nelem */, $3);
                    PKL_AST_LOC ($$) = @$;
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
                    $$ = pkl_ast_make_struct_elem_type (pkl_parser->ast, $2, $1);
                    PKL_AST_LOC ($$) = @$;
                }
        | type_specifier ';'
        	{
                    $$ = pkl_ast_make_struct_elem_type (pkl_parser->ast, NULL, $1);
                    PKL_AST_LOC ($$) = @$;
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
