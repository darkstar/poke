/* pk-cmd.h - terminal related stuff.  */

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

#ifndef PK_TERM_H
#define PK_TERM_H

/* Define ANSI terminal escape sequences to print each object with
   attributes depending on its tag. */

#if 0

#define ESC        "\033"
#define NOATTR     ESC"[0m"
#define BOLD       ESC"[1m"
#define FAINT      ESC"[2m"
#define ITALIC     ESC"[3m"
#define UNDERLINE  ESC"[4m"
#define REVERSE    ESC"[7m"
#define CROSSOUT   ESC"[9m"

#define BLACK        ESC"[0m"ESC"[30m"
#define WHITE        ESC"[1m"ESC"[37m"
#define BLUE         ESC"[0m"ESC"[34m"
#define LIGHTBLUE    ESC"[1m"ESC"[34m"
#define GREEN        ESC"[0m"ESC"[32m"
#define LIGHTGREEN   ESC"[1m"ESC"[32m"
#define CYAN         ESC"[0m"ESC"[36m"
#define LIGHTCYAN    ESC"[1m"ESC"[36m"
#define RED          ESC"[0m"ESC"[31m"
#define LIGHTRED     ESC"[1m"ESC"[31m"
#define MAGENTA      ESC"[0m"ESC"[35m"
#define LIGHTMAGENTA ESC"[1m"ESC"[35m"
#define BROWN        ESC"[0m"ESC"[33m"
#define LIGHTGRAY    ESC"[0m"ESC"[37m"
#define DARKGRAY     ESC"[1m"ESC"[30m"
#define LIGHTBLUE    ESC"[1m"ESC"[34m"
#define YELLOW       ESC"[1m"ESC"[33m"

#else

#define ESC
#define NOATTR
#define BOLD
#define FAINT
#define ITALIC
#define UNDERLINE
#define REVERSE
#define CROSSOUT

#define BLACK
#define WHITE
#define BLUE
#define LIGHTBLUE
#define GREEN
#define LIGHTGREEN
#define CYAN
#define LIGHTCYAN
#define RED
#define LIGHTRED
#define MAGENTA
#define LIGHTMAGENTA
#define BROWN
#define LIGHTGRAY
#define DARKGRAY
#define LIGHTBLUE
#define YELLOW

#endif

#endif /* PK_TERM_H */
