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

int
pkl_compile_buffer (pvm_program *prog,
                    int what, char *buffer, char **end)
{
  pkl_ast ast = NULL;
  int ret;
  struct pkl_gen_payload gen_payload = { NULL, 0 };

  struct pkl_anal_payload anal1_payload = { 0 };
  struct pkl_anal_payload anal2_payload = { 0 };
  struct pkl_anal_payload analf_payload = { 0 };

  struct pkl_trans_payload trans1_payload = { 0 };
  struct pkl_trans_payload trans2_payload = { 0 };
  struct pkl_trans_payload trans3_payload = { 0 };

  struct pkl_typify_payload typify1_payload = { 0 };
  struct pkl_typify_payload typify2_payload = { 0 };


  /* Parse the input program into an AST.  */
  ret = pkl_parse_buffer (&ast, what, buffer, end);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    /* Memory exhaustion.  */
    printf (_("out of memory\n"));

  {
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

    /* Note that gen does subpasses, so no transformation phases
       should be invoked in the bakend pass.  */
    struct pkl_phase *backend_phases[]
      = { &pkl_phase_analf,
          &pkl_phase_gen,
          NULL
        };

    void *backend_payloads[]
      = { &analf_payload,
          &gen_payload
        };

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
    /* pkl_ast_print (stdout, ast->ast);  */
      
    if (!pkl_do_pass (ast, backend_phases, backend_payloads))
      goto error;

    if (analf_payload.errors > 0)
      goto error;
  }

  pkl_ast_free (ast);

  pvm_specialize_program (gen_payload.program);
  *prog = gen_payload.program;

  return 1;

 error:
  pkl_ast_free (ast);
  pvm_destroy_program (gen_payload.program);
  return 0;
}

int
pkl_compile_file (pvm_program *prog,
                  FILE *fd,
                  const char *fname)
{
  int ret;
  pkl_ast ast = NULL;
  pvm_program p = NULL; /* This is to avoid a compiler warning.  */

  ret = pkl_parse_file (&ast, PKL_PARSE_PROGRAM, fd, fname);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    {
      /* Memory exhaustion.  */
      printf (_("out of memory\n"));
    }

  pvm_specialize_program (p);
  *prog = p;

  return 1;

 error:
  pkl_ast_free (ast);
  return 0;
}

void
pkl_error (pkl_ast ast,
           pkl_ast_loc loc,
           const char *fmt,
           ...)
{
  va_list valist;
  size_t i;

  if (PKL_AST_LOC_VALID (loc))
    fprintf (stderr, "%d:%d: ",
             loc.first_line, loc.first_column);
  fputs (RED REVERSE "error: " NOATTR, stderr);
  va_start (valist, fmt);
  vfprintf (stderr, fmt, valist);
  va_end (valist);
  fputc ('\n', stderr);

  /* XXX: make fancy output optional.  */
  if (PKL_AST_LOC_VALID (loc))
  {
    size_t cur_line = 1;
    size_t cur_column = 1;
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
