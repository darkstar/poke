dnl configure.ac for GNU poke
dnl
dnl Please process this file with autoconf to get a 'configure'
dnl script.

dnl Copyright (C) 2019 Jose E. Marchesi

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

AC_INIT([poke], [0.1-beta], [bug-poke@gnu.org])

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

dnl The Boehm-Weiser garbage collector

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(BDW_GC,[bdw-gc])

dnl Jitter

AC_JITTER
if test "x$JITTER" = "x"; then
   AC_MSG_WARN([you need Jitter to regenerate C files])
fi
if test "x$JITTER_CONFIG" = "x"; then
   AC_MSG_ERROR([you need jitter-config to compile])
fi

dnl We need to determine the endianness of the host system.  The
dnl following macro is also supposed to work when cross-compiling.

AC_C_BIGENDIAN

dnl --{enable,disable}-debug controls whether building poke in
dnl debug mode.  In particular, this causes using a more inefficient
dnl jitter model that keeps the dwarf info of the instruction
dnl bodies.

AC_ARG_ENABLE([debug],
              AS_HELP_STRING([--enable-debug],
                             [Build poke in debug mode (default is NO)]),
                             [poke_debug=$enableval], [poke_debug=no])

AM_CONDITIONAL([POKE_DEBUG],
               [test "x$poke_debug" = "xyes"])

dnl Generate output files
AC_CONFIG_FILES(Makefile
                lib/Makefile
                src/Makefile
                pickles/Makefile
                doc/Makefile
                po/Makefile.in
                testsuite/Makefile)
AC_OUTPUT

dnl End of configure.ac
