dnl configure.ac for GNU poke
dnl
dnl Please process this file with autoconf to get a 'configure'
dnl script.

dnl Copyright (C) 2017 Jose E. Marchesi

dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_INIT([poke], [0.1], [bug-poke@gnu.org])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS(src/config.h)
AC_CONFIG_MACRO_DIR([m4])

dnl Autobuild
dnl AB_INIT

AC_PROG_CC
gl_EARLY

LT_INIT
AC_PROG_CC_C99
AM_PROG_CC_C_O

: ${LEX='flex'}
: ${LEXLIB='-lfl'}
: ${LEX_OUTPUT_ROOT='lex.yy'}
AC_ARG_VAR([LEX], [The flex implementation to use.])
AC_ARG_VAR([LEXLIB], [Options for linking with the flex runtime library.])
AC_ARG_VAR([LEX_OUTPUT_ROOT], [Base of the file name that the lexer generates.])

gl_BISON

dnl System
AC_CANONICAL_HOST
canonical=$host

gl_INIT

dnl i18n with gettext
AM_GNU_GETTEXT_VERSION([0.18.1])
AM_GNU_GETTEXT([external])

dnl FUSE and sqlite

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(FUSE,[fuse])
PKG_CHECK_MODULES(SQLITE3,[sqlite3])

dnl Generate output files
AC_CONFIG_FILES(Makefile
                lib/Makefile
                src/Makefile
                doc/Makefile
                po/Makefile.in)
AC_OUTPUT

dnl End of configure.ac
