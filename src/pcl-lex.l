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
%option warn nodefault
%option pointer
%option noyywrap
%option reentrant
%option bison-bridge
%option header-file="pcl-lex.h"
%option bison-locations
%option yylineno   

%top {
   /* This code goes at the "top" of the generated file.  */
   #include <config.h>
}

%{
#include <string.h>
#include <xalloc.h>
#include <pcl-ast.h>
#include <pcl-tab.h>

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno;

/* Convert the printed representation of a number to an integer.  */
  
static int
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

/* Get a docstring literal, remove delimiters, format it according to
   the rules documented below, and return it on a freshly allocated
   string.  */

static char *
make_docstring (const char *str)
{
  /* Ok, these are the rules for docstrings:

   - Docstrings are delimited by <<< and >>> sequences.
   - Docstrings can't be nested.
   - Newlines can be part of docstrings.
   - Blank lines immediately following <<< are removed.
   - Black lines immediately preceding >>> are removed.
   - Any indentation in the first line of the docstring is removed.
   - An uniform amount of indentation from the second and further
     lines of a docstring, equal to the minimum indentation of all
     non-blank lines after the first line, is removed.
   - Relative indentation of the second and later lines in the
     docstring is retained.
   
     Note STR is guaranteed to be a properly formatted docstring.
   */


  size_t len = strlen (str) - 6 + 1;
  char *dstring = xmalloc (len);
  
  strncpy (dstring, str + 3, len - 1);
  dstring [len] = '\0';

  return dstring;
}

 
%}

NEWLINE            \n
BLANK              [ \t\n]
DIGIT              [0-9]
LETTER             [a-zA-Z]
FIELD_NAME         {LETTER}[a-zA-Z0-9_]*
STRING             \"([^"]|\\(.|\n))*\"
DOCSTR		   <<<.*>>>

L [a-zA-Z_]
D [0-9]

%%

{BLANK}		{ /* Ignore whitespace.  */ }

"enum"		{ return PCL_TOK_ENUM; }
"struct"	{ return PCL_TOK_STRUCT; }
"typedef"	{ return PCL_TOK_TYPEDEF; }
"break"		{ return PCL_TOK_BREAK; }
"const"		{ return PCL_TOK_CONST; }
"continue"	{ return PCL_TOK_CONTINUE; }
"else"		{ return PCL_TOK_ELSE; }
"for"		{ return PCL_TOK_FOR; }
"if"		{ return PCL_TOK_IF; }
"sizeof"	{ return PCL_TOK_SIZEOF; }

"lsb"		{ return PCL_TOK_LSB; }
"msb"		{ return PCL_TOK_MSB; }

{DOCSTR} {
   char *dstring = make_docstring (yytext);

   yylval->ast = pcl_ast_make_doc_string (dstring, NULL);
   free (dstring);
   return PCL_TOK_DOCSTR;
}

"*="		{ return PCL_TOK_MULA; }
"/="		{ return PCL_TOK_DIVA; }
"%="		{ return PCL_TOK_MODA; }
"+="		{ return PCL_TOK_ADDA; }
"-="		{ return PCL_TOK_SUBA; }
"<<="		{ return PCL_TOK_SLA; }
">>="		{ return PCL_TOK_SRA; }
"&="		{ return PCL_TOK_BANDA; }
"|="		{ return PCL_TOK_IORA; }
"^="		{ return PCL_TOK_XORA; }

"||"		{ return PCL_TOK_OR; }
"&&"		{ return PCL_TOK_AND; }
"=="		{ return PCL_TOK_EQ; }
"!="		{ return PCL_TOK_NE; }
"<="		{ return PCL_TOK_LE; }
">="		{ return PCL_TOK_GE; }
"<<"		{ return PCL_TOK_SL; }
">>"		{ return PCL_TOK_SR; }
"++"		{ return PCL_TOK_INC; }
"--"		{ return PCL_TOK_DEC; }

"["		{ return '['; }
"]"		{ return ']'; }
"("		{ return '('; }
")"		{ return ')'; }
"{"		{ return '{'; }
"}"		{ return '}'; }
","		{ return ','; }
"="		{ return '='; }
"?"		{ return '?'; }
":"		{ return ':'; }
";"		{ return ';'; }
"|"		{ return '|'; }
"^"		{ return '^'; }
"&"		{ return '&'; }
"<"		{ return '<'; }
">"		{ return '>'; }
"+"		{ return '+'; }
"-"		{ return '-'; }
"*"		{ return '*'; }
"/"		{ return '/'; }
"%"		{ return '%'; }
"!"		{ return '!'; }
"~"		{ return '~'; }
"."		{ return '.'; }

"s"		{ return 's'; }
"u"		{ return 'u'; }

{L}({L}|{D})* {
   pcl_ast type;

   if ((type = pcl_ast_get_type (yytext)) != NULL)
     {
       yylval->ast = type;
       return PCL_TOK_TYPENAME;
     }
   else
     {
       yylval->ast = pcl_ast_get_identifier (yytext);
       return PCL_TOK_ID;
     }
}

"-"?((0x[0-9a-fA-F]+)|{DIGIT}+) {
  int integer_value;

  pcl_atoi (yytext, &integer_value);

  yylval->ast = pcl_ast_make_integer (integer_value);
  return PCL_TOK_INT;    
}

{STRING} {
  /* Strip the quoting characters */
  yytext[strlen(yytext) - 1] = 0;

  yylval->ast = pcl_ast_make_string (yytext + 1);
  return PCL_TOK_STR;
}

. { return PCL_TOK_ERR; }

%%
