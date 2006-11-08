lib_LTLIBRARIES = libthinkfinger.la

# AM_CFLAGS = -std=c99 -Wstrict-prototypes -Wall -fPIC -D_GNU_SOURCE

INCLUDES =

include_HEADERS = libthinkfinger.h

libthinkfinger_la_SOURCES = libthinkfinger.c 		\
			    libthinkfinger.h		\
			    libthinkfinger-crc.c	\
			    libthinkfinger-crc.h

libthinkfinger_la_CFLAGS = -Wall $(USB_CFLAGS)
libthinkfinger_la_LDFLAGS = -version-info 0:0:0 $(USB_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = thinkfinger.pc
