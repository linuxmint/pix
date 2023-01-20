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
#include <glib/gi18n.h>
#include <gthumb.h>
#include "actions.h"
#include "gth-slideshow.h"
#include "gth-transition.h"
#include "preferences.h"


void
gth_browser_activate_slideshow (GSimpleAction *action,
				GVariant      *parameter,
				gpointer       user_data)
{
	GthBrowser   *browser = user_data;
	GSettings    *settings;
	GList        *items;
	GList        *file_list;
	GList        *filtered_list;
	GList        *scan;
	GthProjector *projector;
	GtkWidget    *slideshow;
	GthFileData  *location;
	char         *transition_id;
	GList        *transitions = NULL;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	if ((items == NULL) || (items->next == NULL))
		file_list = gth_file_store_get_visibles (GTH_FILE_STORE (gth_browser_get_file_store (browser)));
	else
		file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	filtered_list = NULL;
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (_g_mime_type_is_image (gth_file_data_get_mime_type (file_data)))
			filtered_list = g_list_prepend (filtered_list, g_object_ref (file_data));
	}
	filtered_list = g_list_reverse (filtered_list);

	if (filtered_list == NULL) {
		_g_object_list_unref (file_list);
		_gtk_tree_path_list_free (items);
		return;
	}

	settings = g_settings_new (GTHUMB_SLIDESHOW_SCHEMA);

	location = gth_browser_get_location_data (browser);
	if (g_file_info_get_attribute_boolean (location->info, "slideshow::personalize"))
		transition_id = g_strdup (g_file_info_get_attribute_string (location->info, "slideshow::transition"));
	else
		transition_id = g_settings_get_string (settings, PREF_SLIDESHOW_TRANSITION);

	projector = NULL;

#ifdef HAVE_CLUTTER
	if (gtk_clutter_init (NULL, NULL) == CLUTTER_INIT_SUCCESS)
		projector = &clutter_projector;
#endif /* HAVE_CLUTTER */

	if ((projector == NULL) || (strcmp (transition_id, "none") == 0))
		projector = &default_projector;

	slideshow = gth_slideshow_new (projector, browser, filtered_list);

	if (g_file_info_get_attribute_boolean (location->info, "slideshow::personalize")) {
		gth_slideshow_set_delay (GTH_SLIDESHOW (slideshow), g_file_info_get_attribute_int32 (location->info, "slideshow::delay"));
		gth_slideshow_set_automatic (GTH_SLIDESHOW (slideshow), g_file_info_get_attribute_boolean (location->info, "slideshow::automatic"));
		gth_slideshow_set_wrap_around (GTH_SLIDESHOW (slideshow), g_file_info_get_attribute_boolean (location->info, "slideshow::wrap-around"));
		gth_slideshow_set_random_order (GTH_SLIDESHOW (slideshow), g_file_info_get_attribute_boolean (location->info, "slideshow::random-order"));
	}
	else {
		gth_slideshow_set_delay (GTH_SLIDESHOW (slideshow), (guint) (1000.0 * g_settings_get_double (settings, PREF_SLIDESHOW_CHANGE_DELAY)));
		gth_slideshow_set_automatic (GTH_SLIDESHOW (slideshow), g_settings_get_boolean (settings, PREF_SLIDESHOW_AUTOMATIC));
		gth_slideshow_set_wrap_around (GTH_SLIDESHOW (slideshow), g_settings_get_boolean (settings, PREF_SLIDESHOW_WRAP_AROUND));
		gth_slideshow_set_random_order (GTH_SLIDESHOW (slideshow), g_settings_get_boolean (settings, PREF_SLIDESHOW_RANDOM_ORDER));
	}

	if (g_file_info_get_attribute_status (location->info, "slideshow::playlist") == G_FILE_ATTRIBUTE_STATUS_SET)
		gth_slideshow_set_playlist (GTH_SLIDESHOW (slideshow),
					    g_file_info_get_attribute_stringv (location->info, "slideshow::playlist"));

	if (strcmp (transition_id, "random") == 0) {
		GList *scan;

		transitions = gth_main_get_registered_objects (GTH_TYPE_TRANSITION);
		for (scan = transitions; scan; scan = scan->next) {
			GthTransition *transition = scan->data;

			if (strcmp (gth_transition_get_id (transition), "none") == 0) {
				transitions = g_list_remove_link (transitions, scan);
				_g_object_list_unref (scan);
				break;
			}
		}
	}
	else {
		GthTransition *transition = gth_main_get_registered_object (GTH_TYPE_TRANSITION, transition_id);

		if (transition != NULL)
			transitions = g_list_append (NULL, transition);
		else
			transitions = NULL;
	}
	gth_slideshow_set_transitions (GTH_SLIDESHOW (slideshow), transitions);

	{
		GdkRectangle  monitor_geometry;
		int           monitor_num;

		if (_gtk_window_get_monitor_info (GTK_WINDOW (browser), &monitor_geometry, &monitor_num, NULL)) {
			gtk_window_set_default_size (GTK_WINDOW (slideshow), monitor_geometry.width, monitor_geometry.height);
			gtk_window_fullscreen_on_monitor (GTK_WINDOW (slideshow), gtk_window_get_screen (GTK_WINDOW (browser)), monitor_num);
		}
		else
			gtk_window_fullscreen (GTK_WINDOW (slideshow));
		gtk_window_present (GTK_WINDOW (slideshow));
	}

	_g_object_list_unref (transitions);
	g_object_unref (settings);
	g_free (transition_id);
	_g_object_list_unref (filtered_list);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_slideshow_activate_close (GSimpleAction *action,
			      GVariant      *parameter,
			      gpointer       user_data)
{
	gth_slideshow_close (GTH_SLIDESHOW (user_data));
}


void
gth_slideshow_activate_toggle_pause (GSimpleAction *action,
				     GVariant      *parameter,
				     gpointer       user_data)
{
	gth_slideshow_toggle_pause (GTH_SLIDESHOW (user_data));
}


void
gth_slideshow_activate_next_image (GSimpleAction *action,
				   GVariant      *parameter,
				   gpointer       user_data)
{
	gth_slideshow_next_image_or_resume (GTH_SLIDESHOW (user_data));
}


void
gth_slideshow_activate_previous_image (GSimpleAction *action,
				       GVariant      *parameter,
				       gpointer       user_data)
{
	gth_slideshow_load_prev_image (GTH_SLIDESHOW (user_data));
}

