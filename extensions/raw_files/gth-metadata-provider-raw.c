/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include <libraw.h>
#include "gth-metadata-provider-raw.h"
#include "main.h"


G_DEFINE_TYPE (GthMetadataProviderRaw, gth_metadata_provider_raw, GTH_TYPE_METADATA_PROVIDER)


static gboolean
gth_metadata_provider_raw_can_read (GthMetadataProvider  *self,
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
gth_metadata_provider_raw_read (GthMetadataProvider *self,
				GthFileData         *file_data,
				const char          *attributes,
				GCancellable        *cancellable)
{
	libraw_data_t *raw_data;
	GInputStream  *istream = NULL;
	int            result;
	void          *buffer = NULL;
	size_t         buffer_size;
	char          *size;
	guint          width, height;

	if (!_g_mime_type_is_raw (gth_file_data_get_mime_type (file_data)))
		return;

	raw_data = libraw_init (LIBRAW_OPIONS_NO_MEMERR_CALLBACK | LIBRAW_OPIONS_NO_DATAERR_CALLBACK);
	if (raw_data == NULL)
		goto fatal_error;

	istream = (GInputStream *) g_file_read (file_data->file, cancellable, NULL);
	if (istream == NULL)
		goto fatal_error;

	if (! _g_input_stream_read_all (istream, &buffer, &buffer_size, cancellable, NULL))
		goto fatal_error;

	result = libraw_open_buffer (raw_data, buffer, buffer_size);
	if (LIBRAW_FATAL_ERROR (result))
		goto fatal_error;

	result = libraw_unpack (raw_data);
	if (result != LIBRAW_SUCCESS)
		goto fatal_error;

	result = libraw_adjust_sizes_info_only (raw_data);
	if (result != LIBRAW_SUCCESS)
		goto fatal_error;

	width = raw_data->sizes.iwidth;
	height = raw_data->sizes.iheight;

	g_file_info_set_attribute_string (file_data->info, "general::format", _("RAW Format"));
	g_file_info_set_attribute_int32 (file_data->info, "image::width", width);
	g_file_info_set_attribute_int32 (file_data->info, "image::height", height);
	g_file_info_set_attribute_int32 (file_data->info, "frame::width", width);
	g_file_info_set_attribute_int32 (file_data->info, "frame::height", height);

	size = g_strdup_printf (_("%d × %d"), width, height);
	g_file_info_set_attribute_string (file_data->info, "general::dimensions", size);
	g_free (size);

	fatal_error:

	if (raw_data != NULL)
		libraw_close (raw_data);
	g_free (buffer);
	_g_object_unref (istream);
}


static void
gth_metadata_provider_raw_class_init (GthMetadataProviderRawClass *klass)
{
	GthMetadataProviderClass *metadata_provider_class;

	metadata_provider_class = GTH_METADATA_PROVIDER_CLASS (klass);
	metadata_provider_class->can_read = gth_metadata_provider_raw_can_read;
	metadata_provider_class->read = gth_metadata_provider_raw_read;
}


static void
gth_metadata_provider_raw_init (GthMetadataProviderRaw *self)
{
	/* void */
}
