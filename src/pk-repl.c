/* pk-repl.c - A REPL ui for poke.  */

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

#include "readline.h"
#if defined HAVE_READLINE_HISTORY_H
# include <readline/history.h>
#endif
#include <gettext.h>
#define _(str) dgettext (PACKAGE, str)

#include "poke.h"
#include "pk-term.h"
#include "pk-cmd.h"
#if HAVE_HSERVER
#  include "pk-hserver.h"
#endif

void
pk_repl (void)
{
  if (!poke_quiet_p)
    {
      pk_print_version ();
      pk_puts ("\n");

#if HAVE_HSERVER
      pk_printf ("hserver listening in port %d.\n",
                 pk_hserver_port ());
      pk_puts ("\n");
#endif

#if HAVE_HSERVER
      {
        char *help_hyperlink
          = pk_hserver_make_hyperlink ('e', ".help");

        pk_puts (_("For help, type \""));
        pk_term_hyperlink (help_hyperlink, NULL);
        pk_puts (".help");
        pk_term_end_hyperlink ();
        pk_puts ("\".\n");
        free (help_hyperlink);
      }
#else 
      pk_puts (_("For help, type \".help\".\n"));
#endif
      pk_puts (_("Type \".exit\" to leave the program.\n"));
    }

  while (!poke_exit_p)
    {
      int ret;
      char *line;

      pk_term_flush ();
      line = readline ("(poke) ");
      if (line == NULL)
        {
          /* EOF in stdin (probably Ctrl-D).  */
          pk_puts ("\n");
          break;
        }

      /* Ignore empty lines.  */
      if (*line == '\0')
        continue;

#if defined HAVE_READLINE_HISTORY_H
      if (line && *line)
        add_history (line);
#endif

      ret = pk_cmd_exec (line);
      if (!ret)
        /* Avoid gcc warning here.  */ ;
      free (line);
    }
}

static int saved_point;
static int saved_end;

void
pk_repl_display_begin (void)
{
  saved_point = rl_point;
  saved_end = rl_end;
  rl_point = rl_end = 0;

  rl_save_prompt ();
  rl_clear_message ();

  pk_puts ("(poke) ");
}

void
pk_repl_display_end (void)
{
  pk_term_flush ();
  rl_restore_prompt ();
  rl_point = saved_point;
  rl_end = saved_end;
  rl_forced_update_display ();
}

void
pk_repl_insert (const char *str)
{
  rl_insert_text (str);
  rl_redisplay ();
}
