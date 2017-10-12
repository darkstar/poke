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
pk_cmd_poke_byte (pk_io_off address, uint8_t byte)
{
  if (pk_io_putc ((int) byte) == PK_EOF)
    {
      printf ("Error writing byte 0x%x to 0x%08jx\n",
              byte, address);
      return 0;
    }

  printf ("0x%08jx <- 0x%x\n", address, byte);
  return 1;
}

static int
pk_cmd_poke (int argc, struct pk_cmd_arg argv[])
{
  /* poke ADDR, VAL */

  pk_io_off address;
  pvm_program prog;
  pvm_val val;
  size_t i;
  int pvm_ret;

  assert (argc == 2);

  assert (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_EXP);
  prog = PK_CMD_ARG_EXP (argv[0]);

  pvm_ret = pvm_run (prog, &val);
  if (pvm_ret != PVM_EXIT_OK)
    goto rterror;

  if (!PVM_IS_NUMBER (val) || PVM_VAL_NUMBER (val) < 0)
    {
      printf ("Bad ADDRESS.\n");
      return 0;
    }
      
  address = PVM_VAL_NUMBER (val);

  if (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_NULL)
    pk_cmd_poke_byte (address, 0);
  else
    {
      assert (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_EXP);
      prog = PK_CMD_ARG_EXP (argv[1]);

      pvm_ret = pvm_run (prog, &val);
      if (pvm_ret != PVM_EXIT_OK)
        goto rterror;

      pk_io_seek (pk_io_cur (), address, PK_SEEK_SET);

      if (PVM_IS_BYTE (val) || PVM_IS_UBYTE (val))
        {
          uint8_t pval = PVM_VAL_BYTE (val);
          pk_cmd_poke_byte (address, pval);
        }
      else if (PVM_IS_HALF (val) || PVM_IS_UHALF (val))
        {
          uint16_t pval = PVM_VAL_HALF (val);

          pk_cmd_poke_byte (address, (pval >> 8) & 0xff);
          pk_cmd_poke_byte (address, pval & 0xff);
        }
      else if (PVM_IS_INT (val) || PVM_IS_UINT (val))
        {
          uint32_t pval = PVM_VAL_NUMBER (val);

          pk_cmd_poke_byte (address, (pval >> 24) & 0xff);
          pk_cmd_poke_byte (address, (pval >> 16) & 0xff);
          pk_cmd_poke_byte (address, (pval >> 8) & 0xff);
          pk_cmd_poke_byte (address, pval & 0xff);
        }
      else if (PVM_IS_LONG (val) || PVM_IS_ULONG (val))
        {
          uint64_t pval = PVM_VAL_NUMBER (val);

          pk_cmd_poke_byte (address, (pval >> 56) & 0xff);
          pk_cmd_poke_byte (address, (pval >> 48) & 0xff);
          pk_cmd_poke_byte (address, (pval >> 40) & 0xff);
          pk_cmd_poke_byte (address, (pval >> 32) & 0xff);
          pk_cmd_poke_byte (address, (pval >> 24) & 0xff);
          pk_cmd_poke_byte (address, (pval >> 16) & 0xff);
          pk_cmd_poke_byte (address, (pval >> 8) & 0xff);
          pk_cmd_poke_byte (address, pval & 0xff);
        }
      else if (PVM_IS_STRING (val))
        {
          const char *pval = PVM_VAL_STR (val);
          size_t slen = strlen (pval);

          for (i = 0; i < slen; i++)
            pk_cmd_poke_byte (address, pval[i]);
        }
      else
        assert (0); /* XXX support more types.  */
    }

  return 1;

 rterror:
  printf ("run-time error: %s\n", pvm_error (pvm_ret));
  return 0;
}

struct pk_cmd poke_cmd =
  {"poke", "e,?e", PK_CMD_F_REQ_IO | PK_CMD_F_REQ_W, NULL,
   pk_cmd_poke, "poke ADDRESS [,VALUE]"};
