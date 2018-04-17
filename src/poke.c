/* poke.c - Interactive editor for binary files.  */

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
#include <progname.h>
#include <xalloc.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <gettext.h>
#define _(str) dgettext (PACKAGE, str)
#include <unistd.h>
#include <string.h>
#include "readline.h"
#if defined HAVE_READLINE_HISTORY_H
# include <readline/history.h>
#endif

#include "pk-io.h"
#include "pk-cmd.h"
#include "pkl.h"
#include "pvm.h"
#include "poke.h"

/* poke can be run either interactively (from a tty) or in batch mode.
   The following predicate records this.  */

int poke_interactive_p;

/* The following global indicates whether poke should be as terse as
   possible in its output.  This is useful when running poke from
   other programs.  */

int poke_quiet_p;

/* This is used by commands to indicate the repl that it must
   exit.  */

int poke_exit_p;
int poke_exit_code;

/* The following global is the poke compiler.  */
pkl_compiler poke_compiler;

/* Command line options management.  */

enum
{
  HELP_ARG,
  VERSION_ARG,
  QUIET_ARG,
};

static const struct option long_options[] =
{
  {"help", no_argument, NULL, HELP_ARG},
  {"version", no_argument, NULL, VERSION_ARG},
  {"quiet", no_argument, NULL, QUIET_ARG},
  {NULL, 0, NULL, 0},
};

static void
print_help ()
{
  /* TRANSLATORS: --help output, gnunity synopsis.
     no-wrap */
  printf (_("\
Usage: poke [OPTION]... [FILE]\n"));

  /* TRANSLATORS: --help output, gnunity summary.
     no-wrap */
  fputs(_("\
Interactive editor for binary files.\n"), stdout);

  puts ("");
  /* TRANSLATORS: --help output, gnunity arguments.
     no-wrap */
  fputs (_("\
      --quiet                         be as terse as possible.\n\
      --help                          print a help message and exit.\n\
      --version                       show version and exit.\n"),
         stdout);

  puts ("");
  /* TRANSLATORS: --help output 5+ (reports)
     TRANSLATORS: the placeholder indicates the bug-reporting address
     for this application.  Please add _another line_ with the
     address for translation bugs.
     no-wrap */
  printf (_("\
Report bugs to: %s\n"), PACKAGE_BUGREPORT);
#ifdef PACKAGE_PACKAGER_BUG_REPORTS
  printf (_("Report %s bugs to: %s\n"), PACKAGE_PACKAGER,
          PACKAGE_PACKAGER_BUG_REPORTS);
#endif
#ifdef PACKAGE_URL
  printf (_("%s home page: <%s>\n"), PACKAGE_NAME, PACKAGE_URL);
#else
  printf (_("%s home page: <http://www.gnu.org/software/poke/>\n"),
          PACKAGE_NAME, PACKAGE);
#endif
  fputs (_("General help using GNU software: <http://www.gnu.org/gethelp/>\n"),
         stdout);
}

void
pk_print_version ()
{
  puts ("     _____");
  puts (" ---'   __\\_______");
  printf ("            ______)  GNU poke %s\n", VERSION);
  puts ("            __)");
  puts ("           __)");
  puts (" ---._______)");
  /* xgettesxt: no-wrap */
  puts ("");

  /* It is important to separate the year from the rest of the message,
     as done here, to avoid having to retranslate the message when a new
     year comes around.  */  
  printf (_("\
Copyright (C) %s Jose E. Marchesi.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n"), "2018");

  puts (_("\
\n\
Perpetrated by Jose E. Marchesi.  To my father."));
}

static void
parse_args (int argc, char *argv[])
{
  char c;
  int ret;

  while ((ret = getopt_long (argc,
                             argv,
                             "",
                             long_options,
                             NULL)) != -1)
    {
      c = ret;
      switch (c)
        {
        case HELP_ARG:
          print_help ();
          exit (EXIT_SUCCESS);
          break;
        case VERSION_ARG:
          pk_print_version ();
          exit (EXIT_SUCCESS);
          break;
        case QUIET_ARG:
          poke_quiet_p = 1;
          break;
        default:
          exit (EXIT_FAILURE);
        }
    }

  if (optind < argc)
    {
      if (!pk_io_open (argv[optind++]))
        exit (EXIT_FAILURE);
    }

  if (optind < argc)
    {
      print_help();
      exit (EXIT_FAILURE);
    }
}

static void
repl ()
{
  if (!poke_quiet_p)
    {
      pk_print_version ();
      puts ("");
    }

  while (!poke_exit_p)
    {
      int ret;
      char *line;

      line = readline ("(poke) ");
      if (line == NULL)
        /* EOF in stdin (probably Ctrl-D).  */
        break;

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

static void
initialize ()
{
  /* Initialize the Poke Virtual Machine.  */
  pvm_init ();

  /* Initialize the poke incremental compiler and load the standard
     library.  */

  poke_compiler = pkl_new ();
  if (!pkl_compile_file (poke_compiler,
                         /* XXX: use POKEDIR  */
                         "/home/jemarch/gnu/hacks/poke/pickles/std.pk"))
    exit (1);
}

static void
finalize ()
{
  pk_io_shutdown ();
  pk_cmd_shutdown ();
  pvm_shutdown ();
  pkl_free (poke_compiler);
}

int
main (int argc, char *argv[])
{
  set_program_name ("poke");
  parse_args (argc, argv);

  /* Determine whether the tool has been invoked interactively.  */
  poke_interactive_p = isatty (fileno (stdin));

  /* Initialization.  */
  initialize ();
  
  /* Enter the REPL.  */
  if (poke_interactive_p)
    repl ();

  /* Cleanup.  */
  finalize ();
  
  return poke_exit_code;
}
