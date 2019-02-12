/* pkl-tab.y - LALR(1) parser for Poke.  */

/* Copyright (C) 2019 Jose E. Marchesi.  */

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

/* Register an argument in the compile-time environment.  This is used
   by function specifiers and try-catch statements.

   Return 0 if there was an error registering, 1 otherwise.  */

int
pkl_register_arg (struct pkl_parser *parser, pkl_ast_node arg)
{
  pkl_ast_node arg_decl;
  pkl_ast_node arg_identifier = PKL_AST_FUNC_ARG_IDENTIFIER (arg);

  pkl_ast_node dummy
    = pkl_ast_make_integer (parser->ast, 0);
  PKL_AST_TYPE (dummy) = ASTREF (PKL_AST_FUNC_ARG_TYPE (arg));
  
  arg_decl = pkl_ast_make_decl (parser->ast,
                                PKL_AST_DECL_KIND_VAR,
                                arg_identifier,
                                dummy,
                                NULL /* source */);
  PKL_AST_LOC (arg_decl) = PKL_AST_LOC (arg);
  
  if (!pkl_env_register (parser->env,
                         PKL_AST_IDENTIFIER_POINTER (arg_identifier),
                         arg_decl))
    {
      pkl_error (parser->ast, PKL_AST_LOC (arg_identifier),
                 "duplicated argument name `%s' in function declaration",
                 PKL_AST_IDENTIFIER_POINTER (arg_identifier));
      /* Make sure to pop the function frame.  */
      parser->env = pkl_env_pop_frame (parser->env);
      return 0;
    }

  return 1;
}

/* Register a list of arguments in the compile-time environment.  This
   is used by function specifiers and try-catch statements.

   Return 0 if there was an error registering, 1 otherwise.  */

