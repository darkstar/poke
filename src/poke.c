/* poke.c - Interactive editor for binary files.  */

/* Copyright (C) 2017 Jose E. Marchesi */

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
#define _(str) gettext (str)
#include <unistd.h>
#include <string.h>
#include "readline.h"
#if defined HAVE_READLINE_HISTORY_H
# include <readline/history.h>
#endif

#include "pcl-parser.h"
#include "poke.h"

/* poke can be run either interactively (from a tty) or in batch mode.
   The following predicate records this.  */

int poke_interactive_p;

/* Command line options management.  */

enum
{
  HELP_ARG,
  VERSION_ARG
};

static const struct option long_options[] =
{
  {"help", no_argument, NULL, HELP_ARG},
  {"version", no_argument, NULL, VERSION_ARG},
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

static void
print_version ()
{
  /* TRANSLATORS: ascii-art in here.  */
  fprintf (stdout, "     _____\n");
  fprintf (stdout, " ---'   __\\_______\n");
  fprintf (stdout, "            ______)  GNU poke %s\n", VERSION);
  fprintf (stdout, "            __)\n");
  fprintf (stdout, "           __)\n");
  fprintf (stdout, " ---._______)\n");
  /* xgettesxt: no-wrap */
  puts ("");

  /* It is important to separate the year from the rest of the message,
     as done here, to avoid having to retranslate the message when a new
     year comes around.  */  
  printf (_("\
Copyright (C) %s Jose E. Marchesi.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n"), "2017");

  puts (_("\
\n\
Written by Jose E. Marchesi."));

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
          {
            print_help ();
            exit (EXIT_SUCCESS);
            break;
          }
        case VERSION_ARG:
          {
            print_version ();
            exit (EXIT_SUCCESS);
            break;
          }
        default:
          exit (EXIT_FAILURE);
        }
    }

  /* XXX: open IO file.  */
}

static int
repl ()
{
  print_version ();
  puts ("");
  
  while (1)
    {
      char *line = readline ("(poke) ");
      
      if (line == NULL
          || strcmp (line, "exit") == 0)
        {
          free (line);
          break;
        }
          
#if defined HAVE_READLINE_HISTORY_H
      if (line && *line)
        add_history (line);
#endif      

      free (line);
    }
}

/*
 * yyscan_t scanner;
 *
 * yylex_init ( &scanner ); | yylex_init_extra ( YY_EXTRA_TYPE, &scanner );
 * yyset_extra ( YY_EXTRA_TYPE, & scanner );
 * YY_EXTRA_TYPE yyget_extra ( & scanner );
 * yylex ( scanner );
 *  0 => success
 *  ENOMEM
 *  EINVAL
 * yylex_destroy ( &scanner );
 *
 * yyextra stores user-specific data.
 *
 * Parser:
 *
 *  int yyparse (void); (or pcl_tab_parse)
 *
 *    0 -> parsing susccessful.
 *    1 -> syntax error, or YYABORT.
 *    2 -> memory exhaustion.
 *
 * YYACCEPT - return immediately returning 0
 * YYABORT - return immediately returning 1
 *
 * %parse-param: make arguments available in yyparse, and thus
 *  in grammar actions.
 *
 *  yypush_parse () -> returns YYPUSH_MORE if more input is required
 *  to finish parsing the grammar.
 *  yypull_parse () -> parse the rest of the input stream.
 */

int
main (int argc, char *argv[])
{
  set_program_name (xstrdup ("poke"));
  parse_args (argc, argv);

  /* Determine whether the tool has been invoked interactively.  */
  poke_interactive_p = isatty (fileno (stdin));

  /* Enter the REPL.  */
  //  if (poke_interactive_p)
  //    return repl ();

  int ret = pcl_parse_file (stdin);
  if (ret == 1)
    printf ("SYNTAX ERROR\n");
  else if (ret == 2)
    printf ("MEMORY EXHAUSTION\n");

  return 0;
}
