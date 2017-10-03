/* pk-cmd.c - Poke commands.  */

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
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <assert.h>
#include <wordexp.h> /* For tilde-expansion.  */

#include "poke.h"
#include "pk-io.h"
#include "pk-cmd.h"

/* Table of supported commands.  */

extern struct pk_cmd dump_cmd; /* pk-dump.c  */
extern struct pk_cmd peek_cmd; /* pk-peek.c  */
extern struct pk_cmd poke_cmd; /* pk-poke.c  */
extern struct pk_cmd file_cmd; /* pk-file.c  */
extern struct pk_cmd info_cmd; /* pk-info.c  */
extern struct pk_cmd exit_cmd; /* pk-misc.c  */

struct pk_cmd null_cmd =
  {NULL, NULL, 0, NULL, NULL};

static struct pk_cmd *cmds[] =
  {
    &peek_cmd,
    &poke_cmd,
    &dump_cmd,
    &file_cmd,
    &exit_cmd,
    &info_cmd,
    &null_cmd
  };

/* Convenience macros and functions for parsing.  */

static inline char *
skip_blanks (char *p)
{
  while (isblank (*p))
    p++;
  return p;
}

static inline int
pk_atoi (char **p, long int *number)
{
  long int li;
  char *end;

  errno = 0;
  li = strtol (*p, &end, 0);
  if ((errno != 0 && li == 0)
      || end == *p)
    return 0;

  *number = li;
  *p = end;
  return 1;
}

/* Routines to execute a command.  */

#define MAX_CMD_NAME 18

static int
pk_cmd_exec_1 (char *str, struct pk_cmd *cmds[], char *prefix)
{
  int ret = 1;
  size_t i;
  char cmd_name[MAX_CMD_NAME], *p;
  struct pk_cmd *cmd;
  int argc;
  struct pk_cmd_arg argv[8];
  const char *a;
  char filename[NAME_MAX];


  /* Skip blanks, and return if the command is composed by only blank
     characters.  */
  p = skip_blanks (str);
  if (*p == '\0')
    return 0;

  /* Get the command name.  */
  i = 0;
  while (isalnum (*p) || *p == '_')
    cmd_name[i++] = *(p++);
  cmd_name[i] = '\0';

  /* Find the command in the commands table.  */
  for (i = 0, cmd = cmds[0];
       cmd->name != NULL;
       cmd = cmds[++i])
    {
      if (strcmp (cmd->name, cmd_name) == 0)
        break;
    }
  if (cmd->name == NULL)
    {
      if (prefix != NULL)
        printf ("%s ", prefix);
      printf ("%s: command not found.\n", cmd_name);
      return 0;
    }

  /* If this command has subcommands, process them and be done.  */
  if (cmd->sub != NULL)
    {
      p = skip_blanks (p);
      if (*p == '\0')
        goto usage;
      return pk_cmd_exec_1 (p, cmd->sub, cmd_name);
    }
  
  /* Parse arguments.  */
  argc = 0;
  a = cmd->arg_fmt;
  while (*a != '\0')
    {
      /* Handle an argument. */
      int match = 0;

      p = skip_blanks (p);
      if (*a == '?' && ((*p == ',' || *p == '\0')))
        {
          if (*p == ',')
            p++;
          argv[argc].type = PK_CMD_ARG_NULL;
          match = 1;
        }
      else
        {
          if (*a == '?')
            a++;
          
          /* Try the different options, in order, until one succeeds or
             the next argument or the end of the input is found.  */
          while (*a != ',' && *a != '\0')
            {
              char *beg = p;
              
              switch (*a)
                {
                case 'i':
                case 'n':
                  /* Parse an integer or natural.  */
                  p = skip_blanks (p);
                  if (pk_atoi (&p, &(argv[argc].val.integer))
                      && (*a == 'i' || argv[argc].val.integer >= 0))
                    {
                      p = skip_blanks (p);
                      if (*p == ',' || *p == '\0')
                        {
                          argv[argc].type = PK_CMD_ARG_INT;
                          match = 1;
                        }
                    }
                  
                  break;
                case 'a':
                  /* Parse an address.  */
                  p = skip_blanks (p);
                  if (pk_atoi (&p, &(argv[argc].val.addr)))
                    {
                      p = skip_blanks (p);
                      if (*p == ',' || *p == '\0')
                        {
                          argv[argc].type = PK_CMD_ARG_ADDR;
                          match = 1;
                        }
                    }
                  
                  break;
                case 't':
                  /* Parse a #N tag.  */
                  p = skip_blanks (p);
                  if (*p == '#'
                      && p++
                      && pk_atoi (&p, &(argv[argc].val.tag))
                      && argv[argc].val.tag >= 0)
                    {
                      if (*p == ',' || *p == '\0')
                        {
                          argv[argc].type = PK_CMD_ARG_TAG;
                          match = 1;
                        }
                    }
                  
                  break;
                case 'f':
                  {
                    /* Parse a filename.  */

                    size_t i;
                    wordexp_t exp_result;

                    p = skip_blanks (p);
                    i = 0;
                    while (!isblank (*p) && *p != '\0' && *p != ',')
                      filename[i++] = *(p++);
                    filename[i] = '\0';

                    if (filename[0] == '\0')
                      goto usage;

                    switch (wordexp (filename, &exp_result, 0))
                      {
                      case 0: /* Successful.  */
                        break;
                      case WRDE_NOSPACE:
                        wordfree (&exp_result);
                      default:
                        goto usage;
                      }

                    if (exp_result.we_wordc != 1)
                      {
                        wordfree (&exp_result);
                        goto usage;
                      }

                    strcpy (filename, exp_result.we_wordv[0]);
                    wordfree (&exp_result);

                    if (*p == ',' || *p == '\0')
                      {
                        argv[argc].type = PK_CMD_ARG_STR;
                        argv[argc].val.str = filename;
                        match = 1;
                      }
                    
                    break;
                  }
                default:
                  /* This should NOT happen.  */
                  assert (1);
                }
              
              if (match)
                break;
              

              /* Rewind input and investigate next option.  */
              p = beg;
              a++;
            }
        }
      
      /* Boo, could not find valid input for this argument.  */
      if (!match)
        goto usage;

      if (*p == ',')
        p++;

      /* Skip any further options for this argument.  */
      while (*a != ',' && *a != '\0')
        a++;
      if (*a == ',')
        a++;

      /* Ok, next argument!  */
      argc++;
    }

  /* Make sure there is no trailer contents in the input.  */
  p = skip_blanks (p);
  if (*p != '\0')
    goto usage;

  /* Process command flags.  */
  if (cmd->flags & PK_CMD_F_REQ_IO
      && pk_io_cur () == NULL)
    {
      puts ("This command requires an IO stream.  Use the `file' command.");
      return 0;
    }

  if (cmd->flags & PK_CMD_F_REQ_W)
    {
      pk_io cur_io = pk_io_cur ();
      if (cur_io == NULL
          || !(PK_IO_MODE (cur_io) & O_RDWR))
        puts ("This command requires a writable IO stream.");
    }

  /* Call the command handler, passing the arguments.  */
  ret = (*cmd->handler) (argc, argv);
  
  return ret;

 usage:
  printf ("Usage: %s\n", cmd->usage);
  return 0;
}

int
pk_cmd_exec (char *str)
{
  return pk_cmd_exec_1 (str, cmds, NULL);
}
