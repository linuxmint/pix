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
#if HAVE_LIBJXL
#include <jxl/decode.h>
#endif /* HAVE_LIBJXL */
#include "gth-metadata-provider-image.h"


#define BUFFER_SIZE 1024


G_DEFINE_TYPE (GthMetadataProviderImage, gth_metadata_provider_image, GTH_TYPE_METADATA_PROVIDER)


static gboolean
gth_metadata_provider_image_can_read (GthMetadataProvider  *self,
				      GthFileData          *file_data,
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
	char             *description = NULL;
	gboolean          free_description = FALSE;
	int               width;
	int               height;
	const char       *mime_type = NULL;

	format_recognized = FALSE;

	stream = g_file_read (file_data->file, cancellable, NULL);
	if (stream != NULL) {
		int     buffer_size;
		guchar *buffer;
		gsize   size;

		buffer_size = BUFFER_SIZE;
		buffer = g_new (guchar, buffer_size);
		if (! g_input_stream_read_all (G_INPUT_STREAM (stream),
					       buffer,
					       buffer_size,
					       &size,
					       cancellable,
					       NULL))
		{
			size = 0;
		}

		if (size > 0) {
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

				JpegInfoData jpeg_info;

				_jpeg_info_data_init (&jpeg_info);

				if (g_seekable_can_seek (G_SEEKABLE (stream))) {
					g_seekable_seek (G_SEEKABLE (stream), 0, G_SEEK_SET, cancellable, NULL);
				}
				else {
					g_object_unref (stream);
					stream = g_file_read (file_data->file, cancellable, NULL);
				}

				_jpeg_info_get_from_stream (G_INPUT_STREAM (stream),
							    _JPEG_INFO_IMAGE_SIZE | _JPEG_INFO_EXIF_ORIENTATION,
							    &jpeg_info,
							    cancellable,
							    NULL);

				if (jpeg_info.valid & _JPEG_INFO_IMAGE_SIZE) {
					width = jpeg_info.width;
					height = jpeg_info.height;
					description = _("JPEG");
					mime_type = "image/jpeg";
					format_recognized = TRUE;

					if (jpeg_info.valid & _JPEG_INFO_EXIF_ORIENTATION) {
						if ((jpeg_info.orientation == GTH_TRANSFORM_ROTATE_90)
						     ||	(jpeg_info.orientation == GTH_TRANSFORM_ROTATE_270)
						     ||	(jpeg_info.orientation == GTH_TRANSFORM_TRANSPOSE)
						     ||	(jpeg_info.orientation == GTH_TRANSFORM_TRANSVERSE))
						{
							int tmp = width;
							width = height;
							height = tmp;
						}
					}
				}

				_jpeg_info_data_dispose (&jpeg_info);
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

#if HAVE_LIBJXL
			else if (size >= 12) {
				JxlSignature sig = JxlSignatureCheck(buffer, size);
				if (sig > JXL_SIG_INVALID) {
					JxlDecoder *dec = JxlDecoderCreate(NULL);
					if (dec != NULL) {
						if (JxlDecoderSubscribeEvents(dec, JXL_DEC_BASIC_INFO) == JXL_DEC_SUCCESS) {
							if (JxlDecoderSetInput(dec, buffer, size) == JXL_DEC_SUCCESS) {
								JxlDecoderStatus status = JXL_DEC_NEED_MORE_INPUT;
								while (status != JXL_DEC_SUCCESS) {
									status = JxlDecoderProcessInput(dec);
									if (status == JXL_DEC_ERROR)
										break;
									if (status == JXL_DEC_NEED_MORE_INPUT)
										break;
									if (status == JXL_DEC_BASIC_INFO) {
										JxlBasicInfo info;
										if (JxlDecoderGetBasicInfo(dec, &info) == JXL_DEC_SUCCESS) {
											width = info.xsize;
											height = info.ysize;
											if (sig == JXL_SIG_CONTAINER)
												description = _("JPEG XL container");
											else
												description = _("JPEG XL");
											mime_type = "image/jxl";
											format_recognized = TRUE;
										}
										break;
									}
								}
							}
						}
						JxlDecoderDestroy(dec);
					}
				}
			}
#endif /* HAVE_LIBJXL */

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
				free_description = TRUE;
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

	if (free_description)
		g_free (description);
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
