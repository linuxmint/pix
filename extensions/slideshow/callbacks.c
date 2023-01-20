	/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009-2010 Free Software Foundation, Inc.
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
#include <glib-object.h>
#include <gthumb.h>
#include "actions.h"
#include "callbacks.h"
#include "gth-slideshow-preferences.h"
#include "preferences.h"
#include "shortcuts.h"


static const GActionEntry actions[] = {
	{ "slideshow", gth_browser_activate_slideshow }
};


static const GthShortcut shortcuts[] = {
	{ "slideshow", N_("Start presentation"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_VIEWER, "F5" },
	{ "slideshow-close", N_("Terminate presentation"), GTH_SHORTCUT_CONTEXT_SLIDESHOW, GTH_SHORTCUT_CATEGORY_SLIDESHOW, "Escape" },
	{ "slideshow-toggle-pause", N_("Pause/Resume presentation"), GTH_SHORTCUT_CONTEXT_SLIDESHOW, GTH_SHORTCUT_CATEGORY_SLIDESHOW, "p" },
	{ "slideshow-next-image", N_("Show next file"), GTH_SHORTCUT_CONTEXT_SLIDESHOW, GTH_SHORTCUT_CATEGORY_SLIDESHOW, "space" },
	{ "slideshow-previous-image", N_("Show previous file"), GTH_SHORTCUT_CONTEXT_SLIDESHOW, GTH_SHORTCUT_CATEGORY_SLIDESHOW, "b" },
};


void
ss__gth_browser_construct_cb (GthBrowser *browser)
{
	g_return_if_fail (GTH_IS_BROWSER (browser));

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);

	gth_window_add_shortcuts (GTH_WINDOW (browser),
				  shortcuts,
				  G_N_ELEMENTS (shortcuts));

	gth_browser_add_header_bar_button (browser,
					   GTH_BROWSER_HEADER_SECTION_BROWSER_VIEW,
					   "view-presentation-symbolic",
					   _("Presentation"),
					   "win.slideshow",
					   NULL);
}


void
ss__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	GtkTreeModel *file_store;
	gboolean      sensitive;

	file_store = gth_file_view_get_model (GTH_FILE_VIEW (gth_browser_get_file_list_view (browser)));
	sensitive = (gth_file_store_n_visibles (GTH_FILE_STORE (file_store)) > 0);
	g_object_set (g_action_map_lookup_action (G_ACTION_MAP (browser), "slideshow"), "enabled", sensitive, NULL);
}


void
ss__slideshow_cb (GthBrowser *browser)
{
	gth_browser_activate_slideshow (NULL, NULL, browser);
}


void
ss__gth_catalog_read_metadata (GthCatalog  *catalog,
			       GthFileData *file_data)
{
	if (g_file_info_get_attribute_status (file_data->info, "slideshow::personalize") == G_FILE_ATTRIBUTE_STATUS_SET)
		g_value_hash_set_boolean (catalog->attributes,
					  "slideshow::personalize",
					  g_file_info_get_attribute_boolean (file_data->info, "slideshow::personalize"));
	if (g_file_info_get_attribute_status (file_data->info, "slideshow::automatic") == G_FILE_ATTRIBUTE_STATUS_SET)
		g_value_hash_set_boolean (catalog->attributes,
					  "slideshow::automatic",
					  g_file_info_get_attribute_boolean (file_data->info, "slideshow::automatic"));
	if (g_file_info_get_attribute_status (file_data->info, "slideshow::wrap-around") == G_FILE_ATTRIBUTE_STATUS_SET)
		g_value_hash_set_boolean (catalog->attributes,
					  "slideshow::wrap-around",
					  g_file_info_get_attribute_boolean (file_data->info, "slideshow::wrap-around"));
	if (g_file_info_get_attribute_status (file_data->info, "slideshow::random-order") == G_FILE_ATTRIBUTE_STATUS_SET)
		g_value_hash_set_boolean (catalog->attributes,
					  "slideshow::random-order",
					  g_file_info_get_attribute_boolean (file_data->info, "slideshow::random-order"));
	if (g_file_info_get_attribute_status (file_data->info, "slideshow::delay") == G_FILE_ATTRIBUTE_STATUS_SET)
		g_value_hash_set_int (catalog->attributes,
				      "slideshow::delay",
				      g_file_info_get_attribute_int32 (file_data->info, "slideshow::delay"));
	if (g_file_info_get_attribute_status (file_data->info, "slideshow::transition") == G_FILE_ATTRIBUTE_STATUS_SET)
		g_value_hash_set_string (catalog->attributes,
					 "slideshow::transition",
				 	 g_file_info_get_attribute_string (file_data->info, "slideshow::transition"));
	if (g_file_info_get_attribute_status (file_data->info, "slideshow::playlist") == G_FILE_ATTRIBUTE_STATUS_SET)
		g_value_hash_set_stringv (catalog->attributes,
					  "slideshow::playlist",
					  g_file_info_get_attribute_stringv (file_data->info, "slideshow::playlist"));
}


