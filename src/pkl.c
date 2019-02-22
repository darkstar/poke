/* pkl.c - Poke compiler.  */

/* Copyright (C) 2019 Jose E. Marchesi */

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

#include <config.h>

#include <gettext.h>
#define _(str) gettext (str)
#include <stdarg.h>
#include <tmpdir.h>
#include <tempname.h>
#include <stdio.h> /* For fopen, etc */
#include <stdlib.h>
#include <string.h>
#include <xalloc.h>

#include "pk-term.h"

#include "pkl.h"
#include "pkl-ast.h"
#include "pkl-parser.h"
#include "pkl-pass.h"
#include "pkl-gen.h"
#include "pkl-trans.h"
#include "pkl-anal.h"
#include "pkl-trans.h"
#include "pkl-typify.h"
#include "pkl-promo.h"
#include "pkl-fold.h"
#include "pkl-env.h"

#include "pvm.h"
#include "poke.h"

#define PKL_COMPILING_EXPRESSION 0
#define PKL_COMPILING_PROGRAM    1
#define PKL_COMPILING_STATEMENT  2

struct pkl_compiler
{
  pkl_env env;  /* Compiler environment.  */
  int bootstrapped;
  int compiling;
  /* XXX: put a link to the run-time top-level closure here.  */
};

pkl_compiler
pkl_new ()
{
  pkl_compiler compiler
    = xmalloc (sizeof (struct pkl_compiler));

  memset (compiler, 0, sizeof (struct pkl_compiler));

  /* Create the top-level compile-time environment.  This will be used
     for as long as the incremental compiler lives.  */
  compiler->env = pkl_env_new ();

  /* Bootstrap the compiler.  An error bootstraping is an internal
     error and should be reported as such.  */
  {
    pvm_val val;
    pvm_program pkl_prog;
    char *poke_rt_pk;

    poke_rt_pk = xmalloc (strlen (poke_datadir) + strlen ("/pkl-rt.pk") + 1);
    strcpy (poke_rt_pk, poke_datadir);
    strcat (poke_rt_pk, "/pkl-rt.pk");
    pkl_prog = pkl_compile_file (compiler, poke_rt_pk);
    free (poke_rt_pk);

    if (pkl_prog == NULL
        || (pvm_run (poke_vm, pkl_prog, &val) != PVM_EXIT_OK))
      {
        fprintf (stderr,
                 "Internal error: compiler failed to bootstrap itself\n");
        exit (1);
      }

    compiler->bootstrapped = 1;
    /* XXX: disable compiler built-ins from this point on.  */
  }
  
  return compiler;
}

void
pkl_free (pkl_compiler compiler)
{
  pkl_env_free (compiler->env);
  free (compiler);
}

static pvm_program
rest_of_compilation (pkl_compiler compiler,
                     pkl_ast ast)
{
  struct pkl_gen_payload gen_payload;
  
  struct pkl_anal_payload anal1_payload = { 0 };
  struct pkl_anal_payload anal2_payload = { 0 };
  struct pkl_anal_payload analf_payload = { 0 };

  struct pkl_trans_payload transl_payload;
  struct pkl_trans_payload trans1_payload;
  struct pkl_trans_payload trans2_payload;
  struct pkl_trans_payload trans3_payload;
  struct pkl_trans_payload trans4_payload;
  
  struct pkl_typify_payload typify1_payload = { 0 };
  struct pkl_typify_payload typify2_payload = { 0 };

  struct pkl_phase *lex_phases[]
    = { &pkl_phase_transl,
        NULL
  };

  void *lex_payloads[]
    = { &transl_payload };
  
  struct pkl_phase *frontend_phases[]
    = { &pkl_phase_trans1,
        &pkl_phase_anal1,
        &pkl_phase_typify1,
        &pkl_phase_promo,
        &pkl_phase_trans2,
        /*            &pkl_phase_fold,*/
        &pkl_phase_typify2,
        &pkl_phase_trans3,
        &pkl_phase_anal2,
        NULL,
  };
  
  void *frontend_payloads[]
    = { &trans1_payload,
        &anal1_payload,
        &typify1_payload,
        NULL, /* promo */
        &trans2_payload,
        /*  NULL,*/ /* fold */
        &typify2_payload,
        &trans3_payload,
        &anal2_payload,
  };

  void *middleend_payloads[]
    = { &trans4_payload,
        &analf_payload,
  };

  struct pkl_phase *middleend_phases[]
    = { &pkl_phase_trans4,
        &pkl_phase_analf,
        NULL
  };

  /* Note that gen does subpasses, so no transformation phases should
     be invoked in the bakend pass.  */
  struct pkl_phase *backend_phases[]
    = { &pkl_phase_gen,
        NULL
  };

  void *backend_payloads[]
    = { &gen_payload
  };

  /* Initialize payloads.  */
  pkl_trans_init_payload (&transl_payload);
  pkl_trans_init_payload (&trans1_payload);
  pkl_trans_init_payload (&trans2_payload);
  pkl_trans_init_payload (&trans3_payload);
  pkl_trans_init_payload (&trans4_payload);
  pkl_gen_init_payload (&gen_payload, compiler);

  if (!pkl_do_pass (ast, lex_phases, lex_payloads))
    goto error;

  if (transl_payload.errors > 0)
    goto error;

  /* XXX */
  /* pkl_ast_print (stdout, ast->ast); */
      
  if (!pkl_do_pass (ast, frontend_phases, frontend_payloads))
    goto error;
    
  if (trans1_payload.errors > 0
      || trans2_payload.errors > 0
      || trans3_payload.errors > 0
      || anal1_payload.errors > 0
      || anal2_payload.errors > 0
      || typify1_payload.errors > 0
      || typify2_payload.errors > 0)
    goto error;

  /* XXX */
  /* pkl_ast_print (stdout, ast->ast); */

  if (!pkl_do_pass (ast, middleend_phases, middleend_payloads))
    goto error;

  if (trans4_payload.errors > 0)
    goto error;

  /* XXX */
  /* pkl_ast_print (stdout, ast->ast); */
  
  if (!pkl_do_pass (ast, backend_phases, backend_payloads))
    goto error;
  
  if (analf_payload.errors > 0)
    goto error;

  pkl_ast_free (ast);
  return gen_payload.program;

 error:
  pkl_ast_free (ast);
  return NULL;
}

