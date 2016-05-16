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


GFile *
gth_import_utils_get_file_destination (GthFileData        *file_data,
				       GFile              *destination,
				       GthSubfolderType    subfolder_type,
				       GthSubfolderFormat  subfolder_format,
				       gboolean            single_subfolder,
				       const char         *custom_format,
				       const char         *event_name,
				       GTimeVal            import_start_time)
{
	GTimeVal  timeval;
	char     *child;
	GFile    *file_destination;

	if (subfolder_type == GTH_SUBFOLDER_TYPE_FILE_DATE) {
		GthMetadata *metadata;

		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Embedded::Photo::DateTimeOriginal");
		if (metadata != NULL)
			_g_time_val_from_exif_date (gth_metadata_get_raw (metadata), &timeval);
		else
			g_file_info_get_modification_time (file_data->info, &timeval);

		if (timeval.tv_sec == 0)
			subfolder_type = GTH_SUBFOLDER_TYPE_CURRENT_DATE;
	}

	if (subfolder_type == GTH_SUBFOLDER_TYPE_CURRENT_DATE)
		timeval = import_start_time;

	switch (subfolder_type) {
	case GTH_SUBFOLDER_TYPE_FILE_DATE:
	case GTH_SUBFOLDER_TYPE_CURRENT_DATE:
		if (subfolder_format != GTH_SUBFOLDER_FORMAT_CUSTOM) {
			GDate  *date;
			char  **parts;

			date = g_date_new ();
			g_date_set_time_val (date, &timeval);

			parts = g_new0 (char *, 4);
			parts[0] = g_strdup_printf ("%04d", g_date_get_year (date));
			if (subfolder_format != GTH_SUBFOLDER_FORMAT_YYYY) {
				parts[1] = g_strdup_printf ("%02d", g_date_get_month (date));
				if (subfolder_format != GTH_SUBFOLDER_FORMAT_YYYYMM)
					parts[2] = g_strdup_printf ("%02d", g_date_get_day (date));
			}

			if (single_subfolder)
				child = g_strjoinv ("-", parts);
			else
				child = g_strjoinv ("/", parts);

			g_strfreev (parts);
			g_date_free (date);
		}
		else {
			char *format = NULL;

			if (event_name != NULL) {
				GRegex *re;

				re = g_regex_new ("%E", 0, 0, NULL);
				format = g_regex_replace_literal (re, custom_format, -1, 0, event_name, 0, NULL);

				g_regex_unref (re);
			}
			if (format == NULL)
				format = g_strdup (custom_format);
			child = _g_time_val_strftime (&timeval, format);

			g_free (format);
		}
		break;

	case GTH_SUBFOLDER_TYPE_NONE:
	default:
		child = NULL;
		break;
	}
	file_destination = _g_file_append_path (destination, child);

	g_free (child);

	return file_destination;
}
