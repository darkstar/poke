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

static struct ios *ios;
static struct ios *cur_io;

/* The available backends are implemented in their own files, and
   provide the following interfaces.  */

extern struct ios_dev_if ios_dev_file; /* ios-dev-file.c */

static struct *ios_dev_ifs =
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
  while (ios)
    ios_close (ios);
}

int
ios_open (const char *handler)
{
  struct ios io = NULL;
  struct ios_dev_if *dev_if = NULL;

  /* Allocate and initialize the new IO space.  */
  io = xmalloc (sizeof (struct ios));
  io->next = NULL;
  io->handler = xstrdup (handler);

  /* Look for a device interface suitable to operate on the given
     handler.  */
  for (dev_if = ios_dev_ifs; dev_if; ++dev_if)
    {
      if (dev_if->handler_p (handler))
        break;
    }

  if (dev_if == NULL)
    goto error;

  io->dev_if = dev_if;
  
  /* Open the device using the interface found above.  */
  io->dev = io->dev_if->open (handler);
  if (io->dev == NULL)
    goto error;

  /* Add the newly created space to the list, and update the current
     space.  */
  if (ios == NULL)
    ios = io;
  else
    {
      io->next = ios;
      ios = io;
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

  /* Unlink the IO from the list.  */
  assert (ios != NULL); /* The list must contain at least io.  */
  if (ios == io)
    ios = ios->next;
  else
    {
      for (tmp = ios; tmp->next != io; tmp = tmp->next)
        ;
      tmp->next = io->next;
    }
  free (io);
  
  /* Set the new current IO.  */
  cur_io = ios;
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

void
ios_map (ios_map_fn cb, void *data)
{
  ios io;

  for (io = ios; io; io = io->next)
    (*cb) (io, data);
}

ios
ios_search (const char *handler)
{
  ios io;

  for (io = ios; io; io = io->next)
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

  for (io = ios; io && n > 0; n--, io = io->next)
    ;

  return io;
}