pvm_program
pkl_compile_buffer (pkl_compiler compiler, int what,
                    char *buffer, char **end)
{
  pkl_ast ast = NULL;
  pvm_program program;
  int ret;
  pkl_env env = NULL;
  int parse_what;

  compiler->compiling
    = (what == PKL_WHAT_EXPRESSION
       ? PKL_COMPILING_EXPRESSION
       : what == PKL_WHAT_STATEMENT
       ? PKL_COMPILING_STATEMENT
       : PKL_COMPILING_PROGRAM);

  env = pkl_env_dup_toplevel (compiler->env);
  
  /* Decide the entity to parse.  */
  switch (what)
    {
    case PKL_WHAT_PROGRAM: parse_what = PKL_PARSE_PROGRAM; break;
    case PKL_WHAT_EXPRESSION: parse_what = PKL_PARSE_EXPRESSION; break;
    case PKL_WHAT_DECLARATION: parse_what = PKL_PARSE_DECLARATION; break;
    case PKL_WHAT_STATEMENT: parse_what = PKL_PARSE_STATEMENT; break;
    default:
      assert (0);
    }

  /* Parse the input program into an AST.  */
  ret = pkl_parse_buffer (&env, &ast, parse_what, buffer, end);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    /* Memory exhaustion.  */
    printf (_("out of memory\n"));
  
  program = rest_of_compilation (compiler, ast);
  if (program == NULL)
    goto error;

  compiler->env = env;
  pvm_specialize_program (program);
  /* XXX */
  /* pvm_print_program (stdout, program); */
  return program;

 error:
  pkl_env_free (env);
  return NULL;
}

pvm_program
pkl_compile_file (pkl_compiler compiler, const char *fname)
{
  int ret;
  pkl_ast ast = NULL;
  pvm_program program;
  FILE *fd;
  pkl_env env = NULL;

  compiler->compiling = PKL_COMPILING_PROGRAM;

  fd = fopen (fname, "rb");
  if (!fd)
    {
      perror (fname);
      return NULL;
    }

  env = pkl_env_dup_toplevel (compiler->env);
  ret = pkl_parse_file (&env,  &ast, fd, fname);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    {
      /* Memory exhaustion.  */
      printf (_("out of memory\n"));
    }

  program = rest_of_compilation (compiler, ast);
  if (program == NULL)
    goto error;

  compiler->env = env;
  pvm_specialize_program (program);
  /* XXX */  
  /* pvm_print_program (stdout, program); */
  fclose (fd);
  return program;

 error:
  fclose (fd);
  pkl_env_free (env);
  return NULL;
}

pkl_env
pkl_get_env (pkl_compiler compiler)
{
  return compiler->env;
}

int
pkl_bootstrapped_p (pkl_compiler compiler)
{
  return compiler->bootstrapped;
}

