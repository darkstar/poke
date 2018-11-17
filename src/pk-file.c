/* pk-file.c - Commands for operating files.  */

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
#include <assert.h>
#include <unistd.h>
#include <gettext.h>
#define _(str) dgettext (PACKAGE, str)

#include "ios.h"
#include "poke.h"
#include "pk-cmd.h"

static int
pk_cmd_file (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* file FILENAME */

  assert (argc == 1);

  if (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_TAG)
    {
      /* Switch to an already opened IO space.  */

      int io_id;
      ios io;

      io_id = PK_CMD_ARG_TAG (argv[0]);
      io = ios_get (io_id);
      if (io == NULL)
        {
          printf (_("No such file #%d\n"), io_id);
          return 0;
        }

      ios_set_cur (io);
    }
  else
    {
      /* Create a new IO space.  */
      const char *arg_str = PK_CMD_ARG_STR (argv[0]);
      char *filename
        = xmalloc (strlen ("file://") + strlen (arg_str) + 1);

      if (access (arg_str, R_OK) != 0)
        {
          printf (_("%s: file cannot be read\n"), arg_str);
          return 0;
        }
      
      strcpy (filename, "file://");
      strcat (filename, arg_str);
      
      if (ios_search (filename) != NULL)
        {
          printf (_("File %s already opened.  Use `file #N' to switch.\n"),
                  filename);
          return 0;
        }
      
      ios_open (filename);
      free (filename);
    }

  if (poke_interactive_p)
    printf (_("The current file is now `%s'.\n"),
            ios_handler (ios_cur ()) + strlen ("file://"));

  return 1;
}

static int
pk_cmd_close (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* close [#ID]  */
  ios io;
  int changed;

  assert (argc == 1);

  if (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_NULL)
    io = ios_cur ();
  else
    {
      int io_id = PK_CMD_ARG_TAG (argv[0]);

      io = ios_get (io_id);
      if (io == NULL)
        {
          printf (_("No such file #%d\n"), io_id);
          return 0;
        }
    }

  changed = (io == ios_cur ());
  ios_close (io);

  if (changed)
    {
      if (ios_cur () == NULL)
        puts (_("No more IO spaces."));
      else
        printf (_("The current file is now `%s'.\n"),
                ios_handler (ios_cur ()));
    }
  
  return 1;
}

static void
print_info_file (ios io, void *data)
{
  int *i = (int *) data;
  printf ("%s#%d\t%s\t0x%08jx#b\t%s\n",
          io == ios_cur () ? "* " : "  ",
          (*i)++,
          ios_mode (io) & IOS_M_RDWR ? "rw" : "r ",
          ios_tell (io), ios_handler (io));
}

static int
pk_cmd_info_files (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  int id;

  assert (argc == 0);

  id = 0;
  printf (_("  Id\tMode\tPosition\tFilename\n"));
  ios_map (print_info_file, &id);

  return 1;
}

static int
pk_cmd_load_file (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* load FILENAME */

  const char *filename;
  pvm_program program;
  int pvm_ret;
  pvm_val val;

  assert (argc == 1);
  filename = PK_CMD_ARG_STR (argv[0]);

  if (access (filename, R_OK) != 0)
    {
      printf (_("%s: file cannot be read\n"), filename);
      return 0;
    }

  program = pkl_compile_file (poke_compiler, filename);
  if (program == NULL)
    /* Note that the compiler emits it's own error messages.  */
    return 0;

  pvm_ret = pvm_run (poke_vm, program, &val);
  if (pvm_ret != PVM_EXIT_OK)
    {
      printf (_("run-time error: %s\n"), pvm_error (pvm_ret));
      return 0;
    }

  return 1;
}

struct pk_cmd file_cmd =
  {"file", "tf", "", 0, NULL, pk_cmd_file, "file (FILENAME|#ID)"};

struct pk_cmd close_cmd =
  {"close", "?t", "", PK_CMD_F_REQ_IO, NULL, pk_cmd_close, "close [#ID]"};

struct pk_cmd info_files_cmd =
  {"files", "", "", 0, NULL, pk_cmd_info_files, "info files"};

struct pk_cmd load_cmd =
  {"load", "f", "", 0, NULL, pk_cmd_load_file, "load FILENAME"};
