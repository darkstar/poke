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

%destructor {
  if ($$)
    {
      switch (PKL_AST_CODE ($$))
        {
        case PKL_AST_COMP_STMT:
          /* XXX: for comp_stmt, we should pop N-levels.  */
          assert (0);
          break;
        case PKL_AST_TYPE:
          if (PKL_AST_TYPE_CODE ($$) == PKL_TYPE_STRUCT)
            pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
          break;
        case PKL_AST_FUNC:
          pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
          break;
        default:
          break;
        }
    }

  $$ = ASTREF ($$); pkl_ast_node_free ($$);
 } <ast>

/* Primaries.  */

%token <ast> INTEGER
%token <ast> CHAR
%token <ast> STR
%token <ast> IDENTIFIER
%token <ast> TYPENAME
%token <ast> UNIT

/* Reserved words.  */

%token ENUM
%token STRUCT
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
%token INTCONSTR UINTCONSTR OFFSETCONSTR
%token DEFUN DEFSET DEFTYPE DEFVAR
%token RETURN
%token STRING

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

/* This is for the dangling ELSE.  */

%precedence THEN
%precedence ELSE

/* Operator tokens and their precedences, in ascending order.  */

%right '?' ':'
%left OR
%left AND
%left '|'
%left '^'
%left '&'
%left EQ NE
%left LE GE '<' '>'
%left SL SR
%left '+' '-'
%left '*' '/' '%'
%right '@'
%nonassoc UNIT
%right UNARY INC DEC AS
%left HYPERUNARY
%left '.'

%type <opcode> unary_operator

%type <ast> start program program_elem_list program_elem
%type <ast> expression primary identifier
%type <ast> funcall funcall_arg_list funcall_arg
%type <ast> array array_initializer_list array_initializer
%type <ast> struct struct_elem_list struct_elem
%type <ast> type_specifier simple_type_specifier
%type <ast> struct_type_specifier struct_elem_type_list struct_elem_type
%type <ast> declaration
%type <ast> function_specifier function_arg_list function_arg
%type <ast> comp_stmt stmt_decl_list stmt

/* The following two tokens are used in order to support several start
   rules: one is for parsing an expression and the other for parsing a
   full poke programs.  This trick is explained in the Bison Manual in
   the "Multiple start-symbols" section.  */

%token START_EXP START_DECL START_PROGRAM;

%start start

%% /* The grammar follows.  */

pushlevel:
	  %empty
		{
                  pkl_parser->env = pkl_env_push_frame (pkl_parser->env);
                }
        ;

start:
	  START_EXP expression
          	{
                  $$ = pkl_ast_make_program (pkl_parser->ast, $2);
                  PKL_AST_LOC ($$) = @$;
                  pkl_parser->ast->ast = ASTREF ($$);
                }
        | START_EXP expression ','
          	{
                  $$ = pkl_ast_make_program (pkl_parser->ast, $2);
                  PKL_AST_LOC ($$) = @$;
                  pkl_parser->ast->ast = ASTREF ($$);
                  YYACCEPT;
                }
        | START_DECL declaration
        	{
                  $$ = pkl_ast_make_program (pkl_parser->ast, $2);
                  PKL_AST_LOC ($$) = @$;
                  pkl_parser->ast->ast = ASTREF ($$);
                }
        | START_DECL declaration ','
        	{
                  $$ = pkl_ast_make_program (pkl_parser->ast, $2);
                  PKL_AST_LOC ($$) = @$;
                  pkl_parser->ast->ast = ASTREF ($$);
                }

        | START_PROGRAM program
        	{
                  $$ = pkl_ast_make_program (pkl_parser->ast, $2);
                  PKL_AST_LOC ($$) = @$;
                  pkl_parser->ast->ast = ASTREF ($$);
                }
        ;

program:
	  %empty
		{
                  $$ = NULL;
                }
	| program_elem_list
        ;

program_elem_list:
	  program_elem
        | program_elem_list program_elem
        	{
                  $$ = pkl_ast_chainon ($1, $2);
                }
	;

program_elem:
	  declaration
        ;

/*
 * Identifiers.
 */

