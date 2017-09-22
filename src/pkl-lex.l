/* -*- mode: C -*- */
/* pkl-lex.l - Lexer for the Poke Command Language.  */

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
%option prefix="pkl_tab_"
%option warn nodefault
%option pointer
%option noyywrap
%option reentrant
%option bison-bridge
%option header-file="pkl-lex.h"
%option bison-locations
%option yylineno
%option extra-type="struct pkl_parser *"

%top {
   /* This code goes at the "top" of the generated file.  */
   #include <config.h>
}

%{
#include <string.h>
#include <unistd.h>
#include <xalloc.h>
#include <pkl-ast.h>
#include <pkl-parser.h>
#include <pkl-tab.h>
#include "readline.h"
#if defined HAVE_READLINE_HISTORY_H
# include <readline/history.h>
#endif

#define YY_USER_ACTION                                          \
  do                                                            \
    {                                                           \
    yylloc->first_line = yylloc->last_line = yylineno;          \
    if (!isspace (*yytext))                                     \
      {                                                         \
        yyextra->at_start = 0;                                  \
      }                                                         \
 } while (0);

#define YY_INPUT(buf,result,max_size) \
  result = pkl_read (yyin, buf, max_size, yyextra);
  
size_t
pkl_read (FILE *file, char *buf, size_t max,
          struct pkl_parser *parser)
{
  size_t len;
  
  /* If yyin == NULL then we are interactive.  */
  if (file == NULL || isatty (fileno (file)))
    {
      char *line;

      if (parser->at_end)
        return YY_NULL;
      
      line = readline (parser->at_start ? parser->ps1 : parser->ps2);

      if (line == NULL
          || strncmp (line, ":exit", strlen (":exit")) == 0)
        exit (1); /* XXX: this should probably be a longjmp.  */

      if (strncmp (line, ":hello", strlen (":hello")) == 0)
        {
          printf ("hello!\n");
          return YY_NULL;
        }

#if defined HAVE_READLINE_HISTORY_H
      if (line && *line)
        add_history (line);
#endif

      len = strlen (line);
      if (len > max - 2)
        len = max - 2;

      memcpy(buf, line, len);
      buf[len++] = '\n';
      free (line);

      /* If the string is empty and we are continuing a command,
         return a newline.  We assume that `max' is at least 1.  */
      if (!parser->at_start && len == 0)
        {
          buf[len] = '\n';
          len++;
        }
      
      return len;
    }
  else
    {
      /* Non interactive.  */
      len = fread (buf, 1, max, file);
      if (len == 0 && ferror (file))
        {
          /* TODO: propagate error value.  */
        }
      return len;
    }
}

/* The following macro is used in the lexer rules below, to handle
   continuation prompts while operating in interactive mode.  */

#define AFTER_END                               \
  do                                            \
    {                                           \
      yyextra->at_end = 0;                      \
    } while (0)
 
/* Convert the printed representation of a number to an integer.  */
  
static int
pkl_atoi (const char *str, int *number)
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

static pkl_ast_node cur_docstr;
 
%}

NEWLINE            \n
BLANK              [ \t\n]
DIGIT              [0-9]
LETTER             [a-zA-Z]
FIELD_NAME         {LETTER}[a-zA-Z0-9_]*
STRING             \"([^"]|\\(.|\n))*\"

L [a-zA-Z_]
D [0-9]

%x C_COMMENT C_DOCSTR

%%

{BLANK}		{ /* Ignore whitespace.  */ }

"/*" { BEGIN(C_COMMENT); }          
<C_COMMENT>"*/" { BEGIN(INITIAL); } 
<C_COMMENT>. { }                    
<C_COMMENT>\n { }                   

"<<<" 		{ BEGIN(C_DOCSTR); }
<C_DOCSTR>">>>" {
     BEGIN(INITIAL);
     yylval->ast = cur_docstr;
     return DOCSTR;
}
<C_DOCSTR>([^>]|>[^>]|>>[^>])* {
     cur_docstr = pkl_ast_make_doc_string (yytext, NULL);
}      

