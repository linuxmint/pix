if BUILD_TEST_SUITE
noinst_PROGRAMS = dom-test glib-utils-test gsignature-test oauth-test
endif

dom_test_SOURCES = dom-test.c $(top_srcdir)/gthumb/dom.c
dom_test_LDADD = $(GTHUMB_LIBS) 
dom_test_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir)/gthumb

glib_utils_test_SOURCES = glib-utils-test.c $(top_srcdir)/gthumb/glib-utils.c
glib_utils_test_LDADD = $(GTHUMB_LIBS) 
glib_utils_test_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir)/gthumb

gsignature_test_SOURCES = gsignature-test.c $(top_srcdir)/gthumb/gsignature.c
gsignature_test_LDADD = $(GTHUMB_LIBS) 
gsignature_test_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir)/gthumb

oauth_test_SOURCES = oauth-test.c $(top_srcdir)/gthumb/gsignature.c
oauth_test_LDADD = $(GTHUMB_LIBS)
oauth_test_CFLAGS = $(GTHUMB_CFLAGS) -I$(top_srcdir)/gthumb

-include $(top_srcdir)/git.mk
