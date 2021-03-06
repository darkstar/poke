# GNU poke

# Copyright (C) 2019 Jose E. Marchesi

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


dist_pkgdata_DATA = pkl-rt.pk std.pk \
                    pk-cmd.pk pk-dump.pk

bin_PROGRAMS = poke
poke_SOURCES = poke.c poke.h \
               ios.c ios.h ios-dev.h \
               ios-dev-file.c \
               pk-term.c pk-term.h \
               pk-repl.c pk-repl.h \
               pk-cmd.c pk-cmd.h \
               pk-file.c \
               pk-info.c pk-misc.c pk-help.c pk-vm.c \
               pk-print.c pk-def.c pk-set.c \
               pkl.h pkl.c \
               pkl-ast.h pkl-ast.c \
               pkl-env.h pkl-env.c \
               pkl-pass.h pkl-pass.c \
               pkl-promo.h pkl-promo.c \
               pkl-fold.h pkl-fold.c \
               pkl-typify.h pkl-typify.c \
	       pkl-anal.h pkl-anal.c \
               pkl-trans.h pkl-trans.c \
               pkl-tab.y pkl-lex.l \
               pkl-gen.h pkl-gen.c \
               pkl-asm.h pkl-asm.c \
               pkl-parser.h pkl-parser.c \
               pvm.h pvm.c \
               pvm-alloc.h pvm-alloc.c \
               pvm-val.h pvm-val.c \
               pvm-env.h pvm-env.c \
               pvm.jitter \
               pvm-vm.h pvm-vm1.c pvm-vm2.c \
               pkl-gen.pks pkl-asm.pks \
               pkl-gen.pkc pkl-asm.pkc \
               pkl-insn.def pkl-ops.def pkl-attrs.def


.pks.pkc:
	srcdir=$(srcdir) $(srcdir)/ras $< > $@

# XXX this shouldn't be necessary?
EXTRA_DIST = pkl-lex.h

BUILT_SOURCES = pkl-tab.h pkl-lex.c \
                pkl-gen.pkc pkl-asm.pkc \
                pvm-vm.h pvm-vm1.c pvm-vm2.c
AM_YFLAGS = -d -t --report=state
AM_LFLAGS = -d

poke_CPPFLAGS = -I$(top_builddir)/lib -I$(top_srcdir)/lib \
                -DPKL_DEBUG \
                -DPKGDATADIR=\"$(pkgdatadir)\" \
                -DJITTER_VERSION=\"$(JITTER_VERSION)\" \
                -DLOCALEDIR=\"$(localedir)\"
poke_CFLAGS = -Wall $(BDW_GC_CFLAGS)
poke_LDADD = $(top_builddir)/lib/libpoke.la \
             $(LTLIBREADLINE) $(BDW_GC_LIBS) $(LIBTEXTSTYLE)
poke_LDFLAGS =

# Integration with jitter.

poke_CPPFLAGS += $(JITTER_CPPFLAGS)
poke_CFLAGS += $(JITTER_CFLAGS)
poke_LDADD += $(JITTER_LDADD)
poke_LDFLAGS += $(JITTER_LDFLAGS)

BUILT_SOURCES += pvm-vm.h pvm-vm1.c pvm-vm2.c

pvm-vm.h pvm-vm1.c pvm-vm2.c: pvm.jitter
	$(JITTER) $(srcdir)/pvm.jitter --output $(builddir)

# End of Makefile.am
