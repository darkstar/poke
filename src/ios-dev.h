/* ios-dev.h - IO devices interface.  */

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

/* An IO space operates on one or more "IO devices", which are
   abstractions providing byte-oriented operations, such as
   positioning, reading bytes, and writing bytes.  Typical abstracted
   entities are files stored in some filesystem, the memory of a
   process, etc.

   Since the IO devices are byte-oriented, aspects like endianness,
   alignment and negative encoding are not of consideration.

   IOD offsets shall always be interpreted as numbers of bytes.  */

typedef uint64_t ios_dev_off;

/* The following macros are part of the device interface.  */

#define IOD_EOF -1
#define IOD_SEEK_SET 0
#define IOD_SEEK_CUR 1
#define IOD_SEEK_END 2

/* Each IO backend should implement a device interface, by filling an
   instance of the struct defined below.  */

struct ios_dev_if
{
  /* Determine whether the provided HANDLER is recognized as a valid
     device spec by this backend.  Return 1 if the handler is
     recognized, 0 otherwise.  */

  int (*handler_p) (const char *handler);

  /* Open a device using the provided HANDLER.  Return the opened
     device, or NULL if there was an error.  Note that this function
     assumes that HANDLER is recognized as a handler by the backend,
     i.e. HANDLER_P returns 1 if HANDLER is passed to it.  */

  void *(*open) (const char *handler);

  /* Close the given device.  Return 0 if there was an error during

     the operation, 1 otherwise.  */

  int (*close) (void *dev);

  /* Return the current position in the given device.  Return -1 on
     error.  */

  ios_dev_off (*tell) (void *dev);

  /* Change the current position in the given device according to
     OFFSET and WHENCE.  WHENCE can be one of IOD_SEEK_SET,
     IOD_SEEK_CUR and IOD_SEEK_END.  Return 0 on successful
     completion, and -1 on error.  */

  int (*seek) (void *dev, ios_dev_off offset, int whence);

  /* Read a byte from the given device at the current position.
     Return the byte in an int, or IOD_EOF on error.  */

  int (*getc) (void *dev);

  /* Write a byte to the given device at the current position.  Return
     the character written as an int, or IOD_EOF on error.  */
  
  int (*putc) (void *dev, int c);
};

