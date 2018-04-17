/* pk-cmd.c - Poke commands.  */

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
#include <xalloc.h>
#include <ctype.h>
#include <gettext.h>
#define _(str) dgettext (PACKAGE, str)

#include "poke.h"
#include "pkl.h" /* For pkl_compile_expression */
#include "pkl-parser.h"
#include "pk-io.h"
#include "pk-cmd.h"

/* Table of supported commands.  */

extern struct pk_cmd dump_cmd; /* pk-dump.c  */
extern struct pk_cmd peek_cmd; /* pk-peek.c  */
extern struct pk_cmd poke_cmd; /* pk-poke.c  */
extern struct pk_cmd file_cmd; /* pk-file.c  */
extern struct pk_cmd close_cmd; /* pk-file.c */
extern struct pk_cmd load_cmd; /* pk-file.c */
extern struct pk_cmd info_cmd; /* pk-info.c  */
extern struct pk_cmd exit_cmd; /* pk-misc.c  */
extern struct pk_cmd version_cmd; /* pk-misc.c */
extern struct pk_cmd help_cmd; /* pk-help.c */
extern struct pk_cmd vm_cmd; /* pk-vm.c  */
extern struct pk_cmd print_cmd; /* pk-print.c */

struct pk_cmd null_cmd =
  {NULL, NULL, NULL, 0, NULL, NULL};

