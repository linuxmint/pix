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
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gthumb.h>
#include "exiv2-utils.h"
#include "gth-metadata-provider-exiv2.h"


struct _GthMetadataProviderExiv2Private {
	GSettings *general_settings;
};


G_DEFINE_TYPE (GthMetadataProviderExiv2, gth_metadata_provider_exiv2, GTH_TYPE_METADATA_PROVIDER)


static void
gth_metadata_provider_exiv2_finalize (GObject *object)
{
	GthMetadataProviderExiv2 *self;

	self = GTH_METADATA_PROVIDER_EXIV2 (object);

	_g_object_unref (self->priv->general_settings);

	G_OBJECT_CLASS (gth_metadata_provider_exiv2_parent_class)->finalize (object);
}


static gboolean
gth_metadata_provider_exiv2_can_read (GthMetadataProvider  *self,
				      const char           *mime_type,
				      char                **attribute_v)
{
	if (! g_str_equal (mime_type, "*") && ! _g_content_type_is_a (mime_type, "image/*"))
		return FALSE;

	return _g_file_attributes_matches_any_v ("Exif::*,"
						 "Xmp::*,"
						 "Iptc::*,"
						 "Embedded::Image::*,"
						 "Embedded::Photo::*,"
						 "general::datetime,"
						 "general::title,"
						 "general::description,"
						 "general::location,"
						 "general::tags",
					         attribute_v);
}


static gboolean
gth_metadata_provider_exiv2_can_write (GthMetadataProvider  *self,
				       const char           *mime_type,
				       char                **attribute_v)
{
	if (! exiv2_supports_writes (mime_type))
		return FALSE;

	return _g_file_attributes_matches_any_v ("Exif::*,"
						 "Xmp::*,"
						 "Iptc::*,"
						 "Embedded::Image::*,"
						 "Embedded::Photo::*,"
						 "general::datetime,"
						 "general::title,"
						 "general::description,"
						 "general::location,"
						 "general::tags",
					         attribute_v);
}


static void
gth_metadata_provider_exiv2_read (GthMetadataProvider *base,
				  GthFileData         *file_data,
				  const char          *attributes,
				  GCancellable        *cancellable)
{
	GthMetadataProviderExiv2 *self = GTH_METADATA_PROVIDER_EXIV2 (base);
	GFile                    *sidecar;
	GthFileData              *sidecar_file_data;
	gboolean                  update_general_attributes;

	if (! g_content_type_is_a (gth_file_data_get_mime_type (file_data), "image/*"))
		return;

	/* The embedded metadata is likely to be outdated if the user chooses to
	 * not store metadata in files. */

	if (self->priv->general_settings == NULL)
		self->priv->general_settings = g_settings_new (GTHUMB_GENERAL_SCHEMA);
	update_general_attributes = g_settings_get_boolean (self->priv->general_settings, PREF_GENERAL_STORE_METADATA_IN_FILES);

	/* this function is executed in a secondary thread, so calling
	 * slow sync functions is not a problem. */

	exiv2_read_metadata_from_file (file_data->file,
				       file_data->info,
				       update_general_attributes,
				       cancellable,
				       NULL);

	/* sidecar data */

	sidecar = exiv2_get_sidecar (file_data->file);
	sidecar_file_data = gth_file_data_new (sidecar, NULL);
	if (g_file_query_exists (sidecar_file_data->file, cancellable)) {
		gth_file_data_update_info (sidecar_file_data, "time::*");
		if (g_file_query_exists (sidecar_file_data->file, cancellable))
			exiv2_read_sidecar (sidecar_file_data->file,
					    file_data->info,
					    update_general_attributes);
	}

	g_object_unref (sidecar_file_data);
	g_object_unref (sidecar);
}


