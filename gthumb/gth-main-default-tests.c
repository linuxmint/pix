/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include "glib-utils.h"
#include "gth-main.h"
#include "gth-test-aspect-ratio.h"
#include "gth-test-category.h"
#include "gth-test-simple.h"
#include "gth-time.h"


static int
is_file_test (GthTest        *test,
	      GthFileData    *file_data,
	      gconstpointer  *data,
	      GDestroyNotify *data_destroy_func)
{
	return TRUE;
}


static int
is_image_test (GthTest        *test,
	       GthFileData    *file_data,
	       gconstpointer  *data,
	       GDestroyNotify *data_destroy_func)
{
	gboolean result = FALSE;

	if (file_data->info != NULL)
		result = _g_mime_type_is_image (gth_file_data_get_mime_type (file_data));

	return result;
}


static int
is_jpeg_test (GthTest        *test,
	      GthFileData    *file_data,
	      gconstpointer  *data,
	      GDestroyNotify *data_destroy_func)
{
	gboolean result = FALSE;

	if (file_data->info != NULL)
		result = g_content_type_equals (gth_file_data_get_mime_type (file_data), "image/jpeg");

	return result;
}


static int
is_raw_test (GthTest        *test,
	     GthFileData    *file_data,
	     gconstpointer  *data,
	     GDestroyNotify *data_destroy_func)
{
	gboolean result = FALSE;

	if (file_data->info != NULL)
		result = _g_mime_type_is_raw (gth_file_data_get_mime_type (file_data));

	return result;
}


static int
is_video_test (GthTest        *test,
	       GthFileData    *file_data,
	       gconstpointer  *data,
	       GDestroyNotify *data_destroy_func)
{
	gboolean result = FALSE;

	if (file_data->info != NULL)
		result = _g_mime_type_is_video (gth_file_data_get_mime_type (file_data));

	return result;
}


static int
is_audio_test (GthTest        *test,
	       GthFileData    *file_data,
	       gconstpointer  *data,
	       GDestroyNotify *data_destroy_func)
{
	gboolean result = FALSE;

	if (file_data->info != NULL)
		result = _g_mime_type_is_audio (gth_file_data_get_mime_type (file_data));

	return result;
}


static int
is_media_test (GthTest        *test,
	       GthFileData    *file_data,
	       gconstpointer  *data,
	       GDestroyNotify *data_destroy_func)
{
	gboolean result = FALSE;

	if (file_data->info != NULL) {
		const char *content_type = gth_file_data_get_mime_type (file_data);
		result = (_g_mime_type_is_image (content_type)
			  || _g_mime_type_is_video (content_type)
			  || _g_mime_type_is_audio (content_type));
	}

	return result;
}


static int
is_text_test (GthTest        *test,
	      GthFileData    *file_data,
	      gconstpointer  *data,
	      GDestroyNotify *data_destroy_func)
{
	gboolean result = FALSE;

	if (file_data->info != NULL) {
		const char *content_type = gth_file_data_get_mime_type (file_data);
		result = g_content_type_is_a (content_type, "text/*");
	}

	return result;
}


static int
get_filename_for_test (GthTest        *test,
		       GthFileData    *file_data,
		       gconstpointer  *data,
		       GDestroyNotify *data_destroy_func)
{
	*data = g_file_info_get_display_name (file_data->info);
	return 0;
}


static int
get_filesize_for_test (GthTest        *test,
		       GthFileData    *file_data,
		       gconstpointer  *data,
		       GDestroyNotify *data_destroy_func)
{
	guint64 *size = (guint64 *) data;
	*size = g_file_info_get_size (file_data->info);
	return 0;
}


static int
get_modified_date_for_test (GthTest        *test,
			    GthFileData    *file_data,
			    gconstpointer  *data,
			    GDestroyNotify *data_destroy_func)
{
	GTimeVal     timeval;
	GthDateTime *datetime;

	datetime = gth_datetime_new ();
	g_file_info_get_modification_time (file_data->info, &timeval);
	gth_datetime_from_timeval (datetime, &timeval);

	*data = g_date_new ();
	*((GDate *)*data) = *datetime->date;
	*data_destroy_func = (GDestroyNotify) g_date_free;

	gth_datetime_free (datetime);

	return 0;
}


static int
get_original_date_for_test (GthTest        *test,
			    GthFileData    *file_data,
			    gconstpointer  *data,
			    GDestroyNotify *data_destroy_func)
{
	GthDateTime *datetime;
	GthMetadata *metadata;

	datetime = gth_datetime_new ();
	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Embedded::Photo::DateTimeOriginal");
	if (metadata != NULL)
		gth_datetime_from_exif_date (datetime, gth_metadata_get_raw (metadata));

	*data = g_date_new ();
	*((GDate *)*data) = *datetime->date;
	*data_destroy_func = (GDestroyNotify) g_date_free;

	gth_datetime_free (datetime);

	return 0;
}


static int
get_embedded_title_for_test (GthTest        *test,
			     GthFileData    *file,
			     gconstpointer  *data,
			     GDestroyNotify *data_destroy_func)
{
	GthMetadata *metadata;

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file->info, "general::title");
	if (metadata != NULL)
		*data = gth_metadata_get_formatted (metadata);
	else
		*data = NULL;

	return 0;
}


