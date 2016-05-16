/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <config.h>


#ifdef ENABLE_LIBOPENRAW


#define GDK_PIXBUF_ENABLE_BACKEND
#include <gtk/gtk.h>
#include <gthumb.h>
#include <libopenraw/libopenraw.h>


static void
free_pixels (guchar   *pixels,
	     gpointer  data)
{
	free (pixels);
}


static GdkPixbuf *
_or_thumbnail_to_pixbuf (ORThumbnailRef thumbnail,
			 int32_t        orientation)
{
	GdkPixbuf    *pixbuf = NULL;
	const guchar *buf;
	size_t        buf_size;
	or_data_type  format;

	buf = (const guchar *) or_thumbnail_data (thumbnail);
	buf_size = or_thumbnail_data_size (thumbnail);
	format = or_thumbnail_format (thumbnail);
	switch (format) {
	case OR_DATA_TYPE_PIXMAP_8RGB: {
		guchar   *data;
		uint32_t  x, y;

		data = (guchar*) malloc (buf_size);
		memcpy (data, buf, buf_size);
		or_thumbnail_dimensions (thumbnail, &x, &y);
		pixbuf = gdk_pixbuf_new_from_data (data,
						   GDK_COLORSPACE_RGB,
						   FALSE,
						   8,
						   x,
						   y,
						   x * 3,
						   free_pixels,
						   NULL);
		break;
	}
	case OR_DATA_TYPE_JPEG:
	case OR_DATA_TYPE_TIFF:
	case OR_DATA_TYPE_PNG: {
		GdkPixbufLoader *loader;

		loader = gdk_pixbuf_loader_new ();
		gdk_pixbuf_loader_write (loader, buf, buf_size, NULL);
		gdk_pixbuf_loader_close (loader, NULL);
		pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
		break;
	}
	default:
		break;
	}

	return pixbuf;
}


static GdkPixbuf *
openraw_extract_thumbnail_from_file (GthFileData  *file_data,
				     int           requested_size,
				     GError      **error)
{
	GdkPixbuf    *pixbuf = NULL;
	char         *filename;
	ORRawFileRef  raw_file = NULL;

	filename = g_file_get_path (file_data->file);
	if (filename == NULL)
		return NULL;

	raw_file = or_rawfile_new (filename, OR_DATA_TYPE_NONE);
	if (raw_file != NULL) {
		int32_t        orientation;
		ORThumbnailRef thumbnail;
		or_error       err;

		orientation = or_rawfile_get_orientation (raw_file);
		thumbnail = or_thumbnail_new ();
		err = or_rawfile_get_thumbnail (raw_file, requested_size, thumbnail);
		if (err == OR_ERROR_NONE) {
			GdkPixbuf *tmp;

			tmp = _or_thumbnail_to_pixbuf (thumbnail, orientation);
			pixbuf = _gdk_pixbuf_transform (tmp, orientation);
			g_object_unref (tmp);
		}

		or_thumbnail_release (thumbnail);
		or_rawfile_release (raw_file);
	}

	g_free (filename);

	return pixbuf;
}


static void
free_bitmapdata (guchar   *pixels,
		 gpointer  data)
{
	or_bitmapdata_release ((ORBitmapDataRef) data);
}


static GdkPixbuf *
openraw_get_pixbuf_from_file (GthFileData  *file_data,
			      GError      **error)
{
	GdkPixbuf    *pixbuf = NULL;
	char         *filename;
	ORRawFileRef  raw_file = NULL;

	filename = g_file_get_path (file_data->file);
	if (filename == NULL)
		return NULL;

	raw_file = or_rawfile_new (filename, OR_DATA_TYPE_NONE);
	if (raw_file != NULL) {
		ORBitmapDataRef bitmapdata;
		or_error        err;

		bitmapdata = or_bitmapdata_new ();
		err = or_rawfile_get_rendered_image (raw_file, bitmapdata, 0);
		if (err == OR_ERROR_NONE) {
			uint32_t x, y;

			or_bitmapdata_dimensions (bitmapdata, &x, &y);
			pixbuf = gdk_pixbuf_new_from_data (or_bitmapdata_data (bitmapdata),
							   GDK_COLORSPACE_RGB,
							   FALSE,
							   8,
							   x,
							   y,
							   (x - 2) * 3,
							   free_bitmapdata,
							   bitmapdata);
		}

		or_rawfile_release (raw_file);
	}

	g_free (filename);

	return pixbuf;
}


