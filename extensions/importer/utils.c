/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "preferences.h"
#include "utils.h"


GFile *
gth_import_preferences_get_destination (void)
{
	GSettings *settings;
	char      *last_destination;
	GFile     *folder;

	settings = g_settings_new (GTHUMB_IMPORTER_SCHEMA);
	last_destination = g_settings_get_string (settings, PREF_IMPORTER_DESTINATION);
	if ((last_destination == NULL) || (*last_destination == 0))
		folder = g_file_new_for_path (g_get_user_special_dir (G_USER_DIRECTORY_PICTURES));
	else
		folder = g_file_new_for_uri (last_destination);

	g_free (last_destination);
	g_object_unref (settings);

	return folder;
}


typedef struct {
	GthFileData *file_data;
	const char  *event_name;
	GTimeVal     import_time;
	GTimeVal     file_time;
} TemplateData;


static gboolean
template_eval_cb (TemplateFlags   flags,
		  gunichar        parent_code,
		  gunichar        code,
		  char          **args,
		  GString        *result,
		  gpointer        user_data)
{
	TemplateData *template_data = user_data;
	char         *text = NULL;

	if ((parent_code == 'D') || (parent_code == 'T')) {
		/* strftime code, return the code itself. */
		_g_string_append_template_code (result, code, args);
		return FALSE;
	}

	switch (code) {
	case 'D': /* File date */
		text = _g_time_val_strftime (&template_data->file_time,
					     (args[0] != NULL) ? args[0] : DEFAULT_STRFTIME_FORMAT);
		break;

	case 'T': /* Timestamp */
		text = _g_time_val_strftime (&template_data->import_time,
					     (args[0] != NULL) ? args[0] : DEFAULT_STRFTIME_FORMAT);
		break;

	case 'E': /* Event description */
		if (template_data->event_name != NULL)
			g_string_append (result, template_data->event_name);
		break;

	default:
		break;
	}

	if (text != NULL) {
		g_string_append (result, text);
		g_free (text);
	}

	return FALSE;
}


GFile *
gth_import_utils_get_file_destination (GthFileData *file_data,
				       GFile       *destination,
				       const char  *subfolder_template,
				       const char  *event_name,
				       GTimeVal     import_time)
{
	TemplateData  template_data;
	char         *subfolder;
	GFile        *file_destination;

	template_data.file_data = file_data;
	template_data.event_name = event_name;
	template_data.import_time = import_time;

	{
		GthMetadata *metadata;

		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Embedded::Photo::DateTimeOriginal");
		if (metadata != NULL)
			_g_time_val_from_exif_date (gth_metadata_get_raw (metadata), &template_data.file_time);
		else
			g_file_info_get_modification_time (file_data->info, &template_data.file_time);

		if (template_data.file_time.tv_sec == 0)
			template_data.file_time = import_time;
	}

	subfolder = _g_template_eval (subfolder_template,
				      TEMPLATE_FLAGS_NO_ENUMERATOR,
				      template_eval_cb,
				      &template_data);
	if (subfolder != NULL) {
		file_destination = _g_file_append_path (destination, subfolder);
		g_free (subfolder);
	}
	else
		file_destination = g_file_dup (destination);

	return file_destination;

}
