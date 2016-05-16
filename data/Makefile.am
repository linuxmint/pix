SUBDIRS = icons ui

desktopdir = $(datadir)/applications
desktop_in_in_files = gthumb.desktop.in.in gthumb-import.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in) 
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 

@INTLTOOL_DESKTOP_RULE@

gsettings_ENUM_NAMESPACE = org.gnome.gthumb
gsettings_ENUM_FILES = $(top_srcdir)/gthumb/*.h $(top_srcdir)/extensions/*/*.h
gsettings_schema_in_files = 					\
	org.gnome.gthumb.gschema.xml.in				\
	org.gnome.gthumb.change-date.gschema.xml.in		\
	org.gnome.gthumb.comments.gschema.xml.in		\
	org.gnome.gthumb.contact-sheet.gschema.xml.in		\
	org.gnome.gthumb.convert-format.gschema.xml.in		\
	org.gnome.gthumb.crop.gschema.xml.in			\
	org.gnome.gthumb.facebook.gschema.xml.in		\
	org.gnome.gthumb.file-manager.gschema.xml.in		\
	org.gnome.gthumb.flickr.gschema.xml.in			\
	org.gnome.gthumb.gstreamer-tools.gschema.xml.in		\
	org.gnome.gthumb.image-print.gschema.xml.in		\
	org.gnome.gthumb.image-viewer.gschema.xml.in		\
	org.gnome.gthumb.importer.gschema.xml.in		\
	org.gnome.gthumb.photo-importer.gschema.xml.in		\
	org.gnome.gthumb.picasaweb.gschema.xml.in		\
	org.gnome.gthumb.pixbuf-savers.gschema.xml.in		\
	org.gnome.gthumb.rename-series.gschema.xml.in		\
	org.gnome.gthumb.resize.gschema.xml.in			\
	org.gnome.gthumb.resize-images.gschema.xml.in		\
	org.gnome.gthumb.rotate.gschema.xml.in			\
	org.gnome.gthumb.slideshow.gschema.xml.in		\
	org.gnome.gthumb.webalbums.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_schema_in_files:.xml.in=.xml)

@INTLTOOL_XML_NOMERGE_RULE@

@GSETTINGS_RULES@

convertdir = $(datadir)/GConf/gsettings
convert_DATA = gthumb.convert

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gthumb-$(GTHUMB_API_VERSION).pc

man_MANS=gthumb.1

EXTRA_DIST =                    	\
	$(desktop_in_in_files)  	\
	$(gsettings_schema_in_files)    \
	$(pkgconfig_DATA)		\
	gthumb.convert			\
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