static GthImage *
openraw_pixbuf_animation_new_from_file (GthFileData  *file_data,
					int           requested_size,
					GError      **error)
{
	GthImage  *image = NULL;
	GdkPixbuf *pixbuf;

	if (requested_size == 0)
		pixbuf = openraw_extract_thumbnail_from_file (file_data, requested_size, error);
	else
		pixbuf = openraw_get_pixbuf_from_file (file_data, error);

	if (pixbuf != NULL) {
		image = gth_image_new_for_pixbuf (pixbuf);
		g_object_unref (pixbuf);
	}

	return image;
}


#else /* ! ENABLE_LIBOPENRAW */


#define GDK_PIXBUF_ENABLE_BACKEND
#include <gtk/gtk.h>
#include <gthumb.h>


static gboolean
_g_mime_type_is_raw (const char *mime_type)
{
	return (g_content_type_is_a (mime_type, "application/x-crw")	/* ? */
		|| g_content_type_is_a (mime_type, "image/x-raw")       /* mimelnk */
		|| g_content_type_is_a (mime_type, "image/x-dcraw"));	/* freedesktop.org.xml - this should
									   catch most RAW formats, which are
									   registered as sub-classes of
									   image/x-dcraw */
}


static gboolean
_g_mime_type_is_hdr (const char *mime_type)
{
	/* Note that some HDR file extensions have been hard-coded into
	   the get_file_mime_type function above. */
	return g_content_type_is_a (mime_type, "image/x-hdr");
}


static char *
get_cache_full_path (const char *filename,
		     const char *extension)
{
	char  *name;
	GFile *file;
	char  *cache_filename;

	if (extension == NULL)
		name = g_strdup (filename);
	else
		name = g_strconcat (filename, ".", extension, NULL);
	file = gth_user_dir_get_file_for_write (GTH_DIR_CACHE, GTHUMB_DIR, name, NULL);
	cache_filename = g_file_get_path (file);

	g_object_unref (file);
	g_free (name);

	return cache_filename;
}


static time_t
get_file_mtime (const char *path)
{
	GFile  *file;
	time_t  t;

	file = g_file_new_for_path (path);
	t = _g_file_get_mtime (file);
	g_object_unref (file);

	return t;
}


