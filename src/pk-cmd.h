/* pk-cmd.h - Poke commands.  Definitions.  */

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

#ifndef PK_H_CMD
#define PK_H_CMD

#include <config.h>

#include "pvm.h" /* For pvm_program */
#include "pk-io.h"

enum pk_cmd_arg_type
{
  PK_CMD_ARG_NULL,
  PK_CMD_ARG_EXP,
  PK_CMD_ARG_DEF,
  PK_CMD_ARG_INT,
  PK_CMD_ARG_ADDR,
  PK_CMD_ARG_STR,
  PK_CMD_ARG_TAG
};

#define PK_CMD_ARG_TYPE(arg) ((arg).type)
#define PK_CMD_ARG_EXP(arg) ((arg).val.exp)
#define PK_CMD_ARG_DEF(arg) 0
#define PK_CMD_ARG_INT(arg) ((arg).val.integer)
#define PK_CMD_ARG_ADDR(arg) ((arg).val.addr)
#define PK_CMD_ARG_STR(arg) ((arg).val.str)
#define PK_CMD_ARG_TAG(arg) ((arg).val.tag)

struct pk_cmd_arg
{
  enum pk_cmd_arg_type type;
  union
  {
    pvm_program exp;
    int64_t integer;
    pk_io_off addr;
    const char *str;
    int64_t tag;
  } val;
};

typedef int (*pk_cmd_fn) (int argc, struct pk_cmd_arg argv[], uint64_t uflags);

#define PK_CMD_F_REQ_IO 0x1  /* Command requires an IO stream.  */
#define PK_CMD_F_REQ_W  0x2  /* Command requires a writable IO stream.  */

struct pk_cmd
{
  /* Name of the command.  It is a NULL-terminated string composed by
     alphanumeric characters and '_'.  */
  const char *name;
  /* String specifying the arguments accepted by the command.  */
  const char *arg_fmt;
  /* String specifying the user flags accepted by the command.  */
  const char *uflags;
  /* A value composed of or-ed PK_CMD_F_* flags.  See above.  */
  int flags;
  /* Subcommands.  */
  struct pk_trie **subtrie;
  /* Function implementing the command.  */
  pk_cmd_fn handler;
  /* Usage message.  */
  const char *usage;
};

/* Parse STR and execute a command.  */

int pk_cmd_exec (char *str);

/* Shutdown the cmd subsystem, freeing all used resources.  */

void pk_cmd_shutdown (void);

#endif /* ! PK_H_CMD */