static void
gth_metadata_provider_exiv2_write (GthMetadataProvider   *base,
				   GthMetadataWriteFlags  flags,
				   GthFileData           *file_data,
				   const char            *attributes,
				   GCancellable          *cancellable)
{
	GthMetadataProviderExiv2 *self = GTH_METADATA_PROVIDER_EXIV2 (base);
	void                     *buffer = NULL;
	gsize                     size;
	GError                   *error = NULL;
	GObject                  *metadata;
	int                       i;

	if (self->priv->general_settings == NULL)
		self->priv->general_settings = g_settings_new (GTHUMB_GENERAL_SCHEMA);

	if (! (flags & GTH_METADATA_WRITE_FORCE_EMBEDDED)
	    && ! g_settings_get_boolean (self->priv->general_settings, PREF_GENERAL_STORE_METADATA_IN_FILES))
		return;

	if (! exiv2_supports_writes (gth_file_data_get_mime_type (file_data)))
		return;

	if (! _g_file_load_in_buffer (file_data->file, &buffer, &size, cancellable, &error))
		return;

	metadata = g_file_info_get_attribute_object (file_data->info, "general::description");
	if (metadata != NULL) {
		const char *tags_to_remove[] = {
			"Exif::Image::ImageDescription",
			"Xmp::tiff::ImageDescription",
			"Iptc::Application2::Headline",
			NULL
		};
		const char *tags_to_update[] = {
			"Exif::Photo::UserComment",
			"Xmp::dc::description",
			"Iptc::Application2::Caption",
			NULL
		};

		for (i = 0; tags_to_remove[i] != NULL; i++)
			g_file_info_remove_attribute (file_data->info, tags_to_remove[i]);

		/* Remove the value type to use the default type for each field
		 * as described in exiv2_tools/main.c */

		g_object_set (metadata, "value-type", NULL, NULL);

		for (i = 0; tags_to_update[i] != NULL; i++) {
			GObject *orig_metadata;

			orig_metadata = g_file_info_get_attribute_object (file_data->info, tags_to_update[i]);
			if (orig_metadata != NULL) {
				/* keep the original value type */

				g_object_set (orig_metadata,
					      "raw", gth_metadata_get_raw (GTH_METADATA (metadata)),
					      "formatted", gth_metadata_get_formatted (GTH_METADATA (metadata)),
					      NULL);
			}
			else
				g_file_info_set_attribute_object (file_data->info, tags_to_update[i], metadata);
		}
	}
	else {
		for (i = 0; _DESCRIPTION_TAG_NAMES[i] != NULL; i++)
			g_file_info_remove_attribute (file_data->info, _DESCRIPTION_TAG_NAMES[i]);
	}

	metadata = g_file_info_get_attribute_object (file_data->info, "general::title");
	if (metadata != NULL) {
		g_object_set (metadata, "value-type", NULL, NULL);
		for (i = 0; _TITLE_TAG_NAMES[i] != NULL; i++)
			g_file_info_set_attribute_object (file_data->info, _TITLE_TAG_NAMES[i], metadata);
	}
	else {
		for (i = 0; _TITLE_TAG_NAMES[i] != NULL; i++)
			g_file_info_remove_attribute (file_data->info, _TITLE_TAG_NAMES[i]);
	}

	metadata = g_file_info_get_attribute_object (file_data->info, "general::location");
	if (metadata != NULL) {
		g_object_set (metadata, "value-type", NULL, NULL);
		for (i = 0; _LOCATION_TAG_NAMES[i] != NULL; i++)
			g_file_info_set_attribute_object (file_data->info, _LOCATION_TAG_NAMES[i], metadata);
	}
	else {
		for (i = 0; _LOCATION_TAG_NAMES[i] != NULL; i++)
			g_file_info_remove_attribute (file_data->info, _LOCATION_TAG_NAMES[i]);
	}

	metadata = g_file_info_get_attribute_object (file_data->info, "general::tags");
	if (metadata != NULL) {
		if (GTH_IS_METADATA (metadata))
			g_object_set (metadata, "value-type", NULL, NULL);
		for (i = 0; _KEYWORDS_TAG_NAMES[i] != NULL; i++)
			g_file_info_set_attribute_object (file_data->info, _KEYWORDS_TAG_NAMES[i], metadata);
	}
	else {
		for (i = 0; _KEYWORDS_TAG_NAMES[i] != NULL; i++)
			g_file_info_remove_attribute (file_data->info, _KEYWORDS_TAG_NAMES[i]);
	}

	metadata = g_file_info_get_attribute_object (file_data->info, "general::rating");
	if (metadata != NULL) {
		if (GTH_IS_METADATA (metadata))
			g_object_set (metadata, "value-type", NULL, NULL);
		for (i = 0; _RATING_TAG_NAMES[i] != NULL; i++)
			g_file_info_set_attribute_object (file_data->info, _RATING_TAG_NAMES[i], metadata);
	}
	else {
		for (i = 0; _RATING_TAG_NAMES[i] != NULL; i++)
			g_file_info_remove_attribute (file_data->info, _RATING_TAG_NAMES[i]);
	}

	metadata = g_file_info_get_attribute_object (file_data->info, "general::datetime");
	if (metadata != NULL) {
		GthMetadata *xmp_metadata = NULL;
		GTimeVal     timeval;

		if (_g_time_val_from_exif_date (gth_metadata_get_raw (GTH_METADATA (metadata)), &timeval)) {
			char *xmp_format;

			xmp_metadata = gth_metadata_new ();
			xmp_format = _g_time_val_to_xmp_date (&timeval);
			g_object_set (xmp_metadata,
				      "raw", xmp_format,
				      "formatted", gth_metadata_get_formatted (GTH_METADATA (metadata)),
				      "value-type", NULL, /* use the default type as described in extensions/exiv2_tools/main.c */
				      NULL);

			g_free (xmp_format);
		}

		for (i = 0; _ORIGINAL_DATE_TAG_NAMES[i] != NULL; i++) {
			if (g_str_has_prefix (_ORIGINAL_DATE_TAG_NAMES[i], "Xmp::")) {
				if (xmp_metadata != NULL)
					g_file_info_set_attribute_object (file_data->info, _ORIGINAL_DATE_TAG_NAMES[i], G_OBJECT (xmp_metadata));
			}
			else
				g_file_info_set_attribute_object (file_data->info, _ORIGINAL_DATE_TAG_NAMES[i], metadata);
		}

		_g_object_unref (xmp_metadata);
	}
	else {
		for (i = 0; _ORIGINAL_DATE_TAG_NAMES[i] != NULL; i++)
			g_file_info_remove_attribute (file_data->info, _ORIGINAL_DATE_TAG_NAMES[i]);
	}

	if (exiv2_write_metadata_to_buffer (&buffer,
					    &size,
					    file_data->info,
					    NULL,
					    &error))
	{
		GFileInfo *tmp_info;

		_g_file_write (file_data->file,
			       FALSE,
			       G_FILE_CREATE_NONE,
			       buffer,
			       size,
			       cancellable,
			       &error);

		tmp_info = g_file_info_new ();
		g_file_info_set_attribute_uint64 (tmp_info,
						  G_FILE_ATTRIBUTE_TIME_MODIFIED,
						  g_file_info_get_attribute_uint64 (file_data->info, G_FILE_ATTRIBUTE_TIME_MODIFIED));
		g_file_info_set_attribute_uint32 (tmp_info,
						  G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
						  g_file_info_get_attribute_uint32 (file_data->info, G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC));
		g_file_set_attributes_from_info (file_data->file,
						 tmp_info,
						 G_FILE_QUERY_INFO_NONE,
						 NULL,
						 NULL);

		g_object_unref (tmp_info);
	}

	if (buffer != NULL)
		g_free (buffer);
	g_clear_error (&error);
}


static void
gth_metadata_provider_exiv2_class_init (GthMetadataProviderExiv2Class *klass)
{
	GObjectClass             *object_class;
	GthMetadataProviderClass *mp_class;

	g_type_class_add_private (klass, sizeof (GthMetadataProviderExiv2Private));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_metadata_provider_exiv2_finalize;

	mp_class = GTH_METADATA_PROVIDER_CLASS (klass);
	mp_class->can_read = gth_metadata_provider_exiv2_can_read;
	mp_class->can_write = gth_metadata_provider_exiv2_can_write;
	mp_class->read = gth_metadata_provider_exiv2_read;
	mp_class->write = gth_metadata_provider_exiv2_write;
}


static void
gth_metadata_provider_exiv2_init (GthMetadataProviderExiv2 *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_METADATA_PROVIDER_EXIV2, GthMetadataProviderExiv2Private);
	self->priv->general_settings = NULL;
}
