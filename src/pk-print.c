/* pk-peek.c - `print' command.  */

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
#include <assert.h>
#include <stdio.h> /* For stdout */

#include "pk-cmd.h"

static void
print_val (pvm_val val)
{
  if (PVM_IS_INT (val) || PVM_IS_HALF (val) || PVM_IS_BYTE (val)
      || PVM_IS_LONG (val))
    printf ("%ld", PVM_VAL_NUMBER (val));
  else if (PVM_IS_UINT (val) || PVM_IS_UHALF (val) || PVM_IS_UBYTE (val)
           || PVM_IS_ULONG (val))
      printf ("%lu", PVM_VAL_NUMBER (val));
  else if (PVM_IS_STRING (val))
    printf ("\"%s\"", PVM_VAL_STR (val));
  else if (PVM_IS_ARR (val))
    {
      size_t nelem, idx;
      
      nelem = PVM_VAL_ARR_NELEM (val);
      printf ("[");
      for (idx = 0; idx < nelem; idx++)
        {
          if (idx != 0)
            printf (",");
          print_val (PVM_VAL_ARR_ELEM (val, idx));
        }
      printf ("]");
    }
  else if (PVM_IS_TUP (val))
    {
      size_t nelem, idx;

      nelem = PVM_VAL_TUP_NELEM (val);
      printf ("{");
      for (idx = 0; idx < nelem; ++idx)
        {
          pvm_val name = PVM_VAL_TUP_ELEM_NAME(val, idx);
          pvm_val value = PVM_VAL_TUP_ELEM_VALUE(val, idx);

          if (idx != 0)
            printf (",");
          if (name != PVM_NULL)
            printf ("%s=", PVM_VAL_STR (name));
          print_val (value);
        }
      printf ("}");
    }
  else
    assert (0); /* XXX support more types.  */
}

static int
pk_cmd_print (int argc, struct pk_cmd_arg argv[])
{
  /* print EXP */

  pvm_program prog;
  pvm_val val;
  int pvm_ret;

  assert (argc == 1);
  assert (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_EXP);

  prog = PK_CMD_ARG_EXP (argv[0]);

  /* jitter_disassemble_program (prog, true, JITTER_CROSS_OBJDUMP, NULL); */
  pvm_ret = pvm_run (prog, &val);
  if (pvm_ret != PVM_EXIT_OK)
    goto rterror;

  print_val (val);
  printf ("\n");
  return 1;

 rterror:
  printf ("run-time error: %s\n", pvm_error (pvm_ret));
  return 0;
}

struct pk_cmd print_cmd =
  {"print", "e", 0, NULL, pk_cmd_print, "print EXP"};