int
pkl_register_args (struct pkl_parser *parser, pkl_ast_node arg_list)
{
  pkl_ast_node arg;

  for (arg = arg_list; arg; arg = PKL_AST_CHAIN (arg))
    {
      pkl_ast_node arg_decl;
      pkl_ast_node arg_identifier = PKL_AST_FUNC_ARG_IDENTIFIER (arg);

      pkl_ast_node dummy
        = pkl_ast_make_integer (parser->ast, 0);
      PKL_AST_TYPE (dummy) = ASTREF (PKL_AST_FUNC_ARG_TYPE (arg));

      arg_decl = pkl_ast_make_decl (parser->ast,
                                    PKL_AST_DECL_KIND_VAR,
                                    arg_identifier,
                                    dummy,
                                    NULL /* source */);
      PKL_AST_LOC (arg_decl) = PKL_AST_LOC (arg);

      if (!pkl_env_register (parser->env,
                             PKL_AST_IDENTIFIER_POINTER (arg_identifier),
                             arg_decl))
        {
          pkl_error (parser->ast, PKL_AST_LOC (arg_identifier),
                     "duplicated argument name `%s' in function declaration",
                     PKL_AST_IDENTIFIER_POINTER (arg_identifier));
          /* Make sure to pop the function frame.  */
          parser->env = pkl_env_pop_frame (parser->env);
          return 0;
        }
    }

  return 1;
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
          /*          if (PKL_AST_TYPE_CODE ($$) == PKL_TYPE_STRUCT)
                      pkl_parser->env = pkl_env_pop_frame (pkl_parser->env); */
          break;
        case PKL_AST_FUNC:
          if (PKL_AST_FUNC_ARGS ($$))
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
%token IF
%token WHILE
%token FOR
%token IN
%token WHERE
%token SIZEOF
%token ASSERT
%token ERR
%token INTCONSTR UINTCONSTR OFFSETCONSTR
%token DEFUN DEFSET DEFTYPE DEFVAR
%token RETURN
%token STRING
%token TRY CATCH RAISE
%token VOID
%token ANY
%token ISA
%token PRINT

/* ATTRIBUTE operator.  */

%token <ast> ATTR

/* Compiler builtins.  */

/* Opcodes.  */

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
%token THREEDOTS

/* This is for the dangling ELSE.  */

%precedence THEN
%precedence ELSE

/* Operator tokens and their precedences, in ascending order.  */

%right <ast> NARG
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
%left BCONC
%right '@'
%nonassoc UNIT
%right UNARY INC DEC AS ISA
%left HYPERUNARY
%left '.'
%left ATTR

%type <opcode> unary_operator

%type <ast> start program program_elem_list program_elem
%type <ast> expression primary identifier
%type <ast> funcall funcall_arg_list funcall_arg
%type <ast> array array_initializer_list array_initializer
%type <ast> struct struct_elem_list struct_elem
%type <ast> type_specifier simple_type_specifier
%type <ast> function_type_specifier function_type_arg_list function_type_arg
%type <ast> struct_type_specifier struct_elem_type_list struct_elem_type
%type <ast> declaration
%type <ast> function_specifier function_arg_list function_arg function_arg_initial
%type <ast> comp_stmt stmt_decl_list stmt
%type <ast> funcall_stmt funcall_stmt_arg_list funcall_stmt_arg

/* The following two tokens are used in order to support several start
   rules: one is for parsing an expression, declaration or sentence,
   and the other for parsing a full poke programs.  This trick is
   explained in the Bison Manual in the "Multiple start-symbols"
   section.  */

%token START_EXP START_DECL START_STMT START_PROGRAM;

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
	| START_STMT stmt
                {
                  $$ = pkl_ast_make_program (pkl_parser->ast, $2);
                  PKL_AST_LOC ($$) = @$;
                  pkl_parser->ast->ast = ASTREF ($$);
                }
	| START_STMT stmt ';'
                {
                  /* This rule is to allow the presence of an extra
                     ';' after the sentence.  This to allow the poke
                     command manager to ease the handlign of
                     semicolons in the command line.  */
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
	| SIZEOF '(' simple_type_specifier ')' %prec HYPERUNARY
        	{
                  $$ = pkl_ast_make_unary_exp (pkl_parser->ast, PKL_AST_OP_SIZEOF, $3);
                  PKL_AST_LOC ($$) = @1;
                }
        | expression ATTR
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_ATTR,
                                                $1, $2);
                  PKL_AST_LOC ($2) = @2;
                  PKL_AST_LOC ($$) = @$;
                }
        | expression '+' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_ADD,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression '-' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_SUB,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression '*' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_MUL,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression '/' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_DIV,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression '%' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_MOD,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
	| expression BCONC expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_BCONC,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression SL expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_SL,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression SR expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_SR,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression EQ expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_EQ,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
	| expression NE expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_NE,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression '<' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_LT,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression '>' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_GT,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression LE expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_LE,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
	| expression GE expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_GE,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression '|' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_IOR,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression '^' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_XOR,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
	| expression '&' expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_BAND,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression AND expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_AND,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
	| expression OR expression
        	{
                  $$ = pkl_ast_make_binary_exp (pkl_parser->ast, PKL_AST_OP_OR,
                                                $1, $3);
                  PKL_AST_LOC ($$) = @$;
                }
	| expression AS simple_type_specifier
        	{
                  $$ = pkl_ast_make_cast (pkl_parser->ast, $3, $1);
                  PKL_AST_LOC ($$) = @$;
                }
	| expression ISA simple_type_specifier
        	{
                  $$ = pkl_ast_make_isa (pkl_parser->ast, $3, $1);
                  PKL_AST_LOC ($$) = @$;
                }
        | simple_type_specifier '@' expression
                {
                    $$ = pkl_ast_make_map (pkl_parser->ast, $1, $3);
                    PKL_AST_LOC ($$) = @$;

                    /* If the type specifier is of a struct, then look
                       for the lexical address of its mapper function
                       in the compilation environment and set it in
                       the map AST node.  */

                    if (PKL_AST_TYPE_CODE ($1) == PKL_TYPE_STRUCT)
                      {
                        int mapper_back;
                        int mapper_over;
                        
                        const char *type_name
                          = PKL_AST_IDENTIFIER_POINTER (PKL_AST_TYPE_NAME ($1));
                          
                        char *mapper_name = xmalloc (strlen (type_name) +
                                                     strlen ("_pkl_mapper_") + 1);
                        
                        pkl_ast_node mapper_decl;

                        strcpy (mapper_name, "_pkl_mapper_");
                        strcat (mapper_name, type_name);
                        
                        mapper_decl = pkl_env_lookup (pkl_parser->env,
                                                      mapper_name,
                                                      &mapper_back, &mapper_over);
                        assert (mapper_decl);

                        PKL_AST_MAP_MAPPER_BACK($$) = mapper_back;
                        PKL_AST_MAP_MAPPER_OVER($$) = mapper_over;

                        free (mapper_name);
                      }
                }
	| TYPENAME struct
          	{
                  pkl_ast_node type;
                  pkl_ast_node constructor_decl;
                  const char *type_name;
                  char *constructor_name;
                  int constructor_back, constructor_over;

                  pkl_ast_node decl = pkl_env_lookup (pkl_parser->env,
                                                      PKL_AST_IDENTIFIER_POINTER ($1),
                                                      NULL, NULL);
                  assert (decl != NULL
                          && PKL_AST_DECL_KIND (decl) == PKL_AST_DECL_KIND_TYPE);

                  type = PKL_AST_DECL_INITIAL (decl);
                  if (PKL_AST_TYPE_CODE (type) != PKL_TYPE_STRUCT)
                    {
                      pkl_error (pkl_parser->ast, @1,
                                 "expected struct type in constructor");
                      YYERROR;
                    }
                  PKL_AST_TYPE_NAME (type) = ASTREF ($1);

                  type_name =
                    PKL_AST_IDENTIFIER_POINTER ($1);

                  constructor_name = xmalloc (strlen (type_name) +
                                              strlen ("_pkl_constructor_") + 1);
                  strcpy (constructor_name, "_pkl_constructor_");
                  strcat (constructor_name, type_name);

                  constructor_decl = pkl_env_lookup (pkl_parser->env,
                                                     constructor_name,
                                                     &constructor_back, &constructor_over);
                  assert (constructor_decl);

                  $$ = pkl_ast_make_scons (pkl_parser->ast,
                                           type,
                                           $2,
                                           constructor_back,
                                           constructor_over);
                  PKL_AST_LOC ($$) = @$;

                  free (constructor_name);
        	}
        | UNIT
		{
                    $$ = pkl_ast_make_offset (pkl_parser->ast, NULL, $1);
                    PKL_AST_LOC ($1) = @1;
                    if (PKL_AST_TYPE ($1))
                        PKL_AST_LOC (PKL_AST_TYPE ($1)) = @1;
                    PKL_AST_LOC ($$) = @$;
                }
        | expression UNIT
        	{
                    $$ = pkl_ast_make_offset (pkl_parser->ast, $1, $2);
                    PKL_AST_LOC ($2) = @2;
                    if (PKL_AST_TYPE ($2))
                        PKL_AST_LOC (PKL_AST_TYPE ($2)) = @2;
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

                  const char *name = PKL_AST_IDENTIFIER_POINTER ($1);

                  pkl_ast_node decl
                    = pkl_env_lookup (pkl_parser->env,
                                      name, &back, &over);
                  if (!decl
                      || (PKL_AST_DECL_KIND (decl) != PKL_AST_DECL_KIND_VAR
                          && PKL_AST_DECL_KIND (decl) != PKL_AST_DECL_KIND_FUNC))
                    {
                      pkl_error (pkl_parser->ast, @1,
                                 "undefined variable '%s'", name);
                      YYERROR;
                    }

                  $$ = pkl_ast_make_var (pkl_parser->ast,
                                         $1, /* name.  */
                                         decl,
                                         back, over);
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
                                                 $1, NULL /* name */);
                  PKL_AST_LOC ($$) = @$;
                }
        ;

