/* pk-io.c - IO spaces.  Definitions.  */

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

#ifndef PK_IO_H
#define PK_IO_H

#include <config.h>
#include <stdint.h>

/* The following two functions intialize and shutdown the IO poke
   subsystem.  */

void pk_io_init (void);
void pk_io_shutdown (void);

/* "IO spaces" are the entities used in poke in order to abstract the
   heterogeneous devices that are suitable to be edited, such as
   files, filesystems, memory images of processes, etc.

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

typedef int64_t pk_io_off;

/* The following macros should be used in order to abstract the
   internal representation of the offsets.  */

#define PK_IO_O_NEW(BYTES,BITS) \
  ((((BYTES) + (BITS) / 8) << 3) | ((BITS) % 0x3))

#define PK_IO_O_BYTES(O) ((O) >> 3)
#define PK_IO_O_BITS(O)  ((O) & 0x3)

/* Return the current IO space.  */

pk_io pk_io_cur (void);

/* Set the current IO space to IO.  */

void pk_io_set_cur (pk_io io);

/* Map over all the IO spaces executing a handler.  */

typedef void (*pk_io_map_fn) (pk_io io, void *data);
void pk_io_map (pk_io_map_fn cb, void *data);

/* XXX: peek/poke API.
   
   This should include special versions of poke that bypass update
   hooks, and also versions bypassing the cache.
 */

int32_t pk_io_peek_int (pk_io io, int bits,
                        pk_io_off offset, pk_io_endian endian,
                        pk_io_nenc nenc);

uint32_t pk_io_peek_uint (pk_io io, int bits,
                          pk_io_off offset, pk_io_endian endian);

int64_t pk_io_peek_long (pk_io io, int bits,
                         pk_io_off offset, pk_io_endian endian,
                         pk_io_nenc nenc);

uint64_t pk_io_peek_ulong (pk_io io, int bits,
                           pk_io_ff offset, pk_io_endian endian);

char *pk_io_peek_string (pk_io io, pk_io_off offset);

/* XXX: update hooks API.  */

/* XXX: the interface and contents below should be moved to
   pk-io-file.[ch]
*****************************************************************/

#define PK_EOF EOF

#define PK_SEEK_SET SEEK_SET
#define PK_SEEK_CUR SEEK_CUR
#define PK_SEEK_END SEEK_END

/* Type representing an IO space, and accessor macros.  */

#define PK_IO_FILE(io) ((io)->file)
#define PK_IO_FILENAME(io) ((io)->filename)
#define PK_IO_MODE(io) ((io)->mode)

struct pk_io
{
  /* XXX: status saved or not saved.  */

  struct pk_io *next;
};

typedef struct pk_io *pk_io;

/* Return the IO space with the given filename.  Return NULL if no
   such IO space exists.  */

pk_io pk_io_search (const char *filename);

/* Return the Nth IO space.  If N is negative or bigger than the
   number of IO spaces, return NULL.  */

pk_io pk_io_get (int n);


/*********** IO backends *****************************************/


/* XXX: IO backends provide access to "devices", which can be files,
   processes, etc.

   XXX: IO devices are byte-oriented, which means they are oblivious
   to endianness, alignment and negative encoding considerations.  */

typedef uint64_t pk_io_boff;

/* The struct below represents the interface for IO backends.  */

struct pk_io_be
{
  /* Backend initialization.  This hook is invoked exactly once,
     before any other backend hook.

     Return 1 if the initialization is successful, 0 otherwise.  */

  int (*init_fn) (void);

  /* Backend finalization.  This hook is invoked exactly one, and
     subsequently no other backend hook is ever invoked with the
     exception of `init'.  

     Return 1 if the finalization is successful, 0 otherwise.  */

  int (*fini_fn) (void);

  /* Open a device using the IO backend, using the provided HANDLER.
     The contents of the handler are parsed and interpreted by the
     backend.  This can be for example the name of a file to open, or
     an URL, or a process PID.

     Return the opened device, or NULL if there was an error.  */

  void *(*open_fn) (const char *handler);

  /* Close the given device.  */

  int (*close_fn) (void *iod);

  /* Return the current position in the given device.  Return -1 on
     error.  */

  pk_io_boff (*tell_fn) (void *iod);

  /* Change the current position in the given device according to
     OFFSET and WHENCE.  WHENCE can be one of PK_SEEK_SET, PK_SEEK_CUR
     and PK_SEEK_END.  Return 0 on successful completion, and -1 on
     error.  */

  int (*seek_fn) (void *iod, pk_io_boff offset, int whence);

  /* Read a byte from the given device at the current position.
     Return the byte in an int, or PK_EOF on error.  */

  int (*getc_fn) (void *iod);

  /* Write a byte to the given device at the current position.  Return
     the character written as an int, or PK_EOF on error.  */

  int (*putc_fn) (void *iod, int c);
};

#endif /* ! PK_IO_H */
