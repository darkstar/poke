/* pk-io.c - IO access for poke.  Definitions.  */

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

#ifndef PK_IO_H
#define PK_IO_H

#include <config.h>
#include <stdio.h>
#include <fcntl.h>

#define PK_EOF EOF

#define PK_SEEK_SET SEEK_SET
#define PK_SEEK_CUR SEEK_CUR
#define PK_SEEK_END SEEK_END

/* Offset into an IO stream.  */
typedef off_t pk_io_off;

/* Type representing an IO stream, and accessor macros.  */

#define PK_IO_FILE(io) ((io)->file)
#define PK_IO_FILENAME(io) ((io)->filename)
#define PK_IO_MODE(io) ((io)->mode)

struct pk_io
{
  FILE *file;
  char *filename;
  mode_t mode;
  /* XXX: status saved or not saved.  */

  struct pk_io *next;
};

typedef struct pk_io *pk_io;

/* Create an IO stream reading and writing to FILENAME and set it as
   the current stream.  Return 0 if there was an error opening the
   file, 1 otherwise.  */

int pk_io_open (const char *filename);

/* Close the given IO stream and perform any other cleanup.  */

void pk_io_close (pk_io io);

/* Return the current position in the given IO stream.  Return -1 on
   error.  */

pk_io_off pk_io_tell (pk_io io);

/* Change the current position in the given IO according to OFFSET and
   WHENCE.  WHENCE can be one of PK_SEEK_SET, PK_SEEK_CUR and
   PK_SEEK_END.  Return 0 on successful completion, and -1 on
   error.  */

int pk_io_seek (pk_io io, pk_io_off offset, int whence);

/* Read the next character from the current IO stream and return it as
   an unsigned char cast to an int, or PK_EOF on end of file or
   error.  */

int pk_io_getc (void);

/* Write a character in the current IO stream.  Return the character
   written as an unsigned char cast ot an int, or PK_EOF on error.  */

int pk_io_putc (int c);

/* Return the current IO stream.  */

pk_io pk_io_cur (void);

/* Set the current IO stream to IO.  */

void pk_io_set_cur (pk_io io);

/* Map over all the IO streams executing a handler.  */

typedef void (*pk_io_map_fn) (pk_io io, void *data);
void pk_io_map (pk_io_map_fn cb, void *data);

/* Return the IO stream with the given filename.  Return NULL if no
   such IO stream exists.  */

pk_io pk_io_search (const char *filename);

/* Return the Nth IO stream.  If N is negative or bigger than the
   number of IO streams, return NULL.  */

pk_io pk_io_get (int n);

/* Shutdown the IO subsystem.  */

void pk_io_shutdown (void);

#endif /* ! PK_IO_H */
