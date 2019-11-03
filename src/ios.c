/* ios.c - IO spaces for poke.  */

/* Copyright (C) 2019 Jose E. Marchesi */

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
#include <gettext.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#define _(str) gettext (str)
#include <streq.h>

#include "ios.h"
#include "ios-dev.h"

#define STREQ(a, b) (strcmp (a, b) == 0)

#define IOS_GET_C_ERR_CHCK(c, io)	\
{					\
  (c) = io->dev_if->get_c ((io)->dev); 	\
  if ((c) == IOD_EOF)			\
    return IOS_EIOFF;			\
}

#define IOS_READ_INTO_CHARRAY_1BYTE(charray)		\
{							\
  IOS_GET_C_ERR_CHCK((charray)[0], io);			\
}

#define IOS_READ_INTO_CHARRAY_2BYTES(charray)		\
{							\
  IOS_READ_INTO_CHARRAY_1BYTE(charray);			\
  IOS_GET_C_ERR_CHCK((charray)[1], io);			\
}

#define IOS_READ_INTO_CHARRAY_3BYTES(charray)		\
{							\
  IOS_READ_INTO_CHARRAY_2BYTES(charray);		\
  IOS_GET_C_ERR_CHCK((charray)[2], io);			\
}

#define IOS_READ_INTO_CHARRAY_4BYTES(charray)		\
{							\
  IOS_READ_INTO_CHARRAY_3BYTES(charray);		\
  IOS_GET_C_ERR_CHCK((charray)[3], io);			\
}

#define IOS_READ_INTO_CHARRAY_5BYTES(charray)		\
{							\
  IOS_READ_INTO_CHARRAY_4BYTES(charray);		\
  IOS_GET_C_ERR_CHCK((charray)[4], io);			\
}

#define IOS_READ_INTO_CHARRAY_6BYTES(charray)		\
{							\
  IOS_READ_INTO_CHARRAY_5BYTES(charray);		\
  IOS_GET_C_ERR_CHCK((charray)[5], io);			\
}

#define IOS_READ_INTO_CHARRAY_7BYTES(charray)		\
{							\
  IOS_READ_INTO_CHARRAY_6BYTES(charray);		\
  IOS_GET_C_ERR_CHCK((charray)[6], io);			\
}

#define IOS_READ_INTO_CHARRAY_8BYTES(charray)		\
{							\
  IOS_READ_INTO_CHARRAY_7BYTES(charray);		\
  IOS_GET_C_ERR_CHCK((charray)[7], io);			\
}

#define IOS_READ_INTO_CHARRAY_9BYTES(charray)		\
{							\
  IOS_READ_INTO_CHARRAY_8BYTES(charray);		\
  IOS_GET_C_ERR_CHCK((charray)[8], io);			\
}

/* The following struct implements an instance of an IO space.

   HANDLER is a copy of the handler string used to open the space.

   DEV is the device operated by the IO space.
   DEV_IF is the interface to use when operating the device.

   NEXT is a pointer to the next open IO space, or NULL.

   XXX: add status, saved or not saved.
 */

struct ios
{
  char *handler;
  void *dev;
  struct ios_dev_if *dev_if;
  int mode;

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

  if (*dev_if == NULL)
    goto error;

  io->dev_if = *dev_if;

  /* Open the device using the interface found above.  */
  io->dev = io->dev_if->open (handler);
  if (io->dev == NULL)
    goto error;

  /* Add the newly created space to the list, and update the current
     space.  */
  io->next = io_list;
  io_list = io;

  cur_io = io;

  return 1;

