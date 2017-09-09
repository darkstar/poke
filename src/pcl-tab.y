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
%name-prefix="pcl"
%parse-param {pcl_parser_t pcl_parser}
%lex-param { void *scanner }

%{
#include <config.h>
#include <stdarg.h>

#include <pcl.h>
#include "pcl-tab.h"

#define scanner (pcl_parser_scanner (pcl_parser))

/* The following function builds nodes and links them in the AST.  */

static inline struct pcl_ast_node *
pcl_ast_node (enum pcl_ast_node_type type,
              int nchildren,
              ...)
{
  va_list children;
  struct pcl_ast_node *node;

  node = pcl_ast_node_new ();

  if (node)
    {
      int i;
      
      pcl_ast_node_set_type (node, type);

      va_start (children, nchildren);
      for (i = 0; i < nchildren; i++)
        {
          pcl_ast_link (node, va_arg (children, i));
        }
      va_end (children);
    }

  return node;
};
  
%}

%token <node> PCL_TOK_INT
%token <node> PCL_TOK_STRING
%token <node> PCL_TOK_ID
%token <node> REC_SEX_TOK_ERR

%union {
  struct pcl_ast_node *node;
}

%% /* The grammar follows.  */

typedef: 'typedef' type PCL_TOK_ID opt';'
       {
         $$ = pcl_ast_node (NODE_TYPEDEF, 2, $2, $3);
       }
       | 'typedef' PCL_TOK_ID PCL_TOK_ID ';'
       {
         $$ = pcl_ast_node (NODE_TYPEDEF, 2, $2, $3); }
       }
       ;

type: PCL_TOK_INT ':' PCL_TOK_ID
    {
      $$ = pcl_ast_node (PCL_AST_TYPE, 2, $1, $3));
    }
    ;

%%
