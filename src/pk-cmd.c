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
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <assert.h>
#include <wordexp.h> /* For tilde-expansion.  */

#include "poke.h"
#include "pk-io.h"
#include "pk-cmd.h"

/* Escape sequences for changing text attributes on the terminal.  */

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define KBOLD (poke_interactive_p ? "\033[1m" : "")
#define KNONE (poke_interactive_p ? "\033[0m" : "")

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
  if (errno != 0 && li == 0
      || end == *p)
    return 0;

  *number = li;
  *p = end;
  return 1;
}


/* Commands follow.  */

static int
pk_cmd_file (int argc, struct pk_cmd_arg argv[])
{
  /* file FILENAME */

  assert (argc == 1);

  if (argv[0].type == PK_CMD_ARG_TAG)
    {
      /* Switch to an already opened IO stream.  */

      long int io_id;
      pk_io io;

      io_id = argv[0].val.tag;
      io = pk_io_get (io_id);
      if (io == NULL)
        {
          printf ("No such file #%d\n", io_id);
          return 0;
        }

      pk_io_set_cur (io);
    }
  else
    {
      /* Create a new IO stream.  */
      size_t i;
      const char *filename = argv[0].val.str;
      
      if (access (filename, R_OK) != 0)
        {
          printf ("%s: file cannot be read\n", filename);
          return 0;
        }
      
      if (pk_io_search (filename) != NULL)
        {
          printf ("File %s already opened.  Use `file #N' to switch.\n",
                  filename);
          return 0;
        }
      
      pk_io_open (filename);
    }

  if (poke_interactive_p)
    printf ("The current file is now `%s'.\n", PK_IO_FILENAME (pk_io_cur ()));

  return 1;
}

static int
pk_cmd_dump (int argc, struct pk_cmd_arg argv[])
{
  /* dump [ADDR] [,COUNT]  */

  int c = 0;
  char string[18], *p;
  string[0] = ' ';
  string[17] = '\0';
  pk_io_off cur, address, count, top;

  assert (argc == 2);

  if (argv[0].type == PK_CMD_ARG_NULL)
    address = pk_io_tell (pk_io_cur ());
  else
    address = argv[0].val.addr;

  if (argv[1].type == PK_CMD_ARG_NULL)
    count = 8;
  else
    count = argv[1].val.integer;

  top = address + 0x10 * count;

  /* Dump the requested address.  */

  cur = pk_io_tell (pk_io_cur ());
  pk_io_seek (pk_io_cur (), address, PK_SEEK_SET);

  if (poke_interactive_p)
    printf ("%s87654321  0011 2233 4455 6677 8899 aabb ccdd eeff  0123456789ABCDEF%s\n",
            KBOLD, KNONE);
  
  for (; 0 <= c && address < top; address += 0x10)
    {
      int i;
      for (i = 0; i < 16; i++)
        {
          if (0 <= c)
            c = pk_io_getc ();
          if (c < 0)
            {
              if (!i)
                break;
              
              fputs ("  ", stdout);
              string[i + 1] = '\0';
            }
          else
            {
              if (!i)
                printf ("%s%08jx: %s", KBOLD, address, KNONE);
              
              string[i + 1]
                = (c < 0x20 || (0x7F <= c && (c < 0xa0))
                   ? '.'
                   : isprint (c) ? c : '?');
              
              printf ("%02x", c + 0u);
            }

          if (i & 0x1)
            putchar (' ');
        }
      
      if (i)
        puts (string);
    }

  /* XXX: save last position in case the next command is also a
     dump.  */
  /* pk_io_seek (pk_io_cur (), cur, PK_SEEK_SET); */

  return 1;
}

#define MAX_CMD_NAME 18

static void
pk_cmd_info_file (pk_io io, void *data)
{
  int *i = (int *) data;
  printf ("%s#%d\t%s\t0x%08jx\t%s\n",
          io == pk_io_cur () ? "* " : "  ",
          (*i)++,
          PK_IO_MODE (io) & O_RDWR ? "rw" : "r ",
          pk_io_tell (io), PK_IO_FILENAME (io));
}

static int
pk_cmd_info_files (int argc, struct pk_cmd_arg argv[])
{
  int id;

  assert (argc == 0);

  id = 0;
  printf ("  Id\tMode\tPosition\tFilename\n");
  pk_io_map (pk_cmd_info_file, &id);

  return 1;
}

static int
pk_cmd_poke (int argc, struct pk_cmd_arg argv[])
{
  /* poke ADDR, VAL */

  pk_io_off address;
  long int value;

  if (argv[0].type == PK_CMD_ARG_NULL)
    address = pk_io_tell (pk_io_cur ());
  else
    address = argv[0].val.addr;
  
  if (argv[1].type == PK_CMD_ARG_NULL)
    value = 0;
  else
    value = argv[1].val.integer;

  /* XXX: endianness, and what not.   */
  pk_io_seek (pk_io_cur (), address, PK_SEEK_SET);
  if (pk_io_putc ((int) value) == PK_EOF)
    {
      printf ("Error writing byte 0x%x to 0x%08jx\n",
              value, address);
    }
  else
    printf ("0x%08jx <- 0x%x\n", address, value);

  return 1;
}

static int
pk_cmd_exit (int argc, struct pk_cmd_arg argv[])
{
  /* exit CODE */

  int code;
  assert (argc == 1);

  if (argv[0].type == PK_CMD_ARG_NULL)
    code = 0;
  else
    code = (int) argv[0].val.integer;
  
  if (poke_interactive_p)
    {
      /* XXX: if unsaved changes, ask and save.  */
    }

  poke_exit_p = 1;
  poke_exit_code = code;
  return 1;
}

/* Table of commands.  */

static struct pk_cmd info_cmds[] =
  {
    {"files", "", 0, NULL, pk_cmd_info_files, "info files"}
  };

static struct pk_cmd cmds[] =
  {
    {"poke", "a,?i", PK_CMD_F_REQ_IO | PK_CMD_F_REQ_W, NULL, pk_cmd_poke,
     "poke ADDRESS [,VALUE]"},
    {"dump", "?a,?n", PK_CMD_F_REQ_IO, NULL, pk_cmd_dump,
     "dump [ADDRESS] [,COUNT]"},
    {"file", "tf", 0, NULL, pk_cmd_file, "file (FILENAME|#ID)"},
    {"exit", "?i", 0, NULL, pk_cmd_exit, "exit [CODE]"},
    {"info", "", 0, info_cmds, NULL, "info (files)"},
    {NULL, NULL, 0, NULL, NULL}
  };

static int
pk_cmd_exec_1 (char *str, struct pk_cmd cmds[], char *prefix)
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
  for (cmd = &cmds[0]; cmd->name != NULL; cmd++)
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
                      && *a == 'i' || argv[argc].val.integer >= 0)
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