 error:
  if (io)
    free (io->handler);
  free (io);

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
  assert (io_list != NULL); /* The list must contain at least one IO
                               space.  */
  if (io_list == io)
    io_list = io_list->next;
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

int
ios_mode (ios io)
{
  return io->mode;
}

ios_off
ios_tell (ios io)
{
  ios_dev_off dev_off = io->dev_if->tell (io->dev);
  return dev_off * 8;
}

const char *
ios_handler (ios io)
{
  return io->handler;
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
    if (STREQ (io->handler, handler))
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

inline static void
ios_mask_first_byte(uint64_t *byte, int significant_bits)
{
  switch (significant_bits)
    {
    case 1:
      *byte &= (char) 0x01;
      return;
    case 2:
      *byte &= (char) 0x03;
      return;
    case 3:
      *byte &= (char) 0x07;
      return;
    case 4:
      *byte &= (char) 0x0f;
      return;
    case 5:
      *byte &= (char) 0x1f;
      return;
    case 6:
      *byte &= (char) 0x3f;
      return;
    case 7:
      *byte &= (char) 0x7f;
      return;
    case 8:
      return;
    default:
      assert (0);
      return;
    }
}

inline static void
ios_mask_last_byte(uint64_t *byte, int significant_bits)
{
  switch (significant_bits)
    {
    case 1:
      *byte &= (char) 0x80;
      return;
    case 2:
      *byte &= (char) 0xc0;
      return;
    case 3:
      *byte &= (char) 0xe0;
      return;
    case 4:
      *byte &= (char) 0xf0;
      return;
    case 5:
      *byte &= (char) 0xf8;
      return;
    case 6:
      *byte &= (char) 0xfc;
      return;
    case 7:
      *byte &= (char) 0xfe;
      return;
    case 8:
      return;
    default:
      assert (0);
      return;
    }
}

static inline int
ios_read_int_common (ios io, ios_off offset, int flags,
		     int bits,
		     enum ios_endian endian,
		     uint64_t *value)
{
  /* 64 bits might span at most 9 bytes.  */
  uint64_t c[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

  /* Number of signifcant bits in the first byte.  */
  int firstbyte_bits = 8 - (offset % 8);

  /* (Total number of bytes that need to be read) - 1.  */
  int bytes_minus1 = (bits - firstbyte_bits + 7) / 8;

  /* Number of significant bits in the last byte.  */
  int lastbyte_bits = (bits + (offset % 8)) % 8;
  lastbyte_bits = lastbyte_bits == 0 ? 8 : lastbyte_bits;

  /* Read the first byte and mask the unused bits.  */
  IOS_GET_C_ERR_CHCK(c[0], io);
  ios_mask_first_byte(&c[0], firstbyte_bits);

  switch (bytes_minus1)
  {
  case 0:
    *value = c[0] >> (8 - lastbyte_bits);
    return IOS_OK;

  case 1:
    IOS_READ_INTO_CHARRAY_1BYTE(c+1);
    ios_mask_last_byte(&c[1], lastbyte_bits);
    if (endian == IOS_ENDIAN_LSB)
      {
	if (bits <= 8)
	  /* We need to shift to align the least significant bit.  */
	  *value = (c[0] << lastbyte_bits) | (c[1] >> (8 - lastbyte_bits));
	else if ((offset % 8) == 0)
	  /* If little endian and the least significant byte is 8 bits aligned,
	  then we can handle the information byte by byte as we read.  */
	  *value = (c[1] << lastbyte_bits) | c[0];
	else
	  {
	    /* Consider the order of bits in a little endian number:
	    7-6-5-4-3-2-1-0-15-14-13-12-11-10-9-8- ... If such an
	    encoding is not byte-aligned, we have to first shift to fill the
	    least significant byte to get the right bits in the same bytes.  */
	    uint64_t reg;
	    reg = (c[0] << (8 + offset % 8)) | (c[1] << offset % 8);
	    *value = ((reg & 0xff) << (bits % 8)) | (reg >> 8);
	  }
      }
    else
      {
	/* We should shift to fill the least significant byte
	which is the last 8 bits.  */
	*value = (c[0] << lastbyte_bits) | (c[1] >> (8 - lastbyte_bits));
      }
    return IOS_OK;

  case 2:
    IOS_READ_INTO_CHARRAY_2BYTES(c+1);
    ios_mask_last_byte(&c[2], lastbyte_bits);
    if (endian == IOS_ENDIAN_LSB)
      {
	if ((offset % 8) == 0)
	  /* If little endian and the least significant byte is 8 bits aligned,
	  then we can handle the information byte by byte as we read.  */
	  *value = (c[2] << (8 + lastbyte_bits)) | (c[1] << 8) | c[0];
	else
	  {
	    /* We have to shift to fill the least significant byte to get
	       the right bits in the same bytes.  */
	    uint64_t reg;
	    reg = (c[0] << (56 + offset % 8)) | (c[1] << (48 + offset % 8))
		  | (c[2] << (40 + offset % 8));
	    /* The bits in the most-significant-byte-to-be is alligned to left,
	       shift it towards right! */
	    if (bits <= 16)
	      reg = ((reg & 0x00ff000000000000LL) >> (16 - bits))
		    | (reg & 0xff00ffffffffffffLL);
	    else
	      reg = ((reg & 0x0000ff0000000000LL) >> (24 - bits))
		    | (reg & 0xffff00ffffffffffLL);
	    /* Now we can place the bytes correctly.  */
	    *value = __bswap_64(reg);
	  }
      }
    else
      {
	/* We should shift to fill the least significant byte
	which is the last 8 bits.  */
	*value = (c[0] << (8 + lastbyte_bits)) | (c[1] << lastbyte_bits)
		 | (c[2] >> (8 - lastbyte_bits));
      }
    return IOS_OK;

  case 3:
    IOS_READ_INTO_CHARRAY_3BYTES(c+1);
    ios_mask_last_byte(&c[3], lastbyte_bits);
    if (endian == IOS_ENDIAN_LSB)
      {
	if ((offset % 8) == 0)
	  /* If little endian and the least significant byte is 8 bits aligned,
	  then we can handle the information byte by byte as we read.  */
	  *value = (c[3] << (16 + lastbyte_bits)) | (c[2] << 16) | (c[1] << 8)
		   | c[0];
	else
	  {
	    /* We have to shift to fill the least significant byte to get
	       the right bits in the same bytes.  */
	    uint64_t reg;
	    reg = (c[0] << (56 + offset % 8)) | (c[1] << (48 + offset % 8))
		  | (c[2] << (40 + offset % 8)) | (c[3] << (32 + offset % 8));
	    /* The bits in the most-significant-byte-to-be is alligned to left,
	       shift it towards right! */
	    if (bits <= 24)
	      reg = ((reg & 0x0000ff0000000000LL) >> (24 - bits))
		    | (reg & 0xffff00ffffffffffLL);
	    else
	      reg = ((reg & 0x000000ff00000000LL) >> (32 - bits))
		    | (reg & 0xffffff00ffffffffLL);
	    /* Now we can place the bytes correctly.  */
	    *value = __bswap_64(reg);
	  }
      }
    else
      {
	/* We should shift to fill the least significant byte
	which is the last 8 bits.  */
	*value = (c[0] << (16 + lastbyte_bits)) | (c[1] << (8 + lastbyte_bits))
		 | (c[2] << lastbyte_bits) | (c[3] >> (8 - lastbyte_bits));
      }
    return IOS_OK;

  case 4:
    IOS_READ_INTO_CHARRAY_4BYTES(c+1);
    ios_mask_last_byte(&c[4], lastbyte_bits);
    if (endian == IOS_ENDIAN_LSB)
      {
	if ((offset % 8) == 0)
	  /* If little endian and the least significant byte is 8 bits aligned,
	  then we can handle the information byte by byte as we read.  */
	  *value = (c[4] << (24 + lastbyte_bits)) | (c[3] << 24) | (c[2] << 16)
		   | (c[1] << 8) | c[0];
	else
	  {
	    /* We have to shift to fill the least significant byte to get
	       the right bits in the same bytes.  */
	    uint64_t reg;
	    reg = (c[0] << (56 + offset % 8)) | (c[1] << (48 + offset % 8))
		  | (c[2] << (40 + offset % 8)) | (c[3] << (32 + offset % 8))
		  | (c[4] << (24 + offset % 8));
	    /* The bits in the most-significant-byte-to-be is alligned to left,
	       shift it towards right! */
	    if (bits <= 32)
	      reg = ((reg & 0x000000ff00000000LL) >> (32 - bits))
		    | (reg & 0xffffff00ffffffffLL);
	    else
	      reg = ((reg & 0x00000000ff000000LL) >> (40 - bits))
		    | (reg & 0xffffffff00ffffffLL);
	    /* Now we can place the bytes correctly.  */
	    *value = __bswap_64(reg);
	  }
      }
    else
      {
	/* We should shift to fill the least significant byte
	which is the last 8 bits.  */
	*value = (c[0] << (24 + lastbyte_bits)) | (c[1] << (16 + lastbyte_bits))
		 | (c[2] << (8 + lastbyte_bits)) | (c[3] << lastbyte_bits)
		 | (c[4] >> (8 - lastbyte_bits));
      }
    return IOS_OK;

  case 5:
    IOS_READ_INTO_CHARRAY_5BYTES(c+1);
    ios_mask_last_byte(&c[5], lastbyte_bits);
    if (endian == IOS_ENDIAN_LSB)
      {
	if ((offset % 8) == 0)
	  /* If little endian and the least significant byte is 8 bits aligned,
	  then we can handle the information byte by byte as we read.  */
	  *value = (c[5] << (32 + lastbyte_bits)) | (c[4] << 32) | (c[3] << 24)
		   | (c[2] << 16) | (c[1] << 8) | c[0];
	else
	  {
	    /* We have to shift to fill the least significant byte to get
	       the right bits in the same bytes.  */
	    uint64_t reg;
	    reg = (c[0] << (56 + offset % 8)) | (c[1] << (48 + offset % 8))
		  | (c[2] << (40 + offset % 8)) | (c[3] << (32 + offset % 8))
		  | (c[4] << (24 + offset % 8)) | (c[5] << (16 + offset % 8));
	    /* The bits in the most-significant-byte-to-be is alligned to left,
	       shift it towards right! */
	    if (bits <= 40)
	      reg = ((reg & 0x00000000ff000000LL) >> (40 - bits))
		    | (reg & 0xffffffff00ffffffLL);
	    else
	      reg = ((reg & 0x0000000000ff0000LL) >> (48 - bits))
		    | (reg & 0xffffffffff00ffffLL);
	    /* Now we can place the bytes correctly.  */
	    *value = __bswap_64(reg);
	  }
      }
    else
      {
	/* We should shift to fill the least significant byte
	which is the last 8 bits.  */
	*value = (c[0] << (32 + lastbyte_bits)) | (c[1] << (24 + lastbyte_bits))
		 | (c[2] << (16 + lastbyte_bits)) | (c[3] << (8 + lastbyte_bits))
		 | (c[4] << lastbyte_bits) | (c[5] >> (8 - lastbyte_bits));
      }
    return IOS_OK;

  case 6:
    IOS_READ_INTO_CHARRAY_6BYTES(c+1);
    ios_mask_last_byte(&c[6], lastbyte_bits);
    if (endian == IOS_ENDIAN_LSB)
      {
	if ((offset % 8) == 0)
	  /* If little endian and the least significant byte is 8 bits aligned,
	  then we can handle the information byte by byte as we read.  */
	  *value = (c[6] << (40 + lastbyte_bits)) | (c[5] << 40) | (c[4] << 32)
		   | (c[3] << 24) | (c[2] << 16) | (c[1] << 8) | c[0];
	else
	  {
	    /* We have to shift to fill the least significant byte to get
	       the right bits in the same bytes.  */
	    uint64_t reg;
	    reg = (c[0] << (56 + offset % 8)) | (c[1] << (48 + offset % 8))
		  | (c[2] << (40 + offset % 8)) | (c[3] << (32 + offset % 8))
		  | (c[4] << (24 + offset % 8)) | (c[5] << (16 + offset % 8))
		  | (c[6] << (8 + offset % 8));
	    /* The bits in the most-significant-byte-to-be is alligned to left,
	       shift it towards right! */
	    if (bits <= 48)
	      reg = ((reg & 0x0000000000ff0000LL) >> (48 - bits))
		    | (reg & 0xffffffffff00ffffLL);
	    else
	      reg = ((reg & 0x000000000000ff00LL) >> (56 - bits))
		    | (reg & 0xffffffffffff00ffLL);
	    /* Now we can place the bytes correctly.  */
	    *value = __bswap_64(reg);
	  }
      }
    else
      {
	/* We should shift to fill the least significant byte
	which is the last 8 bits.  */
	*value = (c[0] << (40 + lastbyte_bits)) | (c[1] << (32 + lastbyte_bits))
		 | (c[2] << (24 + lastbyte_bits)) | (c[3] << (16 + lastbyte_bits))
		 | (c[4] << (8 + lastbyte_bits)) | (c[5] << lastbyte_bits)
		 | (c[6] >> (8 - lastbyte_bits));
      }
    return IOS_OK;

  case 7:
    IOS_READ_INTO_CHARRAY_7BYTES(c+1);
    ios_mask_last_byte(&c[7], lastbyte_bits);
    if (endian == IOS_ENDIAN_LSB)
      {
	if ((offset % 8) == 0)
	  /* If little endian and the least significant byte is 8 bits aligned,
	  then we can handle the information byte by byte as we read.  */
	  *value = (c[7] << (48 + lastbyte_bits)) | (c[6] << 48) | (c[5] << 40)
		   | (c[4] << 32) | (c[3] << 24) | (c[2] << 16) | (c[1] << 8)
		   | c[0];
	else
	  {
	    /* We have to shift to fill the least significant byte to get
	       the right bits in the same bytes.  */
	    uint64_t reg;
	    reg = (c[0] << (56 + offset % 8)) | (c[1] << (48 + offset % 8))
		  | (c[2] << (40 + offset % 8)) | (c[3] << (32 + offset % 8))
		  | (c[4] << (24 + offset % 8)) | (c[5] << (16 + offset % 8))
		  | (c[6] << (8 + offset % 8)) | (c[7] << offset % 8);
	    /* The bits in the most-significant-byte-to-be is alligned to left,
	       shift it towards right! */
	    if (bits <= 56)
	      reg = ((reg & 0x000000000000ff00LL) >> (56 - bits))
		    | (reg & 0xffffffffffff00ffLL);
	    else
	      reg = ((reg & 0x00000000000000ffLL) >> (64 - bits))
		    | (reg & 0xffffffffffffff00LL);
	    /* Now we can place the bytes correctly.  */
	    *value = __bswap_64(reg);
	  }
      }
    else
      {
	/* We should shift to fill the least significant byte
	which is the last 8 bits.  */
	*value = (c[0] << (48 + lastbyte_bits)) | (c[1] << (40 + lastbyte_bits))
		 | (c[2] << (32 + lastbyte_bits)) | (c[3] << (24 + lastbyte_bits))
		 | (c[4] << (16 + lastbyte_bits)) | (c[5] << (8 + lastbyte_bits))
		 | (c[6] << lastbyte_bits) | (c[7] >> (8 - lastbyte_bits));
      }
    return IOS_OK;

  case 8:
    IOS_READ_INTO_CHARRAY_8BYTES(c+1);
    ios_mask_last_byte(&c[8], lastbyte_bits);
    if (endian == IOS_ENDIAN_LSB)
      {
	/* We have to shift to fill the least significant byte to get
	   the right bits in the same bytes.  */
	uint64_t reg;
	reg = (c[0] << (56 + offset % 8)) | (c[1] << (48 + offset % 8))
	      | (c[2] << (40 + offset % 8)) | (c[3] << (32 + offset % 8))
	      | (c[4] << (24 + offset % 8)) | (c[5] << (16 + offset % 8))
	      | (c[6] << (8 + offset % 8)) | (c[7] << offset % 8)
	      | (c[8] >> firstbyte_bits);
	/* The bits in the most-significant-byte-to-be is alligned to left,
	   shift it towards right! */
	reg = ((reg & 0x00000000000000ffLL) >> (64 - bits))
	      | (reg & 0xffffffffffffff00LL);
	/* Now we can place the bytes correctly.  */
	*value = __bswap_64(reg);
      }
    else
      {
	/* We should shift to fill the least significant byte
	which is the last 8 bits.  */
	*value = (c[0] << (56 + lastbyte_bits)) | (c[1] << (48 + lastbyte_bits))
		 | (c[2] << (40 + lastbyte_bits)) | (c[3] << (32 + lastbyte_bits))
		 | (c[4] << (24 + lastbyte_bits)) | (c[5] << (16 + lastbyte_bits))
		 | (c[6] << (8 + lastbyte_bits)) | (c[7] << lastbyte_bits)
		 | (c[8] >> (8 - lastbyte_bits));
      }
    return IOS_OK;

  default:
    assert(0);
  }
}

int
ios_read_int (ios io, ios_off offset, int flags,
              int bits,
              enum ios_endian endian,
              enum ios_nenc nenc,
              int64_t *value)
{
  /* We always need to start reading from offset / 8  */
  if (io->dev_if->seek (io->dev, offset / 8, IOD_SEEK_SET) == -1)
    return IOS_EIOFF;

  /* Fast track for byte-aligned 8x bits  */
  if (offset % 8 == 0 && bits % 8 == 0)
    {
      switch (bits) {
      case 8:
	{
	  int8_t c[1] = {0};
	  IOS_READ_INTO_CHARRAY_1BYTE(c);
	  *value = c[0];
	  return IOS_OK;
	}

      case 16:
	{
	  int16_t c[2] = {0, 0};  
	  IOS_READ_INTO_CHARRAY_2BYTES(c);
	  if (endian == IOS_ENDIAN_LSB)
	    *value = (c[1] << 8) | c[0];
	  else
	    *value = (c[0] << 8) | c[1];
	  return IOS_OK;
	}

      case 24:
	{ 
	  int64_t c[3] = {0, 0, 0};  
	  IOS_READ_INTO_CHARRAY_3BYTES(c);
	  if (endian == IOS_ENDIAN_LSB)
	    *value = (c[2] << 16) | (c[1] << 8) | c[0];
	  else
	    *value = (c[0] << 16) | (c[1] << 8) | c[2];
	  *value <<= 40;
	  *value >>= 40;
	  return IOS_OK;
	}

      case 32:
        {
	  int32_t c[4] = {0, 0, 0, 0};  
	  IOS_READ_INTO_CHARRAY_4BYTES(c);
	  if (endian == IOS_ENDIAN_LSB)
	    *value = (c[3] << 24) | (c[2] << 16) | (c[1] << 8) | c[0];
	  else
	    *value = (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3];
	  return IOS_OK;
	}

      case 40:
        {
	  int64_t c[5] = {0, 0, 0, 0, 0};  
	  IOS_READ_INTO_CHARRAY_5BYTES(c);
	  if (endian == IOS_ENDIAN_LSB)
	    *value = (c[4] << 32) | (c[3] << 24) | (c[2] << 16) | (c[1] << 8)
		     | c[0];
	  else
	    *value = (c[0] << 32) | (c[1] << 24) | (c[2] << 16) | (c[3] << 8)
		     | c[4];
	  *value <<= 24;
	  *value >>= 24;
	  return IOS_OK;
	}

      case 48:
	{
	  int64_t c[6] = {0, 0, 0, 0, 0, 0};  
	  IOS_READ_INTO_CHARRAY_6BYTES(c);
	  if (endian == IOS_ENDIAN_LSB)
	    *value = (c[5] << 40) | (c[4] << 32) | (c[3] << 24) | (c[2] << 16)
		     | (c[1] << 8) | c[0];
	  else
	    *value = (c[0] << 40) | (c[1] << 32) | (c[2] << 24) | (c[3] << 16)
		     | (c[4] << 8) | c[5];
	  *value <<= 16;
	  *value >>= 16;
	  return IOS_OK;
	}

      case 56:
	{
	  int64_t c[7] = {0, 0, 0, 0, 0, 0, 0};  
	  IOS_READ_INTO_CHARRAY_7BYTES(c);
	  if (endian == IOS_ENDIAN_LSB)
	    *value = (c[6] << 48) | (c[5] << 40) | (c[4] << 32) | (c[3] << 24)
		     | (c[2] << 16) | (c[1] << 8) | c[0];
	  else
	    *value = (c[0] << 48) | (c[1] << 40) | (c[2] << 32) | (c[3] << 24)
		     | (c[4] << 16) | (c[5] << 8) | c[6];
	  *value <<= 8;
	  *value >>= 8;
	  return IOS_OK;
	}

      case 64:
	{
	  int64_t c[8] = {0, 0, 0, 0, 0, 0, 0, 0};  
	  IOS_READ_INTO_CHARRAY_8BYTES(c);
	  if (endian == IOS_ENDIAN_LSB)
	    *value = (c[7] << 56) | (c[6] << 48) | (c[5] << 40) | (c[4] << 32)
		     | (c[3] << 24) | (c[2] << 16) | (c[1] << 8) | c[0];
	  else
	    *value = (c[0] << 56) | (c[1] << 48) | (c[2] << 40) | (c[3] << 32)
		     | (c[4] << 24) | (c[5] << 16) | (c[6] << 8) | c[7];
	  return IOS_OK;
	}
      }
    }

  /* Fall into the case for the unaligned and the sizes other than 8k.  */
  int ret_val = ios_read_int_common(io, offset, flags, bits, endian,
				    (uint64_t *) value);
  if (ret_val == IOS_OK)
    {
      *value <<= 64 - bits;
      *value >>= 64 - bits;
      return IOS_OK;
    }
  return ret_val;
}

int
ios_read_uint (ios io, ios_off offset, int flags,
               int bits,
               enum ios_endian endian,
               uint64_t *value)
{
  /* When aligned, 1 to 64 bits can span at most 8 bytes.  */
  uint64_t c[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  /* We always need to start reading from offset / 8  */
  if (io->dev_if->seek (io->dev, offset / 8, IOD_SEEK_SET) == -1)
    return IOS_EIOFF;

  /* Fast track for byte-aligned 8x bits  */
  if (offset % 8 == 0 && bits % 8 == 0)
    {
      switch (bits) {
      case 8:
	IOS_READ_INTO_CHARRAY_1BYTE(c);
	*value = c[0];
	return IOS_OK;

      case 16:
	IOS_READ_INTO_CHARRAY_2BYTES(c);
	if (endian == IOS_ENDIAN_LSB)
	  *value = (c[1] << 8) | c[0];
	else
	  *value = (c[0] << 8) | c[1];
	return IOS_OK;

      case 24:
	IOS_READ_INTO_CHARRAY_3BYTES(c);
	if (endian == IOS_ENDIAN_LSB)
	  *value = (c[2] << 16) | (c[1] << 8) | c[0];
	else
	  *value = (c[0] << 16) | (c[1] << 8) | c[2];
	return IOS_OK;

      case 32:
	IOS_READ_INTO_CHARRAY_4BYTES(c);
	if (endian == IOS_ENDIAN_LSB)
	  *value = (c[3] << 24) | (c[2] << 16) | (c[1] << 8) | c[0];
	else
	  *value = (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3];
	return IOS_OK;

      case 40:
	IOS_READ_INTO_CHARRAY_5BYTES(c);
	if (endian == IOS_ENDIAN_LSB)
	  *value = (c[4] << 32) | (c[3] << 24) | (c[2] << 16) | (c[1] << 8)
		   | c[0];
	else
	  *value = (c[0] << 32) | (c[1] << 24) | (c[2] << 16) | (c[3] << 8)
		   | c[4];
	return IOS_OK;

      case 48:
	IOS_READ_INTO_CHARRAY_6BYTES(c);
	if (endian == IOS_ENDIAN_LSB)
	  *value = (c[5] << 40) | (c[4] << 32) | (c[3] << 24) | (c[2] << 16)
		   | (c[1] << 8) | c[0];
	else
	  *value = (c[0] << 40) | (c[1] << 32) | (c[2] << 24) | (c[3] << 16)
		   | (c[4] << 8) | c[5];
	return IOS_OK;

      case 56:
	IOS_READ_INTO_CHARRAY_7BYTES(c);
	if (endian == IOS_ENDIAN_LSB)
	  *value = (c[6] << 48) | (c[5] << 40) | (c[4] << 32) | (c[3] << 24)
		   | (c[2] << 16) | (c[1] << 8) | c[0];
	else
	  *value = (c[0] << 48) | (c[1] << 40) | (c[2] << 32) | (c[3] << 24)
		   | (c[4] << 16) | (c[5] << 8) | c[6];
	return IOS_OK;

      case 64:
	IOS_READ_INTO_CHARRAY_8BYTES(c);
	if (endian == IOS_ENDIAN_LSB)
	  *value = (c[7] << 56) | (c[6] << 48) | (c[5] << 40) | (c[4] << 32)
		   | (c[3] << 24) | (c[2] << 16) | (c[1] << 8) | c[0];
	else
	  *value = (c[0] << 56) | (c[1] << 48) | (c[2] << 40) | (c[3] << 32)
		   | (c[4] << 24) | (c[5] << 16) | (c[6] << 8) | c[7];
	return IOS_OK;
      }
    }

  /* Fall into the case for the unaligned and the sizes other than 8k.  */
  return ios_read_int_common (io, offset, flags, bits, endian, value);
}

int
ios_read_string (ios io, ios_off offset, int flags, char **value)
{
  char *str = NULL;
  size_t i = 0;
  int c;

  if (io->dev_if->seek (io->dev, offset / 8, IOD_SEEK_SET)
      == -1)
    return IOS_EIOFF;

  do
    {
      if (i % 128 == 0)
        str = xrealloc (str, i + 128 * sizeof (char));

      c = io->dev_if->get_c (io->dev);
      if (c == IOD_EOF)
        str[i] = '\0';
      else
        str[i] = (char) c;
    }
  while (str[i++] != '\0');

  *value = str;
  return IOS_OK;
}

int
ios_write_int (ios io, ios_off offset, int flags,
               int bits,
               enum ios_endian endian,
               enum ios_nenc nenc,
               int64_t value)
{
  if (offset % 8 == 0)
    {
      if (io->dev_if->seek (io->dev, offset / 8, IOD_SEEK_SET)
          == -1)
        return IOS_EIOFF;

      switch (bits)
        {
        case 32:
          {
            int32_t c1, c2, c3, c4;

            c1 = (value >> 24) & 0xff;
            c2 = (value >> 16) & 0xff;
            c3 = (value >> 8) & 0xff;
            c4 = value & 0xff;

            if (io->dev_if->put_c (io->dev, c1)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c2)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c3)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c4)
                == IOD_EOF)
              return IOS_EIOFF;

            break;
          }
        case 64:
          {
            int32_t c1, c2, c3, c4, c5, c6, c7, c8;

            c1 = (value >> 56) & 0xff;
            c2 = (value >> 48) & 0xff;
            c3 = (value >> 40) & 0xff;
            c4 = (value >> 32) & 0xff;
            c5 = (value >> 24) & 0xff;
            c6 = (value >> 16) & 0xff;
            c7 = (value >> 8) & 0xff;
            c8 = value & 0xff;

            if (io->dev_if->put_c (io->dev, c1)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c2)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c3)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c4)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c5)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c6)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c7)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c8)
                == IOD_EOF)
              return IOS_EIOFF;
            break;
          }
        default:
          assert (0);
          break;
        }
    }
  else
    assert (0);

  return IOS_OK;
}

