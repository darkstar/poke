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
pcl_ast_add_child (struct pcl_ast_node *node,
                   struct pcl_ast_node *child)
{
  if (node->nchildren % PCL_AST_CHILDREN_STEP == 0)
    node->children = xrealloc (node->children,
                               (node->nchildren + PCL_AST_CHILDREN_STEP)
                               * sizeof (node));

  node->children[node->nchildren++] = child;
}

static inline struct pcl_ast_node *
pcl_ast_node (enum pcl_ast_node_type type,
              int nchildren,
              ...)
{
  va_list valist;
  struct pcl_ast_node *node;

  node = xmalloc (sizeof (struct pcl_ast_node));
  node->nchildren = 0;
  node->children = NULL;

  if (node)
    {
      int i;
      
      node->type = type;

      va_start (valist, nchildren);
      for (i = 0; i < nchildren; i++)
        pcl_ast_add_child (node,
                           va_arg (valist, struct pcl_ast_node *));
      va_end (valist);
    }

  return node;
};

void
pcl_tab_error (const char *err)
{
  /* Do nothing. */
}
 
%}

%token <node> PCL_TOK_INT
%token <node> PCL_TOK_STR
%token <node> PCL_TOK_ID

%token <node> PCL_TOK_ENUM
%token <node> PCL_TOK_STRUCT
%token <node> PCL_TOK_TYPEDEF

%token <node> PCL_TOK_ERR

%type <node> typedef
%type <node> type

%union {
  struct pcl_ast_node *node;
  int keyword;
}

%% /* The grammar follows.  */

typedef: PCL_TOK_TYPEDEF type PCL_TOK_ID ';'
       {
         $$ = pcl_ast_node (PCL_AST_TYPEDEF, 2, $2, $3);
       }
       | PCL_TOK_TYPEDEF PCL_TOK_ID PCL_TOK_ID ';'
       {
         $$ = pcl_ast_node (PCL_AST_TYPEDEF, 2, $2, $3);
       }
       ;

type: PCL_TOK_INT ':' PCL_TOK_ID
    {
      $$ = pcl_ast_node (PCL_AST_TYPE, 2, $1, $3);
    }
    ;

%%
