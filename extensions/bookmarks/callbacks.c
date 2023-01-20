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
#include <glib-object.h>
#include <gthumb.h>
#include "actions.h"
#include "callbacks.h"


#define BROWSER_DATA_KEY "bookmarks-browser-data"


static const GActionEntry actions[] = {
	{ "bookmarks-add", gth_browser_activate_bookmarks_add },
	{ "bookmarks-edit", gth_browser_activate_bookmarks_edit }
};


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	guint       bookmarks_changed_id;
	guint       entry_points_changed_id;
	GMenu      *system_bookmarks_menu;
	GMenu      *entry_points_menu;
	GMenu      *bookmarks_menu;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	if (data->bookmarks_changed_id != 0) {
		g_signal_handler_disconnect (gth_main_get_default_monitor (),
					     data->bookmarks_changed_id);
		data->bookmarks_changed_id = 0;
	}
	if (data->entry_points_changed_id != 0) {
		g_signal_handler_disconnect (gth_main_get_default_monitor (),
					     data->entry_points_changed_id);
		data->entry_points_changed_id = 0;
	}
	_g_object_unref (data->builder);
	g_free (data);
}


#define BUFFER_SIZE 4096


typedef struct {
	GthBrowser   *browser;
	GInputStream *stream;
	char          buffer[BUFFER_SIZE];
	GString      *file_content;
} UpdateBookmarksData;


static void
update_bookmakrs_data_free (UpdateBookmarksData *data)
{
	g_input_stream_close (data->stream, NULL, NULL);
	g_object_unref (data->stream);
	g_string_free (data->file_content, TRUE);
	g_object_unref (data->browser);
	g_free (data);
}


static void
update_system_bookmark_list_from_content (GthBrowser *browser,
					  const char *content)
{
	BrowserData   *data;
	char        **lines;
	int           i;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	lines = g_strsplit (content, "\n", -1);
	for (i = 0; lines[i] != NULL; i++) {
		char      **line;
		char       *uri;
		GFile      *file;
		char       *first_space;
		char       *name;
		GMenuItem  *item;

		line = g_strsplit (lines[i], " ", 2);
		uri = line[0];
		if (uri == NULL) {
			g_strfreev (line);
			continue;
		}

		file = g_file_new_for_uri (uri);
		first_space = strchr (lines[i], ' ');
		name = (first_space != NULL) ? g_strdup (first_space + 1) : NULL;
		item = _g_menu_item_new_for_file (file, name);
		g_menu_item_set_action_and_target (item, "win.go-to-location", "s", uri);
		g_menu_append_item (data->system_bookmarks_menu, item);

		g_object_unref (item);
		g_free (name);
		g_object_unref (file);
		g_strfreev (line);
	}

	g_strfreev (lines);
}


static void
update_system_bookmark_list_ready (GObject      *source_object,
				   GAsyncResult *result,
				   gpointer      user_data)
{
	UpdateBookmarksData *data = user_data;
	gssize               size;

	size = g_input_stream_read_finish (data->stream, result, NULL);
	if (size < 0) {
		update_bookmakrs_data_free (data);
		return;
	}

	if (size > 0) {
		data->buffer[size + 1] = '\0';
		g_string_append (data->file_content, data->buffer);

		g_input_stream_read_async (data->stream,
				   	   data->buffer,
				   	   BUFFER_SIZE - 1,
					   G_PRIORITY_DEFAULT,
					   NULL,
					   update_system_bookmark_list_ready,
					   data);
		return;
	}

	update_system_bookmark_list_from_content (data->browser, data->file_content->str);
	update_bookmakrs_data_free (data);
}


static void
_gth_browser_update_system_bookmark_list (GthBrowser *browser)
{
	BrowserData         *browser_data;
	GFile               *bookmark_file;
	GFileInputStream    *input_stream;
	UpdateBookmarksData *data;

	browser_data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (browser_data != NULL);

	g_menu_remove_all (browser_data->system_bookmarks_menu);

	/* give priority to XDG_CONFIG_HOME/gtk-3.0/bookmarks if not found
	 * try the old ~/.gtk-bookmarks */

	bookmark_file = gth_user_dir_get_file_for_read (GTH_DIR_CONFIG, "gtk-3.0", "bookmarks", NULL);
	if (! g_file_query_exists (bookmark_file, NULL)) {
		char *path;

		g_object_unref (bookmark_file);
		path = g_build_filename (g_get_home_dir (), ".gtk-bookmarks", NULL);
		bookmark_file = g_file_new_for_path (path);

		g_free (path);
	}

	input_stream = g_file_read (bookmark_file, NULL, NULL);
	g_object_unref (bookmark_file);

	if (input_stream == NULL)
		return;

	data = g_new0 (UpdateBookmarksData, 1);
	data->browser = g_object_ref (browser);
	data->stream = (GInputStream*) input_stream;
	data->file_content = g_string_new ("");

	g_input_stream_read_async (data->stream,
				   data->buffer,
				   BUFFER_SIZE - 1,
				   G_PRIORITY_DEFAULT,
				   NULL,
				   update_system_bookmark_list_ready,
				   data);
}


