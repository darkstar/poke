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

AC_INIT([GNU poke], [0.1-beta], [poke-devel@nongnu.org], [poke],
        [http://www.jemarch.net/poke.html])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS(src/config.h)
AC_CONFIG_MACRO_DIR([m4])

# Include the Autoconf macros from Jitter.
m4_include([m4/jitter.m4])


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
AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT([external])

dnl The Boehm-Weiser garbage collector

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(BDW_GC,[bdw-gc])

dnl Jitter

AC_JITTER_SUBPACKAGE([jitter])

dnl Used in Makefile.am.  See the note there.
WITH_JITTER=$with_jitter
AC_SUBST([WITH_JITTER])

dnl We need to determine the endianness of the host system.  The
dnl following macro is also supposed to work when cross-compiling.

AC_C_BIGENDIAN

dnl Use libtextstyle if available.  Otherwise, use the dummy header
dnl file provided by gnulib's libtextstyle-optional module.

gl_LIBTEXTSTYLE_OPTIONAL
AX_LIBTEXTSTYLE_HYPERLINK

dnl Generate output files
AC_CONFIG_FILES(Makefile
                lib/Makefile
                src/Makefile
                pickles/Makefile
                doc/Makefile
                po/Makefile.in
                etc/Makefile
                testsuite/Makefile)
AC_OUTPUT

dnl Report warnings

if test "x$gl_cv_lib_readline" = "xno"; then
   echo "warning: an usable libreadline was not found in the system."
   echo "warning: poke will use a dummy replacement with no edition "
   echo "warning: capabilities."
fi

if test "x$HAVE_LIBTEXTSTYLE" = "xno"; then
   echo "warning: libtextstyle was not found in the system."
   echo "warning: poke's output won't be styled."
fi

dnl End of configure.ac
