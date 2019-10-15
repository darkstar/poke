/* poke.c - Interactive editor for binary files.  */

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
#include <locale.h>

#include "ios.h"
#include "pk-cmd.h"
#include "pkl.h"
#include "pvm.h"
#include "pk-term.h"
#include "poke.h"

/* poke can be run either interactively (from a tty) or in batch mode.
   The following predicate records this.  */

int poke_interactive_p;

/* The following global indicates whether poke should be as terse as
   possible in its output.  This is useful when running poke from
   other programs.  */

int poke_quiet_p;

/* The following global contains the directory holding the program's
   architecture independent files, such as scripts.  */

char *poke_datadir;

/* This is used by commands to indicate the repl that it must
   exit.  */

int poke_exit_p;
int poke_exit_code;

/* The following global indicates the numeration base used when
   printing PVM values at the REPL.  It defaults to decimal (10).  */

int poke_obase = 10;

/* The following global is the poke compiler.  */
pkl_compiler poke_compiler;

/* The following global indicates whether to load an user
   initialization file.  It default to 1.  */

int poke_load_init_file = 1;

/* the following global is the poke virtual machine.  */
pvm poke_vm;

/* Command line options management.  */

enum
{
  HELP_ARG,
  VERSION_ARG,
  QUIET_ARG,
  LOAD_ARG,
  CMD_ARG,
  NO_INIT_FILE_ARG,
  SCRIPT_ARG,
  COLOR_ARG,
  STYLE_ARG
};

static const struct option long_options[] =
{
  {"help", no_argument, NULL, HELP_ARG},
  {"version", no_argument, NULL, VERSION_ARG},
  {"quiet", no_argument, NULL, QUIET_ARG},
  {"load", required_argument, NULL, LOAD_ARG},
  {"command", required_argument, NULL, CMD_ARG},
  {"script", required_argument, NULL, SCRIPT_ARG},
  {"no-init-file", no_argument, NULL, NO_INIT_FILE_ARG},
  {"color", required_argument, NULL, COLOR_ARG},
  {"style", required_argument, NULL, STYLE_ARG},
  {NULL, 0, NULL, 0},
};

static void
print_help ()
{
  /* TRANSLATORS: --help output, GNU poke synopsis.
     no-wrap */
  pk_puts (_("\
Usage: poke [OPTION]... [FILE]\n"));

  /* TRANSLATORS: --help output, GNU poke summary.
     no-wrap */
  pk_puts (_("\
Interactive editor for binary files.\n"));

  pk_puts ("\n");
  /* TRANSLATORS: --help output, GNU poke arguments.
     no-wrap */
  pk_puts (_("\
  -l, --load=FILE                     load the given pickle at startup.\n"));

  pk_puts ("\n");

  /* TRANSLATORS: --help output, GNU poke arguments.
     no-wrap */
  pk_puts (_("\
Commanding poke from the command line:\n\
  -c, --command=CMD                   execute the given command.\n\
  -s, --script=FILE                   execute commands from FILE.\n"));

  pk_puts ("\n");
  pk_puts (_("\
Styling text output:\n\
      --color=(yes|no|auto|html|test) emit styled output.\n\
      --style=STYLE_FILE              style file to use when styling.\n"));

  pk_puts ("\n");
  /* TRANSLATORS: --help output, less used GNU poke arguments.
     no-wrap */
  pk_puts (_("\
  -q, --no-init-file                  do not load an init file.\n\
      --quiet                         be as terse as possible.\n\
      --help                          print a help message and exit.\n\
      --version                       show version and exit.\n"));

  pk_puts ("\n");
  /* TRANSLATORS: --help output 5+ (reports)
     TRANSLATORS: the placeholder indicates the bug-reporting address
     for this application.  Please add _another line_ with the
     address for translation bugs.
     no-wrap */
  pk_printf (_("\
Report bugs to: %s\n"), PACKAGE_BUGREPORT);
#ifdef PACKAGE_PACKAGER_BUG_REPORTS
  printf (_("Report %s bugs to: %s\n"), PACKAGE_PACKAGER,
          PACKAGE_PACKAGER_BUG_REPORTS);
#endif
  pk_printf (_("%s home page: <%s>\n"), PACKAGE_NAME, PACKAGE_URL);
  pk_puts (_("General help using GNU software: <http://www.gnu.org/gethelp/>\n"));
}

void
pk_print_version ()
{
  pk_term_class ("logo");
  pk_puts ("     _____\n");
  pk_puts (" ---'   __\\_______\n");
  pk_printf ("            ______)  GNU poke %s\n", VERSION);
  pk_puts ("            __)\n");
  pk_puts ("           __)\n");
  pk_puts (" ---._______)\n");
  pk_term_end_class ("logo");
  /* xgettesxt: no-wrap */
  pk_puts ("\n");

  /* It is important to separate the year from the rest of the message,
     as done here, to avoid having to retranslate the message when a new
     year comes around.  */
  pk_term_class ("copyright");
  pk_printf (_("\
Copyright (C) %s Jose E. Marchesi.\n\
License GPLv3+: GNU GPL version 3 or later"), "2019");
  pk_term_hyperlink ("http://gnu.org/licenses/gpl.html", NULL);
  pk_puts (" <http://gnu.org/licenses/gpl.html>");
  pk_term_end_hyperlink ();
  pk_puts (".\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n");
  pk_term_end_class ("copyright");

    pk_printf (_("\
\nPowered by Jitter %s."), JITTER_VERSION);

    pk_puts (_("\
\n\
Perpetrated by Jose E. Marchesi.\n"));

}

