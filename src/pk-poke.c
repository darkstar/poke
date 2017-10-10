/* pk-poke.c - `poke' command.  */

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

#include "pk-cmd.h"
#include "pk-io.h"

static int
pk_cmd_poke (int argc, struct pk_cmd_arg argv[])
{
  /* poke ADDR, VAL */

  pk_io_off address;
  int value;
  char *svalue = NULL;
  pvm_program prog;

  assert (argc == 2);

  assert (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_EXP);
  prog = PK_CMD_ARG_EXP (argv[0]);
  pvm_execute (prog);
  if (pvm_exit_code () == PVM_EXIT_OK)
    {
      pvm_stack res = pvm_result ();
      assert (res != NULL);
      
      if (PVM_STACK_TYPE (res) != PVM_STACK_INT)
        /* XXX This should print usage!  */
        return 0;
      
      address = PVM_STACK_INTEGER (res);
    }
  else
    {
      printf ("run-time error\n");
      return 0;
    }

  if (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_NULL)
    value = 0;
  else
    {
      assert (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_EXP);
      prog = PK_CMD_ARG_EXP (argv[0]);
      pvm_execute (prog);
      if (pvm_exit_code () == PVM_EXIT_OK)
        {
          pvm_stack res = pvm_result ();
          assert (res != NULL);

          value = 0;
          switch (PVM_STACK_TYPE (res))
            {
            case PVM_STACK_INT:
              value = PVM_STACK_INTEGER (res);
              break;
            case PVM_STACK_STR:
              printf ("XXX: '%s'\n", PVM_STACK_STRING (res));
              svalue = xstrdup (PVM_STACK_STRING (res));
              break;
            }
        }
      else
        {
          printf ("run-time error\n");
          return 0;
        }
    }

  /* XXX: endianness, and what not.   */
  pk_io_seek (pk_io_cur (), address, PK_SEEK_SET);

  if (svalue != NULL)
    {
      size_t i, slen;

      slen = strlen (svalue);
      for (i = 0; i < slen; i++)
        {
          if (pk_io_putc ((int) svalue[i]) == PK_EOF)
            printf ("Error writing byte 0x%x to 0x%08jx\n",
                    svalue[i], address);
          else
            printf ("0x%08jx <- 0x%x\n", address, svalue[i]);
        }
      free (svalue);
    }
  else
    {
      if (pk_io_putc ((int) value) == PK_EOF)
        printf ("Error writing byte 0x%x to 0x%08jx\n",
                value, address);
      else
        printf ("0x%08jx <- 0x%x\n", address, value);
    }

  return 1;
}

struct pk_cmd poke_cmd =
  {"poke", "e,?e", PK_CMD_F_REQ_IO | PK_CMD_F_REQ_W, NULL,
   pk_cmd_poke, "poke ADDRESS [,VALUE]"};
