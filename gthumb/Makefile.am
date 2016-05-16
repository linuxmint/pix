SUBDIRS = resources

bin_PROGRAMS = gthumb

ENUM_TYPES =						\
	gth-enum-types.h				\
	gth-enum-types.c

MARSHALLERS =						\
	gth-marshal.c					\
	gth-marshal.h

EXTERNAL = 						\
	gedit-message-area.c				\
	gedit-message-area.h				\
	gnome-desktop-thumbnail.c			\
	gnome-desktop-thumbnail.h			\
	gnome-thumbnail-pixbuf-utils.c

PUBLIC_BUILT_HEADER_FILES =				\
	gth-enum-types.h				\
	gth-marshal.h

PUBLIC_HEADER_FILES = 					\
	$(PUBLIC_BUILT_HEADER_FILES)			\
	cairo-scale.h					\
	cairo-utils.h					\
	dom.h						\
	gedit-message-area.h				\
	gfixed.h					\
	gio-utils.h					\
	glib-utils.h					\
	gnome-desktop-thumbnail.h			\
	gsignature.h					\
	gth-auto-paned.h				\
	gth-async-task.h				\
	gth-buffer-data.h				\
	gth-browser.h					\
	gth-color-scale.h				\
	gth-dumb-notebook.h				\
	gth-duplicable.h				\
	gth-embedded-dialog.h				\
	gth-empty-list.h				\
	gth-extensions.h				\
	gth-file-chooser-dialog.h			\
	gth-error.h					\
	gth-file-data.h					\
	gth-file-list.h					\
	gth-file-properties.h				\
	gth-file-selection.h				\
	gth-file-source.h				\
	gth-file-source-vfs.h				\
	gth-file-store.h				\
	gth-file-tool.h					\
	gth-file-view.h					\
	gth-filter.h					\
	gth-filterbar.h					\
	gth-filter-editor-dialog.h			\
	gth-filter-file.h				\
	gth-folder-tree.h				\
	gth-grid-view.h					\
	gth-histogram.h					\
	gth-histogram-view.h				\
	gth-hook.h					\
	gth-icon-cache.h				\
	gth-image.h					\
	gth-image-dragger.h				\
	gth-image-history.h				\
	gth-image-list-task.h				\
	gth-image-loader.h				\
	gth-image-navigator.h				\
	gth-image-preloader.h				\
	gth-image-saver.h				\
	gth-image-selector.h				\
	gth-image-task.h				\
	gth-image-utils.h				\
	gth-image-viewer.h				\
	gth-image-viewer-tool.h				\
	gth-info-bar.h					\
	gth-load-file-data-task.h			\
	gth-location-chooser.h				\
	gth-main.h					\
	gth-menu-action.h				\
	gth-menu-button.h				\
	gth-metadata.h					\
	gth-metadata-chooser.h				\
	gth-metadata-provider.h				\
	gth-monitor.h					\
	gth-multipage.h					\
	gth-overwrite-dialog.h				\
	gth-paned.h					\
	gth-preferences.h				\
	gth-progress-dialog.h				\
	gth-request-dialog.h				\
	gth-save-file-data-task.h			\
	gth-save-image-task.h				\
	gth-screensaver.h				\
	gth-sidebar.h					\
	gth-statusbar.h					\
	gth-source-tree.h				\
	gth-stock.h					\
	gth-string-list.h				\
	gth-tags-entry.h				\
	gth-tags-file.h					\
	gth-task.h					\
	gth-test.h					\
	gth-test-category.h				\
	gth-test-chain.h				\
	gth-test-selector.h				\
	gth-test-simple.h				\
	gth-thumb-loader.h				\
	gth-time.h					\
	gth-time-selector.h				\
	gth-toggle-menu-action.h			\
	gth-toggle-menu-tool-button.h			\
	gth-toolbox.h					\
	gth-uri-list.h					\
	gth-user-dir.h					\
	gth-viewer-page.h				\
	gth-window.h					\
	gtk-utils.h					\
	gvaluehash.h					\
	main.h						\
	pixbuf-cache.h					\
	pixbuf-io.h					\
	pixbuf-utils.h					\
	typedefs.h					\
	zlib-utils.h					\
	$(NULL)
	
