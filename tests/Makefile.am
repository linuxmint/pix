if BUILD_TEST_SUITE
noinst_PROGRAMS = dom-test glib-utils-test gsignature-test oauth-test
endif

dom_test_SOURCES = dom-test.c $(top_srcdir)/pix/dom.c
dom_test_LDADD = $(PIX_LIBS) 
dom_test_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir)/pix

glib_utils_test_SOURCES = glib-utils-test.c $(top_srcdir)/pix/glib-utils.c
glib_utils_test_LDADD = $(PIX_LIBS) 
glib_utils_test_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir)/pix

gsignature_test_SOURCES = gsignature-test.c $(top_srcdir)/pix/gsignature.c
gsignature_test_LDADD = $(PIX_LIBS) 
gsignature_test_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir)/pix

oauth_test_SOURCES = oauth-test.c $(top_srcdir)/pix/gsignature.c
oauth_test_LDADD = $(PIX_LIBS)
oauth_test_CFLAGS = $(PIX_CFLAGS) -I$(top_srcdir)/pix

-include $(top_srcdir)/git.mk