static int
get_embedded_description_for_test (GthTest        *test,
				   GthFileData    *file,
				   gconstpointer  *data,
				   GDestroyNotify *data_destroy_func)
{
	GthMetadata *metadata;

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file->info, "general::description");
	if (metadata != NULL)
		*data = gth_metadata_get_formatted (metadata);
	else
		*data = NULL;

	return 0;
}


static int
get_embedded_rating_for_test (GthTest        *test,
			      GthFileData    *file,
			      gconstpointer  *data,
			      GDestroyNotify *data_destroy_func)
{
	GthMetadata *metadata;

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file->info, "general::rating");
	if (metadata != NULL) {
		int rating;

		sscanf (gth_metadata_get_raw (metadata), "%d", &rating);
		return rating;
	}

	return 0;
}


void
gth_main_register_default_tests (void)
{
	gth_main_register_object (GTH_TYPE_TEST,
				  "file::type::is_file",
				  GTH_TYPE_TEST_SIMPLE,
				  "display-name", _("All Files"),
				  "data-type", GTH_TEST_DATA_TYPE_NONE,
				  "get-data-func", is_file_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "file::type::is_image",
				  GTH_TYPE_TEST_SIMPLE,
				  "display-name", _("All Images"),
				  "data-type", GTH_TEST_DATA_TYPE_NONE,
				  "get-data-func", is_image_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "file::type::is_jpeg",
				  GTH_TYPE_TEST_SIMPLE,
				  "display-name", _("JPEG Images"),
				  "data-type", GTH_TEST_DATA_TYPE_NONE,
				  "get-data-func", is_jpeg_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "file::type::is_raw",
				  GTH_TYPE_TEST_SIMPLE,
				  "display-name", _("Raw Photos"),
				  "data-type", GTH_TEST_DATA_TYPE_NONE,
				  "get-data-func", is_raw_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "file::type::is_video",
				  GTH_TYPE_TEST_SIMPLE,
				  "display-name", _("Video"),
				  "data-type", GTH_TEST_DATA_TYPE_NONE,
				  "get-data-func", is_video_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "file::type::is_audio",
				  GTH_TYPE_TEST_SIMPLE,
				  "display-name", _("Audio"),
				  "data-type", GTH_TEST_DATA_TYPE_NONE,
				  "get-data-func", is_audio_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "file::type::is_media",
				  GTH_TYPE_TEST_SIMPLE,
				  "display-name", _("Media"),
				  "data-type", GTH_TEST_DATA_TYPE_NONE,
				  "get-data-func", is_media_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "file::type::is_text",
				  GTH_TYPE_TEST_SIMPLE,
				  "display-name", _("Text Files"),
				  "data-type", GTH_TEST_DATA_TYPE_NONE,
				  "get-data-func", is_text_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "file::name",
				  GTH_TYPE_TEST_SIMPLE,
				  "attributes", "standard::display-name",
				  "display-name", _("Filename"),
				  "data-type", GTH_TEST_DATA_TYPE_STRING,
				  "get-data-func", get_filename_for_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "file::size",
				  GTH_TYPE_TEST_SIMPLE,
				  "attributes", "gth::file::size",
				  "display-name", _("Size"),
				  "data-type", GTH_TEST_DATA_TYPE_SIZE,
				  "get-data-func", get_filesize_for_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "file::mtime",
				  GTH_TYPE_TEST_SIMPLE,
				  "attributes", "time::modified,time::modified-usec",
				  "display-name", _("File modified date"),
				  "data-type", GTH_TEST_DATA_TYPE_DATE,
				  "get-data-func", get_modified_date_for_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "Embedded::Photo::DateTimeOriginal",
				  GTH_TYPE_TEST_SIMPLE,
				  "attributes", "Embedded::Photo::DateTimeOriginal",
				  "display-name", _("Date photo was taken"),
				  "data-type", GTH_TEST_DATA_TYPE_DATE,
				  "get-data-func", get_original_date_for_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "general::title",
				  GTH_TYPE_TEST_SIMPLE,
				  "attributes", "general::title",
				  "display-name", _("Title (embedded)"),
				  "data-type", GTH_TEST_DATA_TYPE_STRING,
				  "get-data-func", get_embedded_title_for_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "general::description",
				  GTH_TYPE_TEST_SIMPLE,
				  "attributes", "general::description",
				  "display-name", _("Description (embedded)"),
				  "data-type", GTH_TEST_DATA_TYPE_STRING,
				  "get-data-func", get_embedded_description_for_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "general::rating",
				  GTH_TYPE_TEST_SIMPLE,
				  "attributes", "general::rating",
				  "display-name", _("Rating"),
				  "data-type", GTH_TEST_DATA_TYPE_INT,
				  "get-data-func", get_embedded_rating_for_test,
				  "max-int", 5,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "general::tags",
				  GTH_TYPE_TEST_CATEGORY,
				  "attributes", "general::tags",
				  "display-name", _("Tag (embedded)"),
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "frame::aspect-ratio",
				  GTH_TYPE_TEST_ASPECT_RATIO,
				  "attributes", "frame::width, frame::height",
				  "display-name", _("Aspect ratio"),
				  NULL);
}
