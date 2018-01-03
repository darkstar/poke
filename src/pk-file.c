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

#include "poke.h"
#include "pk-cmd.h"
#include "pk-io.h"

static int
pk_cmd_file (int argc, struct pk_cmd_arg argv[])
{
  /* file FILENAME */

  assert (argc == 1);

  if (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_TAG)
    {
      /* Switch to an already opened IO stream.  */

      int io_id;
      pk_io io;

      io_id = PK_CMD_ARG_TAG (argv[0]);
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
      const char *filename = PK_CMD_ARG_STR (argv[0]);
      
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
pk_cmd_close (int argc, struct pk_cmd_arg argv[])
{
  /* close [#ID]  */
  pk_io io;
  int changed;

  assert (argc == 1);

  if (PK_CMD_ARG_TYPE (argv[0]) == PK_CMD_ARG_NULL)
    io = pk_io_cur ();
  else
    {
      int io_id = PK_CMD_ARG_TAG (argv[0]);

      io = pk_io_get (io_id);
      if (io == NULL)
        {
          printf ("No such file #%d\n", io_id);
          return 0;
        }
    }

  changed = (io == pk_io_cur ());
  pk_io_close (io);

  if (changed)
    {
      if (pk_io_cur () == NULL)
        puts ("No more IO streams.");
      else
        printf ("The current file is now `%s'.\n",
                PK_IO_FILENAME (pk_io_cur ()));
    }
  
  return 1;
}

static void
print_info_file (pk_io io, void *data)
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
  pk_io_map (print_info_file, &id);

  return 1;
}

struct pk_cmd file_cmd =
  {"file", "tf", 0, NULL, pk_cmd_file, "file (FILENAME|#ID)"};


struct pk_cmd close_cmd =
  {"close", "?t", PK_CMD_F_REQ_IO, NULL, pk_cmd_close, "close [#ID]"};

struct pk_cmd info_files_cmd =
  {"files", "", 0, NULL, pk_cmd_info_files, "info files"};