void
ss__gth_catalog_write_metadata (GthCatalog  *catalog,
			        GthFileData *file_data)
{
	if (g_value_hash_is_set (catalog->attributes, "slideshow::personalize")) {
		g_file_info_set_attribute_boolean (file_data->info,
						   "slideshow::personalize",
						   g_value_hash_get_boolean (catalog->attributes, "slideshow::personalize"));
		g_file_info_set_attribute_status (file_data->info,
						  "slideshow::personalize",
						  G_FILE_ATTRIBUTE_STATUS_SET);
	}
	if (g_value_hash_is_set (catalog->attributes, "slideshow::automatic")) {
		g_file_info_set_attribute_boolean (file_data->info,
						   "slideshow::automatic",
						   g_value_hash_get_boolean (catalog->attributes, "slideshow::automatic"));
		g_file_info_set_attribute_status (file_data->info,
						  "slideshow::automatic",
						  G_FILE_ATTRIBUTE_STATUS_SET);
	}
	if (g_value_hash_is_set (catalog->attributes, "slideshow::wrap-around")) {
		g_file_info_set_attribute_boolean (file_data->info,
						   "slideshow::wrap-around",
						   g_value_hash_get_boolean (catalog->attributes, "slideshow::wrap-around"));
		g_file_info_set_attribute_status (file_data->info,
						  "slideshow::wrap-around",
						  G_FILE_ATTRIBUTE_STATUS_SET);
	}
	if (g_value_hash_is_set (catalog->attributes, "slideshow::random-order")) {
		g_file_info_set_attribute_boolean (file_data->info,
						   "slideshow::random-order",
						   g_value_hash_get_boolean (catalog->attributes, "slideshow::random-order"));
		g_file_info_set_attribute_status (file_data->info,
						  "slideshow::random-order",
						  G_FILE_ATTRIBUTE_STATUS_SET);
	}
	if (g_value_hash_is_set (catalog->attributes, "slideshow::delay")) {
		g_file_info_set_attribute_int32 (file_data->info,
						 "slideshow::delay",
						 g_value_hash_get_int (catalog->attributes, "slideshow::delay"));
		g_file_info_set_attribute_status (file_data->info,
						  "slideshow::delay",
						  G_FILE_ATTRIBUTE_STATUS_SET);
	}
	if (g_value_hash_is_set (catalog->attributes, "slideshow::transition")) {
		g_file_info_set_attribute_string (file_data->info,
						  "slideshow::transition",
						  g_value_hash_get_string (catalog->attributes, "slideshow::transition"));
		g_file_info_set_attribute_status (file_data->info,
						  "slideshow::transition",
						  G_FILE_ATTRIBUTE_STATUS_SET);
	}
	if (g_value_hash_is_set (catalog->attributes, "slideshow::playlist")) {
		g_file_info_set_attribute_stringv (file_data->info,
						   "slideshow::playlist",
						   g_value_hash_get_stringv (catalog->attributes, "slideshow::playlist"));
		g_file_info_set_attribute_status (file_data->info,
						  "slideshow::playlist",
						  G_FILE_ATTRIBUTE_STATUS_SET);
	}
}


