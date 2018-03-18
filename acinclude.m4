# Autoconf macros for Jitter.
# Copyright (C) 2017, 2018 Luca Saiu
# Written by Luca Saiu

# This file is part of Jitter.

# Jitter is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# Jitter is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with Jitter.  If not, see <http://www.gnu.org/licenses/>.


# The Autoconf manual says (about writing Autoconf macros):
# No Autoconf macro should ever enter the user-variable name space;
# i.e., except for the variables that are the actual result of running the
# macro, all shell variables should start with 'ac_'.  In addition, small
# macros or any macro that is likely to be embedded in other macros should
# be careful not to use obvious names.


# Generic M4 machinery.
################################################################

# jitter_tocpp([my-text])
# -----------------------
# Expand to the argument where each letter is converted to upper case, and
# each dash is replaced by an underscore.
# Example: jitter_tocpp([my-teXt_4]) --> MY_TEXT_4
# About the namespace, see the comment for ac_jitter_using_automake.
m4_define([jitter_tocpp],
          [m4_translit([m4_toupper([$1])], ][-][, ][_][)])dnl

# jitter_define_iterator([iterator_name], [list])
# -----------------------------------------------
# Expand to the *definition* of an iterator like jitter_for_dispatch or
# jitter_for_flag , below.  Notice that there is an empty line appended to
# each repetition of the body.
# This higher-order macro is used internally to define convenient, nestable
# iterator macros.
m4_define([jitter_define_iterator],
  [m4_define([$1],
             [m4_foreach(][$][1][, [$2],
                         ][$][2][
)])])
# FIXME: no, this doesn't get quoting quite right.  Not using it.  Test case:
# jitter_for_dispatch([j],
#   [echo "quoted-[j] is unquoted-j"])
# =>
# echo "quoted-switch is unquoted-switch"
# echo "quoted-direct-threading is unquoted-direct-threading"
# echo "quoted-minimal-threading is unquoted-minimal-threading"
# echo "quoted-no-threading is unquoted-no-threading"
#
# Instead, by using directly m4_foreach , I get what I want:
# m4_foreach([i],
#            [jitter_dispatches],
#   [echo "quoted-[i] is unquoted-i"
# ])
# =>
# echo "quoted-i is unquoted-switch"
# echo "quoted-i is unquoted-direct-threading"
# echo "quoted-i is unquoted-minimal-threading"
# echo "quoted-i is unquoted-no-threading"

# jitter_for_dispatch([a_dispatch_variable], [body])
# --------------------------------------------------
# Expand to a sequence of bodies, each copy with a_dispatch_variable replaced
# by each of the dispatches in the order of jitter_dispatches; each body
# repetition has a newline at the end.
# Example:
# jitter_for_dispatch([a],
#   [jitter_for_dispatch([b],
#     [echo [Here is a dispatch pair: ]a[, ]b])])
#
# FIXME: this commented-out definition is not correct with respect to
# quoting: see the comment above.
# # jitter_define_iterator([jitter_for_dispatch],
# #                        [jitter_dispatches])
m4_define([jitter_for_dispatch],
  [m4_foreach([$1], [jitter_dispatches], [$2
])])

# jitter_for_flag([a_flag_variable], [body])
# ------------------------------------------
# Like jitter_for_dispatch, using jitter_flags instead of jitter_dispatches .
#
# FIXME: this commented-out definition is not correct with respect to
# quoting: see the comment above.
# jitter_define_iterator([jitter_for_flag],
#                        [jitter_flags])
m4_define([jitter_for_flag],
  [m4_foreach([$1], [jitter_flags], [$2
])])


# Jitter global definitions.
################################################################

# jitter_dispatches
# -----------------
# An M4 quoted list of every existing Jitter dispatch, including the ones which
# are not enabled, lower-case with dashes.
m4_define([jitter_dispatches],
          [[switch], [direct-threading], [minimal-threading], [no-threading]])

# jitter_flags
# ------------
# An M4 quoted list of flag names, lower-case with dashes.
m4_define([jitter_flags],
          [[cflags], [cppflags], [ldadd], [ldflags]])


# Jitter internal Autoconf macros.
################################################################

# The Autoconf macros in this section are subject to change in the future, and
# should not be directly invoked by the user.

# AC_JITTER_USING_AUTOMAKE
# ------------------------
# This is only used internally.  If Automake is being used, then define
# the shell variable ac_jitter_using_automake to "yes".
AC_DEFUN([AC_JITTER_USING_AUTOMAKE], [
# Define ac_jitter_using_automake to a non-empty iff the configuration system is
# used Automake and well, and not just Autoconf.  I have found no documented way
# of doing this, so I am relying on am__api_version being defined, which
# happens, indirectly, when AM_INIT_AUTOMAKE is called.
# This is not particularly related to Jitter but I found no predefined way
# of doing it, and I'm adding the variable to the Jitter namespace just so as
# not to pollute the generic ones.
if test "x$am__api_version" != "x"; then
  ac_jitter_using_automake="yes"
fi
]) # AC_JITTER_USING_AUTOMAKE


