/* ios-dev-file.c - File IO devices.  */

/* Copyright (C) 2019 Jose E. Marchesi */

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
#include <stdlib.h>
#include <unistd.h>

/* We want 64-bit file offsets in all systems.  */
#define _FILE_OFFSET_BITS 64

#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <xalloc.h>
#include <string.h>

#include "ios-dev.h"

/* State associated with a file device.  */

struct ios_dev_file
{
  FILE *file;
  char *filename;
  char *mode;
};

static int
ios_dev_file_handler_p (const char *handler)
{
  /* The compiler should be able to optimize the strlen of the
     constant string...   GCC does on -O */
#define FILEH "file://"
  return (strlen (handler) > strlen (FILEH)
          && strncmp (handler, FILEH, strlen (FILEH)) == 0);
#undef FILEH
}

static void *
ios_dev_file_open (const char *handler)
{
  const char *mode;
  struct ios_dev_file *fio;
  FILE *f;

  /* Skip the file:// part in the handler.  */
  handler += strlen ("file://");

  /* Open the requested file.  The open mode is read-write if
     possible.  Otherwise read-only.  */

  mode =
    access (handler, R_OK | W_OK) != 0 ? "rb" : "r+b";
  
  f = fopen (handler, mode);
  if (!f)
    {
      perror (handler);
      return 0;
    }

  fio = xmalloc (sizeof (struct ios_dev_file));
  fio->file = f;
  fio->filename = xstrdup (handler);
  fio->mode = xstrdup (mode);

  return fio;
}

static int
ios_dev_file_close (void *iod)
{
  struct ios_dev_file *fio = iod;

  if (fclose (fio->file) != 0)
    perror (fio->filename);
  free (fio->filename);
  free (fio->mode);

  return 1;
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
  return ret == EOF ? IOD_EOF : ret;
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
    case IOD_SEEK_SET: fwhence = SEEK_SET; break;
    case IOD_SEEK_CUR: fwhence = SEEK_CUR; break;
    case IOD_SEEK_END: fwhence = SEEK_END; break;
    default:
      assert (0);
    }

  return fseeko (fio->file, offset, fwhence);
}

struct ios_dev_if ios_dev_file =
  {
   .handler_p = ios_dev_file_handler_p,
   .open = ios_dev_file_open,
   .close = ios_dev_file_close,
   .tell = ios_dev_file_tell,
   .seek = ios_dev_file_seek,
   .getc = ios_dev_file_getc,
   .putc = ios_dev_file_putc,
  };
