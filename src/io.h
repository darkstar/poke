/* io.h - IO spaces for poke.  Definitions.  */

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

#ifndef IO_H
#define IO_H

#include <config.h>
#include <stdint.h>

/* The following two functions intialize and shutdown the IO poke
   subsystem.  */

void io_init (void);
void io_shutdown (void);

/* "IO spaces" are the entities used in poke in order to abstract the
   heterogeneous devices that are suitable to be edited, such as
   files, filesystems, memory images of processes, etc.

        "spaces"                     "devices"

   Space of IO objects <=======> Space of bytes
                             
                             +------+  
                      +----->| File |
       +-------+      |      +------+
       |  IO   |      |       
       | space |<-----+      +---------+
       |       |      +----->| Process |
       +-------+      |      +---------+
                   
                      :           :
                   
                      |      +------------+
                      +----->| Filesystem |
                             +------------+

   IO spaces are bit-addressable spaces of "IO objects", which can be
   generally read (peeked) and written (poked).  The kind of objects
   supported are:

   - "ints", which are signed integers from 1 to 64 bits wide.  They
     can be stored using either msb or lsb endianness.  Negative
     quantities are encoded using one of the supported negative
     encodings.

   - "uints", which are unsigned integers from 1 to 64 bits wide.
     They can be stored using either msb or lsb endianness.

   - "strings", which are sequences of bytes terminated by a NULL
     byte, much like C strings.

   IO spaces also provide caching capabilities, transactions,
   serialization of concurrent accesses, and more goodies.  */

/* IO spaces are bit-addressable.  "Offsets" characterize positions
   into IO spaces.

   Offsets are encoded in 64-bit integers, which denote the number of
   bits since the beginning of the space.  They can be added,
   subtracted and multiplied.

   Since negative offsets are possible, the maximum size of any given
   IO space is 2^60 bytes.  */

typedef int64_t io_off;

/* The following macros should be used in order to abstract the
   internal representation of the offsets.  */

#define IO_O_NEW(BYTES,BITS) \
  ((((BYTES) + (BITS) / 8) << 3) | ((BITS) % 0x3))

#define IO_O_BYTES(O) ((O) >> 3)
#define IO_O_BITS(O)  ((O) & 0x3)

/* Open an IO space using a handler.  The handler is tried with all
   the supported backends until one recognizes it.  This can be the
   name of a file to open, or an URL, a process PID, etc.  See XXX.

   Return the IO space, or NULL if some error occurred (such as an
   invalid handler).  */

io io_open (const char *handler);

/* Return the current IO space.  */

io io_cur (void);

/* Set the current IO space to IO.  */

void io_set_cur (io io);

/* Map over all the IO spaces executing a handler.  */

typedef void (*io_map_fn) (io io, void *data);
void io_map (io_map_fn cb, void *data);

/* XXX: peek/poke API.
   
   This should include special versions of poke that bypass update
   hooks, and also versions bypassing the cache.
 */

int32_t io_peek_int (io io, int bits,
                        io_off offset, io_endian endian,
                        io_nenc nenc);

uint32_t io_peek_uint (io io, int bits,
                          io_off offset, io_endian endian);

int64_t io_peek_long (io io, int bits,
                         io_off offset, io_endian endian,
                         io_nenc nenc);

uint64_t io_peek_ulong (io io, int bits,
                           io_ff offset, io_endian endian);

char *io_peek_string (io io, io_off offset);

/* XXX: 'update' hooks API.  */

/* Type representing an IO space, and accessor macros.  */

#define IO_FILE(io) ((io)->file)
#define IO_FILENAME(io) ((io)->filename)
#define IO_MODE(io) ((io)->mode)

struct io
{
  /* XXX: status saved or not saved.  */

  struct io *next;
};

typedef struct io *io;

/* Return the IO space with the given filename.  Return NULL if no
   such IO space exists.  */

io io_search (const char *filename);

/* Return the Nth IO space.  If N is negative or bigger than the
   number of IO spaces, return NULL.  */

io io_get (int n);

#endif /* ! IO_H */
