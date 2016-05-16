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
#include <setjmp.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gthumb.h>
#if HAVE_LIBJPEG
#include <extensions/jpeg_utils/jpeg-info.h>
#endif /* HAVE_LIBJPEG */
#if HAVE_LIBWEBP
#include <webp/decode.h>
#endif /* HAVE_LIBWEBP */
#include "gth-metadata-provider-image.h"


#define BUFFER_SIZE 1024


G_DEFINE_TYPE (GthMetadataProviderImage, gth_metadata_provider_image, GTH_TYPE_METADATA_PROVIDER)


static gboolean
gth_metadata_provider_image_can_read (GthMetadataProvider  *self,
				      const char           *mime_type,
				      char                **attribute_v)
{
	return _g_file_attributes_matches_any_v ("general::format,"
			                         "general::dimensions,"
						 "image::width,"
						 "image::height,"
						 "frame::width,"
						 "frame::height",
					         attribute_v);
}


static void
gth_metadata_provider_image_read (GthMetadataProvider *self,
				  GthFileData         *file_data,
				  const char          *attributes,
				  GCancellable        *cancellable)
{
	gboolean          format_recognized;
	GFileInputStream *stream;
	char             *description;
	int               width;
	int               height;
	const char       *mime_type = NULL;

	format_recognized = FALSE;

	stream = g_file_read (file_data->file, cancellable, NULL);
	if (stream != NULL) {
		int     buffer_size;
		guchar *buffer;
		gssize  size;

		buffer_size = BUFFER_SIZE;
		buffer = g_new (guchar, buffer_size);
		size = g_input_stream_read (G_INPUT_STREAM (stream),
					    buffer,
					    buffer_size,
					    cancellable,
					    NULL);
		if (size >= 0) {
			if ((size >= 24)

			    /* PNG signature */

			    && (buffer[0] == 0x89)
			    && (buffer[1] == 0x50)
			    && (buffer[2] == 0x4E)
			    && (buffer[3] == 0x47)
			    && (buffer[4] == 0x0D)
			    && (buffer[5] == 0x0A)
			    && (buffer[6] == 0x1A)
			    && (buffer[7] == 0x0A)

			    /* IHDR Image header */

			    && (buffer[12] == 0x49)
    			    && (buffer[13] == 0x48)
    			    && (buffer[14] == 0x44)
    			    && (buffer[15] == 0x52))
			{
				/* PNG */

				width  = (buffer[16] << 24) + (buffer[17] << 16) + (buffer[18] << 8) + buffer[19];
				height = (buffer[20] << 24) + (buffer[21] << 16) + (buffer[22] << 8) + buffer[23];
				description = _("PNG");
				mime_type = "image/png";
				format_recognized = TRUE;
			}

#if HAVE_LIBJPEG
			else if ((size >= 4)
				 && (buffer[0] == 0xff)
				 && (buffer[1] == 0xd8)
				 && (buffer[2] == 0xff))
			{
				/* JPEG */

				GthTransform orientation;

				if (g_seekable_can_seek (G_SEEKABLE (stream))) {
					g_seekable_seek (G_SEEKABLE (stream), 0, G_SEEK_SET, cancellable, NULL);
				}
				else {
					g_object_unref (stream);
					stream = g_file_read (file_data->file, cancellable, NULL);
				}

				if (_jpeg_get_image_info (G_INPUT_STREAM (stream),
							  &width,
							  &height,
							  &orientation,
							  cancellable,
							  NULL))
				{
					description = _("JPEG");
					mime_type = "image/jpeg";
					format_recognized = TRUE;

					if ((orientation == GTH_TRANSFORM_ROTATE_90)
					     ||	(orientation == GTH_TRANSFORM_ROTATE_270)
					     ||	(orientation == GTH_TRANSFORM_TRANSPOSE)
					     ||	(orientation == GTH_TRANSFORM_TRANSVERSE))
					{
						int tmp = width;
						width = height;
						height = tmp;
					}
				}
			}
#endif /* HAVE_LIBJPEG */

#if HAVE_LIBWEBP
			else if ((size > 15) && (memcmp (buffer + 8, "WEBPVP8", 7) == 0)) {
				WebPDecoderConfig config;

				if (WebPInitDecoderConfig (&config)) {
					if (WebPGetFeatures (buffer, buffer_size, &config.input) == VP8_STATUS_OK) {
						width = config.input.width;
						height = config.input.height;
						description = _("WebP");
						mime_type = "image/webp";
						format_recognized = TRUE;
					}
					WebPFreeDecBuffer (&config.output);
				}
			}
#endif /* HAVE_LIBWEBP */

			else if ((size >= 26)
				 && (strncmp ((char *) buffer, "gimp xcf ", 9) == 0))
			{
				/* XCF */

				GInputStream      *mem_stream;
				GDataInputStream  *data_stream;

				mem_stream = g_memory_input_stream_new_from_data (buffer, BUFFER_SIZE, NULL);
				data_stream = g_data_input_stream_new (mem_stream);
				g_data_input_stream_set_byte_order (data_stream, G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN);

				if (g_seekable_seek (G_SEEKABLE (data_stream), 14, G_SEEK_SET, NULL, NULL)) {
					int base_type;

					width  = g_data_input_stream_read_uint32 (data_stream, NULL, NULL);
					height = g_data_input_stream_read_uint32 (data_stream, NULL, NULL);
					base_type = g_data_input_stream_read_uint32 (data_stream, NULL, NULL);
					if (base_type == 0)
						description = "XCF RGB";
					else if (base_type == 1)
						description = "XCF grayscale";
					else if (base_type == 2)
						description = "XCF indexed";
					else
						description = "XCF";
					mime_type = "image/x-xcf";
					format_recognized = TRUE;
				}

				g_object_unref (data_stream);
				g_object_unref (mem_stream);
			}
		}

		g_free (buffer);
		g_object_unref (stream);
	}

	if (! format_recognized) { /* use gdk_pixbuf_get_file_info */
		char *filename;

		filename = g_file_get_path (file_data->file);
		if (filename != NULL) {
			GdkPixbufFormat  *format;

			format = gdk_pixbuf_get_file_info (filename, &width, &height);
			if (format != NULL) {
				format_recognized = TRUE;
				description = gdk_pixbuf_format_get_description (format);
			}

			g_free (filename);
		}
	}

	if (format_recognized) {
		char *size;

		g_file_info_set_attribute_string (file_data->info, "general::format", description);

		g_file_info_set_attribute_int32 (file_data->info, "image::width", width);
		g_file_info_set_attribute_int32 (file_data->info, "image::height", height);
		g_file_info_set_attribute_int32 (file_data->info, "frame::width", width);
		g_file_info_set_attribute_int32 (file_data->info, "frame::height", height);

		if (mime_type != NULL)
			gth_file_data_set_mime_type (file_data, mime_type);

		size = g_strdup_printf (_("%d × %d"), width, height);
		g_file_info_set_attribute_string (file_data->info, "general::dimensions", size);

		g_free (size);
	}
}


static void
gth_metadata_provider_image_class_init (GthMetadataProviderImageClass *klass)
{
	GthMetadataProviderClass *metadata_provider_class;

	metadata_provider_class = GTH_METADATA_PROVIDER_CLASS (klass);
	metadata_provider_class->can_read = gth_metadata_provider_image_can_read;
	metadata_provider_class->read = gth_metadata_provider_image_read;
}


static void
gth_metadata_provider_image_init (GthMetadataProviderImage *self)
{
	/* void */
}
