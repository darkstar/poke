# libtextstyle-hyperlink.m4 serial 1
dnl Copyright (C) 2019 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Darshit Shah

dnl AX_LIBTEXTSTYLE_HYPERLINK
dnl Checks if the version of libtextstyle available in the system is recent
dnl enough to support hyperlinks.
dnl It AC_DEFINEs HAVE_TEXTSTYLE_HYPERLINK_SUPPORT based on the result of the
dnl test.

AC_DEFUN([AX_LIBTEXTSTYLE_HYPERLINK],
[
  AC_REQUIRE([gl_LIBTEXTSTYLE_OPTIONAL])
  if test $HAVE_LIBTEXTSTYLE = yes; then
    AC_CHECK_LIB([textstyle], [styled_ostream_set_hyperlink])
    has_hyper_support=${ac_cv_lib_textstyle_styled_ostream_set_hyperlink}
  else
    dnl If HAVE_LIBTEXTSTYLE is no, then we assume that the gnulib stubs of
    dnl textstyle are available. Those stubs implement the dummy functions
    dnl required and hence we can assume that hyperlink support is available
    has_hyper_support=yes
  fi

  if test $has_hyper_support = yes; then
      AC_DEFINE([HAVE_TEXTSTYLE_HYPERLINK_SUPPORT], 1,
                [Defined if libtextstyle has support for terminal hyperlinks])
  fi
])
