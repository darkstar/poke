/* pkl.c - Poke compiler.  */

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
#include "pkl-anal.h"
#include "pkl-trans.h"
#include "pkl-typify.h"
#include "pkl-promo.h"
#include "pkl-fold.h"
#include "pkl-env.h"

#include "pvm.h"
#include "poke.h"

struct pkl_compiler
{
  pkl_env env;  /* Compiler environment.  */
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

  /* XXX: bootstrap the compiler: Load pkl-rt.pk and execute it.  An
     error bootstraping is an internal error and should be reported as
     such.  */
  {
    pvm_val val;
    pvm_program pkl_prog
      = pkl_compile_file (compiler,
                          /* XXX: use datadir/bleh */
                          "/home/jemarch/gnu/hacks/poke/src/pkl.pk");

    if (pkl_prog == NULL
        || (pvm_run (poke_vm, pkl_prog, &val) != PVM_EXIT_OK))
      {
        fprintf (stderr,
                 "Internal error: compiler failed to bootstrap itself\n");
        exit (1);
      }

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
  
  struct pkl_trans_payload trans1_payload = { 0 };
  struct pkl_trans_payload trans2_payload = { 0 };
  struct pkl_trans_payload trans3_payload = { 0 };
  struct pkl_trans_payload trans4_payload = { 0 };
  
  struct pkl_typify_payload typify1_payload = { 0 };
  struct pkl_typify_payload typify2_payload = { 0 };
  
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
  };

  struct pkl_phase *middleend_phases[]
    = { &pkl_phase_trans4,
        NULL
  };

  /* Note that gen does subpasses, so no transformation phases should
     be invoked in the bakend pass.  */
  struct pkl_phase *backend_phases[]
    = { &pkl_phase_analf,
        &pkl_phase_gen,
        NULL
  };

  void *backend_payloads[]
    = { &analf_payload,
        &gen_payload
  };

  /* Initialize payloads.  */
  pkl_gen_init_payload (&gen_payload);

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
pkl_compile_buffer (pkl_compiler compiler, char *buffer,
                    char **end)
{
  pkl_ast ast = NULL;
  pvm_program program;
  int ret;
  pkl_env env = NULL;

  env = pkl_env_dup_toplevel (compiler->env);
  
  /* Parse the input program into an AST.  */
  ret = pkl_parse_buffer (&env, &ast,
                          PKL_PARSE_DECLARATION, buffer, end);
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

pvm_program
pkl_compile_expression (pkl_compiler compiler,
                        char *buffer, char **end)
{
  pkl_ast ast = NULL;
  pvm_program program;
  int ret;

  /* Parse the input program into an AST.  */
  ret = pkl_parse_buffer (&compiler->env,
                          &ast,
                          PKL_PARSE_EXPRESSION, buffer, end);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    /* Memory exhaustion.  */
    printf (_("out of memory\n"));

  program = rest_of_compilation (compiler, ast);
  if (program == NULL)
    goto error;
  
  pvm_specialize_program (program);
  return program;

 error:
  pvm_destroy_program (program);
  return NULL;
}

pkl_env
pkl_get_env (pkl_compiler compiler)
{
  return compiler->env;
}

void
pkl_error (pkl_ast ast,
           pkl_ast_loc loc,
           const char *fmt,
           ...)
{
  va_list valist;
  size_t i;

  if (ast->filename)
    fprintf (stderr, "%s:", ast->filename);
  
  if (PKL_AST_LOC_VALID (loc))
    fprintf (stderr, "%d:%d: ",
             loc.first_line, loc.first_column);
  fputs (RED REVERSE "error: " NOATTR, stderr);
  va_start (valist, fmt);
  vfprintf (stderr, fmt, valist);
  va_end (valist);
  fputc ('\n', stderr);

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
