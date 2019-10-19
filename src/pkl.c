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
  int error_on_warning;
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
    char *poke_rt_pk;

    poke_rt_pk = xmalloc (strlen (poke_datadir) + strlen ("/pkl-rt.pk") + 1);
    strcpy (poke_rt_pk, poke_datadir);
    strcat (poke_rt_pk, "/pkl-rt.pk");

    if (!pkl_compile_file (compiler, poke_rt_pk))
      {
        pk_term_class ("error");
        pk_puts ("internal error: ");
        pk_term_end_class ("error");
        pk_puts ("compiler failed to bootstrap itself\n");

        exit (EXIT_FAILURE);
      }
    free (poke_rt_pk);

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
                     pkl_ast ast,
                     void **pointers)
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

  struct pkl_fold_payload fold_payload = { 0 };

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
        &pkl_phase_fold,
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
        &fold_payload,
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

  if (!pkl_do_pass (poke_compiler, ast, lex_phases, lex_payloads, 0))
    goto error;

  if (transl_payload.errors > 0)
    goto error;

  /* XXX */
  /* pkl_ast_print (stdout, ast->ast); */

  if (!pkl_do_pass (poke_compiler, ast,
                    frontend_phases, frontend_payloads, PKL_PASS_F_TYPES))
    goto error;

  if (trans1_payload.errors > 0
      || trans2_payload.errors > 0
      || trans3_payload.errors > 0
      || anal1_payload.errors > 0
      || anal2_payload.errors > 0
      || typify1_payload.errors > 0
      || fold_payload.errors > 0
      || typify2_payload.errors > 0)
    goto error;

  /* XXX */
  /* pkl_ast_print (stdout, ast->ast); */

  if (!pkl_do_pass (poke_compiler, ast,
                    middleend_phases, middleend_payloads, PKL_PASS_F_TYPES))
    goto error;

  if (trans4_payload.errors > 0)
    goto error;

  /* XXX */
  /* pkl_ast_print (stdout, ast->ast); */

  if (!pkl_do_pass (poke_compiler, ast,
                    backend_phases, backend_payloads, 0))
    goto error;

  if (analf_payload.errors > 0)
    goto error;

  pkl_ast_free (ast);
  *pointers = gen_payload.pointers;
  return gen_payload.program;

 error:
  pkl_ast_free (ast);
  return NULL;
}

int
pkl_compile_buffer (pkl_compiler compiler,
                    char *buffer, char **end)
{
  pkl_ast ast = NULL;
  pvm_program program;
  int ret;
  pkl_env env = NULL;

  /* Note that the sole purpose of `pointers' is to serve as a root
     (in the stack) for the GC, to prevent the boxed values in PROGRAM
     to be collected.  Ugly as shit, but conservative garbage
     collection doesn't really work.  */
  void *pointers;

  compiler->compiling = PKL_COMPILING_PROGRAM;
  env = pkl_env_dup_toplevel (compiler->env);

  /* Parse the input program into an AST.  */
  ret = pkl_parse_buffer (&env, &ast,
                          PKL_PARSE_PROGRAM,
                          buffer, end);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    /* Memory exhaustion.  */
    printf (_("out of memory\n"));

  program = rest_of_compilation (compiler, ast, &pointers);
  if (program == NULL)
    goto error;

  pvm_specialize_program (program);
  /* XXX */
  /* pvm_print_program (stdout, program); */

  /* Execute the program in the poke vm.  */
  {
    pvm_val val;

    if (pvm_run (poke_vm, program, &val) != PVM_EXIT_OK)
      goto error;

    /* Discard the value.  */
  }

  pvm_destroy_program (program);
  pkl_env_free (compiler->env);
  compiler->env = env;
  return 1;

 error:
  pkl_env_free (env);
  return 0;
}

int
pkl_compile_statement (pkl_compiler compiler,
                       char *buffer, char **end,
                       pvm_val *val)
{
  pkl_ast ast = NULL;
  pvm_program program;
  int ret;
  pkl_env env = NULL;

  /* Note that the sole purpose of `pointers' is to serve as a root
     (in the stack) for the GC, to prevent the boxed values in PROGRAM
     to be collected.  Ugly as shit, but conservative garbage
     collection doesn't really work.  */
  void *pointers;

  compiler->compiling = PKL_COMPILING_STATEMENT;
  env = pkl_env_dup_toplevel (compiler->env);

  /* Parse the input program into an AST.  */
  ret = pkl_parse_buffer (&env, &ast,
                          PKL_PARSE_STATEMENT,
                          buffer, end);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    /* Memory exhaustion.  */
    printf (_("out of memory\n"));

  program = rest_of_compilation (compiler, ast, &pointers);
  if (program == NULL)
    goto error;

  pvm_specialize_program (program);
  /* XXX */
  /* pvm_print_program (stdout, program); */

  /* Execute the program in the poke vm.  */
  if (pvm_run (poke_vm, program, val) != PVM_EXIT_OK)
    goto error;

  pvm_destroy_program (program);
  pkl_env_free (compiler->env);
  compiler->env = env;
  return 1;

 error:
  pkl_env_free (env);
  return 0;
}


pvm_program
pkl_compile_expression (pkl_compiler compiler,
                        char *buffer, char **end, void **pointers)
{
  pkl_ast ast = NULL;
  pvm_program program;
  int ret;
  pkl_env env = NULL;

  compiler->compiling = PKL_COMPILING_EXPRESSION;
  env = pkl_env_dup_toplevel (compiler->env);

  /* Parse the input program into an AST.  */
  ret = pkl_parse_buffer (&env, &ast,
                          PKL_PARSE_EXPRESSION,
                          buffer, end);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    /* Memory exhaustion.  */
    printf (_("out of memory\n"));

  program = rest_of_compilation (compiler, ast, pointers);
  if (program == NULL)
    goto error;

  pkl_env_free (compiler->env);
  compiler->env = env;
  pvm_specialize_program (program);
  /* XXX */
  /* pvm_print_program (stdout, program); */
  return program;

 error:
  pkl_env_free (env);
  return NULL;
}


int
pkl_compile_file (pkl_compiler compiler, const char *fname)
{
  int ret;
  pkl_ast ast = NULL;
  pvm_program program;
  FILE *fd;
  pkl_env env = NULL;

  /* Note that the sole purpose of `pointers' is to serve as a root
     (in the stack) for the GC, to prevent the boxed values in PROGRAM
     to be collected.  Ugly as shit, but conservative garbage
     collection doesn't really work.  */
  void *pointers;

  compiler->compiling = PKL_COMPILING_PROGRAM;

  fd = fopen (fname, "rb");
  if (!fd)
    {
      perror (fname);
      return 0;
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

  program = rest_of_compilation (compiler, ast, &pointers);
  if (program == NULL)
    goto error;

  pvm_specialize_program (program);
  /* XXX */
  /* pvm_print_program (stdout, program); */
  fclose (fd);

  /* Execute the program in the poke vm.  */
  {
    pvm_val val;

    if (pvm_run (poke_vm, program, &val) != PVM_EXIT_OK)
      goto error_no_close;

    /* Discard the value.  */
  }

  pvm_destroy_program (program);
  pkl_env_free (compiler->env);
  compiler->env = env;
  return 1;

 error:
  fclose (fd);
 error_no_close:
  pkl_env_free (env);
  return 0;
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

static void
pkl_detailed_location (pkl_ast ast, pkl_ast_loc loc,
                       const char *style_class)
{
  size_t cur_line = 1;
  size_t cur_column = 1;
  int i;

  if (!PKL_AST_LOC_VALID (loc))
    return;

  if (ast->buffer)
    {
      char *p;
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
                pk_printf ("%c", *p);
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
                  if (c != '\n')
                    pk_printf ("%c", c);
                  c = fgetc (fd);
                }
              while (c != EOF && c != '\0' && c != '\n');
              break;
            }
        }

      /* Restore the file position so parsing can continue.  */
      assert (fseeko (fd, cur_pos, SEEK_SET) == 0);
    }

  pk_puts ("\n");

  for (i = 1; i < loc.first_column; ++i)
    pk_puts (" ");

  pk_term_class (style_class);
  for (; i < loc.last_column; ++i)
    if (i == loc.first_column)
      pk_puts ("^");
    else
      pk_puts ("~");
  pk_term_end_class (style_class);
  pk_puts ("\n");
}

