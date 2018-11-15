/* ios.c - IO spaces for poke.  */

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

#include <config.h>
#include <xalloc.h>
#include <stdio.h>
#include <gettext.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#define _(str) gettext (str)

#include "ios.h"
#include "ios-dev.h"

/* The following struct implements an instance of an IO space.

   HANDLER is a copy of the handler string used to open the space.

   DEV is the device operated by the IO space.
   DEV_IF is the interface to use when operating the device.

   NEXT is a pointer to the next open IO space, or NULL.

   XXX: add status, saved or not saved, also mode.
 */

struct ios
{
  char *handler;
  void *dev;
  struct ios_dev_if *dev_if;

  struct ios *next;
};

/* List of IO spaces, and pointer to the current one.  */

static struct ios *io_list;
static struct ios *cur_io;

/* The available backends are implemented in their own files, and
   provide the following interfaces.  */

extern struct ios_dev_if ios_dev_file; /* ios-dev-file.c */

static struct ios_dev_if *ios_dev_ifs[] =
  {
   &ios_dev_file,
   NULL,
  };

void
ios_init (void)
{
  /* Nothing to do here... yet.  */
}

void
ios_shutdown (void)
{
  /* Close and free all open IO spaces.  */
  while (io_list)
    ios_close (io_list);
}

int
ios_open (const char *handler)
{
  struct ios *io = NULL;
  struct ios_dev_if **dev_if = NULL;

  /* Allocate and initialize the new IO space.  */
  io = xmalloc (sizeof (struct ios));
  io->next = NULL;
  io->handler = xstrdup (handler);

  /* Look for a device interface suitable to operate on the given
     handler.  */
  for (dev_if = ios_dev_ifs; *dev_if; ++dev_if)
    {
      if ((*dev_if)->handler_p (handler))
        break;
    }

  if (dev_if == NULL)
    goto error;

  io->dev_if = *dev_if;
  
  /* Open the device using the interface found above.  */
  io->dev = io->dev_if->open (handler);
  if (io->dev == NULL)
    goto error;

  /* Add the newly created space to the list, and update the current
     space.  */
  if (io_list == NULL)
    io_list = io;
  else
    {
      io->next = io_list;
      io_list = io;
    }

  cur_io = io;

  return 1;

 error:
  free (io);
  if (io)
    free (io->handler);

  return 0;
}

void
ios_close (ios io)
{
  struct ios *tmp;
  
  /* XXX: if not saved, ask before closing.  */

  /* Close the device operated by the IO space.
     XXX: handle errors.  */
  assert (io->dev_if->close (io->dev));

  /* Unlink the IOS from the list.  */
  assert (io_list != NULL); /* The list must contain at least one
                               ios.  */
  if (io_list == io)
    io = io_list->next;
  else
    {
      for (tmp = io_list; tmp->next != io; tmp = tmp->next)
        ;
      tmp->next = io->next;
    }
  free (io);
  
  /* Set the new current IO.  */
  cur_io = io_list;
}

ios
ios_cur (void)
{
  return cur_io;
}

void
ios_set_cur (ios io)
{
  cur_io = io;
}

ios
ios_search (const char *handler)
{
  ios io;

  for (io = io_list; io; io = io->next)
    if (strcmp (io->handler, handler) == 0)
      break;

  return io;
}

ios
ios_get (int n)
{
  ios io;
  
  if (n < 0)
    return NULL;

  for (io = io_list; io && n > 0; n--, io = io->next)
    ;

  return io;
}

void
ios_map (ios_map_fn cb, void *data)
{
  ios io;

  for (io = io_list; io; io = io->next)
    (*cb) (io, data);
}

int
ios_read_int (ios io, ios_off offset, int flags,
              int bits,
              enum ios_endian endian,
              enum ios_nenc nenc,
              int64_t *value)
{
  /* XXX: writeme  */
  *value = 666;
  return IOS_OK;
}

int
ios_read_uint (ios io, ios_off offset, int flags,
               int bits,
               enum ios_endian endian,
               uint64_t *value)
{
  /* XXX: writeme  */
  *value = 666;
  return IOS_OK;
}

int
ios_read_string (ios io, ios_off offset, int flags, char **value)
{
  /* XXX: writeme.  */
  *value = xstrdup ("bleh");
  return IOS_OK;
}

int
ios_write_int (ios io, ios_off offset, int flags,
               int bits,
               enum ios_endian endian,
               enum ios_nenc nenc,
               int64_t value)
{
#if 0
  char aux;
  int nbytes;
  uint64_t uvalue;

  uint8_t head_mask, tail_mask;

  int v_head = 64 - bits;  /* Size of the unused head in the uvalue.  */
  int o_head = offset % 8; /* Size of the unused head in the IOD.  */

  /* Handle the negative encoding, changing the bits in `value' as
     needed.  */

  switch (nenc)
    {
    case IOS_NENC_2:
      /* No need to convert here.  */
      uvalue = value;
      break;
    case IOS_NENC_1: /* XXX supportme  */
    default:
      assert (1);
      break;
    }

  /* XXX the code after this point should be the same for signed and
     unsigned integers.  */
  
  /* Now shift the value in order to match the final disposition in
     the IOD bytes.

     Origin of most significant bit: 64 - bits
     Destination of most significant bit: offset % 8

                    value
     +---------------------------------+
     |                                 |
     +---------------------------------+

                   uvalue                   aux
     +------+---...-------------+------+  +------+
     | head |                   | tail |  | tail |
     +------+---...-------------+------+  +------+
  */

  if (v_head == o_head)
    /* No need to shift uvalue.  */
  else if (v_head > o_head)
    {
      /* We have to shift left.  */
      uvalue <<= v_head - o_head;
    }
  else /* v_head < o_head  */
    {
      /* We need to save part of the value in `aux' in this case, as
         the final number of bytes to update in the IOD exceeds
         64-bit.  */

      aux = (char) ((uvalue & 0xff) << (8 - (o_head - v_head)));
      uvalue >>= o_head - v_head;
    }

  /* Write the bytes of `uvalue' and `aux' to the IOD.  The bytes may
     need to be "completed" from the existing information in the IOD,
     using appropriate masks.  */

  if (offset < 0 ||
      io->dev_if->seek (io->dev, offset / 8, IOD_SEEK_SET) == -1)
    return IOS_EIOFF;

  for (n = 0; n < 8; ++n)
    {
      /* Calculate mask for the current byte.  */

      /* If the mask is not zero, read the byte from the IOD and
         complete the value in uvalue.  */

      /* Write the resulting byte to the IOD.  */
      
      if (io->dev_if->putc (io->dev, uvalue >> (64 - n) & 0xff) == IOD_EOF)
        return IOS_EIOFF;
    }

  if (io->dev_if->putc (io->dev, tail) == IOD_EOF)
    return IOS_EIOFF;
#endif

  return IOS_OK;
}

int
ios_write_uint (ios io, ios_off offset, int flags,
                int bits,
                enum ios_endian endian,
                uint64_t value)
{
  /* XXX: writeme.  */
  return IOS_OK;
}

int
ios_write_string (ios io, ios_off offset, int flags,
                  const char *value)
{
  /* XXX: writeme.  */
  return IOS_OK;
}
