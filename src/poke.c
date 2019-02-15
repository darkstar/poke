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

#include "ios.h"
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
  SCRIPT_ARG,
};

static const struct option long_options[] =
{
  {"help", no_argument, NULL, HELP_ARG},
  {"version", no_argument, NULL, VERSION_ARG},
  {"quiet", no_argument, NULL, QUIET_ARG},
  {"load", required_argument, NULL, LOAD_ARG},
  {"command", required_argument, NULL, CMD_ARG},
  {"script", required_argument, NULL, SCRIPT_ARG},
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
  /* TRANSLATORS: --help output, poke arguments.
     no-wrap */
  fputs (_("\
  -l, --load=FILE                     load the given pickle at startup.\n"),
         stdout);

  puts ("");

  /* TRANSLATORS: --help output, poke arguments.
     no-wrap */
  fputs(_("\
Commanding poke from the command line:\n\
  -c, --command=CMD                   execute the given command.\n\
  -s, --script=FILE                   execute commands from FILE.\n"),
         stdout);

  puts ("");
  /* TRANSLATORS: --help output, less used poke arguments.
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
  printf (_("%s home page: <http://www.gnu.org/s/poke/>\n"),
          PACKAGE_NAME);
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
There is NO WARRANTY, to the extent permitted by law.\n"), "2019");

  puts (_("\
\n\
Perpetrated by Jose E. Marchesi."));
}

static void
parse_args (int argc, char *argv[])
{
  char c;
  int ret;

  while ((ret = getopt_long (argc,
                             argv,
                             "l:c:s:",
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
        case 'l':
        case LOAD_ARG:
          {
            pvm_val val;
            int ret;
            pvm_program program
              = pkl_compile_file (poke_compiler, optarg);

            if (program == NULL)
              /* The compiler emits it's own error messages.  */
              exit (EXIT_FAILURE);

            ret = pvm_run (poke_vm, program, &val);
            if (ret != PVM_EXIT_OK)
              {
                fprintf (stderr, _("run-time error\n"));
                exit (EXIT_FAILURE);
              }
            
            break;
          }
        case 'c':
        case CMD_ARG:
          {
            int ret = pk_cmd_exec (optarg);
            if (!ret)
              exit (EXIT_FAILURE);
            poke_interactive_p = 0;
            break;
          }
        case 's':
        case SCRIPT_ARG:
          {
            int ret = pk_cmd_exec_script (optarg);
            if (!ret)
              exit (EXIT_FAILURE);
            poke_interactive_p = 0;
            break;
          }
        default:
          exit (EXIT_FAILURE);
        }
    }

  if (optind < argc)
    {
      if (!ios_open (argv[optind++]))
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
  /* This is used by the `progname' gnulib module.  */
  set_program_name ("poke");
  
  /* Determine whether the tool has been invoked interactively.  */
  poke_interactive_p = isatty (fileno (stdin));
  
  /* Determine the directory containing poke's scripts and other
     architecture-independent data.  */
  poke_datadir = getenv ("POKEDATADIR");
  if (poke_datadir == NULL)
    poke_datadir = PKGDATADIR;

  /* Initialize the Poke Virtual Machine.  Note this should be done
     before initializing the compiler, since the later constructs and
     runs pvm programs internally.  */
  poke_vm = pvm_init ();

  /* Initialize the poke incremental compiler and load the standard
     library.  */
  poke_compiler = pkl_new ();
  {
    pvm_program program;
    int pvm_ret;
    pvm_val val;
    char *poke_std_pk;

    poke_std_pk = xmalloc (strlen (poke_datadir) + strlen ("/std.pk") + 1);
    strcpy (poke_std_pk, poke_datadir);
    strcat (poke_std_pk, "/std.pk");
    program = pkl_compile_file (poke_compiler, poke_std_pk);
    free (poke_std_pk);

    if (program == NULL)
      /* XXX explanatory error message.  */
      exit (1);

    pvm_ret = pvm_run (poke_vm, program, &val);
    if (pvm_ret != PVM_EXIT_OK)
      exit (1);
  }

  /* Initialize the command subsystem.  This should be done even if
     called non-interactively.  */
  pk_cmd_init ();

  /* Initialize the IO subsystem.  Ditto.  */
  ios_init ();
}

static void
finalize ()
{
  ios_shutdown ();
  pk_cmd_shutdown ();
  pkl_free (poke_compiler);
  pvm_shutdown (poke_vm);
}

int
main (int argc, char *argv[])
{
  /* Initialization.  */
  initialize ();

  /* Parse args, loading files, opening files for IO, etc etc */
  parse_args (argc, argv);

  /* Enter the REPL.  */
  if (poke_interactive_p)
    repl ();

  /* Cleanup.  */
  finalize ();
  
  return poke_exit_code;
}