void
pkl_error (pkl_ast ast,
           pkl_ast_loc loc,
           const char *fmt,
           ...)
{
  va_list valist;
  char *errmsg, *p;

  /* Write out the error message, line by line.  */
  va_start (valist, fmt);
  vasprintf (&errmsg, fmt, valist);
  va_end (valist);

  p = errmsg;
  while (*p != '\0')
    {
      pk_term_class ("error-filename");
      if (ast->filename)
        pk_printf ("%s:", ast->filename);
      else
        pk_puts ("<stdin>:");
      pk_term_end_class ("error-filename");

      if (PKL_AST_LOC_VALID (loc))
        {
          pk_term_class ("error-location");
          if (poke_quiet_p)
            pk_printf ("%d: ", loc.first_line);
          else
            pk_printf ("%d:%d: ",
                       loc.first_line, loc.first_column);
          pk_term_end_class ("error-location");
        }

      pk_term_class ("error");
      pk_puts ("error: ");
      pk_term_end_class ("error");

      while (*p != '\n' && *p != '\0')
        {
          pk_printf ("%c", *p);
          p++;
        }
      if (*p == '\n')
        p++;
      pk_puts ("\n");
    }
  free (errmsg);

  if (!poke_quiet_p)
    pkl_detailed_location (ast, loc, "error");
}


void
pkl_warning (pkl_ast ast,
             pkl_ast_loc loc,
             const char *fmt,
             ...)
{
  va_list valist;
  char *msg;

  va_start(valist, fmt);
  vasprintf (&msg, fmt, valist);
  va_end (valist);

  pk_term_class ("error-filename");
  if (ast->filename)
    pk_printf ("%s:", ast->filename);
  else
    pk_puts ("<stdin>:");
  pk_term_end_class ("error-filename");

  if (PKL_AST_LOC_VALID (loc))
    {
      pk_term_class ("error-location");
      pk_printf ("%d:%d: ", loc.first_line, loc.first_column);
      pk_term_end_class ("error-location");
    }
  pk_term_class ("warning");
  pk_puts ("warning: ");
  pk_term_end_class ("warning");
  pk_puts (msg);
  pk_puts ("\n");

  free (msg);

  if (!poke_quiet_p)
    pkl_detailed_location (ast, loc, "warning");
}

void
pkl_ice (pkl_ast ast,
         pkl_ast_loc loc,
         const char *fmt,
         ...)
{
  va_list valist;
  char tmpfile[1024];

  /* XXX: dump the AST plus additional details on the current state to
     a temporary file.  */
  {
    int des;
    FILE *out;

    if ((des = path_search (tmpfile, PATH_MAX, NULL, "poke", true) == -1)
        || ((des = mkstemp (tmpfile)) == -1))
      {
        pk_term_class ("error");
        pk_puts ("internal error: ");
        pk_term_end_class ("error");
        pk_puts ("determining a temporary file name\n");

        return;
      }

    out = fdopen (des, "w");
    if (out == NULL)
      {
        pk_term_class ("error");
        pk_puts ("internal error: ");
        pk_term_end_class ("error");
        pk_printf ("opening temporary file `%s'\n", tmpfile);
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
    {
      pk_term_class ("error-location");
      pk_printf ("%d:%d: ", loc.first_line, loc.first_column);
      pk_term_end_class ("error-location");
    }
  pk_puts ("internal compiler error: ");
  {
    char *msg;

    va_start (valist, fmt);
    vasprintf (&msg, fmt, valist);
    va_end (valist);

    pk_puts (msg);
    free (msg);
  }
  pk_puts ("\n");
  pk_printf ("Important information has been dumped in %s.\n",
             tmpfile);
  /* XXX hyperlink */
  pk_puts ("Please attach it to a bug report and send it to bug-poke@gnu.org.\n");
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

int
pkl_error_on_warning (pkl_compiler compiler)
{
  return compiler->error_on_warning;
}

void
pkl_set_error_on_warning (pkl_compiler compiler,
                          int error_on_warning)
{
  compiler->error_on_warning = error_on_warning;
}