identifier:
	  TYPENAME
        | IDENTIFIER
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
        | SIZEOF '(' expression ')' %prec UNARY
        	{
                  $$ = pkl_ast_make_unary_exp (pkl_parser->ast,
                                               PKL_AST_OP_SIZEOF, $3);
                  PKL_AST_LOC ($$) = @1;
                }
	| SIZEOF '(' simple_type_specifier ')' %prec HYPERUNARY
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
	| expression AS simple_type_specifier
        	{
                  $$ = pkl_ast_make_cast (pkl_parser->ast, $3, $1);
                  PKL_AST_LOC ($$) = @2;
                }
        | simple_type_specifier '@' expression
                {
                    $$ = pkl_ast_make_map (pkl_parser->ast, $1, $3);
                    PKL_AST_LOC ($$) = @$;
                }
        | UNIT
		{
                    $$ = pkl_ast_make_offset (pkl_parser->ast, NULL, $1);
                    PKL_AST_LOC ($1) = @1;
                    PKL_AST_LOC ($$) = @$;
                }
        | expression UNIT
        	{
                  $$ = pkl_ast_make_offset (pkl_parser->ast, $1, $2);
                  PKL_AST_LOC ($2) = @2;
                  PKL_AST_LOC ($$) = @$;
                }
   	| struct
        ;

unary_operator:
	  '-'		{ $$ = PKL_AST_OP_NEG; }
	| '+'		{ $$ = PKL_AST_OP_POS; }
	| '~'		{ $$ = PKL_AST_OP_BNOT; }
	| '!'		{ $$ = PKL_AST_OP_NOT; }
	;

