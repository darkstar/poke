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

%pure-parser
%name-prefix "pcl_tab_"
 /* %parse-param {pcl_parser_t pcl_parser} */
 /*%lex-param { void *scanner } */

%{
#include <config.h>
#include <stdarg.h>
#include <stdlib.h>
#include <xalloc.h>

#include <pcl.h>
#include "pcl-tab.h"

/* #define scanner (pcl_parser_scanner (pcl_parser)) */

#define PCL_AST_CHILDREN_STEP 12
  
static inline void
pcl_ast_add_child (struct pcl_ast *ast,
                   struct pcl_ast *child)
{
  if (ast->nchildren % PCL_AST_CHILDREN_STEP == 0)
    ast->children = xrealloc (ast->children,
                               (ast->nchildren + PCL_AST_CHILDREN_STEP)
                               * sizeof (ast));

  ast->children[ast->nchildren++] = child;
}

static inline struct pcl_ast *
pcl_ast_new (enum pcl_ast_type type, int nchildren, ...)
{
  va_list valist;
  struct pcl_ast *ast;

  ast = xmalloc (sizeof (struct pcl_ast));
  ast->nchildren = 0;
  ast->children = NULL;

  if (ast)
    {
      int i;
      
      ast->type = type;

      va_start (valist, nchildren);
      for (i = 0; i < nchildren; i++)
        pcl_ast_add_child (ast,
                           va_arg (valist, struct pcl_ast *));
      va_end (valist);
    }

  return ast;
};

void
pcl_tab_error (const char *err)
{
  /* Do nothing. */
}
 
%}

%token <ast> PCL_TOK_INT
%token <ast> PCL_TOK_STR
%token <ast> PCL_TOK_ID

%token <ast> PCL_TOK_ENUM
%token <ast> PCL_TOK_STRUCT
%token <ast> PCL_TOK_TYPEDEF

%token <ast> PCL_TOK_ERR

%type <ast> typedef
%type <ast> type

%union {
  struct pcl_ast *ast;
}

%% /* The grammar follows.  */

typedef: PCL_TOK_TYPEDEF type PCL_TOK_ID ';'
       {
         $$ = pcl_ast_new (PCL_AST_TYPEDEF, 2, $2, $3);
       }
       | PCL_TOK_TYPEDEF PCL_TOK_ID PCL_TOK_ID ';'
       {
         $$ = pcl_ast_new (PCL_AST_TYPEDEF, 2, $2, $3);
       }
       ;

type: PCL_TOK_INT ':' PCL_TOK_ID
    {
      $$ = pcl_ast_new (PCL_AST_TYPE, 2, $1, $3);
    }
    ;

%%
