SUBDIRS = icons ui

desktopdir = $(datadir)/applications
desktop_in_in_files = pix.desktop.in.in pix-import.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in) 
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 

@INTLTOOL_DESKTOP_RULE@

gsettings_ENUM_NAMESPACE = org.x.pix
gsettings_ENUM_FILES = $(top_srcdir)/pix/*.h $(top_srcdir)/extensions/*/*.h
gsettings_schema_in_files = 					\
	org.x.pix.gschema.xml.in				\
	org.x.pix.change-date.gschema.xml.in		\
	org.x.pix.comments.gschema.xml.in		\
	org.x.pix.contact-sheet.gschema.xml.in		\
	org.x.pix.convert-format.gschema.xml.in		\
	org.x.pix.crop.gschema.xml.in			\
	org.x.pix.facebook.gschema.xml.in		\
	org.x.pix.file-manager.gschema.xml.in		\
	org.x.pix.flickr.gschema.xml.in			\
	org.x.pix.gstreamer-tools.gschema.xml.in		\
	org.x.pix.image-print.gschema.xml.in		\
	org.x.pix.image-viewer.gschema.xml.in		\
	org.x.pix.importer.gschema.xml.in		\
	org.x.pix.photo-importer.gschema.xml.in		\
	org.x.pix.picasaweb.gschema.xml.in		\
	org.x.pix.pixbuf-savers.gschema.xml.in		\
	org.x.pix.rename-series.gschema.xml.in		\
	org.x.pix.resize.gschema.xml.in			\
	org.x.pix.resize-images.gschema.xml.in		\
	org.x.pix.rotate.gschema.xml.in			\
	org.x.pix.slideshow.gschema.xml.in		\
	org.x.pix.webalbums.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_schema_in_files:.xml.in=.xml)

@INTLTOOL_XML_NOMERGE_RULE@

@GSETTINGS_RULES@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pix-$(PIX_API_VERSION).pc

man_MANS=pix.1

EXTRA_DIST =                    	\
	$(desktop_in_in_files)  	\
	$(gsettings_schema_in_files)    \
	$(pkgconfig_DATA)		\
	$(man_MANS)

DISTCLEANFILES =                	\
	$(desktop_in_files)		\
	$(pkgconfig_DATA)

CLEANFILES =				\
	$(desktop_DATA)         	\
	$(gsettings_SCHEMAS)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk
