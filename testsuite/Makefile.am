# Copyright (C) 2019 Jose E. Marchesi
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

EXTRA_DIST = config lib poke.cmd poke.map poke.pkl poke.std

AUTOMAKE_OPTIONS = dejagnu

check-DEJAGNU: site.exp
	@srcdir=`cd $(srcdir) && pwd`; export srcdir; \
	r=`pwd`; export r; \
	LC_ALL=C; export LC_ALL; \
	EXPECT=$(EXPECT); export EXPECT; \
	runtest=$(RUNTEST); \
	if $(SHELL) -c "$$runtest --version" > /dev/null 2>&1; then \
	  CC_FOR_TARGET="$(CC_FOR_TARGET)" CFLAGS_FOR_TARGET="$(CFLAGS)" \
	  HAVE_LIBTEXTSTYLE="$(HAVE_LIBTEXTSTYLE)" \
          POKESTYLESDIR="$(top_srcdir)/etc" \
          POKEDATADIR="$(top_srcdir)/src" \
		$$runtest --tool $(DEJATOOL) --srcdir $${srcdir} --objdir $(builddir) \
			$(RUNTESTFLAGS); \
	else echo "WARNING: could not find \`runtest'" 1>&2; :;\
	fi
