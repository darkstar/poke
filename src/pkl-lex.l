/* pkl-lex.l - Lexer for Poke.  */

/* Copyright (C) 2018 Jose E. Marchesi */

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
%option noinput
%option nounput
   /* %option yylineno */
%option extra-type="struct pkl_parser *"

%top {
   /* This code goes at the "top" of the generated file.  */
   #include <config.h>
}

%{
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <xalloc.h>
#include <assert.h>
#include <pkl-ast.h>
#include <pkl-parser.h>
#define YYLTYPE pkl_ast_loc
#define YYDEBUG 1
#include <pkl-tab.h>
#include "pk-cmd.h"
#include "readline.h"
#if defined HAVE_READLINE_HISTORY_H
# include <readline/history.h>
#endif

#define YY_USER_ACTION                                  \
  do                                                    \
    {                                                   \
     int i;                                             \
                                                        \
     yylloc->first_line = yylloc->last_line;            \
     yylloc->first_column = yylloc->last_column;        \
                                                        \
     for(i = 0; yytext[i] != '\0'; i++)                 \
       {                                                \
         if(yytext[i] == '\n')                          \
           {                                            \
             yylloc->last_line++;                       \
             yylloc->last_column = 0;                   \
           }                                            \
         else                                           \
           yylloc->last_column++;                       \
       }                                                \
                                                        \
     yyextra->nchars += yyleng;                         \
    } while (0);

%}

NEWLINE            \n
BLANK              [ \t]
LETTER             [a-zA-Z]
FIELD_NAME         {LETTER}[a-zA-Z0-9_]*
CHAR		   '(.|\\[n0t])'
STRING             \"([^"]|\\(.|\n))*\"
HEXCST		   0[xX][0-9a-fA-F][0-9a-fA-F_]*
BINCST		   0[bB][01][01_]*
OCTCST             0[0-9][0-9_]*
DECCST             [0-9][0-9_]*
IS		   ((u|U)|(u|U)?(l|L|b|B|h|H)|(l|L|b|B|h|H)(u|U))

L [a-zA-Z_]
D [0-9]

%x C_COMMENT

%%

{NEWLINE}	{ /* Ignore newlines.  */  }
{BLANK}		{ /* Ignore whitespace.  */ }

"/*" { BEGIN(C_COMMENT); }          
<C_COMMENT>"*/" { BEGIN(INITIAL); } 
<C_COMMENT>.    {  }                    
<C_COMMENT>\n   {  }

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

"lsb"		{ return LSB; }
"msb"		{ return MSB; }

"uint<"         { return UINTCONSTR; }
"int<"          { return INTCONSTR; }
"offset<"       { return OFFSETCONSTR; }

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
"@"		{ return '@'; }

{L}({L}|{D})* {
   pkl_ast_node type;

   if ((type = pkl_ast_get_registered (yyextra->ast, 
                                       yytext, PKL_AST_TYPE)) != NULL)
     {
       yylval->ast = type;
       return TYPENAME;
     }
   else
     {
       yylval->ast = pkl_ast_make_identifier (yyextra->ast, yytext);
       return IDENTIFIER;
     }
}

({HEXCST}|{BINCST}|{OCTCST}|{DECCST}){IS}? {
  uint64_t value;
  int base, offset, signed_p, width;
  char *p, *end;
  pkl_ast_node type;

  /* First of all, strip any '_' character from yytext.  */
  for (p = yytext; *p != '\0'; p++)
  {
    char *tmp = p;

    while (*p == '_')
     for (tmp = p; *tmp != '\0'; tmp++)
       *tmp = *(tmp + 1);
  }

  /* We support hexadecimal, octal, decimal and binary integer
     constants.  Set the base of the match and skip prefixes.  */

  base = 10;
  offset = 0;

  if (yytext[0] == '0')
  {
    if (yytext[1] == 'x' || yytext[1] == 'X')
    {
      base = 16;
      offset = 2;
    }
    else if (yytext[1] == 'b' || yytext[1] == 'B')
    {
      base = 2;
      offset = 2;
    }
    else if (isdigit (yytext[1]))
    {
      base = 8;
      offset = 1;
    }
  }

  /* Note that strtol can't fail in this context.  */
  value = strtoll (yytext + offset, &end, base);

  signed_p = 1;
  if (*end == 'u' || *end == 'U'
      || (*end != '\0' && (*(end + 1) == 'u' || *(end + 1) == 'U')))
    signed_p = 0;

  width = 0;
  if (*end == 'l' || *end == 'L'
      || ((*end != '\0') && ((*(end + 1) == 'l' || *(end + 1) == 'L'))))
    width = 64;
  else  if (*end == 'h' || *end == 'H'
      || ((*end != '\0') && ((*(end + 1) == 'h' || *(end + 1) == 'H'))))
    width = 16;
  else  if (*end == 'b' || *end == 'B'
      || ((*end != '\0') && ((*(end + 1) == 'b' || *(end + 1) == 'B'))))
    width = 8;

  /* If not specified with the 'l' or 'L' suffix, the type of the
     integer constant is the smallest signed or unsigned integer
     capable of holding it, starting with 32 bits, in steps of power
     of two and up to 64 bits.  But note these are positive values! */

  if (width == 0)
  {
    if (value >= 0x0000000080000000 && value <= 0x00000000ffffffff)
      width = signed_p ? 64 : 32;
    else if (value & 0xffffffff80000000)
      width = 64;
    else
      width = 32;
  }

  type = pkl_ast_make_integral_type (yyextra->ast, width, signed_p);

  yylval->ast = pkl_ast_make_integer (yyextra->ast, value);
  PKL_AST_TYPE (yylval->ast) = ASTREF (type);

  return INTEGER;    
}

{CHAR} {
   uint8_t value;
   pkl_ast_node type;

   if (yytext[1] == '\\')
    {
      switch (yytext[2])
      {
      case 'n':
        value = '\n';
        break;
      case 't':
        value = '\t';
        break;
      case '0':
        value = '\0';
        break;
      default:
        /* To avoid compiler warnings.  */
        value = 0;
        break;
      }
    }
   else
     value = yytext[1];

   type = pkl_ast_make_integral_type (yyextra->ast, 8, 0);
   PKL_AST_TYPE_COMPLETE (type) = PKL_AST_TYPE_COMPLETE_YES;

   yylval->ast = pkl_ast_make_integer (yyextra->ast, value);
   PKL_AST_TYPE (yylval->ast) = ASTREF (type);

   return CHAR;  
}

{STRING} {
   pkl_ast_node type;

   /* Strip the quoting characters */
   yytext[strlen(yytext) - 1] = 0;

   /* XXX: handle escape characters.  */

   type = pkl_ast_make_string_type (yyextra->ast);

   yylval->ast = pkl_ast_make_string (yyextra->ast, yytext + 1);
   PKL_AST_TYPE (yylval->ast) = ASTREF (type);

   return STR;
}

. { return ERR; }

%%

/*
Local variables:
mode:c
End:
*/
