/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include "actions.h"
#include "gth-selections-manager.h"


void
gth_browser_show_selection (GthBrowser *browser,
			    int         n_selection)
{
	char  *uri;
	GFile *location;

	uri = g_strdup_printf ("selection:///%d", n_selection);
	location = g_file_new_for_uri (uri);

	if (_g_file_equal (location, gth_browser_get_location (browser))) {
		if (! gth_browser_restore_state (browser))
			gth_browser_load_location (browser, location);
	}
	else {
		gth_browser_save_state (browser);
		gth_browser_load_location (browser, location);
	}

	g_object_unref (location);
	g_free (uri);
}


void
gth_browser_activate_go_to_selection (GSimpleAction	 *action,
				      GVariant		 *parameter,
				      gpointer		  user_data)
{
	gth_browser_show_selection (GTH_BROWSER (user_data),
				    g_variant_get_int32 (parameter));
}


void
gth_browser_add_to_selection(GthBrowser *browser,
			     int         n_selection)
{
	char  *uri;
	GFile *folder;
	GList *items;
	GList *file_list = NULL;
	GList *files;

	uri = g_strdup_printf ("selection:///%d", n_selection);
	folder = g_file_new_for_uri (uri);
	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	files = gth_file_data_list_to_file_list (file_list);
	gth_selections_manager_add_files (folder, files, -1);

	_g_object_list_unref (files);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
	g_object_unref (folder);
	g_free (uri);
}


void
gth_browser_activate_add_to_selection (GSimpleAction	 *action,
				       GVariant		 *parameter,
				       gpointer		  user_data)
{
	gth_browser_add_to_selection (GTH_BROWSER (user_data),
				      g_variant_get_int32 (parameter));
}


void
gth_browser_activate_go_to_file_container (GSimpleAction *action,
					   GVariant	 *parameter,
					   gpointer	  user_data)
{
	GthBrowser *browser = user_data;
	GList      *items;
	GList      *file_list = NULL;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	if (file_list != NULL) {
		GthFileData *first_file = file_list->data;
		GFile       *parent;

		parent = g_file_get_parent (first_file->file);
		gth_browser_go_to (browser, parent, first_file->file);

		g_object_unref (parent);
	}

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_remove_from_selection (GthBrowser *browser,
				   int         n_selection)
{
	char  *uri;
	GFile *folder;
	GList *items;
	GList *file_list = NULL;
	GList *files;

	uri = g_strdup_printf ("selection:///%d", n_selection);
	folder = g_file_new_for_uri (uri);
	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	files = gth_file_data_list_to_file_list (file_list);
	gth_selections_manager_remove_files (folder, files, TRUE);

	_g_object_list_unref (files);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
	g_object_unref (folder);
	g_free (uri);
}


void
gth_browser_activate_remove_from_selection (GSimpleAction	 *action,
					    GVariant		 *parameter,
					    gpointer		  user_data)
{
	gth_browser_remove_from_selection (GTH_BROWSER (user_data),
					   g_variant_get_int32 (parameter));
}


void
gth_browser_activate_remove_from_current_selection (GSimpleAction *action,
						    GVariant	  *parameter,
						    gpointer	   user_data)
{
	GthBrowser *browser = user_data;
	int         n_selection;

	n_selection = _g_file_get_n_selection (gth_browser_get_location (browser));
	if (n_selection >= 0)
		gth_browser_remove_from_selection (browser, n_selection);
}
