/* ios-dev-file.c - File IO devices.  */

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

/* We want 64-bit file offsets in all systems.  */
#define _FILE_OFFSET_BITS 64

#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <xalloc.h>

#include "ios-dev.h"

/* State associated with a file device.  */

struct ios_dev_file
{
  FILE *file;
  char *filename;
  mode_t mode;
};

static int
ios_dev_file_handler_p (const char *handler)
{
  /* XXX: implement handler formats.  */
  return 1;
}

static void *
ios_dev_file_open (void *iod, const char *handler)
{
  struct ios_dev_file *iof = iod;

  const char *mode;
  ios_dev io;
  FILE *f;

  /* XXX: parse the filename and the offset from HANDLER.  */

  /* Open the requested file.  The open mode is read-write if
     possible.  Otherwise read-only.  */

  mode =
    access (handler, R_OK | W_OK) != 0 ? "rb" : "r+b";
  
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
ios_dev_file_close (void *iod)
{
  struct ios_dev_file *fio = iod;

  if (fclose (fio->file) != 0)
    perror (fio->filename);
  free (fio->filename);
}

static int
ios_dev_file_getc (void *iod)
{
  struct ios_dev_file *fio = iod;
  return fgetc (fio->file);
}

static int
ios_dev_file_putc (void *iod, int c)
{
  struct ios_dev_file *fio = iod;
  int ret = putc (c, fio->file);
  return ret == EOF ? PK_EOF : ret;
}

static ios_dev_off
ios_dev_file_tell (void *iod)
{
  struct ios_dev_file *fio = iod;
  return ftello (fio->file);
}

static int
ios_dev_file_seek (void *iod, ios_dev_off offset, int whence)
{
  struct ios_dev_file *fio = iod;
  int fwhence;

  switch (whence)
    {
    case PK_SEEK_SET: fwhence = SEEK_SET; break;
    case PK_SEEK_CUR: fwhence = SEEK_CUR; break;
    case PK_SEEK_END: fwhence = SEEK_END; break;
    default:
      assert (0);
    }
  
  return fseeko (fio->file, offset, fwhence);
}

struct ios_dev_if ios_dev_file =
  {
   .open = ios_dev_file_open,
   .close = ios_dev_file_close,
   .tell = ios_dev_file_tell,
   .seek = ios_dev_file_seek,
   .getc = ios_dev_file_getc,
   .putc = ios_dev_file_putc,
  };
