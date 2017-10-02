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

/* Types and constants.  */

#define PK_EOF EOF

#define PK_SEEK_SET SEEK_SET
#define PK_SEEK_CUR SEEK_CUR
#define PK_SEEK_END SEEK_END

typedef off_t pk_io_off;

/* Set FILENAME as the current IO stream and open it.  Return 0 if
   there was an error opening the file, 1 otherwise.  */

int pk_io_open (const char *filename);

/* Close the IO stream and perform any other cleanup.  */

void pk_io_close (void);

/* Return the current position in the current IO stream.  Return -1 on
   error.  */

pk_io_off pk_io_tell (void);

/* Change the current IO position according to OFFSET and WHENCE.
   WHENCE can be one of PK_SEEK_SET, PK_SEEK_CUR and PK_SEEK_END.
   Return 0 on successful completion, and -1 on error.  */

int pk_io_seek (pk_io_off offset, int whence);

/* Read the next character from the IO stream and return it as an
   unsigned char cast to an int, or PK_EOF on end of file or
   error.  */

int pk_io_getc (void);

#endif /* ! PK_IO_H */
