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

/* IO spaces are bit-addressable.  "Offsets" characterize positions
   into IO spaces.

   Offsets are encoded in 64-bit integers, which denote the number of
   bits since the beginning of the space.  They can be added,
   subtracted and multiplied.

   Since negative offsets are possible, the maximum size of any given
   IO space is 2^60 bytes.  */

typedef int64_t ios_off;

/* The following status codes are used in the several APIs defined
   below in the file.  */

#define IOS_OK 0      /* The operation was performed to completion, in
                         the expected way.  */

#define IOS_ERROR -1  /* An unspecified error condition happened.  */

/* **************** IO space collection API ****************

   The collection of open IO spaces are organized in a global list.
   At every moment some given space is the "current space", unless
   there are no spaces open:

          space1  ->  space2  ->  ...  ->  spaceN
 
                        ^
                        |
                 
                      current

   The functions declared below are used to manage this
   collection.  */

/* Open an IO space using a handler and make it the current space.
   Return IOS_ERROR if there is an error opening the space (such as an
   unrecognized handler), IOS_OK otherwise.  */

int ios_open (const char *handler);

/* Close the given IO space, freing all used resources and flushing
   the space cache associated with the space.  */

void ios_close (ios io);

/* Depending on the underlying IOD, an IO space may allow several
   operations but not others.  For example, a read-only file won't
   allow being written to.  In order to reflect this, every IO space
   features a "mode" bitmap that can be queried by the user using the
   function below.  The several bits in which a given IO space can be
   are summarized in the IOS_M_* constants, also defined below.  */

#define IOS_M_RDWR 1

int ios_mode (ios io);

/* Many IO devices are able to maintain a current read/write pointer.
   The function below can be used to retrieve it, as an IOS
   offset.  */

ios_off ios_tell (ios io);

/* The following function returns the handler operated by the given IO
   space.  */

const char *ios_handler (ios io);

/* Return the current IO space, or NULL if there are no open
   spaces.  */

ios ios_cur (void);

/* Set the current IO space to IO.  */

void ios_set_cur (ios io);

/* Return the IO space operating the given HANDLER.  Return NULL if no
   such space exists.  */

ios ios_search (const char *handler);

/* Return the Nth IO space.  If N is negative or bigger than the
   number of IO spaces which are currently opened, return NULL.  */

ios ios_get (int n);

/* Map over all the open IO spaces executing a handler.  */

typedef void (*ios_map_fn) (ios io, void *data);
void ios_map (ios_map_fn cb, void *data);

/* **************** Object read/write API ****************  */

/* An integer with flags is passed to the read/write operations,
   impacting the way the operation is performed.  */

#define IOS_F_BYPASS_CACHE  1  /* Bypass the IO space cache.  This
                                  makes this write operation to
                                  immediately write to the underlying
                                  IO device.  */

#define IOS_F_BYPASS_UPDATE 2  /* Do not call update hooks that would
                                  be triggered by this write
                                  operation.  Note that this can
                                  obviously lead to inconsistencies
                                  ;) */

/* The functions conforming the read/write API below return an integer
   that reflects the state of the requested operation.  The following
   values are supported, as well as the more generic IOS_OK and
   IOS_ERROR, */
           
#define IOS_EIOFF -2  /* The provided offset is invalid.  This happens
                         for example when the offset translates into a
                         byte offset that exceeds the capacity of the
                         underlying IO device, or when a negative
                         offset is provided in the wrong context.  */

#define IOS_EIOBJ -3  /* A valid object couldn't be found at the
                         requested offset.  This happens for example
                         when an end-of-file condition happens in the
                         underlying IO device.  */

/* When reading and writing integers from/to IO spaces, it is needed
   to specify some details on how the integers values are encoded in
   the underlying storage.  The following enumerations provide the
   supported byte endianness and negative encodings.  The later are
   obviously used when reading and writing signed integers.  */

enum ios_nenc
  {
   IOS_NENC_1, /* One's complement.  */
   IOS_NENC_2  /* Two's complement.  */
  };

enum ios_endian
  {
   IOS_ENDIAN_LSB, /* Byte little endian.  */
   IOS_ENDIAN_MSB  /* Byte big endian.  */
  };

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

int ios_read_string (ios io, ios_off offset, int flags, char **value);

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
   the value.  */

int ios_write_uint (ios io, ios_off offset, int flags,
                    int bits,
                    enum ios_endian endian,
                    uint64_t value);

/* Write the NULL-terminated string in VALUE to the space IO, at the
   given OFFSET.  */

int ios_write_string (ios io, ios_off offset, int flags,
                      const char *value);

/* **************** Update API **************** */

/* XXX: writeme.  */

/* **************** Transaction API **************** */

/* XXX: writeme.  */

#endif /* ! IOS_H */
