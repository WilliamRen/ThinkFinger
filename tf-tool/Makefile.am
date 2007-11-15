sbin_PROGRAMS = tf-tool

INCLUDES = -I$(top_srcdir)/libthinkfinger

tf_tool_SOURCES = tf-tool.c
tf_tool_LDADD = $(top_builddir)/libthinkfinger/libthinkfinger.la 
tf_tool_CFLAGS = $(CFLAGS)
if HAVE_BASH
 completiondir = $(sysconfdir)/bash_completion.d
 completion_SCRIPTS = tf-tool_completion
endif
