/* ios.h - IO spaces for poke.  */

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

#ifndef IOS_H
#define IOS_H

#include <config.h>
#include <stdint.h>

/* The following two functions intialize and shutdown the IO poke
   subsystem.  */

void ios_init (void);
void ios_shutdown (void);

/* "IO spaces" are the entities used in poke in order to abstract the
   heterogeneous devices that are suitable to be edited, such as
   files, filesystems, memory images of processes, etc.

        "IO spaces"               "IO devices"

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

typedef struct ios *ios;

/* Endianness and negative encoding.

   XXX: explain.  */

enum ios_nenc { IOS_1C, IOS_2C };
enum ios_endian { IOS_LSB, IOS_MSB };

/* IO spaces are bit-addressable.  "Offsets" characterize positions
   into IO spaces.

   Offsets are encoded in 64-bit integers, which denote the number of
   bits since the beginning of the space.  They can be added,
   subtracted and multiplied.

   Since negative offsets are possible, the maximum size of any given
   IO space is 2^60 bytes.  */

typedef int64_t ios_off;

/* The following macros should be used in order to abstract the
   internal representation of the offsets.  */

#define IOS_O_NEW(BYTES,BITS) \
  ((((BYTES) + (BITS) / 8) << 3) | ((BITS) % 0x3))

#define IOS_O_BYTES(O) ((O) >> 3)
#define IOS_O_BITS(O)  ((O) & 0x3)

/* Open an IO space using a handler.  The handler is tried with all
   the supported backends until one recognizes it.  This can be the
   name of a file to open, or an URL, a process PID, etc.

   XXX: document device specs.

   Return the IO space, or NULL if some error occurred (such as an
   invalid handler).  */

ios ios_open (const char *handler);

/* Return the current IO space.  */

ios ios_cur (void);

/* Set the current IO space to IO.  */

void ios_set_cur (ios io);

/* Map over all the IO spaces executing a handler.  */

typedef void (*ios_map_fn) (ios io, void *data);
void ios_map (ios_map_fn cb, void *data);

/* XXX: peek/poke API.
   
   This should include special versions of poke that bypass update
   hooks, and also versions bypassing the cache.
 */

int32_t ios_peek_int (ios io, int bits,
                      ios_off offset,
                      enum ios_endian endian,
                      enum ios_nenc nenc);

uint32_t ios_peek_uint (ios io, int bits,
                        ios_off offset,
                        enum ios_endian endian);

int64_t ios_peek_long (ios io, int bits,
                       ios_off offset,
                       enum ios_endian endian,
                       enum ios_nenc nenc);

uint64_t ios_peek_ulong (ios io, int bits,
                         ios_off offset,
                         enum ios_endian endian);

char *ios_peek_string (ios io, ios_off offset);

/* XXX: 'update' hooks API.  */

typedef struct ios *ios;

/* Return the IO space with the given filename.  Return NULL if no
   such IO space exists.  */

ios ios_search (const char *filename);

/* Return the Nth IO space.  If N is negative or bigger than the
   number of IO spaces, return NULL.  */

ios ios_get (int n);

#endif /* ! IOS_H */
