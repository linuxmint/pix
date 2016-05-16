AFTER_EXIV2 =			\
	comments		\
	$(NULL)
	
FLICKR_EXPORTERS = 		\
	23hq			\
	flicker			\
	$(NULL)	
	
EXPORTERS = 			\
	export_tools		\
	burn_disc		\
	contact_sheet		\
	facebook		\
	flicker_utils		\
	$(FLICKR_EXPORTERS)	\
	photobucket		\
	picasaweb		\
	webalbums		\
	$(NULL)
	
IMPORTERS = 			\
	photo_importer		\
	$(NULL)

JPEG_TOOLS = 			\
	cairo_io		\
	image_rotation		\
	image_viewer		\
	importer		\
	$(NULL)

OTHER_TOOLS = 			\
	desktop_background	\
	file_tools		\
	find_duplicates		\
	red_eye_removal		\
	$(NULL)

SUBDIRS = 			\
	bookmarks		\
	catalogs		\
	change_date		\
	convert_format		\
	edit_metadata		\
	exiv2_tools		\
	$(AFTER_EXIV2)		\
	file_manager		\
	file_viewer		\
	gstreamer_utils		\
	gstreamer_tools		\
	image_print		\
	jpeg_utils		\
	$(JPEG_TOOLS)		\
	list_tools		\
	map_view		\
	oauth			\
	raw_files		\
	rename_series 		\
	resize_images		\
	search			\
	selections		\
	slideshow		\
	$(EXPORTERS)		\
	$(IMPORTERS)		\
	$(OTHER_TOOLS)		\
	$(NULL)

EXTRA_DIST = example

-include $(top_srcdir)/git.mk
