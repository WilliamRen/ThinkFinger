bin_PROGRAMS = tf-tool

INCLUDES = -I$(top_srcdir)/libthinkfinger

tf_tool_SOURCES = tf-tool.c
tf_tool_LDFLAGS = `pkg-config --libs libusb`
tf_tool_LDADD = $(top_builddir)/libthinkfinger/libthinkfinger.la 
tf_tool_CFLAGS = -Wall -g `pkg-config --cflags libusb`