PRIVATE_HEADER_FILES = 					\
	dlg-location.h					\
	dlg-preferences-extensions.h			\
	gth-browser-actions-callbacks.h			\
	gth-browser-actions-entries.h			\
	gth-browser-ui.h				\
	gth-metadata-provider-file.h			\
	dlg-personalize-filters.h			\
	dlg-preferences.h				\
	dlg-sort-order.h				\
	gth-window-actions-callbacks.h			\
	gth-window-actions-entries.h			\
	main-migrate.h					\
	$(NULL)

RESOURCES = 						\
	gth-resources.c					\
	gth-resources.h					\
	$(NULL)

gthumb_SOURCES = 					\
	$(ENUM_TYPES)					\
	$(MARSHALLERS)					\
	$(EXTERNAL)					\
	$(PUBLIC_HEADER_FILES)				\
	$(PRIVATE_HEADER_FILES)				\
	$(RESOURCES)					\
	cairo-scale.c					\
	cairo-utils.c					\
	dlg-location.c					\
	dlg-personalize-filters.c			\
	dlg-preferences.c				\
	dlg-preferences-extensions.c			\
	dlg-sort-order.c				\
	dom.c						\
	gio-utils.c					\
	glib-utils.c					\
	gsignature.c					\
	gth-auto-paned.c				\
	gth-async-task.c				\
	gth-browser.c					\
	gth-browser-actions-callbacks.c			\
	gth-buffer-data.c				\
	gth-color-scale.c				\
	gth-dumb-notebook.c				\
	gth-duplicable.c				\
	gth-embedded-dialog.c				\
	gth-empty-list.c				\
	gth-error.c					\
	gth-extensions.c				\
	gth-file-chooser-dialog.c			\
	gth-file-data.c					\
	gth-file-list.c					\
	gth-file-properties.c				\
	gth-file-selection.c				\
	gth-file-source.c				\
	gth-file-source-vfs.c				\
	gth-file-store.c				\
	gth-file-tool.c					\
	gth-file-view.c					\
	gth-filter.c					\
	gth-filterbar.c					\
	gth-filter-editor-dialog.c			\
	gth-filter-file.c				\
	gth-folder-tree.c				\
	gth-grid-view.c					\
	gth-histogram.c					\
	gth-histogram-view.c				\
	gth-hook.c					\
	gth-icon-cache.c				\
	gth-image.c					\
	gth-image-dragger.c				\
	gth-image-history.c				\
	gth-image-list-task.c				\
	gth-image-loader.c				\
	gth-image-navigator.c				\
	gth-image-preloader.c				\
	gth-image-saver.c				\
	gth-image-selector.c				\
	gth-image-task.c				\
	gth-image-utils.c				\
	gth-image-viewer.c				\
	gth-image-viewer-tool.c				\
	gth-info-bar.c					\
	gth-load-file-data-task.c			\
	gth-location-chooser.c				\
	gth-main.c					\
	gth-main-default-hooks.c			\
	gth-main-default-metadata.c			\
	gth-main-default-sort-types.c			\
	gth-main-default-tests.c			\
	gth-main-default-types.c			\
	gth-menu-action.c				\
	gth-menu-button.c				\
	gth-metadata.c					\
	gth-metadata-chooser.c				\
	gth-metadata-provider.c				\
	gth-metadata-provider-file.c			\
	gth-monitor.c					\
	gth-multipage.c					\
	gth-overwrite-dialog.c				\
	gth-paned.c					\
	gth-preferences.c				\
	gth-progress-dialog.c				\
	gth-request-dialog.c				\
	gth-save-file-data-task.c			\
	gth-save-image-task.c				\
	gth-screensaver.c				\
	gth-sidebar.c					\
	gth-source-tree.c				\
	gth-statusbar.c					\
	gth-string-list.c				\
	gth-tags-entry.c				\
	gth-tags-file.c					\
	gth-task.c					\
	gth-test.c					\
	gth-test-category.c				\
	gth-test-chain.c				\
	gth-test-selector.c				\
	gth-test-simple.c				\
	gth-thumb-loader.c				\
	gth-time.c					\
	gth-time-selector.c				\
	gth-toggle-menu-action.c			\
	gth-toggle-menu-tool-button.c			\
	gth-toolbox.c					\
	gth-uri-list.c					\
	gth-user-dir.c					\
	gth-viewer-page.c				\
	gth-window.c					\
	gth-window-actions-callbacks.c			\
	gtk-utils.c					\
	gvaluehash.c					\
	main.c						\
	main-migrate-catalogs.c				\
	pixbuf-cache.c					\
	pixbuf-io.c					\
	pixbuf-utils.c					\
	zlib-utils.c					\
	$(NULL)

