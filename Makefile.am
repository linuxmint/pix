SUBDIRS = m4 copy-n-paste data po pix extensions tests help

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST =                    \
	AUTHORS                 \
	ChangeLog.pre-git	\
        MAINTAINERS             \
        NEWS                    \
        README                  \
	config.rpath		\
	intltool-merge.in       \
	intltool-update.in      \
	intltool-extract.in

DISTCLEANFILES =                        \
	po/.intltool-merge-cache        \
	intltool-extract                \
	intltool-merge                  \
	intltool-update

MAINTAINERCLEANFILES = 			\
	$(srcdir)/INSTALL		\
	$(srcdir)/aclocal.m4 		\
	$(srcdir)/autoscan.log 		\
	$(srcdir)/compile 		\
	$(srcdir)/config.guess 		\
	$(srcdir)/config.h.in 		\
	$(srcdir)/config.sub 		\
	$(srcdir)/configure.scan 	\
	$(srcdir)/depcomp 		\
	$(srcdir)/gnome-doc-utils.make	\
	$(srcdir)/install-sh 		\
	$(srcdir)/ltmain.sh 		\
	$(srcdir)/omf.make 		\
	$(srcdir)/m4/intltool.m4	\
	$(srcdir)/m4/libtool.m4		\
	$(srcdir)/m4/lt~obsolete.m4	\
	$(srcdir)/m4/ltoptions.m4	\
	$(srcdir)/m4/ltsugar.m4		\
	$(srcdir)/m4/ltversion.m4	\
	$(srcdir)/m4/gnome-doc-utils.m4	\
	$(srcdir)/missing 		\
	$(srcdir)/mkinstalldirs 	\
	$(srcdir)/xmldocs.make 		\
	$(srcdir)/gtk-doc.make 		\
	$(srcdir)/ChangeLog 		\
	`find "$(srcdir)" -type f -name Makefile.in -print`

GITIGNOREFILES = build .autotools .cdt* .cproject .project .settings/ *.patch

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

-include $(top_srcdir)/git.mk
