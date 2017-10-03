/* pk-io.c - IO access for poke.  */

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
#include "pk-io.h"
#include <xalloc.h>
#include <stdio.h>
#include <gettext.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <xalloc.h>
#define _(str) gettext (str)

/* List of IO streams, and pointer to the current one.  */

static struct pk_io *ios;
static struct pk_io *cur_io;

int
pk_io_open (const char *filename)
{
  const char *mode;
  pk_io io;
  FILE *f;
  mode_t fmode = 0;

  /* Open the requested file.  The open mode is read-write if
     possible.  Otherwise read-only.  */

  if (access (filename, R_OK | W_OK) != 0)
    {
      fmode |= O_RDONLY;
      mode = "rb";
    }
  else
    {
      fmode |= O_RDWR;
      mode = "r+b";
    }
  
  f = fopen (filename, mode);
  if (!f)
    {
      perror (filename);
      return 0;
    }

  /* Allocate and initialize the new IO stream.  */
  io = xmalloc (sizeof (struct pk_io));
  io->next = NULL;
  io->mode = fmode;
  io->file = f;
  io->filename = xstrdup (filename);

  /* Add it to the list, and update the current stream.  */
  if (ios == NULL)
    ios = io;
  else
    {
      io->next = ios;
      ios = io;
    }

  cur_io = io;
  
  return 1;
}

void
pk_io_close (pk_io io)
{
  struct pk_io *tmp;
  
  /* Close the file stream and free resources.  */
  /* XXX: if not saved, ask before closing.  */
  if (fclose (io->file) != 0)
    perror (io->filename);
  free (io->filename);

  /* Unlink the IO from the list.  */

  assert (ios != NULL); /* The list must contain at least io.  */

  if (ios == io)
    ios = ios->next;
  else
    {
      for (tmp = ios; tmp->next != io; tmp = tmp->next)
        ;
      tmp->next = io->next;
    }
  free (io);
  
  /* Set the new current IO.  */
  cur_io = ios;
}

int
pk_io_getc (void)
{
  return fgetc (cur_io->file);
}

int
pk_io_putc (int c)
{
  int ret = putc (c, cur_io->file);
  return ret == EOF ? PK_EOF : ret;
}

pk_io_off
pk_io_tell (pk_io io)
{
  return ftello (io->file);
}

int
pk_io_seek (pk_io io, pk_io_off offset, int whence)
{
  return fseeko (io->file, offset, whence);
}

pk_io
pk_io_cur (void)
{
  return cur_io;
}

void
pk_io_set_cur (pk_io io)
{
  cur_io = io;
}

void
pk_io_map (pk_io_map_fn cb, void *data)
{
  pk_io io;

  for (io = ios; io; io = io->next)
    (*cb) (io, data);
}

pk_io
pk_io_search (const char *filename)
{
  pk_io io;

  for (io = ios; io; io = io->next)
    if (strcmp (io->filename, filename) == 0)
      break;

  return io;
}

pk_io
pk_io_get (int n)
{
  pk_io io;
  
  if (n < 0)
    return NULL;

  for (io = ios; io && n > 0; n--, io = io->next)
    ;

  return io;
}

void
pk_io_shutdown (void)
{
  /* Close and free all open IO streams.  */
  while (ios)
    pk_io_close (ios);
}