if PLATFORM_WIN32
gthumb_LDFLAGS = -Wl,--export-all-symbols,--out-implib,.libs/libgthumb.a
endif

gthumb_LDADD =						\
	$(top_builddir)/copy-n-paste/libeggsmclient.la	\
	$(GTHUMB_LIBS)					\
	$(M_LIBS)					\
	$(EXIV2_LIBS)					\
	$(UNIQUE_LIBS)					\
	$(JPEG_LIBS)					\
	$(TIFF_LIBS)					\
	$(CLUTTER_LIBS)					\
	$(SMCLIENT_LIBS)				\
	$(LIBSECRET_LIBS)				\
	$(GSTREAMER_LIBS)				\
	$(LIBBRASERO_LIBS)				\
	$(LIBOPENRAW_LIBS)				\
	$(LIBSOUP_LIBS)					\
	$(LIBCHAMPLAIN_LIBS)				\
	$(LIBRSVG_LIBS)					\
	$(LIBWEBP_LIBS)					\
	$(JSON_GLIB_LIBS)				\
	$(WEBKIT2_LIBS)					\
	$(NULL)

if RUN_IN_PLACE
ui_dir = $(abs_top_srcdir)/data/ui
icon_dir = $(abs_top_srcdir)/data/icons
extensions_ui_dir = $(abs_top_srcdir)/extensions
extensions_dir = $(abs_top_builddir)/extensions
applications_dir = $(abs_top_builddir)/data
else
ui_dir = $(pkgdatadir)/ui
icon_dir = $(pkgdatadir)/icons
extensions_ui_dir = $(pkgdatadir)/ui
extensions_dir = $(pkglibdir)/extensions
applications_dir = $(datadir)/applications
endif

gthumb_CFLAGS =							\
	$(GTHUMB_CFLAGS)					\
	$(EXIV2_CFLAGS)						\
	$(UNIQUE_CFLAGS)					\
	$(JPEG_CFLAGS)						\
	$(TIFF_CFLAGS)						\
	$(CLUTTER_CFLAGS)					\
	$(LIBSECRET_CFLAGS)					\
	$(GSTREAMER_CFLAGS)					\
	$(LIBBRASERO_CFLAGS)					\
	$(LIBOPENRAW_CFLAGS)					\
	$(LIBSOUP_CFLAGS)					\
	$(LIBCHAMPLAIN_CFLAGS)					\
	$(SMCLIENT_CFLAGS)					\
	-I$(top_srcdir)/copy-n-paste/				\
	-DGTHUMB_LOCALEDIR=\"$(datadir)/locale\"		\
	-DGTHUMB_PREFIX=\"$(prefix)\"           		\
	-DGTHUMB_SYSCONFDIR=\"$(sysconfdir)\"   		\
	-DGTHUMB_LIBDIR=\"$(libdir)\" 				\
	-DGTHUMB_PKGDATADIR=\"$(pkgdatadir)\"			\
	-DGTHUMB_UI_DIR=\"$(ui_dir)\"				\
	-DGTHUMB_ICON_DIR=\"$(icon_dir)\"			\
	-DGTHUMB_EXTENSIONS_UI_DIR=\"$(extensions_ui_dir)\"	\
	-DGTHUMB_EXTENSIONS_DIR=\"$(extensions_dir)\"		\
	-DGTHUMB_APPLICATIONS_DIR=\"$(applications_dir)\"	\
	$(NULL)

