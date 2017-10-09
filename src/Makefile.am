# GNU poke

# Copyright (C) 2017 Jose E. Marchesi

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

bin_PROGRAMS = poke
poke_SOURCES = poke.c poke.h \
               pk-io.c pk-io.h \
               pk-cmd.c pk-cmd.h \
               pk-dump.c pk-peek.c pk-poke.c pk-file.c \
               pk-info.c pk-misc.c pk-help.c pk-vm.c \
               pkl.h pkl.c \
               pkl-ast.h pkl-ast.c \
               pkl-tab.y pkl-lex.l \
               pkl-gen.h pkl-gen.c \
               pkl-parser.h pkl-parser.c \
               pvm.h pvm.c \
               pvm-vm.h pvm-vm1.c pvm-vm2.c

BUILT_SOURCES = pkl-tab.h pkl-lex.c
AM_YFLAGS = -d
AM_LFLAGS = -d

poke_CPPFLAGS = -I$(top_srcdir)/lib -DPKL_DEBUG
poke_CFLAGS = $(FUSE_CFLAGS) $(SQLITE3_CFLAGS) -Wall
poke_LDADD = $(top_builddir)/lib/libpoke.la \
             $(LTLIBREADLINE)

# Integration with jitter.

JITTER=/home/jemarch/tmp/jitter
poke_CPPFLAGS += -I$(JITTER)/include
poke_LDADD += -L$(JITTER)/lib -ljitter--no-threading
BUILT_SOURCES += pvm-vm.h pvm-vm1.c pvm-vm2.c

pvm-vm.h pvm-vm1.c pvm-vm2.c: pvm.jitter
	$(JITTER)/bin/jitterc $< \
                   --output $(builddir) \
	           --max-fast-register-no=5 \
	           --max-nonresidual-no=-1 \
	           --template-directory=/home/jemarch/gnu/src/jitter/templates

# End of Makefile.am
