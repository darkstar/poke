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

static pcl_ast cur_docstr;
 
%}

NEWLINE            \n
BLANK              [ \t\n]
DIGIT              [0-9]
LETTER             [a-zA-Z]
FIELD_NAME         {LETTER}[a-zA-Z0-9_]*
STRING             \"([^"]|\\(.|\n))*\"

L [a-zA-Z_]
D [0-9]

%x C_COMMENT C_DOCSTR C_INCLUDE

%%

{BLANK}		{ /* Ignore whitespace.  */ }

"/*" { BEGIN(C_COMMENT); }          
<C_COMMENT>"*/" { BEGIN(INITIAL); } 
<C_COMMENT>. { }                    
<C_COMMENT>\n { }                   

"<<<" { BEGIN(C_DOCSTR); }
<C_DOCSTR>">>>" {
     BEGIN(INITIAL);
     yylval->ast = cur_docstr;
     return DOCSTR;
}
<C_DOCSTR>([^>]|>[^>]|>>[^>])* {
     cur_docstr = pcl_ast_make_doc_string (yytext, NULL);
}      

"enum"		{ return ENUM; }
"struct"	{ return STRUCT; }
"typedef"	{ return TYPEDEF; }
"break"		{ return BREAK; }
"const"		{ return CONST; }
"continue"	{ return CONTINUE; }
"else"		{ return ELSE; }
"for"		{ return FOR; }
"while"		{ return WHILE; }
"if"		{ return IF; }
"sizeof"	{ return SIZEOF; }
"assert"	{ return ASSERT; }

"unsigned"	{ yylval->integer = 0; return UNSIGNED; }
"signed"	{ yylval->integer = 1; return SIGNED; }

"char"		{ return CHAR; }
"short"		{ return SHORT; }
"int"		{ return INT; }
"long"		{ return LONG; }

"lsb"		{ return LSB; }
"msb"		{ return MSB; }

"*="		{ return MULA; }
"/="		{ return DIVA; }
"%="		{ return MODA; }
"+="		{ return ADDA; }
"-="		{ return SUBA; }
"<<="		{ return SLA; }
">>="		{ return SRA; }
"&="		{ return BANDA; }
"|="		{ return IORA; }
"^="		{ return XORA; }

"||"		{ return OR; }
"&&"		{ return AND; }
"=="		{ return EQ; }
"!="		{ return NE; }
"<="		{ return LE; }
">="		{ return GE; }
"<<"		{ return SL; }
">>"		{ return SR; }
"++"		{ return INC; }
"--"		{ return DEC; }

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

{L}({L}|{D})* {
   pcl_ast type;

   if ((type = pcl_ast_get_type (yytext)) != NULL)
     {
       yylval->ast = type;
       return TYPENAME;
     }
   else
     {
       yylval->ast = pcl_ast_get_identifier (yytext);
       return IDENTIFIER;
     }
}

"-"?((0x[0-9a-fA-F]+)|{DIGIT}+) {
  int integer_value;

  pcl_atoi (yytext, &integer_value);

  yylval->ast = pcl_ast_make_integer (integer_value);
  return INTEGER;    
}

{STRING} {
  /* Strip the quoting characters */
  yytext[strlen(yytext) - 1] = 0;

  yylval->ast = pcl_ast_make_string (yytext + 1);
  return STR;
}

. { return ERR; }

%%