gth-enum-types.h: $(PUBLIC_HEADER_FILES)
	$(AM_V_GEN)( $(GLIB_MKENUMS) \
		--fhead "#ifndef GTH_ENUM_TYPES_H\n#define GTH_ENUM_TYPES_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define GTH_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* GTH_ENUM_TYPES_H */" \
		$^> xgen-$(@F) \
	&& mv -f xgen-$(@F) gth-enum-types.h )

gth-enum-types.c: $(PUBLIC_HEADER_FILES)
	$(AM_V_GEN)( $(GLIB_MKENUMS) \
		--fhead "#include <glib-object.h>\n" \
		--fprod "\n/* enumerations from \"@filename@\" */\n#include \"@filename@\"" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$^> xgen-$(@F) \
	&& mv -f xgen-$(@F) gth-enum-types.c )

gth-marshal.h: gth-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)( $(GLIB_GENMARSHAL) $(srcdir)/gth-marshal.list --header --prefix=gth_marshal > $@ )

gth-marshal.c: gth-marshal.h gth-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)( echo "#include \"gth-marshal.h\"" > $@ \
	&& $(GLIB_GENMARSHAL) $(srcdir)/gth-marshal.list --body --prefix=gth_marshal >> $@ ) 

gthumb.h: make-header.sh gthumb.h.template Makefile.am
	$(AM_V_GEN)( $(srcdir)/make-header.sh $(srcdir)/gthumb.h.template  $(PUBLIC_HEADER_FILES) > xgen-$(@F) \
	&& mv -f xgen-$(@F) gthumb.h )

AUTHORS.tab : $(top_srcdir)/AUTHORS
	$(AM_V_GEN)( sed -e 's/^/"/' -e 's/$$/",/' < $(top_srcdir)/AUTHORS > $@.tmp \
	&& mv $@.tmp $@ )

RESOURCES_DEP = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir=$(srcdir) $(srcdir)/gthumb.gresource.xml)

gth-resources.c: gthumb.gresource.xml $(RESOURCES_DEP)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name gth $(srcdir)/gthumb.gresource.xml

gth-resources.h: gthumb.gresource.xml $(RESOURCES_DEP)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name gth $(srcdir)/gthumb.gresource.xml

gthumbincludedir = $(includedir)/gthumb-$(GTHUMB_API_VERSION)
gthumbinclude_HEADERS = gthumb.h

gthumbsubincludedir = $(includedir)/gthumb-$(GTHUMB_API_VERSION)/gthumb
gthumbsubinclude_HEADERS = $(PUBLIC_HEADER_FILES)

BUILT_SOURCES = 		\
	AUTHORS.tab		\
	gthumb.h		\
	$(MARSHALLERS)		\
	$(ENUM_TYPES)		\
	$(RESOURCES)		\
	$(NULL)
	
CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = 			\
	gth-marshal.list	\
	gthumb.h.template	\
	gthumb.gresource.xml	\
	make-header.sh

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

if PLATFORM_WIN32
install-exec-hook:
	$(INSTALL_DATA) .libs/libgthumb.a $(DESTDIR)$(pkglibdir)/libgthumb.a
endif

-include $(top_srcdir)/git.mk
