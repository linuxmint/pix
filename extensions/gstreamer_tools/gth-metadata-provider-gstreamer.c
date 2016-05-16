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
#include <gthumb.h>
#include <extensions/gstreamer_utils/gstreamer-utils.h>
#include "gth-metadata-provider-gstreamer.h"


G_DEFINE_TYPE (GthMetadataProviderGstreamer, gth_metadata_provider_gstreamer, GTH_TYPE_METADATA_PROVIDER)


static gboolean
gth_metadata_provider_gstreamer_can_read (GthMetadataProvider  *self,
				          const char           *mime_type,
				          char                **attribute_v)
{
	if (! g_str_equal (mime_type, "*")
	    && ! _g_content_type_is_a (mime_type, "audio/*")
	    && ! _g_content_type_is_a (mime_type, "video/*"))
	{
		return FALSE;
	}

	return _g_file_attributes_matches_any_v ("general::title,"
						 "general::format,"
						 "general::dimensions,"
						 "frame::width,"
			 	 	 	 "frame::height,"
						 "audio-video::*",
					         attribute_v);
}


static void
gth_metadata_provider_gstreamer_read (GthMetadataProvider *self,
				      GthFileData         *file_data,
				      const char          *attributes,
				      GCancellable        *cancellable)
{
	if (! g_content_type_is_a (gth_file_data_get_mime_type (file_data), "audio/*")
	    && ! g_content_type_is_a (gth_file_data_get_mime_type (file_data), "video/*"))
	{
		return;
	}

	/* this function is executed in a secondary thread, so calling
	 * slow sync functions is not a problem. */

	gstreamer_read_metadata_from_file (file_data->file, file_data->info, NULL);
}


static void
gth_metadata_provider_gstreamer_class_init (GthMetadataProviderGstreamerClass *klass)
{
	GthMetadataProviderClass *metadata_provider_class;

	metadata_provider_class = GTH_METADATA_PROVIDER_CLASS (klass);
	metadata_provider_class->can_read = gth_metadata_provider_gstreamer_can_read;
	metadata_provider_class->read = gth_metadata_provider_gstreamer_read;
}


static void
gth_metadata_provider_gstreamer_init (GthMetadataProviderGstreamer *self)
{
	/* void */
}
