pam_PROGRAMS = pam_thinkfinger.so
pamdir = $(SECUREDIR)

INCLUDES = -I$(top_srcdir)/libthinkfinger

if HAVE_OLD_PAM
pam_thinkfinger_so_SOURCES = pam_thinkfinger-compat.c pam_thinkfinger.c
else
pam_thinkfinger_so_SOURCES = pam_thinkfinger.c
endif
pam_thinkfinger_so_LDFLAGS = -shared --strip-all -Wl,-soname,pam_thinkfinger.so -Wl,--as-needed
pam_thinkfinger_so_CFLAGS = $(CFLAGS)
pam_thinkfinger_so_LDADD = $(top_builddir)/libthinkfinger/libthinkfinger.la $(PAM_LIBS) $(PTHREAD_LIBS)