static GthImage *
openraw_pixbuf_animation_new_from_file (GInputStream  *istream,
					GthFileData   *file_data,
					int            requested_size,
					int           *original_width,
					int           *original_height,
					gpointer       user_data,
					GCancellable  *cancellable,
					GError       **error)
{
	GthImage  *image = NULL;
	GdkPixbuf *pixbuf;
	gboolean   is_thumbnail;
	gboolean   is_raw;
	gboolean   is_hdr;
	char      *local_file;
	char      *local_file_md5;
	char	  *cache_file;
	char	  *cache_file_esc;
	char	  *local_file_esc;
	char	  *command = NULL;

	if (file_data == NULL) {
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME, "Could not load file");
		return NULL;
	}

	is_thumbnail = requested_size > 0;
	is_raw = _g_mime_type_is_raw (gth_file_data_get_mime_type (file_data));
	is_hdr = _g_mime_type_is_hdr (gth_file_data_get_mime_type (file_data));

	/* The output filename, and its persistence, depend on the input file
	 * type, and whether or not a thumbnail has been requested. */

	local_file = g_file_get_path (file_data->file);
	local_file_md5 = gnome_desktop_thumbnail_md5 (local_file);

	if (is_raw && !is_thumbnail)
		/* Full-sized converted RAW file */
		cache_file = get_cache_full_path (local_file_md5, "conv.pnm");
	else if (is_raw && is_thumbnail)
		/* RAW: thumbnails generated in pnm format. The converted file is later removed. */
		cache_file = get_cache_full_path (local_file_md5, "conv-thumb.pnm");
	else if (is_hdr && is_thumbnail)
		/* HDR: thumbnails generated in tiff format. The converted file is later removed. */
		cache_file = get_cache_full_path (local_file_md5, "conv-thumb.tiff");
	else
		/* Full-sized converted HDR files */
		cache_file = get_cache_full_path (local_file_md5, "conv.tiff");

	g_free (local_file_md5);

	if (cache_file == NULL) {
		g_free (local_file);
		return NULL;
	}

	local_file_esc = g_shell_quote (local_file);
	cache_file_esc = g_shell_quote (cache_file);

	/* Do nothing if an up-to-date converted file is already in the cache */
	if (! g_file_test (cache_file, G_FILE_TEST_EXISTS)
	    || (gth_file_data_get_mtime (file_data) > get_file_mtime (cache_file)))
	{
		if (is_raw) {
			if (is_thumbnail) {
				char *first_part;
				char *jpg_thumbnail;
				char *tiff_thumbnail;
				char *ppm_thumbnail;
				char *thumb_command;

				thumb_command = g_strdup_printf ("dcraw -e %s", local_file_esc);
				g_spawn_command_line_sync (thumb_command, NULL, NULL, NULL, NULL);
				g_free (thumb_command);

				first_part = _g_uri_remove_extension (local_file);
				jpg_thumbnail = g_strdup_printf ("%s.thumb.jpg", first_part);
				tiff_thumbnail = g_strdup_printf ("%s.thumb.tiff", first_part);
				ppm_thumbnail = g_strdup_printf ("%s.thumb.ppm", first_part);

				if (g_file_test (jpg_thumbnail, G_FILE_TEST_EXISTS)) {
					g_free (cache_file);
					cache_file = g_strdup (jpg_thumbnail);
				}
				else if (g_file_test (tiff_thumbnail, G_FILE_TEST_EXISTS)) {
					g_free (cache_file);
					cache_file = g_strdup (tiff_thumbnail);
				}
				else if (g_file_test (ppm_thumbnail, G_FILE_TEST_EXISTS)) {
					g_free (cache_file);
					cache_file = g_strdup (ppm_thumbnail);
				}
				else {
					/* No embedded thumbnail. Read the whole file. */
					/* Add -h option to speed up thumbnail generation. */
					command = g_strdup_printf ("dcraw -w -c -h %s > %s",
								   local_file_esc,
								   cache_file_esc);
				}

				g_free (first_part);
				g_free (jpg_thumbnail);
				g_free (tiff_thumbnail);
				g_free (ppm_thumbnail);
			}
			else {
				/* -w option = camera-specified white balance */
				command = g_strdup_printf ("dcraw -w -c %s > %s",
							   local_file_esc,
							   cache_file_esc);
			}
		}

		if (is_hdr) {
			/* HDR files. We can use the pfssize tool to speed up
			   thumbnail generation considerably, so we treat
			   thumbnailing as a special case. */
			char *resize_command;

			if (is_thumbnail)
				resize_command = g_strdup_printf (" | pfssize --maxx %d --maxy %d",
								  requested_size,
								  requested_size);
			else
				resize_command = g_strdup_printf (" ");

			command = g_strconcat ( "pfsin ",
						local_file_esc,
						resize_command,
						" |  pfsclamp  --rgb  | pfstmo_drago03 | pfsout ",
						cache_file_esc,
						NULL );
			g_free (resize_command);
		}

		if (command != NULL) {
			if (system (command) == -1) {
				g_free (command);
				g_free (cache_file_esc);
				g_free (local_file_esc);
				g_free (cache_file);
				g_free (local_file);

				return NULL;
			}
			g_free (command);
		}
	}

	pixbuf = gdk_pixbuf_new_from_file (cache_file, NULL);

	/* Thumbnail files are already cached, so delete the conversion cache copies */
	if (is_thumbnail) {
		GFile *file;

		file = g_file_new_for_path (cache_file);
		g_file_delete (file, NULL, NULL);
		g_object_unref (file);
	}

	if (pixbuf != NULL) {
		image = gth_image_new_for_pixbuf (pixbuf);
		g_object_unref (pixbuf);
	}

	g_free (cache_file_esc);
	g_free (local_file_esc);
	g_free (cache_file);
	g_free (local_file);

	return image;
}


#endif


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	gth_main_register_image_loader_func (openraw_pixbuf_animation_new_from_file,
					     GTH_IMAGE_FORMAT_GDK_PIXBUF,
					     "image/x-adobe-dng",
					     "image/x-canon-cr2",
					     "image/x-canon-crw",
					     "image/x-epson-erf",
					     "image/x-minolta-mrw",
					     "image/x-nikon-nef",
					     "image/x-olympus-orf",
					     "image/x-pentax-pef",
					     "image/x-sony-arw",
					     NULL);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
