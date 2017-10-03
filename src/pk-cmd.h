/* pk-cmd.h - Poke commands.  Definitions.  */

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

#ifndef PK_H_CMD
#define PK_H_CMD

#include <config.h>

#include "pk-io.h"

/* Commands management.  */

enum pk_cmd_arg_type
{
  PK_CMD_ARG_NULL,
  PK_CMD_ARG_INT,
  PK_CMD_ARG_ADDR,
  PK_CMD_ARG_STR,
  PK_CMD_ARG_TAG
};

struct pk_cmd_arg
{
  enum pk_cmd_arg_type type;
  union
  {
    long int integer;
    pk_io_off addr;
    const char *str;
    long int tag;
  } val;
};

typedef int (*pk_cmd_fn) (int argc, struct pk_cmd_arg argv[]);

#define PK_CMD_F_REQ_IO 0x1  /* Command requires an IO stream.  */
#define PK_CMD_F_REQ_W  0x2  /* Command requires a writable IO stream.  */

struct pk_cmd
{
  /* Name of the command.  It is a NULL-terminated string composed by
     alphanumeric characters and '_'.  */
  const char *name;
  /* String specifying the arguments accepted by the command.  */
  const char *arg_fmt;
  /* A value composed of or-ed PK_CMD_F_* flags.  See above.  */
  int flags;
  /* Subcommands.  */
  struct pk_cmd *sub;
  /* Function implementing the command.  */
  pk_cmd_fn handler;
  /* Usage message.  */
  const char *usage;
};

int pk_cmd_exec (char *str);

#endif /* ! PK_H_CMD */
