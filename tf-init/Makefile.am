sbin_PROGRAMS = tf-init

INCLUDES = -I$(top_srcdir)/libthinkfinger

tf_init_SOURCES = tf-init.c
tf_init_LDFLAGS = `pkg-config --libs libusb`
tf_init_LDADD = $(top_builddir)/libthinkfinger/libthinkfinger.la 
tf_init_CFLAGS = -Wall -g `pkg-config --cflags libusb`
