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
   XXX explain.  */

enum ios_nenc
  {
   IOS_NENC_1, /* One's complement.  */
   IOS_NENC_2  /* Two's complement.  */
  };

enum ios_endian
  {
   IOS_ENDIAN_LSB, /* Little endian.  */
   IOS_ENDIAN_MSB  /* Big endian.  */
  };

/* IO spaces are bit-addressable.  "Offsets" characterize positions
   into IO spaces.

   Offsets are encoded in 64-bit integers, which denote the number of
   bits since the beginning of the space.  They can be added,
   subtracted and multiplied.

   Since negative offsets are possible, the maximum size of any given
   IO space is 2^60 bytes.  */

typedef int64_t ios_off;

/* The following macros should be used in order to abstract the
   internal representation of the offsets.

   XXX: how to make negative offsets with IOS_O_NEW?  */

#define IOS_O_NEW(BYTES,BITS) \
  ((((BYTES) + (BITS) / 8) << 3) | ((BITS) % 0x3))

#define IOS_O_BYTES(O) ((O) >> 3)
#define IOS_O_BITS(O)  ((O) & 0x3)

/* Open an IO space using a handler and make it the current space.
   The handler is tried with all the supported backends until one
   recognizes it.  This can be the name of a file to open, or an URL,
   a process PID, etc.

   Return 0 if there is an error opening the space (such as an
   unrecognized handler), 1 otherwise.  */

int ios_open (const char *handler);

/* Return the current IO space.  */

ios ios_cur (void);

/* Set the current IO space to IO.  */

void ios_set_cur (ios io);

/* Map over all the IO spaces executing a handler.  */

typedef void (*ios_map_fn) (ios io, void *data);
void ios_map (ios_map_fn cb, void *data);

/* **************** Object read/write API ****************  */

/* An integer with flags is passed to the read/write operations,
   impacting the way the operation is performed.  */

#define IOS_F_BYPASS_CACHE  1  /* Bypass the object cache.  XXX.  */

#define IOS_F_BYPASS_UPDATE 2  /* Do not call update hooks that would
                                  be triggered by this write
                                  operation.  Note that this can
                                  obviously lead to inconsistencies
                                  ;) */

/* The functions conforming the read/write API below return an integer
   that reflects the state of the requested operation.  */
           
#define IOS_OK 0      /* The operation was performed to completion, in
                         the expected way.  */

#define IOS_ERROR -1  /* An unspecified error condition happened.  */

#define IOS_EIOFF -2  /* The provided offset is invalid.  This happens
                         for example when the offset translates intoa
                         byte offset that exceeds the capacity of the
                         underlying IO device, or when a negative
                         offset is provided in the wrong context.  */

#define IOS_EIOBJ -3  /* A valid object couldn't be found at the
                         requested offset.  This happens for example
                         when an end-of-file condition happens in the
                         underlying IO device.  */

/* Read a signed integer of size BITS located at the given OFFSET, and
   put its value in VALUE.  It is assumed the integer is encoded using
   the ENDIAN byte endianness and NENC negative encoding.  */

int ios_read_int (ios io, ios_off offset, int flags,
                  int bits,
                  enum ios_endian endian,
                  enum ios_nenc nenc,
                  int64_t *value);

/* Read an unsigned integer of size BITS located at the given OFFSET,
   and put its value in VALUE.  It is assumed the integer is encoded
   using the ENDIAN byte endianness.  */

int ios_read_uint (ios io, ios_off offset, int flags,
                   int bits,
                   enum ios_endian endian,
                   uint64_t *value);

/* Read a NULL-terminated string of bytes located at the given OFFSET,
   and put its value in VALUE.  It is up to the caller to free the
   memory occupied by the returned string, when no longer needed.  */

int ios_read_string (ios io, ios_off offset, int flags, char *value);

/* Write the signed integer of size BITS in VALUE to the space IO, at
   the given OFFSET.  Use the byte endianness ENDIAN and encoding NENC
   when writing the value.  */

int ios_write_int (ios io, ios_off offset, int flags,
                   int bits,
                   enum ios_endian endian,
                   enum ios_nenc nenc,
                   int64_t value);

/* Write the unsigned integer of size BITS in VALUE to the space IO,
   at the given OFFSET.  Use the byte endianness ENDIAN when writing
   the value. */

int ios_write_uint (ios io, ios_off offset, int flags,
                    int bits,
                    enum ios_endian endian,
                    uint64_t value);

/* Write the NULL-terminated string in VALUE to the space IO, at the
   given OFFSET.  */

int ios_write_string (ios io, ios_off offset, int flags,
                      const char *value);

/* **************** Updating API **************** */

/* XXX: writeme.  */

/* Return the IO space with the given filename.  Return NULL if no
   such IO space exists.  */

ios ios_search (const char *filename);

/* Return the Nth IO space.  If N is negative or bigger than the
   number of IO spaces, return NULL.  */

ios ios_get (int n);

#endif /* ! IOS_H */
