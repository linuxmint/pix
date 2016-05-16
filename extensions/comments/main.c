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
#include <gtk/gtk.h>
#include <gthumb.h>
#include <extensions/exiv2_tools/exiv2-utils.h>
#include "callbacks.h"
#include "dlg-comments-preferences.h"
#include "gth-comment.h"
#include "gth-metadata-provider-comment.h"
#include "preferences.h"


GthMetadataCategory comments_metadata_category[] = {
	{ "comment", N_("Comment"), 20 },
	{ NULL, NULL, 0 }
};


GthMetadataInfo comments_metadata_info[] = {
	{ "comment::caption", N_("Title"), "comment", 1, NULL, GTH_METADATA_ALLOW_IN_FILE_LIST },
	{ "comment::note", N_("Description"), "comment", 2, NULL, GTH_METADATA_ALLOW_IN_FILE_LIST },
	{ "comment::place", N_("Place"), "comment", 3, NULL, GTH_METADATA_ALLOW_IN_FILE_LIST },
	{ "comment::time", N_("Date"), "comment", 4, NULL, GTH_METADATA_ALLOW_IN_FILE_LIST },
	{ "comment::categories", N_("Tags"), "comment", 5, NULL, GTH_METADATA_ALLOW_IN_FILE_LIST },
	{ "comment::rating", N_("Rating"), "comment", 6, NULL, GTH_METADATA_ALLOW_IN_FILE_LIST },
	{ NULL, NULL, NULL, 0, 0 }
};


static gint64
get_comment_for_test (GthTest        *test,
		      GthFileData    *file,
		      gconstpointer  *data,
		      GDestroyNotify *data_destroy_func)
{
	*data = g_file_info_get_attribute_string (file->info, "comment::note");
	return 0;
}


static gint64
get_place_for_test (GthTest        *test,
		    GthFileData    *file,
		    gconstpointer  *data,
		    GDestroyNotify *data_destroy_func)
{
	*data = g_file_info_get_attribute_string (file->info, "comment::place");
	return 0;
}


static void
comments__add_sidecars_cb (GFile  *file,
			   GList **sidecars)
{
	*sidecars = g_list_prepend (*sidecars, gth_comment_get_comment_file (file));
}


static void
comments__read_metadata_ready_cb (GList      *file_list,
				  const char *attributes)
{
	GSettings *settings;
	gboolean   store_metadata_in_files;
	GList     *scan;
	gboolean   synchronize;

	settings = g_settings_new (GTHUMB_GENERAL_SCHEMA);
	store_metadata_in_files = g_settings_get_boolean (settings, PREF_GENERAL_STORE_METADATA_IN_FILES);
	g_object_unref (settings);

	if (! store_metadata_in_files) {
		/* if PREF_GENERAL_STORE_METADATA_IN_FILES is false, avoid to
		 * synchronize the .comment metadata because the embedded
		 * metadata is likely to be out-of-date.
		 * Give priority to the .comment metadata which, if present,
		 * is the most up-to-date. */

		gboolean can_read_embedded_attributes;

		can_read_embedded_attributes = gth_main_extension_is_active ("exiv2_tools");

		for (scan = file_list; scan; scan = scan->next) {
			GthFileData *file_data = scan->data;

			/* If PREF_GENERAL_STORE_METADATA_IN_FILES is false and
			 * there is no comment file then we are reading
			 * the image metadata for the first time, in this
			 * case update the .comment metadata with the
			 * embedded metadata. */
			if (g_file_info_get_attribute_boolean (file_data->info, "comment::no-comment-file")) {
				if (can_read_embedded_attributes) {
					exiv2_update_general_attributes (file_data->info);
					gth_comment_update_from_general_attributes (file_data);
				}
			}
			else
				gth_comment_update_general_attributes ((GthFileData *) scan->data);
		}
	}
	else {
		/* if PREF_GENERAL_STORE_METADATA_IN_FILES is true, update the .comment
		 * metadata with the embedded metadata.
		 */

		settings = g_settings_new (GTHUMB_COMMENTS_SCHEMA);
		synchronize = g_settings_get_boolean (settings, PREF_COMMENTS_SYNCHRONIZE);
		g_object_unref (settings);

		if (! synchronize)
			return;

		for (scan = file_list; scan; scan = scan->next)
			gth_comment_update_from_general_attributes ((GthFileData *) scan->data);
	}
}


static void
comments__delete_metadata_cb (GFile  *file,
			      void  **buffer,
			      gsize  *size)
{
	GFile *comment_file;

	comment_file = gth_comment_get_comment_file (file);
	if (comment_file != NULL) {
		g_file_delete (comment_file, NULL, NULL);
		g_object_unref (comment_file);
	}
}


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	gth_main_register_metadata_category (comments_metadata_category);
	gth_main_register_metadata_info_v (comments_metadata_info);
	gth_main_register_metadata_provider (GTH_TYPE_METADATA_PROVIDER_COMMENT);
	gth_main_register_object (GTH_TYPE_TEST,
				  "comment::note",
				  GTH_TYPE_TEST_SIMPLE,
				  "attributes", "comment::note",
				  "display-name", _("Description"),
				  "data-type", GTH_TEST_DATA_TYPE_STRING,
				  "get-data-func", get_comment_for_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "comment::place",
				  GTH_TYPE_TEST_SIMPLE,
				  "attributes", "comment::place",
				  "display-name", _("Place"),
				  "data-type", GTH_TEST_DATA_TYPE_STRING,
				  "get-data-func", get_place_for_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "comment::category",
				  GTH_TYPE_TEST_CATEGORY,
				  "attributes", "comment::categories",
				  "display-name", _("Tag"),
				  NULL);

	gth_hook_add_callback ("add-sidecars", 10, G_CALLBACK (comments__add_sidecars_cb), NULL);
	gth_hook_add_callback ("read-metadata-ready", 10, G_CALLBACK (comments__read_metadata_ready_cb), NULL);
	if (gth_main_extension_is_active ("edit_metadata"))
		gth_hook_add_callback ("delete-metadata", 10, G_CALLBACK (comments__delete_metadata_cb), NULL);
	gth_hook_add_callback ("gth-browser-construct", 10, G_CALLBACK (comments__gth_browser_construct_cb), NULL);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return TRUE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
	dlg_comments_preferences (parent);
}
