/* -*- mode: C -*- */
/* pcl-lex.l - Lexer for the Poke Command Language.  */

/* Copyright (C) 2017 Jose E. Marchesi */

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

/* Automake needs this.  */
%option outfile="lex.yy.c"
%option prefix="pcl_tab_"
%option pointer
%option noyywrap
%option reentrant
%option bison-bridge
%option extra-type="void *"

%option header-file="pcl-lex.h"
%option nounput
%option noinput
%top {
   /* This code goes at the "top" of the generated file.  */
   #include <config.h>
}

%{
#include <string.h>
#include <xalloc.h>
#include <pcl.h>
#include <pcl-tab.h>

int
pcl_atoi (const char *str, int *number)
{
  int res;
  long int li;
  char *end;
  int base = 0;

  res = 0;

  li = strtol (str, &end, base);
  if ((*str != '\0') && (*end == '\0'))
    {
      /* The entire string is valid.  */
      res = 0;
      *number = (int) li;
    }

  return res;
}

%}

NEWLINE            \n
BLANK              [ \t\n]
DIGIT              [0-9]
LETTER             [a-zA-Z]
FIELD_NAME         {LETTER}[a-zA-Z0-9_]*
STRING             \"([^"]|\\(.|\n))*\"

%%

{BLANK}  { /* Ignore whitespace */ }

"-"?((0x[0-9a-fA-F]+)|{DIGIT}+) {
  /*  Create an integer node.  */
  int integer_value;

  yylval->node = xmalloc (sizeof (struct pcl_ast_node *));
  pcl_atoi (yytext, &integer_value);
  yylval->node->type = PCL_AST_INT;
  yylval->node->val.integer = integer_value;

  return PCL_TOK_INT;    
}

{STRING} {
  /* Create a string node.  */
  
  /* Strip the quoting characters */
  yytext[strlen(yytext) - 1] = 0;

  yylval->node = xmalloc (sizeof (struct pcl_ast_node *));
  yylval->node->type = PCL_AST_STR;
  yylval->node->val.str = yytext + 1;

  return PCL_TOK_STR;
}

enum    { return PCL_TOK_ENUM; }
struct  { return PCL_TOK_STRUCT; }
typedef { return PCL_TOK_TYPEDEF; }

. { return PCL_TOK_ERR; }

%%