static void
_gth_browser_update_bookmark_list (GthBrowser *browser)
{
	BrowserData    *data;
	GBookmarkFile  *bookmarks;
	char          **uris;
	int             i;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	g_menu_remove_all (data->bookmarks_menu);

	bookmarks = gth_main_get_default_bookmarks ();
	uris = g_bookmark_file_get_uris (bookmarks, NULL);

	for (i = 0; uris[i] != NULL; i++) {
		GFile     *file;
		char      *name;
		GMenuItem *item;

		file = g_file_new_for_uri (uris[i]);
		name = g_bookmark_file_get_title (bookmarks, uris[i], NULL);
		item = _g_menu_item_new_for_file (file, name);
		g_menu_item_set_action_and_target (item, "win.go-to-location", "s", uris[i]);
		g_menu_append_item (data->bookmarks_menu, item);

		g_object_unref (item);
		g_free (name);
		g_object_unref (file);
	}

	_gth_browser_update_system_bookmark_list (browser);

	g_strfreev (uris);
}


static void
bookmarks_changed_cb (GthMonitor *monitor,
		      gpointer    user_data)
{
	BrowserData *data = user_data;
	_gth_browser_update_bookmark_list (data->browser);
}


static void
_gth_browser_update_entry_point_list (GthBrowser *browser)
{
	BrowserData  *data;
	GList        *entry_points;
	GList        *scan;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	g_menu_remove_all (data->entry_points_menu);

	entry_points = gth_main_get_all_entry_points ();
	for (scan = entry_points; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GMenuItem   *item;
		char        *uri;

		item = _g_menu_item_new_for_file_data (file_data);
		uri = g_file_get_uri (file_data->file);
		g_menu_item_set_action_and_target (item, "win.go-to-location", "s", uri);
		g_menu_append_item (data->entry_points_menu, item);

		g_free (uri);
		g_object_unref (item);
	}

	_g_object_list_unref (entry_points);
}


static void
entry_points_changed_cb (GthMonitor *monitor,
			 gpointer    user_data)
{
	BrowserData *data = user_data;
	call_when_idle ((DataFunc) _gth_browser_update_entry_point_list, data->browser);
}


void
bookmarks__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);

	{
		GtkWidget  *button;
		GMenuModel *menu;

		button = _gtk_menu_button_new_for_header_bar ("user-bookmarks-symbolic");
		gtk_widget_set_tooltip_text (button, _("Bookmarks"));

		data->builder = gtk_builder_new_from_resource ("/org/gnome/gThumb/bookmarks/data/ui/bookmarks-menu.ui");
		data->system_bookmarks_menu = G_MENU (gtk_builder_get_object (data->builder, "system-bookmarks"));
		data->entry_points_menu = G_MENU (gtk_builder_get_object (data->builder, "entry-points"));
		data->bookmarks_menu = G_MENU (gtk_builder_get_object (data->builder, "bookmarks"));

		menu = G_MENU_MODEL (gtk_builder_get_object (data->builder, "bookmarks-menu"));
		gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button), menu);
		_gtk_window_add_accelerators_from_menu ((GTK_WINDOW (browser)), menu);

		gtk_widget_show (button);
		gtk_box_pack_end (GTK_BOX (gth_browser_get_headerbar_section (browser, GTH_BROWSER_HEADER_SECTION_BROWSER_LOCATIONS)), button, FALSE, FALSE, 0);
	}

	data->browser = browser;
	data->bookmarks_changed_id = g_signal_connect (gth_main_get_default_monitor (),
						       "bookmarks-changed",
						       G_CALLBACK (bookmarks_changed_cb),
						       data);
	data->entry_points_changed_id = g_signal_connect (gth_main_get_default_monitor (),
							  "entry-points-changed",
							  G_CALLBACK (entry_points_changed_cb),
							  data);
}


void
bookmarks__gth_browser_construct_idle_callback_cb (GthBrowser *browser)
{
	_gth_browser_update_bookmark_list (browser);
	_gth_browser_update_entry_point_list (browser);
}
