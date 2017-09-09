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
poke_SOURCES = poke.c poke.h pcl-tab.y pcl-lex.l

BUILT_SOURCES = pcl-tab.h
AM_YFLAGS = -d
AM_LFLAGS = -d

poke_CPPFLAGS = -I$(top_srcdir)/lib
poke_CFLAGS = $(FUSE_CFLAGS) $(SQLITE3_CFLAGS)
poke_LDADD = $(top_builddir)/lib/libpoke.la \
             $(LTLIBREADLINE)

# End of Makefile.am
