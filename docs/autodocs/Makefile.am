EXTRA_DIST = Doxyfile

all:
	@if [ -n "$(DOXYGEN)" ] ; then \
		doxygen $(srcdir)/Doxyfile ; \
	fi

CLEANFILES = html/*