static struct pk_cmd *cmds[] =
  {
    &peek_cmd,
    &poke_cmd,
    &dump_cmd,
    &file_cmd,
    &exit_cmd,
    &version_cmd,
    &info_cmd,
    &close_cmd,
    &load_cmd,
    &help_cmd,
    &vm_cmd,
    &print_cmd,
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
pk_atoi (char **p, int64_t *number)
{
  long int li;
  char *end;

  errno = 0;
  li = strtoll (*p, &end, 0);
  if ((errno != 0 && li == 0)
      || end == *p)
    return 0;

  *number = li;
  *p = end;
  return 1;
}

/* Little implementation of prefix trees, or tries.  This is used in
   order to support calling to commands and subcommands using
   unambiguous prefixes.  It is also a pretty efficient way to decode
   command names.  */

struct pk_trie
{
  char c;
  struct pk_trie *parent;
  int num_children;
  struct pk_trie *children[256];
  struct pk_cmd *cmd;
};

static struct pk_trie *
pk_trie_new (char c, struct pk_trie *parent)
{
  struct pk_trie *trie;
  size_t i;

  trie = xmalloc (sizeof (struct pk_trie));
  trie->c = c;
  trie->parent = parent;
  trie->cmd = NULL;
  trie->num_children = 0;
  for (i = 0; i < 256; i++)
    trie->children[i] = NULL;

  return trie;
}

static void
pk_trie_free (struct pk_trie *trie)
{
  int i;

  if (trie == NULL)
    return;

  for (i = 0; i < 256; i++)
    pk_trie_free (trie->children[i]);

  free (trie);
  return;
}

static void
pk_trie_expand_cmds (struct pk_trie *root,
                     struct pk_trie *trie)
{
  size_t i;
  struct pk_trie *t;

  if (trie->cmd != NULL)
    {
      t = trie->parent;
      while (t != root && t->num_children == 1)
        {
          t->cmd = trie->cmd;
          t = t->parent;
        }
    }
  else
    for (i = 0; i < 256; i++)
      {
        if (trie->children[i] != NULL)
          pk_trie_expand_cmds (root, trie->children[i]);
      }
}

static struct pk_trie *
pk_trie_from_cmds (struct pk_cmd *cmds[])
{
  size_t i;
  struct pk_trie *root;
  struct pk_trie *t;
  struct pk_cmd *cmd;

  root = pk_trie_new (' ', NULL);
  t = root;

  for (i = 0, cmd = cmds[0];
       cmd->name != NULL;
       cmd = cmds[++i])
    {
      const char *p;

      for (p = cmd->name; *p != '\0'; p++)
        {
          int c = *p;

          if (t->children[c] == NULL)
            {
              t->num_children++;
              t->children[c] = pk_trie_new (c, t);
            }
          t = t->children[c];
        }

      /* Note this assumes no commands with empty names.  */
      t->cmd = cmd;
      t = root;
    }

  pk_trie_expand_cmds (root, root);
  return root;
}

static struct pk_cmd *
pk_trie_get_cmd (struct pk_trie *trie, const char *str)
{
  const char *pc;

  for (pc = str; *pc; pc++)
    {
      int n = *pc;
      
      if (trie->children[n] == NULL)
        return NULL;

      trie = trie->children[n];
    }

  return trie->cmd;
}

#if 0
static void
pk_print_trie (int indent, struct pk_trie *trie)
{
  size_t i;

  for (i = 0; i < indent; i++)
    printf (" ");
  printf ("TRIE:: '%c' cmd='%s'\n",
          trie->c, trie->cmd != NULL ? trie->cmd->name : "NULL");

  for (i =0 ; i < 256; i++)
    if (trie->children[i] != NULL)
      pk_print_trie (indent + 2, trie->children[i]);
}
#endif

/* Routines to execute a command.  */

#define MAX_CMD_NAME 18

static int
pk_cmd_exec_1 (char *str, struct pk_trie *cmds_trie, char *prefix)
{
  int ret = 1;
  size_t i;
  char cmd_name[MAX_CMD_NAME], *p;
  struct pk_cmd *cmd;
  int argc;
  struct pk_cmd_arg argv[8];
  uint64_t uflags;
  const char *a;
  char filename[NAME_MAX];
  int besilent = 0;

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

  /* Look for the command in the prefix table.  */
  cmd = pk_trie_get_cmd (cmds_trie, cmd_name);
  if (cmd == NULL)
    {
      if (prefix != NULL)
        printf ("%s ", prefix);
      printf (_("%s: command not found.\n"), cmd_name);
      return 0;      
    }

  /* Process user flags.  */
  uflags = 0;
  if (*p == '/')
    {
      p++;
      while (isalpha (*p))
        {
          int fi;
          for (fi = 0; cmd->uflags[fi]; fi++)
            if (cmd->uflags[fi] == *p)
              {
                uflags |= 1 << fi;
                break;
              }

          if (cmd->uflags[fi] == '\0')
            {
              printf (_("%s: invalid flag `%c'\n"), cmd_name, *p);
              return 0;
            }

          p++;
        }
    }

  /* If this command has subcommands, process them and be done.  */
  if (cmd->subtrie != NULL)
    {
      p = skip_blanks (p);
      if (*p == '\0')
        goto usage;
      return pk_cmd_exec_1 (p, *cmd->subtrie, cmd_name);
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
                case 'd':
                  {
                    /* Process a poke definition.  */
                    char *program_string;
                    char *end;

                    /* The command name (deftype, defvar, etc) is the
                       first part of the program to compile.  */
                    program_string = xmalloc (strlen (cmd_name) + strlen (p)
                                              + 1);
                    strcpy (program_string, cmd_name);
                    strcat (program_string, " ");
                    strcat (program_string, p);

                    if (!pkl_compile_buffer (poke_compiler,
                                             program_string,
                                             &end))
                      {
                        /* The compiler should have emitted diagnostic
                           messages, so don't bother the user with the
                           usage message.  */
                        besilent = 1;
                      }

                    match = 1;
                    p = end;
                    free (program_string);
                    break;
                  }
                case 'e':
                  {
                    /* Compile a poke expression.  */
                    pvm_program prog;
                    char *end;

                    prog = pkl_compile_expression (poke_compiler,
                                                   p, &end);
                    
                    if (prog != NULL)
                      {
                        argv[argc].val.exp = prog;
                        argv[argc].type = PK_CMD_ARG_EXP;
                        match = 1;
                        p = end;
                      }
                    else
                      /* The compiler should have emitted diagnostic
                         messages, so don't bother the user with the
                         usage message.  */
                      besilent = 1;
                       
                    break;
                  }
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
                  assert (0);
                }
              
              if (match)
                break;
              
              /* Rewind input and try next option.  */
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
      puts (_("This command requires an IO stream.  Use the `file' command."));
      return 0;
    }

  if (cmd->flags & PK_CMD_F_REQ_W)
    {
      pk_io cur_io = pk_io_cur ();
      if (cur_io == NULL
          || !(PK_IO_MODE (cur_io) & O_RDWR))
        {
          puts (_("This command requires a writable IO stream."));
          return 0;
        }
    }

  /* Call the command handler, passing the arguments.  */
  ret = (*cmd->handler) (argc, argv, uflags);

  /* Free arguments occupying memory.  */
  for (i = 0; i < argc; ++i)
    {
      if (argv[i].type == PK_CMD_ARG_EXP)
        pvm_destroy_program (argv[i].val.exp);
    }
  
  return ret;

 usage:
  if (!besilent)
    printf (_("Usage: %s\n"), cmd->usage);
  return 0;
}

extern struct pk_cmd *info_cmds[]; /* pk-info.c  */
extern struct pk_trie *info_trie; /* pk-info.c  */

extern struct pk_cmd *help_cmds[]; /* pk-help.c */
extern struct pk_trie *help_trie; /* pk-help.c */

extern struct pk_cmd *vm_cmds[]; /* pk-vm.c  */
extern struct pk_trie *vm_trie;  /* pk-vm.c  */

static struct pk_trie *cmds_trie;

int
pk_cmd_exec (char *str)
{
  if (cmds_trie == NULL)
    cmds_trie = pk_trie_from_cmds (cmds);
  if (info_trie == NULL)
    info_trie = pk_trie_from_cmds (info_cmds);
  if (help_trie == NULL)
    help_trie = pk_trie_from_cmds (help_cmds);
  if (vm_trie == NULL)
    vm_trie = pk_trie_from_cmds (vm_cmds);

  return pk_cmd_exec_1 (str, cmds_trie, NULL);
}

void
pk_cmd_shutdown (void)
{
  pk_trie_free (cmds_trie);
  pk_trie_free (info_trie);
  pk_trie_free (help_trie);
  pk_trie_free (vm_trie);
}