"enum"		{ AFTER_END; return ENUM; }
"struct"	{ AFTER_END; return STRUCT; }
"typedef"	{ AFTER_END; return TYPEDEF; }
"break"		{ AFTER_END; return BREAK; }
"const"		{ AFTER_END; return CONST; }
"continue"	{ AFTER_END; return CONTINUE; }
"else"		{ AFTER_END; return ELSE; }
"for"		{ AFTER_END; return FOR; }
"while"		{ AFTER_END; return WHILE; }
"if"		{ AFTER_END; return IF; }
"sizeof"	{ AFTER_END; return SIZEOF; }
"assert"	{ AFTER_END; return ASSERT; }

"unsigned"	{ AFTER_END; yylval->integer = 0; return UNSIGNED; }
"signed"	{ AFTER_END; yylval->integer = 1; return SIGNED; }

"lsb"		{ AFTER_END; return LSB; }
"msb"		{ AFTER_END; return MSB; }

"*="		{ AFTER_END; return MULA; }
"/="		{ AFTER_END; return DIVA; }
"%="		{ AFTER_END; return MODA; }
"+="		{ AFTER_END; return ADDA; }
"-="		{ AFTER_END; return SUBA; }
"<<="		{ AFTER_END; return SLA; }
">>="		{ AFTER_END; return SRA; }
"&="		{ AFTER_END; return BANDA; }
"|="		{ AFTER_END; return IORA; }
"^="		{ AFTER_END; return XORA; }

"||"		{ AFTER_END; return OR; }
"&&"		{ AFTER_END; return AND; }
"=="		{ AFTER_END; return EQ; }
"!="		{ AFTER_END; return NE; }
"<="		{ AFTER_END; return LE; }
">="		{ AFTER_END; return GE; }
"<<"		{ AFTER_END; return SL; }
">>"		{ AFTER_END; return SR; }
"++"		{ AFTER_END; return INC; }
"--"		{ AFTER_END; return DEC; }

"["		{ AFTER_END; return '['; }
"]"		{ AFTER_END; return ']'; }
"("		{ AFTER_END; return '('; }
")"		{ AFTER_END; return ')'; }
"{"		{ AFTER_END; return '{'; }
"}"		{ AFTER_END; return '}'; }
","		{ AFTER_END; return ','; }
"="		{ AFTER_END; return '='; }
"?"		{ AFTER_END; return '?'; }
":"		{ AFTER_END; return ':'; }
";"		{ AFTER_END; return ';'; }
"|"		{ AFTER_END; return '|'; }
"^"		{ AFTER_END; return '^'; }
"&"		{ AFTER_END; return '&'; }
"<"		{ AFTER_END; return '<'; }
">"		{ AFTER_END; return '>'; }
"+"		{ AFTER_END; return '+'; }
"-"		{ AFTER_END; return '-'; }
"*"		{ AFTER_END; return '*'; }
"/"		{ AFTER_END; return '/'; }
"%"		{ AFTER_END; return '%'; }
"!"		{ AFTER_END; return '!'; }
"~"		{ AFTER_END; return '~'; }
"."		{ AFTER_END; return '.'; }

{L}({L}|{D})* {
   pkl_ast_node type;

   AFTER_END;

   if ((type = pkl_ast_get_registered (yyextra->ast, 
                                       yytext, PKL_AST_TYPE)) != NULL)
     {
       yylval->ast = type;
       return TYPENAME;
     }
   else
     {
       yylval->ast = pkl_ast_get_identifier (yyextra->ast, yytext);
       return IDENTIFIER;
     }
}

"-"?((0x[0-9a-fA-F]+)|{DIGIT}+) {
  int integer_value;

  AFTER_END;

  pkl_atoi (yytext, &integer_value);

  yylval->ast = pkl_ast_make_integer (integer_value);
  return INTEGER;    
}

{STRING} {
  /* Strip the quoting characters */
  yytext[strlen(yytext) - 1] = 0;

  AFTER_END;

  yylval->ast = pkl_ast_make_string (yytext + 1);
  return STR;
}

. { return ERR; }

%%