static void
finalize ()
{
  ios_shutdown ();
  pk_cmd_shutdown ();
  pkl_free (poke_compiler);
  pvm_shutdown (poke_vm);
  pk_term_shutdown ();
}

static void
parse_args (int argc, char *argv[])
{
  char c;
  int ret;

  while ((ret = getopt_long (argc,
                             argv,
                             "ql:c:s:",
                             long_options,
                             NULL)) != -1)
    {
      c = ret;
      switch (c)
        {
        case HELP_ARG:
          print_help ();
          goto exit_success;
          break;
        case VERSION_ARG:
          pk_print_version ();
          goto exit_success;
          break;
        case QUIET_ARG:
          poke_quiet_p = 1;
          break;
        case 'q':
        case NO_INIT_FILE_ARG:
          poke_load_init_file = 0;
          break;
        case 'l':
        case LOAD_ARG:
          if (!pkl_compile_file (poke_compiler, optarg))
            goto exit_success;

          break;
        case 'c':
        case CMD_ARG:
          {
            int ret = pk_cmd_exec (optarg);
            if (!ret)
              goto exit_failure;
            poke_interactive_p = 0;
            break;
          }
        case 's':
        case SCRIPT_ARG:
          {
            int ret = pk_cmd_exec_script (optarg);
            if (!ret)
              goto exit_failure;
            poke_interactive_p = 0;
            break;
          }
          /* libtextstyle arguments are handled in pk-term.c, not
             here.   */
        case COLOR_ARG:
        case STYLE_ARG:
          break;
        default:
          goto exit_failure;
        }
    }

  if (optind < argc)
    {
      if (!ios_open (argv[optind++]))
        goto exit_failure;

      optind++;
    }

  if (optind < argc)
    {
      print_help();
      goto exit_failure;
    }

  return;

 exit_success:
  finalize ();
  exit (EXIT_SUCCESS);

 exit_failure:
  finalize ();
  exit (EXIT_FAILURE);
}

static void
repl ()
{
  if (!poke_quiet_p)
    {
      pk_print_version ();
      pk_puts ("\n");
      pk_puts (_("For help, type \".help\".\n"));
      pk_puts (_("Type \".exit\" to leave the program.\n"));
    }

  while (!poke_exit_p)
    {
      int ret;
      char *line;

#if 0
      /* This doesn't work well because readline's edition commands
         make use of the lenght of the prompt, i.e. the prompt string
         is expected to be passed to readline().  */
      pk_term_class ("prompt");
      pk_puts ("(poke) ");
      pk_term_end_class ("prompt");
#endif
      pk_term_flush ();
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
initialize (int argc, char *argv[])
{
  /* This is used by the `progname' gnulib module.  */
  set_program_name ("poke");

  /* i18n */
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  /* Determine whether the tool has been invoked interactively.  */
  poke_interactive_p = isatty (fileno (stdin));

  /* Determine the directory containing poke's scripts and other
     architecture-independent data.  */
  poke_datadir = getenv ("POKEDATADIR");
  if (poke_datadir == NULL)
    poke_datadir = PKGDATADIR;

  /* Initialize the terminal output.  */
  pk_term_init (argc, argv);

  /* Initialize the Poke Virtual Machine.  Note this should be done
     before initializing the compiler, since the later constructs and
     runs pvm programs internally.  */
  poke_vm = pvm_init ();

  /* Initialize the poke incremental compiler and load the standard
     library.  */
  poke_compiler = pkl_new ();
  {
    char *poke_std_pk;

    poke_std_pk = xmalloc (strlen (poke_datadir) + strlen ("/std.pk") + 1);
    strcpy (poke_std_pk, poke_datadir);
    strcat (poke_std_pk, "/std.pk");
    if (!pkl_compile_file (poke_compiler, poke_std_pk))
      exit (EXIT_FAILURE);
    free (poke_std_pk);
  }

  /* Initialize the command subsystem.  This should be done even if
     called non-interactively.  */
  pk_cmd_init ();

  /* Initialize the IO subsystem.  Ditto.  */
  ios_init ();
}

static void
initialize_user ()
{
  /* Load the user's initialization file ~/.pokerc, if it exist in the
     HOME directory.  */
  char *homedir = getenv ("HOME");

  if (homedir != NULL)
    {
      int ret;
      char *pokerc;

      pokerc = xmalloc (strlen (homedir) + strlen ("/.pokerc") + 1);
      strcpy (pokerc, homedir);
      strcat (pokerc, "/.pokerc");

      if (access (pokerc, R_OK) == 0)
        {
          ret = pk_cmd_exec_script (pokerc);
          if (ret == 1)
            exit (EXIT_FAILURE);
        }

      free (pokerc);
    }
}

int
main (int argc, char *argv[])
{
  /* Initialization.  */
  initialize (argc, argv);

  /* Parse args, loading files, opening files for IO, etc etc */
  parse_args (argc, argv);

  /* User's initialization.  */
  if (poke_load_init_file)
    initialize_user ();

  /* Enter the REPL.  */
  if (poke_interactive_p)
    repl ();

  /* Cleanup.  */
  finalize ();

  return poke_exit_code;
}