primary:
          IDENTIFIER
          	{
                  /* Search for a variable definition in the
                     compile-time environment, and create a
                     PKL_AST_VAR node with it's lexical environment,
                     annotated with its initialization.  */

                  int back, over;
                  pkl_ast_node var_type;

                  const char *name = PKL_AST_IDENTIFIER_POINTER ($1);

                  pkl_ast_node decl
                    = pkl_env_lookup (pkl_parser->env,
                                      name, &back, &over);
                  if (!decl
                      || PKL_AST_DECL_KIND (decl) != PKL_AST_DECL_KIND_VAR)
                    {
                      pkl_error (pkl_parser->ast, @1,
                                 "undefined variable '%s'", name);
                      YYERROR;
                    }

                  $$ = pkl_ast_make_var (pkl_parser->ast,
                                         $1, /* name.  */
                                         PKL_AST_DECL_INITIAL (decl),
                                         back, over);
                  var_type = PKL_AST_TYPE (PKL_AST_DECL_INITIAL (decl));
                  PKL_AST_TYPE ($$) = ASTREF (var_type);
                  PKL_AST_LOC ($$) = @1;
                }
	| INTEGER
                {
                  $$ = $1;
                  PKL_AST_LOC ($$) = @$;
                  PKL_AST_LOC (PKL_AST_TYPE ($$)) = @$;
                }
        | CHAR
                {
                  $$ = $1;
                  PKL_AST_LOC ($$) = @$;
                  PKL_AST_LOC (PKL_AST_TYPE ($$)) = @$;
                }
        | STR
                {
                  $$ = $1;
                  PKL_AST_LOC ($$) = @$;
                  PKL_AST_LOC (PKL_AST_TYPE ($$)) = @$;
                } 
        | '(' expression ')'
        	{
                  $$ = $2;
                }
        | array
        | primary '.' identifier
		{
                    $$ = pkl_ast_make_struct_ref (pkl_parser->ast, $1, $3);
                    PKL_AST_LOC ($3) = @3;
                    PKL_AST_LOC ($$) = @$;
                }
        | primary '[' expression ']' %prec '.'
                {
                  $$ = pkl_ast_make_array_ref (pkl_parser->ast, $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
	| funcall
	;

funcall:
          primary '(' funcall_arg_list ')' %prec '.'
          	{
                  $$ = pkl_ast_make_funcall (pkl_parser->ast,
                                             $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
	;

funcall_arg_list:
	  %empty
		{ $$ = NULL; }
	| funcall_arg
        | funcall_arg_list ',' funcall_arg
        	{
                  $$ = pkl_ast_chainon ($1, $3);
                }
        ;

funcall_arg:
	   expression
          	{
                  $$ = pkl_ast_make_funcall_arg (pkl_parser->ast,
                                                 $1);
                  PKL_AST_LOC ($$) = @$;
                }
        ;

struct:
	  '{' struct_elem_list '}'
		{
                    $$ = pkl_ast_make_struct (pkl_parser->ast,
                                              0 /* nelem */, $2);
                    PKL_AST_LOC ($$) = @$;
                }
	;

struct_elem_list:
	  %empty
		{ $$ = NULL; }
        | struct_elem
        | struct_elem_list ',' struct_elem
		{
                  /* Note these are chained in reverse order!  */
                  $$ = pkl_ast_chainon ($3, $1);
                }
        ;

struct_elem:
	  expression
          	{
                    $$ = pkl_ast_make_struct_elem (pkl_parser->ast, NULL, $1);
                    PKL_AST_LOC ($$) = @$;
                }
        | '.' identifier '=' expression
	        {
                    $$ = pkl_ast_make_struct_elem (pkl_parser->ast, $2, $4);
                    PKL_AST_LOC ($2) = @2;
                    PKL_AST_LOC ($$) = @$;
                }
        ;

array:
	  '[' array_initializer_list ']'
        	{
                    $$ = pkl_ast_make_array (pkl_parser->ast,
                                             0 /* nelem */,
                                             0 /* ninitializer */,
                                             $2);
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
                    PKL_AST_LOC (PKL_AST_TYPE ($3)) = @3;
                    PKL_AST_LOC ($$) = @$;
                }
        ;

/*
 * Functions.
 */

function_specifier:
          '(' pushlevel function_arg_list ')' comp_stmt
          	{
                  $$ = pkl_ast_make_func (pkl_parser->ast,
                                          NULL /* ret_type */,
                                          $3, $5);
                  /* XXX: create a function type and set $$'s type
                     with it.  */
                  PKL_AST_LOC ($$) = @$;

                  /* Pop the frame introduced by `pushlevel'
                     above.  */
                  pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
                }
        | '(' pushlevel function_arg_list ')' ':' simple_type_specifier comp_stmt
        	{
                  $$ = pkl_ast_make_func (pkl_parser->ast,
                                          $6, $3, $7);
                  PKL_AST_LOC ($$) = @$;

                  /* Pop the frame introduced by `pushlevel'
                     above.  */
                  pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
                }
        ;

function_arg_list:
	  %empty
		{ $$ = NULL; }
	| function_arg
        | function_arg_list ',' function_arg
          	{
                  $$ = pkl_ast_chainon ($1, $3);
                }
        ;

function_arg:
	  simple_type_specifier identifier
          	{
                  $$ = pkl_ast_make_func_arg (pkl_parser->ast,
                                              $1, $2);
                  PKL_AST_LOC ($2) = @2;
                  PKL_AST_LOC ($$) = @$;

                  /* Register the argument in the current environment,
                     which is the function's environment pushed in
                     `function_specifier'.  */

                  pkl_ast_node arg_decl;

                  pkl_ast_node dummy
                    = pkl_ast_make_integer (pkl_parser->ast, 0);
                  PKL_AST_TYPE (dummy) = ASTREF ($1);
                  
                  arg_decl = pkl_ast_make_decl (pkl_parser->ast,
                                                PKL_AST_DECL_KIND_VAR,
                                                $2,
                                                dummy,
                                                NULL /* source */);
                  PKL_AST_LOC (arg_decl) = @$;
                  
                  if (!pkl_env_register (pkl_parser->env,
                                         PKL_AST_IDENTIFIER_POINTER ($2),
                                         arg_decl))
                    {
                      pkl_error (pkl_parser->ast, @2,
                                 "duplicated argument name `%s' in function declaration",
                                 PKL_AST_IDENTIFIER_POINTER ($2));
                      /* Make sure to pop the function frame.  */
                      pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
                      YYERROR;
                    }
                }
        ;

/*
 * Types.
 */

type_specifier:
	  simple_type_specifier
        | struct_type_specifier
        ;

simple_type_specifier:
	  TYPENAME
          	{
                  pkl_ast_node decl = pkl_env_lookup (pkl_parser->env,
                                                      PKL_AST_IDENTIFIER_POINTER ($1),
                                                      NULL, NULL);
                  assert (decl != NULL
                          && PKL_AST_DECL_KIND (decl) == PKL_AST_DECL_KIND_TYPE);
                  $$ = PKL_AST_DECL_INITIAL (decl);
                  PKL_AST_LOC ($$) = @$;
                }
        | STRING
        	{
                  $$ = pkl_ast_make_string_type (pkl_parser->ast);
                  PKL_AST_LOC ($$) = @$;
                }
        | INTCONSTR INTEGER '>'
                {
                    /* XXX: $3 can be any expression!.  */
                    $$ = pkl_ast_make_integral_type (pkl_parser->ast,
                                                     PKL_AST_INTEGER_VALUE ($2), 1 /* signed */);
                    ASTREF ($2); pkl_ast_node_free ($2);
                    PKL_AST_LOC ($$) = @$;
                }
        | UINTCONSTR INTEGER '>'
                {
                    /* XXX: $3 can be any expression!.  */
                    $$ = pkl_ast_make_integral_type (pkl_parser->ast,
                                                     PKL_AST_INTEGER_VALUE ($2), 0 /* signed */);
                    ASTREF ($2); pkl_ast_node_free ($2);
                    PKL_AST_LOC ($$) = @$;
                }
        | OFFSETCONSTR simple_type_specifier ',' IDENTIFIER '>'
                {
                    $$ = pkl_ast_make_offset_type (pkl_parser->ast,
                                                   $2, $4);
                    PKL_AST_LOC ($4) = @4;
                    PKL_AST_LOC ($$) = @$;
                }
        | OFFSETCONSTR simple_type_specifier ',' type_specifier '>'
                {
                    $$ = pkl_ast_make_offset_type (pkl_parser->ast,
                                                   $2, $4);
                    PKL_AST_LOC ($$) = @$;
                }

        | simple_type_specifier '[' expression ']'
          	{
                    $$ = pkl_ast_make_array_type (pkl_parser->ast, $3, $1);
                    PKL_AST_LOC ($$) = @$;
                }
	| simple_type_specifier '[' ']'
        	{
                    $$ = pkl_ast_make_array_type (pkl_parser->ast, NULL, $1);
                    PKL_AST_LOC ($$) = @$;
                }
        ;

struct_type_specifier:
	  pushlevel STRUCT '{' '}'
          	{
                    $$ = pkl_ast_make_struct_type (pkl_parser->ast, 0, NULL);
                    PKL_AST_LOC ($$) = @$;

                    /* The pushlevel in this rule and the subsequent
                       pop_frame, while not strictly needed, is to
                       avoid shift/reduce conflicts with the next
                       rule.  */
                    pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
                }
        | pushlevel STRUCT '{' struct_elem_type_list '}'
        	{
                    $$ = pkl_ast_make_struct_type (pkl_parser->ast, 0 /* nelem */, $4);
                    PKL_AST_LOC ($$) = @$;

                    /* XXX: pop N frames from the current environment,
                       where N is the number of declarations in
                       struct_elem_type_list.  */

                    /* Now pop the frame introduced by the struct type
                       definition itself.  */
                    pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
                }
        ;

struct_elem_type_list:
	  struct_elem_type
        | struct_elem_type_list struct_elem_type
        	{ $$ = pkl_ast_chainon ($1, $2); }
        ;

struct_elem_type:
	  type_specifier identifier ';'
          	{
                    $$ = pkl_ast_make_struct_elem_type (pkl_parser->ast, $2, $1);
                    PKL_AST_LOC ($$) = @$;
                    PKL_AST_LOC ($2) = @2;
                    PKL_AST_TYPE ($2) = pkl_ast_make_string_type (pkl_parser->ast);
                    ASTREF (PKL_AST_TYPE ($2));
                    PKL_AST_LOC (PKL_AST_TYPE ($2)) = @2;
                }
        | type_specifier ';'
        	{
                    $$ = pkl_ast_make_struct_elem_type (pkl_parser->ast, NULL, $1);
                    PKL_AST_LOC ($$) = @$;
                }
        ;

/*
 * Declarations.
 */

declaration:
        DEFUN identifier '=' function_specifier
        	{
                  $$ = pkl_ast_make_decl (pkl_parser->ast,
                                          PKL_AST_DECL_KIND_FUNC, $2, $4,
                                          pkl_parser->filename);
                  PKL_AST_LOC ($2) = @2;
                  PKL_AST_LOC ($$) = @$;

                  if (! pkl_env_toplevel_p (pkl_parser->env))
                    pkl_parser->env = pkl_env_push_frame (pkl_parser->env);

                  if (!pkl_env_register (pkl_parser->env,
                                         PKL_AST_IDENTIFIER_POINTER ($2),
                                         $$))
                    {
                      /* XXX: in the top-level, rename the old
                         declaration to "" and add the new one.  */
                      pkl_error (pkl_parser->ast, @2,
                                 "function or variable `%s' already defined",
                                 PKL_AST_IDENTIFIER_POINTER ($2));
                      /* XXX: also, annotate the decl to be renaming a
                         toplevel variable, so the code generator can
                         do the right thing: to generate a POPSETVAR
                         instruction instead of a POPVAR.  */
                      YYERROR;
                    }
                }
        | DEFVAR identifier '=' expression ';'
        	{
                  $$ = pkl_ast_make_decl (pkl_parser->ast,
                                          PKL_AST_DECL_KIND_VAR, $2, $4,
                                          pkl_parser->filename);
                  PKL_AST_LOC ($2) = @2;
                  PKL_AST_LOC ($$) = @$;

                  if (! pkl_env_toplevel_p (pkl_parser->env))
                    pkl_parser->env = pkl_env_push_frame (pkl_parser->env);


                  if (!pkl_env_register (pkl_parser->env,
                                         PKL_AST_IDENTIFIER_POINTER ($2),
                                         $$))
                    {
                      /* XXX: in the top-level, rename the old
                         declaration to "" and add the new one.  */
                      pkl_error (pkl_parser->ast, @2,
                                 "`%s' is already defined",
                                 PKL_AST_IDENTIFIER_POINTER ($2));
                      YYERROR;
                    }
                }
        | DEFTYPE identifier '=' type_specifier ';'
        	{
                  $$ = pkl_ast_make_decl (pkl_parser->ast,
                                          PKL_AST_DECL_KIND_TYPE, $2, $4,
                                          pkl_parser->filename);
                  PKL_AST_LOC ($2) = @2;
                  PKL_AST_LOC ($$) = @$;

                  if (! pkl_env_toplevel_p (pkl_parser->env))
                    pkl_parser->env = pkl_env_push_frame (pkl_parser->env);

                  if (!pkl_env_register (pkl_parser->env,
                                         PKL_AST_IDENTIFIER_POINTER ($2),
                                         $$))
                    {
                      /* XXX: in the top-level, rename the old
                         declaration to "" and add the new one.  */
                      pkl_error (pkl_parser->ast, @2,
                                 "`%s' is already defined",
                                 PKL_AST_IDENTIFIER_POINTER ($2));
                      YYERROR;
                    }
                }
/*
	| DEFTYPE TYPENAME '=' type_specifier ';'
        	{
                  if (pkl_env_toplevel_p (pkl_parser->env))
                      {
                          / * XXX: in the top-level, rename the old
                             declaration to "" and add the new one.  * /
                      }        

                  pkl_error (pkl_parser->ast, @2,
                             "type already defined");
                  $2 = $2; / * To avoid warning.  * /
                  $4 = $4; / * Likewise.  * /
                  $$ = $$; / * Likewise.  * /
                  YYERROR;
                }
*/
        ;

/*
 * Statements.
 */

comp_stmt:
	  pushlevel '{' stmt_decl_list '}'
        	{
                  pkl_ast_node stmt_decl;
                  
                  $$ = pkl_ast_make_comp_stmt (pkl_parser->ast, $3);
                  PKL_AST_LOC ($$) = @$;

                  /* Pop N frames from the current environment, where
                     N is the number of declarations in
                     stmt_decl_list.  */
                  for (stmt_decl = $3;
                       stmt_decl;
                       stmt_decl = PKL_AST_CHAIN (stmt_decl))
                    {
                      if (PKL_AST_CODE (stmt_decl) == PKL_AST_DECL)
                        pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
                    }

                  /* Now pop the frame introduced by the
                     compound-statement itself.  */
                  pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
                }
        ;

stmt_decl_list:
	  stmt
        | stmt_decl_list stmt
          	{ $$ = pkl_ast_chainon ($1, $2); }
        | declaration
        | stmt_decl_list declaration
          	{ $$ = pkl_ast_chainon ($1, $2); }
	;

stmt:
          comp_stmt
        | ';'
          	{
                  $$ = pkl_ast_make_null_stmt (pkl_parser->ast);
                  PKL_AST_LOC ($$) = @$;
                }
        | primary '=' expression ';'
          	{
                  $$ = pkl_ast_make_ass_stmt (pkl_parser->ast,
                                              $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | IF '(' expression ')' stmt %prec THEN
                {
                  $$ = pkl_ast_make_if_stmt (pkl_parser->ast,
                                             $3, $5, NULL);
                  PKL_AST_LOC ($$) = @$;
                }
        | IF '(' expression ')' stmt ELSE stmt %prec ELSE
                {
                  $$ = pkl_ast_make_if_stmt (pkl_parser->ast,
                                             $3, $5, $7);
                  PKL_AST_LOC ($$) = @$;
                }
        | RETURN expression ';'
                {
                  $$ = pkl_ast_make_return_stmt (pkl_parser->ast,
                                                 $2);
                  PKL_AST_LOC ($$) = @$;
                }
        | funcall ';'
        	{
                  $$ = pkl_ast_make_exp_stmt (pkl_parser->ast,
                                              $1);
                  PKL_AST_LOC ($$) = @$;
                }
        ;

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
