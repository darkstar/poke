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

/* XXX: IO backends provide access to "devices", which can be files,
   processes, etc.

   XXX: IO devices are byte-oriented, which means they are oblivious
   to endianness, alignment and negative encoding considerations.  */

typedef uint64_t ios_dev_off;

/* The macros below are used in the device interface.  */

#define IOD_EOF -1
#define IOD_SEEK_SET 0
#define IOD_SEEK_CUR 1
#define IOD_SEEK_END 2

/* Each IO backend should implement the following interface, by
   filling an instance of the struct defined below.

   HANDLER_P (HANDLER) -> INT

     Determine whether the provided HANDLER is recognized as a valid
     device spec by this backend.  Return 1 if the handler is
     recognized, 0 otherwise.

   OPEN (HANDLER) -> IOD

     Open a device using the provided HANDLER.  Return the opened
     device, or NULL if there was an error, such as an unrecognized
     handler.

   CLOSE (DEV) -> INT

     Close the given device.  Return 0 if there was an error during
     the operation, 1 otherwise.

   TELL (DEV) -> OFFSET
   
     Return the current position in the given device.  Return -1 on
     error.

   SEEK (DEV, OFFSET, WHENCE) -> INT
   
     Change the current position in the given device according to
     OFFSET and WHENCE.  WHENCE can be one of PK_SEEK_SET, PK_SEEK_CUR
     and PK_SEEK_END.  Return 0 on successful completion, and -1 on
     error.

   GETC (DEV) -> INT

     Read a byte from the given device at the current position.
     Return the byte in an int, or PK_EOF on error.

   PUTC (DEV) -> INT

     Write a byte to the given device at the current position.  Return
     the character written as an int, or PK_EOF on error.
*/

struct ios_dev_if
{
  int (*handler_p) (const char *handler);

  void *(*open) (const char *handler);
  int (*close) (void *dev);

  ios_dev_off (*tell) (void *dev);
  int (*seek) (void *dev, ios_dev_off offset, int whence);

  int (*getc) (void *dev);
  int (*putc) (void *iod, int c);
};

