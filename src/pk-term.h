/* pk-cmd.h - terminal related stuff.  */

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

#ifndef PK_TERM_H
#define PK_TERM_H

#include <config.h>

#include <textstyle.h>

/* Initialize and finalize the terminal subsystem.  */
void pk_term_init (int argc, char *argv[]);
void pk_term_shutdown (void);

/* Flush the terminal output.  */
extern void pk_term_flush (void);

/* Print a string to the terminal.  */
extern void pk_puts (const char *str);

/* Print a formatted string to the terminal.  */
extern void pk_printf (const char *format, ...);

/* Class handling.  */

extern void pk_term_class (const char *class);
extern void pk_term_end_class (const char *class);

#endif /* PK_TERM_H */
