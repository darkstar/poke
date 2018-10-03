/* pk-io-file.c - IO backend to handle file devices.  */

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

#include <fcntl.h>
#include <stdio.h>
#include <xalloc.h>

#include "pk-io.h"

/* State associated with a file device.  */

struct pk_io_file
{
  FILE *file;
  char *filename;
  mode_t mode;
};

static int
pk_io_file_init (void)
{
  /* Nothing to do here.  */
  return 1;
}

static int
pk_io_file_fini (void)
{
  /* Nothing to do here.  */
  return 1;
}

static int
pk_io_file_open (void *iod, const char *handler)
{
  struct pk_io_file *iof = iod;

  const char *mode;
  pk_io io;
  FILE *f;
  mode_t fmode = 0;

  /* Open the requested file.  The open mode is read-write if
     possible.  Otherwise read-only.  */
  if (access (handler, R_OK | W_OK) != 0)
    {
      fmode |= O_RDONLY;
      mode = "rb";
    }
  else
    {
      fmode |= O_RDWR;
      mode = "r+b";
    }
  
  f = fopen (handler, mode);
  if (!f)
    {
      perror (filename);
      return 0;
    }

  fio->file = f;
  fio->filename = xstrdup (handler);
  fio->mode = mode;

  return 1;
}

static int
pk_io_file_close (void *iod)
{
  struct pk_io_file *fio = iod;

  if (fclose (fio->file) != 0)
    perror (fio->filename);
  free (fio->filename);
}

static int
pk_io_file_getc (void *iod)
{
  struct pk_io_file *fio = iod;
  return fgetc (fio->file);
}

static int
pk_io_file_putc (void *iod, int c)
{
  struct pk_io_file *fio = iod;
  int ret = putc (c, fio->file);
  return ret == EOF ? PK_EOF : ret;
}

static pk_io_boff
pk_io_file_tell (void *iod)
{
  struct pk_io_file *fio = iod;
  return ftello (fio->file);
}

static int
pk_io_file_seek (void *iod, pk_io_boff offset, int whence)
{
  struct pk_io_file *fio = iod;
  return fseeko (fio->file, offset, whence);
}

struct pk_io_be pk_io_file =
  {
   .open_fn = pk_io_file_open,
   .close_fn = pk_io_file_close,
   .tell_fn = pk_io_file_tell,
   .seek_fn = pk_io_file_seek,
   .getc_fn = pk_io_file_getc,
   .putc_fn = pk_io_file_putc,
  };
