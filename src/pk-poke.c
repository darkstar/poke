/* pk-poke.c - `poke' command.  */

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
#include <assert.h>
#include <gettext.h>
#define _(str) dgettext (PACKAGE, str)

#include "ios.h"
#include "pvm.h"
#include "poke.h"
#include "pk-cmd.h"

static int
poke_byte (ios_off *address, uint8_t byte)
{
  uint64_t value;

  if (ios_read_uint (ios_cur (), *address, 0, 8,
                     IOS_ENDIAN_MSB /* irrelevant  */,
                     &value) != IOS_OK)
    {
      /* XXX: printing the ADDRESS as bits like this can be confusing.
         Print an offset instead.  */
      printf (_("Error writing byte 0x%x to 0x%08jx#b\n"),
              byte, *address);
      return 0;
    }

  printf ("0x%08jx <- 0x%02x\n", *address, byte);
  *address += 8;
  return 1;
}

static int
poke_val (ios_off *address, pvm_val val)
{
  /* XXX: endianness and negative encoding.  */

  if (PVM_IS_INT (val))
    {
      /*      int32_t pval = PVM_VAL_INT (val);
              int size = PVM_VAL_INT_SIZE (val); */

      
    }

  /*
  if (PVM_IS_BYTE (val) || PVM_IS_UBYTE (val))
    {
      uint8_t pval = PVM_VAL_BYTE (val);
      poke_byte (address, pval);
    }
  else if (PVM_IS_HALF (val) || PVM_IS_UHALF (val))
    {
      uint16_t pval = PVM_VAL_HALF (val);
      
      poke_byte (address, (pval >> 8) & 0xff);
      poke_byte (address, pval & 0xff);
    }
  else if (PVM_IS_INT (val) || PVM_IS_UINT (val))
    {
      uint32_t pval = PVM_VAL_INTEGRAL (val);
      
      poke_byte (address, (pval >> 24) & 0xff);
      poke_byte (address, (pval >> 16) & 0xff);
      poke_byte (address, (pval >> 8) & 0xff);
      poke_byte (address, pval & 0xff);
    }
  else if (PVM_IS_LONG (val) || PVM_IS_ULONG (val))
    {
      uint64_t pval = PVM_VAL_INTEGRAL (val);
      
      poke_byte (address, (pval >> 56) & 0xff);
      poke_byte (address, (pval >> 48) & 0xff);
      poke_byte (address, (pval >> 40) & 0xff);
      poke_byte (address, (pval >> 32) & 0xff);
      poke_byte (address, (pval >> 24) & 0xff);
      poke_byte (address, (pval >> 16) & 0xff);
      poke_byte (address, (pval >> 8) & 0xff);
      poke_byte (address, pval & 0xff);
      }*/
  else if (PVM_IS_ARR (val))
    {
      size_t nelem;
      size_t idx;

      nelem = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (val));
      for (idx = 0; idx < nelem; idx++)
        poke_val (address, PVM_VAL_ARR_ELEM_VALUE (val, idx));
    }
  else if (PVM_IS_SCT (val))
    {
      size_t nelem;
      size_t idx;

      nelem = PVM_VAL_ULONG (PVM_VAL_SCT_NELEM (val));
      for (idx = 0; idx < nelem; idx++)
        poke_val (address, PVM_VAL_SCT_ELEM_VALUE (val, idx));
    }
  else if (PVM_IS_STR (val))
    {
      size_t i;
      const char *pval = PVM_VAL_STR (val);
      size_t slen = strlen (pval);
      
      for (i = 0; i < slen; i++)
        poke_byte (address, pval[i]);
    }
  else
    assert (0); /* XXX support more types.  */

  return 1;
}

static int
pk_cmd_poke (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* poke ADDR, VAL */

  ios_off address;
  pvm_program prog;
  pvm_val val;
  int pvm_ret;

  assert (argc == 2);

  assert (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_EXP);
  prog = PK_CMD_ARG_EXP (argv[0]);

  pvm_ret = pvm_run (poke_vm, prog, &val);
  if (pvm_ret != PVM_EXIT_OK)
    goto rterror;

  if (!PVM_IS_INTEGRAL (val) || PVM_VAL_INTEGRAL (val) < 0)
    {
      printf (_("Bad ADDRESS.\n"));
      return 0;
    }
      
  address = PVM_VAL_INTEGRAL (val);

  if (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_NULL)
    poke_byte (&address, 0);
  else
    {
      assert (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_EXP);
      prog = PK_CMD_ARG_EXP (argv[1]);

      pvm_ret = pvm_run (poke_vm, prog, &val);
      if (pvm_ret != PVM_EXIT_OK)
        goto rterror;

      if (!poke_val (&address, val))
        goto error;
    }

  return 1;

 rterror:
  printf ("run-time error\n");
 error:
  return 0;
}

struct pk_cmd poke_cmd =
  {"poke", "e,?e", "", PK_CMD_F_REQ_IO | PK_CMD_F_REQ_W, NULL,
   pk_cmd_poke, "poke ADDRESS [,VALUE]"};