# Jitter exported Autoconf macros.
################################################################

# AC_JITTER_CONFIG
# ----------------
# Look for the jitter-config script, by default in $PATH or, if the option
# --with-jitter="PREFIX" is given, in PREFIX/bin (only).
#
# Define the substitution JITTER_CONFIG to either the full pathname of a
# jitter-config script which appears to be working, or nothing in case of
# problems.
# When Automake is used, also define the following Automake conditional:
# * JITTER_HAVE_JITTER_CONFIG  (true iff jitter-config has been found).
#
# In case jitter-config is found, also substitute:
# * JITTER_CONFIG_VERSION      (the version of Jitter the jitter-config script
#                               comes from)
# * JITTER_DISPATCHES          (all the enabled dispatched, small caps, with
#                               dashes separating words, one space separating
#                               dispatch names);
# * JITTER_BEST_DISPATCH       (the best dispatch in JITTER_DISPATCHES);
# * for every dispatching model $D (in all caps, with underscores separating
#   words):
#   - JITTER_$D_CFLAGS         (CFLAGS for using $D);
#   - JITTER_$D_CPPFLAGS       (CPPFLAGS for using $D);
#   - JITTER_$D_LDADD          (LDADD for using $D);
#   - JITTER_$D_LDFLAGS        (LDFLAGS for using $D).
#
# When Automake is used, also define the following Automake conditionals
# for each dispatching model $D (in all caps, with underscores separating
# words):
# * JITTER_ENABLE_DISPATCH_$D  (true iff $D is enabled).
AC_DEFUN([AC_JITTER_CONFIG], [
# I'd like to define Automake conditionals later on, but that only works if
# the project is actually using Automake.
AC_REQUIRE([AC_JITTER_USING_AUTOMAKE])

# Every test from now on is about C.
AC_LANG_PUSH([C])

# In order to compile Jittery VMs we need a recent C compiler; actually
# I've never tested on anything as old as C99, and that doesn't seem to
# be supported by Autoconf yet.  Let's at least test for C99 and give a
# warning if something is not okay.
AC_REQUIRE([AC_PROG_CC])  # This defines EXEEXT .
AC_REQUIRE([AC_PROG_CC_C99])
if test "x$ac_cv_prog_cc_c99" = "no"; then
  AC_MSG_WARN([the C compiler $CC does not seem to support C99.  I will
               try to go on, but there may be problems])
fi

# Provide an option for the user to explicitly set the prefix to
# bin/jitter .  ac_jitter_path will be defined as either the given
# path with "/bin" appended, or $PATH.
AC_ARG_WITH([jitter],
            [AS_HELP_STRING([--with-jitter="PREFIX"],
               [use the jitter program from the bin directory of the given
                prefix instead of searching for it in $PATH])],
            [ac_jitter_path="$withval/bin"])

# Search for the "jitter-config" script and perform the JITTER_CONFIG
# substitution.
AC_PATH_PROG([JITTER_CONFIG],
             [jitter-config],
             ,
             [$ac_jitter_path])

# However jitter-config was found, verify that it can be used; if not, unset the
# JITTER_CONFIG variable (and substitution).
AS_IF([test "x$JITTER_CONFIG" = "x"],
        [AC_MSG_NOTICE([can't find jitter-config])],
      [! test -r "$JITTER_CONFIG"],
        [AC_MSG_WARN([can't read jitter-config at $JITTER_CONFIG])
         JITTER_CONFIG=""],
      [! test -x "$JITTER_CONFIG"],
        [AC_MSG_WARN([can't execute jitter-config at $JITTER_CONFIG])
         JITTER_CONFIG=""],
      [! "$JITTER_CONFIG" --best-dispatch > /dev/null 2> /dev/null],
        [AC_MSG_WARN([non-working jitter-config at $JITTER_CONFIG])
         JITTER_CONFIG=""])

# Define the Automake conditional JITTER_HAVE_JITTER_CONFIG , if we are using
# Automake.
if test "x$ac_jitter_using_automake" != "x"; then
  AM_CONDITIONAL([JITTER_HAVE_JITTER_CONFIG],
                 [test "x$JITTER_CONFIG" != "x"])
fi

# At this point $JITTER_CONFIG is either the full pathname to an apparently
# working script, or empty.  If it seems to work, use it to define the rest of
# the substitutions.
if test "x$JITTER_CONFIG" != "x"; then
  # Define the jitter-config version.
  AC_SUBST([JITTER_CONFIG_VERSION],
           [$("$JITTER_CONFIG" --dump-version)])

  # Define the list of enabled dispatching models.
  AC_SUBST([JITTER_DISPATCHES],
           [$("$JITTER_CONFIG" --dispatches)])
  AC_MSG_NOTICE([the available Jitter dispatching models are $JITTER_DISPATCHES])

  # Define the best dispatching model.
  AC_SUBST([JITTER_BEST_DISPATCH],
           [$("$JITTER_CONFIG" --best-dispatch)])
  AC_MSG_NOTICE([the best Jitter dispatching model is $JITTER_BEST_DISPATCH])

  # Define flags for the best dispatching model.
  jitter_for_flag([a_flag],
    [AC_SUBST([JITTER_]jitter_tocpp(a_flag),
              [$("$JITTER_CONFIG" --a_flag)])])

  # For every dispatch and flag define a substitution JITTER_$dispatch_$flag .
  jitter_for_dispatch([a_dispatch],
    [if "$JITTER_CONFIG" --has-dispatch=a_dispatch; then
       jitter_for_flag([a_flag],
         [AC_SUBST([JITTER_]jitter_tocpp(a_dispatch)[_]jitter_tocpp(a_flag),
                   [$("$JITTER_CONFIG" --dispatch=a_dispatch --a_flag)])])
     fi])
fi # ...if jitter-config exists and works

# If using Automake then for every dispatch, define an Automake conditional
# telling whether it's enabled.  This has to be done even if we couldn't find a
# usable jitter-config , since Automake conditionals must be always defined:
# they do not default to false.
if test "x$ac_jitter_using_automake" != "x"; then
  jitter_for_dispatch([a_dispatch],
    [AM_CONDITIONAL([JITTER_ENABLE_DISPATCH_]jitter_tocpp(a_dispatch),
                    [   test "x$JITTER_CONFIG" != "x" \
                     && "$JITTER_CONFIG" --has-dispatch=]a_dispatch)])
fi

# We're done testing C features, for the time being.
AC_LANG_POP([C])
]) # AC_JITTER_CONFIG


# AC_JITTER_C_GENERATOR
# ---------------------
# Look for jitter, the C code generator program in $PATH if the option
# --with-jitter="PREFIX" is given, in DIRECTORY/bin (only).
#
# Substitute:
# * JITTER                            (the jitter program full path, or empty
#                                      if not found)
# * JITTER_VERSION                    (the version of Jitter the C generator
#                                      program comes from)
#
# When Automake is used, also define the following Automake conditional:
# * JITTER_HAVE_JITTER_C_GENERATOR    (true iff jitter has been found).
AC_DEFUN([AC_JITTER_C_GENERATOR], [
# Check for a C compiler, if it hasn't been done already.  This defines EXEEXT .
AC_REQUIRE([AC_PROG_CC])

# I'd like to define an Automake conditional later on, but that only works if
# the project is actually using Automake.
AC_REQUIRE([AC_JITTER_USING_AUTOMAKE])

# Search for the "jitter" program and perform the JITTER substitution.
AC_PATH_PROG([JITTER],
             [jitter],
             ,
             [$ac_jitter_path:$PATH])

# However jitter was found, verify that it can be used; if not, unset the JITTER
# variable (and substitution).
AS_IF([test "x$JITTER" = "x"],
        [AC_MSG_NOTICE([can't find jitter])],
      [! test -r "$JITTER"],
        [AC_MSG_WARN([can't read jitter at $JITTER])
         JITTER=""],
      [! test -x "$JITTER"],
        [AC_MSG_WARN([can't execute jitter at $JITTER])
         JITTER=""],
      [! "$JITTER" --dump-version > /dev/null],
        [AC_MSG_WARN([jitter at $JITTER seems to fail])
         JITTER=""])

# Define the Automake conditional JITTER_HAVE_JITTER_C_GENERATOR , if we are
# using Automake.
if test "x$ac_jitter_using_automake" != "x"; then
  AM_CONDITIONAL([JITTER_HAVE_JITTER_C_GENERATOR],
                 [test "x$JITTER" != "x"])
fi

# If we found a C generator, make the other substitutions.
if test "x$JITTER" != "x"; then
  # Define the Jitter C generator version.
  AC_SUBST([JITTER_VERSION],
           [$("$JITTER" --dump-version)])
fi
]) # AC_JITTER_C_GENERATOR


# AC_JITTER
# ---------
# Check for jitter-config and jitter as by calling both AC_JITTER_CONFIG
# and AC_JITTER_C_GENERATOR , performing the same substitutions and
# supporting the same command-line options.
# Warn if jitter-config and jitter have different versions.
#
# This is the only macro the user needs to call to check for a Jitter
# installation, in most cases.
AC_DEFUN([AC_JITTER], [

# Check for jitter-config .
AC_REQUIRE([AC_JITTER_CONFIG])

# Check for jitter .
AC_REQUIRE([AC_JITTER_C_GENERATOR])

# In case we found both, check that their two versions match.
if test "x$JITTER_CONFIG" != "x" && test "x$JITTER" != "x"; then
  if test "x$JITTER_CONFIG_VERSION" != "x$JITTER_VERSION"; then
    AC_MSG_WARN([version mismatch between $JITTER_CONFIG (version
$JITTER_CONFIG_VERSION) and $JITTER (version $JITTER_VERSION)])
  fi
fi
]) # AC_JITTER
