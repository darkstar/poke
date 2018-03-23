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

#include "pkl.h"
#include "pkl-parser.h"
#include "pkl-pass.h"
#include "pkl-gen.h"
#include "pkl-anal.h"

/* Compiler passes and phases.  */

extern struct pkl_phase satanize;  /* pkl-satan.c  */
extern struct pkl_phase pkl_phase_promo; /* pkl-promo.c */
extern struct pkl_phase pkl_phase_fold; /* pkl-fold.c */
extern struct pkl_phase pkl_phase_gen; /* pkl-gen.c */
extern struct pkl_phase pkl_phase_anal1; /* pkl-anal.c */
extern struct pkl_phase pkl_phase_anal2; /* pkl-anal.c */
extern struct pkl_phase pkl_phase_typify;

int
pkl_compile_buffer (pvm_program *prog,
                    int what, char *buffer, char **end)
{
  pkl_ast ast = NULL;
  int ret;
  struct pkl_gen_payload gen_payload = { NULL, 0 };
  struct pkl_anal_payload anal1_payload = { 0 };
  struct pkl_anal_payload anal2_payload = { 0 };

  /* Parse the input program into an AST.  */
  ret = pkl_parse_buffer (&ast, what, buffer, end);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    /* Memory exhaustion.  */
    printf (_("out of memory\n"));

  if (0) /* Multi-pass.  */
    {
      struct pkl_phase *anal1_pass[] = { &pkl_phase_anal1, NULL };
      void *anal1_payloads[] = { &anal1_payload };
      struct pkl_phase *promo_pass[] = { &pkl_phase_promo, NULL };
      struct pkl_phase *fold_pass[] =  { &pkl_phase_fold, NULL };
      struct pkl_phase *gen_pass[] = { &pkl_phase_gen, NULL };
      void *gen_payloads[] = { &gen_payload };


      pkl_ast_print (stdout, ast->ast);

      fprintf (stdout, "===========  ANALYZING 1 ======\n");
      if (!pkl_do_pass (ast, anal1_pass, anal1_payloads))
        goto error;

      if (anal1_payload.errors > 0)
        goto error;

      fprintf (stdout, "===========  PROMOTING ======\n");
      if (!pkl_do_pass (ast, promo_pass, NULL))
        goto error;
      pkl_ast_print (stdout, ast->ast);
      
      fprintf (stdout, "===========  CONSTANT FOLDING ======\n");
      if (!pkl_do_pass (ast, fold_pass, NULL))
        goto error;
      pkl_ast_print (stdout, ast->ast);

      fprintf (stdout, "===========  GENERATING ======\n");
      if (!pkl_do_pass (ast, gen_pass, gen_payloads))
        goto error;
    }
  else
    {
      struct pkl_phase *frontend_phases[]
        = { &pkl_phase_anal1,
            &pkl_phase_typify,
            &pkl_phase_promo,
            /* &pkl_phase_fold */ NULL ,
            &pkl_phase_anal2,
            NULL
          };
      void *frontend_payloads[]
        = { &anal1_payload, /* anal1 */
            NULL, /* typify */
            NULL, /* promo */
            NULL, /* fold */
            &anal2_payload  /* anal2 */
          };

      struct pkl_phase *backend_phases[] = { &pkl_phase_gen, NULL };
      void *backend_payloads[] = { &gen_payload };
      
      if (!pkl_do_pass (ast, frontend_phases, frontend_payloads))
        goto error;

      if (anal1_payload.errors > 0
          || anal2_payload.errors > 0)
        goto error;

      /* XXX */
      pkl_ast_print (stdout, ast->ast);
      
      if (!pkl_do_pass (ast, backend_phases, backend_payloads))
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