struct:
	  STRUCT '{' struct_elem_list '}'
		{
                    $$ = pkl_ast_make_struct (pkl_parser->ast,
                                              0 /* nelem */, $3);
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
	  '(' pushlevel function_arg_list ')'
                {
                    /* Register the function arguments in the
                       compile-time environment.  This is done here
                       and not in the function_arg rule because it
                       should be done in reverse-order, because the
                       callee gets the arguments in a LIFO order.  */

                  if (!pkl_register_args (pkl_parser, $3))
                    YYERROR;
                }
          simple_type_specifier ':' comp_stmt
        	{
                  $$ = pkl_ast_make_func (pkl_parser->ast,
                                          $6, $3, $8);
                  PKL_AST_LOC ($$) = @$;

                  /* Pop the frame introduced by `pushlevel'
                     above.  */
                  pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
                }
	| simple_type_specifier ':' comp_stmt
        	{
                  $$ = pkl_ast_make_func (pkl_parser->ast,
                                          $1, NULL, $3);
                  PKL_AST_LOC ($$) = @$;
                }
        ;

function_arg_list:
          function_arg
        | function_arg ',' function_arg_list
          	{
                  /* Note this is in reverse order.  */
                  $$ = pkl_ast_chainon ($3, $1);
                }
        ;

function_arg:
	  simple_type_specifier identifier function_arg_initial
          	{
                  $$ = pkl_ast_make_func_arg (pkl_parser->ast,
                                              $1, $2, $3);
                  PKL_AST_LOC ($2) = @2;
                  PKL_AST_LOC ($$) = @$;

                  /* Note the arguments are registered in the lexical
                     environment in a mid-term action in the
                     `function_specifier' rule, not here.  */
                }
        ;

function_arg_initial:
	%empty			{ $$ = NULL; }
	| '=' expression	{ $$ = $2; }
      ;

/*
 * Types.
 */

type_specifier:
	  simple_type_specifier
        | struct_type_specifier
        | function_type_specifier
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
        | ANY
        	{
                  $$ = pkl_ast_make_any_type (pkl_parser->ast);
                  PKL_AST_LOC ($$) = @$;
                }
	| VOID
        	{
                  $$ = pkl_ast_make_void_type (pkl_parser->ast);
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
	| simple_type_specifier '[' ']' 
        	{
                  $$ = pkl_ast_make_array_type (pkl_parser->ast, $1);
                  PKL_AST_LOC ($$) = @$;
                }
	| simple_type_specifier '[' expression ']'
        	{
                    /* XXX fix conflicts */
                  $$ = pkl_ast_make_array_type (pkl_parser->ast, $1);
                  PKL_AST_TYPE_A_NELEM ($$) = ASTREF ($3);
                  PKL_AST_LOC ($$) = @$;
                }
        ;

function_type_specifier:
	   '(' function_type_arg_list ')' simple_type_specifier
        	{
                  $$ = pkl_ast_make_function_type (pkl_parser->ast,
                                                   $4, 0 /* narg */,
                                                   $2);
                  PKL_AST_LOC ($$) = @$;
                }
	;

function_type_arg_list:
	  %empty
		{
                  $$ = NULL;
                }
	|  function_type_arg
        |  function_type_arg ',' function_type_arg_list
		{
                  $$ = pkl_ast_chainon ($1, $3);
                }
	;

function_type_arg:
	  simple_type_specifier
          	{
                  $$ = pkl_ast_make_func_type_arg (pkl_parser->ast,
                                                   $1, NULL /* name */);
                  PKL_AST_LOC ($$) = @$;
                }
        | simple_type_specifier '?'
                {
                  $$ = pkl_ast_make_func_type_arg (pkl_parser->ast,
                                                   $1, NULL /* name */);
                  PKL_AST_LOC ($$) = @$;
                  PKL_AST_FUNC_TYPE_ARG_OPTIONAL ($$) = 1;
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

                    /* Pop the frame pushed in the `pushlevel' above.  */
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
        DEFUN identifier
                {
                  /* In order to allow for the function to be called
                     from within itself (recursive calls) we should
                     register a partial declaration in the
                     compile-time environment before processing the
                     `function_specifier' below.  */

                  $<ast>$ = pkl_ast_make_decl (pkl_parser->ast,
                                               PKL_AST_DECL_KIND_FUNC, $2,
                                               NULL /* initial */,
                                               pkl_parser->filename);
                  PKL_AST_LOC ($2) = @2;
                  PKL_AST_LOC ($<ast>$) = @$;

                  if (!pkl_env_register (pkl_parser->env,
                                         PKL_AST_IDENTIFIER_POINTER ($2),
                                         $<ast>$))
                    {
                      /* XXX: in the top-level, rename the old
                         declaration to "" and add the new one.  */
                      pkl_error (pkl_parser->ast, @2,
                                 "function or variable `%s' already defined",
                                 PKL_AST_IDENTIFIER_POINTER ($2));
                      /* XXX: also, annotate the decl to be renaming a
                         toplevel variable, so the code generator can
                         do the right thing: to generate a POPVAR
                         instruction instead of a REGVAR.  */
                      YYERROR;
                    }
                }
        '=' function_specifier
        	{
                  /* Complete the declaration registered above with
                     it's initial value, which is the specifier of the
                     function being defined.  */
                  PKL_AST_DECL_INITIAL ($<ast>3)
                    = ASTREF ($5);
                  $$ = $<ast>3;
                  
                  /* Annotate the contained RETURN statements with
                     their function and their lexical nest level
                     within the function.  */
                  pkl_ast_finish_returns ($5);

                  /* XXX: move to trans1.  */
                  PKL_AST_FUNC_NAME ($5)
                    = xstrdup (PKL_AST_IDENTIFIER_POINTER ($2));
                }
        | DEFVAR identifier '=' expression ';'
        	{
                  $$ = pkl_ast_make_decl (pkl_parser->ast,
                                          PKL_AST_DECL_KIND_VAR, $2, $4,
                                          pkl_parser->filename);
                  PKL_AST_LOC ($2) = @2;
                  PKL_AST_LOC ($$) = @$;

                  if (!pkl_env_register (pkl_parser->env,
                                         PKL_AST_IDENTIFIER_POINTER ($2),
                                         $$))
                    {
                      /* XXX: in the top-level, rename the old
                         declaration to "" and add the new one.  */
                      pkl_error (pkl_parser->ast, @2,
                                 "the variable `%s' is already defined",
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

                  PKL_AST_TYPE_NAME ($4) = ASTREF ($2);

                  if (!pkl_env_register (pkl_parser->env,
                                         PKL_AST_IDENTIFIER_POINTER ($2),
                                         $$))
                    {
                      /* XXX: in the top-level, rename the old
                         declaration to "" and add the new one.  */
                      pkl_error (pkl_parser->ast, @2,
                                 "the type `%s' is already defined",
                                 PKL_AST_IDENTIFIER_POINTER ($2));
                      YYERROR;
                    }

                  /* If the type is a struct, register the mapping and
                     the constructor functions.  In both cases, the
                     INITIAL of the declaration are the struct type
                     itself.  That is what will be used by subsequent
                     passes in the compiler to build these function
                     bodies.  */
                  if (PKL_AST_TYPE_CODE ($4) == PKL_TYPE_STRUCT)
                    {
                      const char *type_name = PKL_AST_IDENTIFIER_POINTER ($2);

                      char *mapper_name = xmalloc (strlen (type_name) +
                                                   strlen ("_pkl_mapper_") + 1);
                      char *constructor_name = xmalloc (strlen (type_name) +
                                                        strlen ("_pkl_constructor_") + 1);

                      
                      strcpy (mapper_name, "_pkl_mapper_");
                      strcat (mapper_name, type_name);
                      
                      strcpy (constructor_name, "_pkl_constructor_");
                      strcat (constructor_name, type_name);
                      
                      {
                        pkl_ast_node mapper_identifier
                          = pkl_ast_make_identifier (pkl_parser->ast, mapper_name);
                        pkl_ast_node constructor_identifier
                          = pkl_ast_make_identifier (pkl_parser->ast, constructor_name);

                        pkl_ast_node mapper_decl
                          = pkl_ast_make_decl (pkl_parser->ast,
                                               PKL_AST_DECL_KIND_FUNC,
                                               mapper_identifier,
                                               $4,
                                               pkl_parser->filename);
                      
                        pkl_ast_node constructor_decl
                          = pkl_ast_make_decl (pkl_parser->ast,
                                               PKL_AST_DECL_KIND_FUNC,
                                               constructor_identifier,
                                               $4,
                                               pkl_parser->filename);

                        if (!pkl_env_register (pkl_parser->env,
                                               mapper_name,
                                               mapper_decl))
                          /* This should never happen.  */
                          assert (0);

                        if (!pkl_env_register (pkl_parser->env,
                                               constructor_name,
                                               constructor_decl))
                          /* This should never happen.  */
                          assert (0);

                        free (mapper_name);
                        free (constructor_name);
                      }
                    }
                }
        ;

/*
 * Statements.
 */

comp_stmt:
	  pushlevel '{' '}'
            {
              $$ = pkl_ast_make_comp_stmt (pkl_parser->ast, NULL);
              PKL_AST_LOC ($$) = @$;

              /* Pop the frame pushed by the `pushlevel' above.  */
              pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
            }
         |  pushlevel '{' stmt_decl_list '}'
            {
              $$ = pkl_ast_make_comp_stmt (pkl_parser->ast, $3);
              PKL_AST_LOC ($$) = @$;
              
              /* Pop the frame pushed by the `pushlevel' above.  */
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
	| WHILE '(' expression ')' stmt
        	{
                  $$ = pkl_ast_make_loop_stmt (pkl_parser->ast,
                                               $3,   /* condition */
                                               NULL, /* iterator */
                                               NULL, /* container */
                                               $5);  /* body */
                  PKL_AST_LOC ($$) = @$;
                }
	| FOR '(' IDENTIFIER IN expression pushlevel
        	{
                  /* Push a new lexical level and register a variable
                     with name IDENTIFIER.  Note that the variable is
                     created with a dummy INITIAL, as there is none.  */

                  pkl_ast_node dummy = pkl_ast_make_integer (pkl_parser->ast,
                                                             0);
                  PKL_AST_LOC (dummy) = @3;

                  $<ast>$ = pkl_ast_make_decl (pkl_parser->ast,
                                               PKL_AST_DECL_KIND_VAR,
                                               $3,
                                               dummy,
                                               pkl_parser->filename);
                  PKL_AST_LOC ($<ast>$) = @3;

                  if (!pkl_env_register (pkl_parser->env,
                                         PKL_AST_IDENTIFIER_POINTER ($3),
                                         $<ast>$))
                    /* This should never happen.  */
                    assert (0);
                }
	  ')' stmt
        	{
                  $$ = pkl_ast_make_loop_stmt (pkl_parser->ast,
                                               NULL, /* condition */
                                               $<ast>7, /* iterator */
                                               $5,   /* container */
                                               $9);  /* body */
                  PKL_AST_LOC ($$) = @$;

                  /* Free the identifier.  */
                  ASTREF ($3); pkl_ast_node_free ($3);

                  /* Pop the frame introduced by `pushlevel'
                     above.  */
                  pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
                }
	| FOR '(' IDENTIFIER IN expression pushlevel
        	{
                  /* XXX: avoid code replication here.  */

                  /* Push a new lexical level and register a variable
                     with name IDENTIFIER.  Note that the variable is
                     created with a dummy INITIAL, as there is none.  */

                  pkl_ast_node dummy = pkl_ast_make_integer (pkl_parser->ast,
                                                             0);
                  PKL_AST_LOC (dummy) = @3;
                  
                  $<ast>$ = pkl_ast_make_decl (pkl_parser->ast,
                                               PKL_AST_DECL_KIND_VAR,
                                               $3,
                                               dummy,
                                               pkl_parser->filename);
                  PKL_AST_LOC ($<ast>$) = @3;
                  
                  if (!pkl_env_register (pkl_parser->env,
                                         PKL_AST_IDENTIFIER_POINTER ($3),
                                         $<ast>$))
                    /* This should never happen.  */
                    assert (0);
                }
	  WHERE expression ')' stmt
        	{
                  $$ = pkl_ast_make_loop_stmt (pkl_parser->ast,
                                               $9,   /* condition */
                                               $<ast>7,   /* iterator */
                                               $5,   /* container */
                                               $11); /* body */
                  PKL_AST_LOC ($3) = @3;
                  PKL_AST_LOC ($$) = @$;

                  /* Pop the frame introduced by `pushlevel'
                     above.  */
                  pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
                }
        | RETURN ';'
        	{
                  $$ = pkl_ast_make_return_stmt (pkl_parser->ast,
                                                 NULL);
                  PKL_AST_LOC ($$) = @$;
                }
        | RETURN expression ';'
                {
                  $$ = pkl_ast_make_return_stmt (pkl_parser->ast,
                                                 $2);
                  PKL_AST_LOC ($$) = @$;
                }
        | TRY stmt CATCH comp_stmt
        	{
                  $$ = pkl_ast_make_try_catch_stmt (pkl_parser->ast,
                                                    $2, $4, NULL, NULL);
                  PKL_AST_LOC ($$) = @$;
                }
	| TRY stmt CATCH IF expression comp_stmt
        	{
                  $$ = pkl_ast_make_try_catch_stmt (pkl_parser->ast,
                                                    $2, $6, NULL, $5);
                  PKL_AST_LOC ($$) = @$;
                }
	| TRY stmt CATCH  '(' pushlevel function_arg
        	{
                  /* Register the argument in the lexical environment.  */
                  if (!pkl_register_args (pkl_parser, $6))
                    YYERROR;
                }
	  ')' comp_stmt
		{
                  $$ = pkl_ast_make_try_catch_stmt (pkl_parser->ast,
                                                    $2, $9, $6, NULL);
                  PKL_AST_LOC ($$) = @$;

                  /* Pop the frame introduced by `pushlevel'
                     above.  */
                  pkl_parser->env = pkl_env_pop_frame (pkl_parser->env);
                }
	| RAISE ';'
        	{
                  $$ = pkl_ast_make_raise_stmt (pkl_parser->ast,
                                                NULL);
                  PKL_AST_LOC ($$) = @$;
                }
	| RAISE expression ';'
        	{
                  $$ = pkl_ast_make_raise_stmt (pkl_parser->ast,
                                                $2);
                  PKL_AST_LOC ($$) = @$;
                }
        | expression ';'
        	{
                  $$ = pkl_ast_make_exp_stmt (pkl_parser->ast,
                                              $1);
                  PKL_AST_LOC ($$) = @$;
                }
	| PRINT ';'
                {
                  $$ = pkl_ast_make_print_stmt (pkl_parser->ast,
                                                NULL);
                  PKL_AST_LOC ($$) = @$;
                }
        | PRINT expression ';'
        	{
                  $$ = pkl_ast_make_print_stmt (pkl_parser->ast,
                                                $2);
                  PKL_AST_LOC ($$) = @$;
                }
	| funcall_stmt ';'
        	{
                  $$ = pkl_ast_make_exp_stmt (pkl_parser->ast,
                                              $1);
                  PKL_AST_LOC ($$) = @$;
                }
        ;

funcall_stmt:
	primary funcall_stmt_arg_list
        	{
                  $$ = pkl_ast_make_funcall (pkl_parser->ast,
                                             $1, $2);
                  PKL_AST_LOC ($$) = @$;
                }
	;

funcall_stmt_arg_list:
	  funcall_stmt_arg
        | funcall_stmt_arg_list funcall_stmt_arg
        	{
                  $$ = pkl_ast_chainon ($1, $2);
                }
        ;

funcall_stmt_arg:
	  NARG expression
          	{
                  $$ = pkl_ast_make_funcall_arg (pkl_parser->ast,
                                                 $2, $1);
                  PKL_AST_LOC ($1) = @1;
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
