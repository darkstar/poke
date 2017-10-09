/* pvm.h - Poke Virtual Machine.  */

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

#include <config.h>
#include <stdlib.h>
#include "pvm.h"

void
pvm_init (void)
{
  pvm_initialize ();
}

void
pvm_shutdown (void)
{
  pvm_finalize ();
}

pvm_stack
pvm_stack_new (void)
{
  pvm_stack s;

  s = xmalloc (sizeof (struct pvm_stack));
  memset (s, 0, sizeof (struct pvm_stack));
  return s;
}

void
pvm_stack_free (pvm_stack s)
{
  switch (PVM_STACK_TYPE (s))
    {
    case PVM_STACK_STR:
      free (s->v.string);
      break;
    default:
      break;
    }
  
  free (s);
}

void
pvm_op_add (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res)
    = PVM_STACK_INTEGER (a) + PVM_STACK_INTEGER (b);
}

void
pvm_op_sub (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res)
    = PVM_STACK_INTEGER (a) - PVM_STACK_INTEGER (b);
}

void
pvm_op_mul (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res)
    = PVM_STACK_INTEGER (a) * PVM_STACK_INTEGER (b);
}
void
pvm_op_div (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res)
    = PVM_STACK_INTEGER (a) / PVM_STACK_INTEGER (b);
}

void
pvm_op_mod (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res)
    = PVM_STACK_INTEGER (a) % PVM_STACK_INTEGER (b);
}

void
pvm_op_ieq (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res)
    = PVM_STACK_INTEGER (a) == PVM_STACK_INTEGER (b);
}

void
pvm_op_ine (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res)
    = PVM_STACK_INTEGER (a) != PVM_STACK_INTEGER (b);
}

void
pvm_op_seq (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) = strcmp (PVM_STACK_STRING (a),
                                    PVM_STACK_STRING (b)) == 0;
}

void
pvm_op_sne (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) = strcmp (PVM_STACK_STRING (a),
                                    PVM_STACK_STRING (b)) != 0;
}

void
pvm_op_ilt (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res)
    = PVM_STACK_INTEGER (a) < PVM_STACK_INTEGER (b);
}

void
pvm_op_ile (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res)
    = PVM_STACK_INTEGER (a) <= PVM_STACK_INTEGER (b);
}

void
pvm_op_igt (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res)
    = PVM_STACK_INTEGER (a) > PVM_STACK_INTEGER (b);
}

void
pvm_op_ige (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res)
    = PVM_STACK_INTEGER (a) >= PVM_STACK_INTEGER (b);
}

void
pvm_op_slt (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res)= strcmp (PVM_STACK_STRING (a),
                                    PVM_STACK_STRING (b)) < 0;
}

void
pvm_op_sle (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) = strcmp (PVM_STACK_STRING (a),
                                    PVM_STACK_STRING (b)) <= 0;
}

void
pvm_op_sgt (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) = strcmp (PVM_STACK_STRING (a),
                                    PVM_STACK_STRING (b)) > 0;
}

void
pvm_op_sge (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) = strcmp (PVM_STACK_STRING (a),
                                    PVM_STACK_STRING (b)) >= 0;
}

void
pvm_op_and (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) =
    PVM_STACK_INTEGER (a) && PVM_STACK_INTEGER (b);
}

void
pvm_op_or (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) =
    PVM_STACK_INTEGER (a) || PVM_STACK_INTEGER (b);
}

void
pvm_op_not (pvm_stack res, pvm_stack a)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) = ! PVM_STACK_INTEGER (a);
}

void
pvm_op_bxor (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) =
    PVM_STACK_INTEGER (a) ^ PVM_STACK_INTEGER (b);
}

void
pvm_op_bior (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) =
    PVM_STACK_INTEGER (a) | PVM_STACK_INTEGER (b);
}

void
pvm_op_band (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) =
    PVM_STACK_INTEGER (a) & PVM_STACK_INTEGER (b);
}

void
pvm_op_bnot (pvm_stack res, pvm_stack a)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) = ~PVM_STACK_INTEGER (a);
}

void
pvm_op_bsl (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) =
    PVM_STACK_INTEGER (a) << PVM_STACK_INTEGER (b);
}


void
pvm_op_bsr (pvm_stack res, pvm_stack a, pvm_stack b)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) =
    PVM_STACK_INTEGER (a) >> PVM_STACK_INTEGER (b);
}

void
pvm_op_neg (pvm_stack res, pvm_stack a)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) = - PVM_STACK_INTEGER (a);
}

void
pvm_op_preinc (pvm_stack res, pvm_stack a)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) = ++PVM_STACK_INTEGER (a);
}

void
pvm_op_predec (pvm_stack res, pvm_stack a)
{
  PVM_STACK_TYPE (res) = PVM_STACK_INT;
  PVM_STACK_INTEGER (res) = --PVM_STACK_INTEGER (a);
}

void
pvm_op_sconc (pvm_stack res, pvm_stack a, pvm_stack b)
{
  char *sres = PVM_STACK_STRING (res);
  char *sa = PVM_STACK_STRING (a);
  char *sb = PVM_STACK_STRING (b);
  
  PVM_STACK_TYPE (res) = PVM_STACK_STR;
  sres = xmalloc (strlen (sa) + strlen (sb) + 1);
  sres = strcpy (sres, sa);
  sres = strcat (sres, sb);
}

