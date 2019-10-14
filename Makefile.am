ACLOCAL_AMFLAGS = -I m4
SUBDIRS = lib pickles src doc testsuite etc po

# This (and the accompanying AC_SUBST of WITH_JITTER) is to be removed
# as soon as we bump to a Jitter version not requiring --with-jitter
# if it is installed in a standard location.
AM_DISTCHECK_CONFIGURE_FLAGS = --with-jitter=$(WITH_JITTER) \
                               --enable-debug=$(ENABLE_DEBUG)
