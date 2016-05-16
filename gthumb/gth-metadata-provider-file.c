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
#include <glib.h>
#include "glib-utils.h"
#include "gth-metadata-provider-file.h"


G_DEFINE_TYPE (GthMetadataProviderFile, gth_metadata_provider_file, GTH_TYPE_METADATA_PROVIDER)


static gboolean
gth_metadata_provider_file_can_read (GthMetadataProvider  *self,
				     const char           *mime_type,
				     char                **attribute_v)
{
	return _g_file_attributes_matches_any_v ("gth::file::display-size,"
						 "gth::file::display-mtime,"
						 "gth::file::content-type,"
						 "gth::file::is-modified,"
						 "gth::file::full-name",
					         attribute_v);
}


static void
gth_metadata_provider_file_read (GthMetadataProvider *self,
				 GthFileData         *file_data,
				 const char          *attributes,
				 GCancellable        *cancellable)
{
	GFileAttributeMatcher *matcher;
	char                  *value;
	GTimeVal              *timeval_p;
	const char            *value_s;

	matcher = g_file_attribute_matcher_new (attributes);

	value = g_format_size (g_file_info_get_size (file_data->info));
	g_file_info_set_attribute_string (file_data->info, "gth::file::display-size", value);
	g_free (value);

	timeval_p = gth_file_data_get_modification_time (file_data);
	value = _g_time_val_strftime (timeval_p, "%x %X");
	g_file_info_set_attribute_string (file_data->info, "gth::file::display-mtime", value);
	g_free (value);

	value = g_file_get_parse_name (file_data->file);
	g_file_info_set_attribute_string (file_data->info, "gth::file::full-name", value);
	g_free (value);

	value_s = get_static_string (g_file_info_get_content_type (file_data->info));
	if (value_s != NULL)
		g_file_info_set_attribute_string (file_data->info, "gth::file::content-type", value_s);

	g_file_attribute_matcher_unref (matcher);
}


static void
gth_metadata_provider_file_class_init (GthMetadataProviderFileClass *klass)
{
	GTH_METADATA_PROVIDER_CLASS (klass)->can_read = gth_metadata_provider_file_can_read;
	GTH_METADATA_PROVIDER_CLASS (klass)->read = gth_metadata_provider_file_read;
}

static void
gth_metadata_provider_file_init (GthMetadataProviderFile *self)
{
}