void
ss__gth_catalog_read_from_doc (GthCatalog *catalog,
			       DomElement *root)
{
	DomElement *node;

	for (node = root->first_child; node; node = node->next_sibling) {
		DomElement *child;

		if (g_strcmp0 (node->tag_name, "slideshow") != 0)
			continue;

		g_value_hash_set_boolean (catalog->attributes,
					  "slideshow::personalize",
					  g_strcmp0 (dom_element_get_attribute (node, "personalize"), "true") == 0);
		g_value_hash_set_boolean (catalog->attributes,
					  "slideshow::automatic",
					  g_strcmp0 (dom_element_get_attribute (node, "automatic"), "true") == 0);
		g_value_hash_set_boolean (catalog->attributes,
					  "slideshow::wrap-around",
					  g_strcmp0 (dom_element_get_attribute (node, "wrap-around"), "true") == 0);
		g_value_hash_set_boolean (catalog->attributes,
					  "slideshow::random-order",
					  g_strcmp0 (dom_element_get_attribute (node, "random-order"), "true") == 0);

		for (child = node->first_child; child; child = child->next_sibling) {
			if (g_strcmp0 (child->tag_name, "delay") == 0) {
				int delay;

				sscanf (dom_element_get_inner_text (child), "%d", &delay);
				g_value_hash_set_int (catalog->attributes,
						      "slideshow::delay",
						      delay);
			}
			else if (g_strcmp0 (child->tag_name, "transition") == 0) {
				g_value_hash_set_string (catalog->attributes,
							 "slideshow::transition",
							 dom_element_get_inner_text (child));
			}
			else if (g_strcmp0 (child->tag_name, "playlist") == 0) {
				DomElement  *file;
				GList       *audio_files;

				audio_files = NULL;
				for (file = child->first_child; file; file = file->next_sibling) {
					if (g_strcmp0 (file->tag_name, "file") == 0)
						audio_files = g_list_prepend (audio_files, g_strdup (dom_element_get_attribute (file, "uri")));
				}
				audio_files = g_list_reverse (audio_files);

				if (audio_files != NULL) {
					char **audio_files_v;

					audio_files_v = _g_string_list_to_strv (audio_files);
					g_value_hash_set_stringv (catalog->attributes,
								  "slideshow::playlist",
								  audio_files_v);

					g_strfreev (audio_files_v);
				}
				else
					g_value_hash_unset (catalog->attributes, "slideshow::playlist");

				_g_string_list_free (audio_files);
			}
		}
	}
}


void
ss__gth_catalog_write_to_doc (GthCatalog  *catalog,
			      DomDocument *doc,
			      DomElement  *root)
{
	DomElement  *slideshow;

	if (! g_value_hash_is_set (catalog->attributes, "slideshow::personalize"))
		return;

	slideshow = dom_document_create_element (doc,
						 "slideshow",
						 "personalize", (g_value_hash_get_boolean_or_default (catalog->attributes, "slideshow::personalize", FALSE) ? "true" : "false"),
						 "automatic", (g_value_hash_get_boolean_or_default (catalog->attributes, "slideshow::automatic", FALSE) ? "true" : "false"),
						 "wrap-around", (g_value_hash_get_boolean_or_default (catalog->attributes, "slideshow::wrap-around", FALSE) ? "true" : "false"),
						 "random-order", (g_value_hash_get_boolean_or_default (catalog->attributes, "slideshow::random-order", FALSE) ? "true" : "false"),
						 NULL);
	dom_element_append_child (root, slideshow);

	if (g_value_hash_is_set (catalog->attributes, "slideshow::delay")) {
		char *delay;

		delay = g_strdup_printf ("%d", g_value_hash_get_int (catalog->attributes, "slideshow::delay"));
		dom_element_append_child (slideshow,
					  dom_document_create_element_with_text (doc, delay, "delay", NULL));

		g_free (delay);
	}

	if (g_value_hash_is_set (catalog->attributes, "slideshow::transition"))
		dom_element_append_child (slideshow,
					  dom_document_create_element_with_text (doc,
										 g_value_hash_get_string (catalog->attributes, "slideshow::transition"),
										 "transition",
										 NULL));

	if (g_value_hash_is_set (catalog->attributes, "slideshow::playlist")) {
		char **playlist_files;

		playlist_files = g_value_hash_get_stringv (catalog->attributes, "slideshow::playlist");
		if (playlist_files[0] != NULL) {
			DomElement *playlist;
			int         i;

			playlist = dom_document_create_element (doc, "playlist", NULL);
			dom_element_append_child (slideshow, playlist);

			for (i = 0; playlist_files[i] != NULL; i++)
				dom_element_append_child (playlist, dom_document_create_element (doc, "file", "uri", playlist_files[i], NULL));
		}
	}
}


