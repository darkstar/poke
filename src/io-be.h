/* io-be.h - IO backend interface.  */

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

typedef uint64_t io_boff;

#define PK_EOF -1

#define PK_SEEK_SET 0
#define PK_SEEK_CUR 1
#define PK_SEEK_END 2

/* The struct below represents the interface for IO backends.  */

struct io_be
{
  /* Backend initialization.  This hook is invoked exactly once,
     before any other backend hook.  Return 1 if the initialization is
     successful, 0 otherwise.  */

  int (*init) (void);

  /* Backend finalization.  This hook is invoked exactly one, and
     subsequently no other backend hook is ever invoked with the
     exception of `init'.  Return 1 if the finalization is successful,
     0 otherwise.  */

  int (*fini) (void);

  /* Determine whether the provided HANDLER is recognized as a valid
     device by this backend.  Return 1 if the handler is recognized, 0
     otherwise.  */

  int (*handler_p) (const char *handler);

  /* Open a device using the provided HANDLER.  Return the opened
     device, or NULL if there was an error, such as an unrecognized
     handler.  */

  void *(*open) (const char *handler);

  /* Close the given device.  */

  int (*close) (void *iod);

  /* Return the current position in the given device.  Return -1 on
     error.  */

  io_boff (*tell) (void *iod);

  /* Change the current position in the given device according to
     OFFSET and WHENCE.  WHENCE can be one of PK_SEEK_SET, PK_SEEK_CUR
     and PK_SEEK_END.  Return 0 on successful completion, and -1 on
     error.  */

  int (*seek) (void *iod, io_boff offset, int whence);

  /* Read a byte from the given device at the current position.
     Return the byte in an int, or PK_EOF on error.  */

  int (*getc) (void *iod);

  /* Write a byte to the given device at the current position.  Return
     the character written as an int, or PK_EOF on error.  */

  int (*putc) (void *iod, int c);
};