int
ios_write_uint (ios io, ios_off offset, int flags,
                int bits,
                enum ios_endian endian,
                uint64_t value)
{
  /* XXX: writeme.  */


  if (offset % 8 == 0)
    {
      if (io->dev_if->seek (io->dev, offset / 8, IOD_SEEK_SET)
          == -1)
        return IOS_EIOFF;

      switch (bits)
        {
        case 8:
          if (io->dev_if->put_c (io->dev, (int) value)
              == IOD_EOF)
            return IOS_EIOBJ;
          break;
        case 32:
          {
            int32_t c1, c2, c3, c4;

            c1 = (value >> 24) & 0xff;
            c2 = (value >> 16) & 0xff;
            c3 = (value >> 8) & 0xff;
            c4 = value & 0xff;

            if (io->dev_if->put_c (io->dev, c1)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c2)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c3)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c4)
                == IOD_EOF)
              return IOS_EIOFF;

            break;
          }
        case 64:
          {
            int32_t c1, c2, c3, c4, c5, c6, c7, c8;

            c1 = (value >> 56) & 0xff;
            c2 = (value >> 48) & 0xff;
            c3 = (value >> 40) & 0xff;
            c4 = (value >> 32) & 0xff;
            c5 = (value >> 24) & 0xff;
            c6 = (value >> 16) & 0xff;
            c7 = (value >> 8) & 0xff;
            c8 = value & 0xff;

            if (io->dev_if->put_c (io->dev, c1)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c2)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c3)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c4)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c5)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c6)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c7)
                == IOD_EOF)
              return IOS_EIOFF;

            if (io->dev_if->put_c (io->dev, c8)
                == IOD_EOF)
              return IOS_EIOFF;
            break;
          }
        default:
          assert (0);
          break;
        }
    }
  else
    assert (0);


  return IOS_OK;
}

int
ios_write_string (ios io, ios_off offset, int flags,
                  const char *value)
{
  /* XXX: writeme.  */
  return IOS_OK;
}