void
ss__dlg_catalog_properties (GtkBuilder  *builder,
			    GthFileData *file_data,
			    GthCatalog  *catalog)
{
	GtkWidget *slideshow_preferences;
	GtkWidget *label;

	if (! g_value_hash_is_set (catalog->attributes, "slideshow::personalize")
	    || ! g_value_hash_get_boolean (catalog->attributes, "slideshow::personalize"))
	{
		GSettings *settings;
		char      *current_transition;

		settings = g_settings_new (GTHUMB_SLIDESHOW_SCHEMA);
		current_transition = g_settings_get_string (settings, PREF_SLIDESHOW_TRANSITION);
		slideshow_preferences = gth_slideshow_preferences_new (current_transition,
								       g_settings_get_boolean (settings, PREF_SLIDESHOW_AUTOMATIC),
								       (int) (1000.0 * g_settings_get_double (settings, PREF_SLIDESHOW_CHANGE_DELAY)),
								       g_settings_get_boolean (settings, PREF_SLIDESHOW_WRAP_AROUND),
								       g_settings_get_boolean (settings, PREF_SLIDESHOW_RANDOM_ORDER));
		gtk_widget_set_sensitive (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences), "personalize_box"), FALSE);

		g_free (current_transition);
		g_object_unref (settings);
	}
	else {
		slideshow_preferences = gth_slideshow_preferences_new (g_value_hash_get_string (catalog->attributes, "slideshow::transition"),
								       g_value_hash_get_boolean (catalog->attributes, "slideshow::automatic"),
								       g_value_hash_get_int (catalog->attributes, "slideshow::delay"),
								       g_value_hash_get_boolean (catalog->attributes, "slideshow::wrap-around"),
								       g_value_hash_get_boolean (catalog->attributes, "slideshow::random-order"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences), "personalize_checkbutton")), TRUE);
		gtk_widget_set_sensitive (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences), "personalize_box"), TRUE);
	}

	if (g_value_hash_is_set (catalog->attributes, "slideshow::playlist"))
		gth_slideshow_preferences_set_audio (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences),
						     g_value_hash_get_stringv (catalog->attributes, "slideshow::playlist"));

	gtk_container_set_border_width (GTK_CONTAINER (slideshow_preferences), 12);
	gtk_widget_show (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences), "personalize_checkbutton"));
	gtk_widget_hide (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences), "slideshow_label"));
	gtk_widget_show (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences), "playlist_box"));
	gtk_widget_show (slideshow_preferences);

#ifndef HAVE_CLUTTER
	gtk_widget_hide (gth_slideshow_preferences_get_widget (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences), "transition_box"));
#endif /* ! HAVE_CLUTTER */

	label = gtk_label_new (_("Presentation"));
	gtk_widget_show (label);

	gtk_notebook_append_page (GTK_NOTEBOOK (_gtk_builder_get_widget (builder, "properties_notebook")), slideshow_preferences, label);
	g_object_set_data (G_OBJECT (builder), "slideshow_preferences", slideshow_preferences);
}


void
ss__dlg_catalog_properties_save (GtkBuilder  *builder,
				 GthFileData *file_data,
				 GthCatalog  *catalog)
{
	GtkWidget  *slideshow_preferences;
	char       *transition_id;
	char      **files;

	slideshow_preferences = g_object_get_data (G_OBJECT (builder), "slideshow_preferences");

	g_value_hash_set_boolean (catalog->attributes,
				  "slideshow::personalize",
				  gth_slideshow_preferences_get_personalize (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences)));

	transition_id = gth_slideshow_preferences_get_transition_id (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences));
	g_value_hash_set_string (catalog->attributes,
				 "slideshow::transition",
				 transition_id);
	g_free (transition_id);

	g_value_hash_set_boolean (catalog->attributes,
				  "slideshow::automatic",
				  gth_slideshow_preferences_get_automatic (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences)));
	g_value_hash_set_int (catalog->attributes,
			      "slideshow::delay",
			      gth_slideshow_preferences_get_delay (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences)));
	g_value_hash_set_boolean (catalog->attributes,
				  "slideshow::wrap-around",
				  gth_slideshow_preferences_get_wrap_around (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences)));
	g_value_hash_set_boolean (catalog->attributes,
				  "slideshow::random-order",
				  gth_slideshow_preferences_get_random_order (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences)));

	files = gth_slideshow_preferences_get_audio_files (GTH_SLIDESHOW_PREFERENCES (slideshow_preferences));
	g_value_hash_set_stringv (catalog->attributes,
				  "slideshow::playlist",
				  files);
	g_strfreev (files);
}
