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
#define _(str) gettext (str)

/* The current IO file.  */

static FILE *pk_io_file;
static char *pk_io_filename;

int
pk_io_open (const char *filename)
{
  pk_io_file = fopen (filename, "rb");
  if (!pk_io_file)
    {
      perror (filename);
      return 0;
    }

  pk_io_filename = xstrdup (filename);
  
  return 1;
}

void
pk_io_close (void)
{
  if (pk_io_file && fclose (pk_io_file) != 0)
    perror (pk_io_filename);
  free (pk_io_filename);
  pk_io_file = NULL;
  pk_io_filename = NULL;
}

int
pk_io_getc (void)
{
  return fgetc (pk_io_file);
}

pk_io_off
pk_io_tell (void)
{
  return ftello (pk_io_file);
}

int
pk_io_seek (pk_io_off offset, int whence)
{
  return fseeko (pk_io_file, offset, whence);
}

int
pk_io_p (void)
{
  return pk_io_file != NULL;
}