void
pkl_error (pkl_ast ast,
           pkl_ast_loc loc,
           const char *fmt,
           ...)
{
  va_list valist;
  size_t i;
  char *errmsg, *p;

  /* Write out the error message, line by line.  */
  va_start (valist, fmt);
  vasprintf (&errmsg, fmt, valist);
  va_end (valist);

  p = errmsg;
  while (*p != '\0')
    {
      if (ast->filename)
        fprintf (stderr, "%s:", ast->filename);
  
      if (PKL_AST_LOC_VALID (loc))
        {
          if (poke_quiet_p)
            fprintf (stderr, "%d: ", loc.first_line);
          else
            fprintf (stderr, "%d:%d: ",
                     loc.first_line, loc.first_column);
        }
      fputs (RED REVERSE "error: " NOATTR, stderr);

      while (*p != '\n' && *p != '\0')
        {
          fputc (*p, stderr);
          p++;
        }
      if (*p == '\n')
        p++;
      fputc ('\n', stderr);
    }
  free (errmsg);

  if (poke_quiet_p)
    return;
  
  /* XXX: cleanup this pile of shit, and make fancy output
     optional.  */
  if (PKL_AST_LOC_VALID (loc))
  {
    size_t cur_line = 1;
    size_t cur_column = 1;

    if (ast->buffer)
      {
        char *p = ast->buffer;
        for (p = ast->buffer; *p != '\0'; ++p)
          {
            if (*p == '\n')
              {
                cur_line++;
                cur_column = 1;
              }
            else
              cur_column++;
            
            if (cur_line >= loc.first_line
                && cur_line <= loc.last_line)
              {
                /* Print until newline or end of string.  */
                for (;*p != '\0' && *p != '\n'; ++p)
                  fputc (*p, stderr);
                break;
              }
          }
      }
    else
      {
        FILE *fd = ast->file;
        int c;

        off64_t cur_pos = ftello (fd);

        /* Seek to the beginning of the file.  */
        assert (fseeko (fd, 0, SEEK_SET) == 0);

        while ((c = fgetc (fd)) != EOF)
          {
            if (c == '\n')
              {
                cur_line++;
                cur_column = 1;
              }
            else
              cur_column++;
            
           if (cur_line >= loc.first_line
                && cur_line <= loc.last_line)
              {
                /* Print until newline or end of string.  */
                do
                  {
                    c = fgetc (fd);
                    if (c != '\n')
                      fputc (c, stderr);
                  }
                while (c != EOF && c != '\0' && c != '\n');
                break;
              }
          }

        /* Restore the file position so parsing can continue.  */
        assert (fseeko (fd, cur_pos, SEEK_SET) == 0);
      }

    fputc ('\n', stderr);

    for (i = 1; i < loc.first_column; ++i)
      fputc (' ', stderr);
    for (; i < loc.last_column; ++i)
      if (i == loc.first_column)
        fputc ('^', stderr);
      else
        fputc ('~', stderr);
    fputc ('\n', stderr);
  }
}


void
pkl_warning (pkl_ast_loc loc,
             const char *fmt,
             ...)
{
  va_list valist;

  va_start (valist, fmt);
  if (PKL_AST_LOC_VALID (loc))
    fprintf (stderr, "%d:%d: ",
             loc.first_line, loc.first_column);
  fputs ("warning: ", stderr);
  vfprintf (stderr, fmt, valist);
  fputc ('\n', stderr);
  va_end (valist);
}

void
pkl_ice (pkl_ast ast,
         pkl_ast_loc loc,
         const char *fmt,
         ...)
{
  va_list valist;
  char tmpfile[PATH_MAX];

  /* XXX: dump the AST plus additional details on the current state to
     a temporary file.  */
  {
    int des;
    FILE *out;

    if ((des = path_search (tmpfile, PATH_MAX, NULL, "poke", true) == -1)
        || ((des = mkstemp (tmpfile)) == -1))
      {
        fputs ("internal error: determining a temporary file name\n",
               stderr);
        return;
      }
    
    out = fdopen (des, "w");
    if (out == NULL)
      {
        fprintf (stderr,
                 "internal error: opening temporary file `%s'\n",
                 tmpfile);
        return;
      }

    fputs ("internal compiler error: ", out);
    va_start (valist, fmt);
    vfprintf (out, fmt, valist);
    va_end (valist);
    fputc ('\n', out);
    pkl_ast_print (out, ast->ast);
    fclose (out);
  }

  if (PKL_AST_LOC_VALID (loc))
    fprintf (stderr, "%d:%d: ",
             loc.first_line, loc.first_column);
  fputs (RED REVERSE "internal compiler error: " NOATTR, stderr);
  va_start (valist, fmt);
  vfprintf (stderr, fmt, valist);
  va_end (valist);
  fputc ('\n', stderr);
  fprintf (stderr, "Important information has been dumped in %s.\n",
           tmpfile);
  fputs ("Please attach it to a bug report and send it to bug-poke@gnu.org.\n", stderr);
}

int
pkl_compiling_expression_p (pkl_compiler compiler)
{
  return compiler->compiling == PKL_COMPILING_EXPRESSION;
}

int
pkl_compiling_statement_p (pkl_compiler compiler)
{
  return compiler->compiling == PKL_COMPILING_STATEMENT;
}
