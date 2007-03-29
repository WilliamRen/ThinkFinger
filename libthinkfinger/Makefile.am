lib_LTLIBRARIES = libthinkfinger.la
include_HEADERS = libthinkfinger.h
libthinkfinger_la_SOURCES = libthinkfinger.c 		\
			    libthinkfinger.h		\
			    libthinkfinger-crc.c	\
			    libthinkfinger-crc.h
libthinkfinger_la_CFLAGS = $(CFLAGS)
libthinkfinger_la_LDFLAGS = -version-info 0:0:0 $(USB_LIBS)
pkgconfigdir = $(LIBDIR)/pkgconfig
pkgconfig_DATA = libthinkfinger.pc

libthinkfinger_so_LDADD = $(PTHREAD_LIBS)
