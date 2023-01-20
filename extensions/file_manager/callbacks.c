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
#include <gdk/gdkkeysyms.h>
#include <gthumb.h>
#include "actions.h"
#include "callbacks.h"
#include "gth-copy-task.h"
#include "gth-reorder-task.h"


#define BROWSER_DATA_KEY             "file-manager-browser-data"
#define URI_LIST_ATOM                (gdk_atom_intern_static_string ("text/uri-list"))
#define XDND_ACTION_DIRECT_SAVE_ATOM (gdk_atom_intern_static_string ("XdndDirectSave0"))
#define GTHUMB_REORDERABLE_LIST_ATOM (gdk_atom_intern_static_string ("gthumb/reorderable-list"))
#define TEXT_PLAIN_ATOM              (gdk_atom_intern_static_string ("text/plain"))
#define SCROLL_TIMEOUT               30 /* autoscroll timeout in milliseconds */
#define UPDATE_OPEN_MENU_DELAY       500


static const GActionEntry actions[] = {
	{ "edit-cut", gth_browser_activate_edit_cut },
	{ "edit-copy", gth_browser_activate_edit_copy },
	{ "edit-paste", gth_browser_activate_edit_paste },
	{ "trash", gth_browser_activate_trash },
	{ "delete", gth_browser_activate_delete },
	{ "remove-from-source", gth_browser_activate_remove_from_source },
	{ "remove-from-source-permanently", gth_browser_activate_remove_from_source_permanently },
	{ "rename", gth_browser_activate_rename },
	{ "file-list-rename", gth_browser_activate_file_list_rename },
	{ "duplicate", gth_browser_activate_duplicate },
	{ "copy-to-folder", gth_browser_activate_copy_to_folder },
	{ "move-to-folder", gth_browser_activate_move_to_folder },
	{ "create-folder", gth_browser_activate_create_folder },
	{ "folder-context-open-with-fm", gth_browser_activate_folder_context_open_in_file_manager },
	{ "folder-context-create", gth_browser_activate_folder_context_create },
	{ "folder-context-rename", gth_browser_activate_folder_context_rename },
	{ "folder-context-cut", gth_browser_activate_folder_context_cut },
	{ "folder-context-copy", gth_browser_activate_folder_context_copy },
	{ "folder-context-paste-into-folder", gth_browser_activate_folder_context_paste_into_folder },
	{ "folder-context-copy-to", gth_browser_activate_folder_context_copy_to },
	{ "folder-context-move-to", gth_browser_activate_folder_context_move_to },
	{ "folder-context-trash", gth_browser_activate_folder_context_trash },
	{ "folder-context-delete", gth_browser_activate_folder_context_delete },
	{ "open-with-gimp", gth_browser_activate_open_with_gimp },
	{ "open-with-application", gth_browser_activate_open_with_application, "i" }
};


static const GthMenuEntry fixed_menu_entries_edit[] = {
	{ N_("Cut"), "win.edit-cut" },
	{ N_("Copy"), "win.edit-copy" },
	{ N_("Paste"), "win.edit-paste" },
};


static const GthMenuEntry fixed_menu_entries_file[] = {
	{ N_("Copy to…"), "win.copy-to-folder" },
	{ N_("Move to…"), "win.move-to-folder" },
	{ N_("Rename"), "win.file-list-rename" },
};


static const GthMenuEntry fixed_menu_entries_delete[] = {
	{ N_("Move to Trash"), "win.trash" },
	{ N_("Delete"), "win.delete" },
};

static const GthMenuEntry folder_context_open_entries[] = {
	{ N_("Open with the File Manager"), "win.folder-context-open-with-fm" }
};


static const GthMenuEntry folder_context_create_entries[] = {
	{ N_("Create Folder"), "win.folder-context-create" }
};


static const GthMenuEntry folder_context_edit_entries[] = {
	{ N_("Cut"), "win.folder-context-cut" },
	{ N_("Copy"), "win.folder-context-copy" },
	{ N_("Paste Into Folder"), "win.folder-context-paste-into-folder" }
};


static const GthMenuEntry folder_context_folder_entries[] = {
	{ N_("Rename"), "win.folder-context-rename" },
	{ N_("Copy to…"), "win.folder-context-copy-to" },
	{ N_("Move to…"), "win.folder-context-move-to" },
	{ N_("Move to Trash"), "win.folder-context-trash" },
	{ N_("Delete"), "win.folder-context-delete" }
};


static const GthMenuEntry vfs_entries[] = {
	{ N_("Duplicate"), "win.duplicate" }
};


static const GthShortcut shortcuts[] = {
	{ "edit-cut", N_("Cut"), GTH_SHORTCUT_CONTEXT_BROWSER | GTH_SHORTCUT_CONTEXT_FIXED, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "<Primary>x" },
	{ "edit-copy", N_("Copy"), GTH_SHORTCUT_CONTEXT_BROWSER | GTH_SHORTCUT_CONTEXT_FIXED, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "<Primary>c" },
	{ "edit-paste", N_("Paste"), GTH_SHORTCUT_CONTEXT_BROWSER | GTH_SHORTCUT_CONTEXT_FIXED, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "<Primary>v" },
	{ "rename", N_("Rename"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "F2" },
	{ "duplicate", N_("Duplicate"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "<Primary>d" },
	{ "remove-from-source", N_("Delete"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "Delete" },
	{ "remove-from-source-permanently", N_("Delete permanently"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "<Shift>Delete" },
	{ "open-with-gimp", N_("Open with Gimp"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "g" },
};


static GtkTargetEntry reorderable_drag_dest_targets[] = {
        { "text/uri-list", 0, 0 },
        { "text/uri-list", GTK_TARGET_SAME_WIDGET, 0 },
	{ "gthumb/reorderable-list", 0, 0 }
};


static GtkTargetEntry reorderable_drag_source_targets[] = {
	{ "gthumb/reorderable-list", 0, 0 }
};


static GtkTargetEntry non_reorderable_drag_dest_targets[] = {
        { "text/uri-list", GTK_TARGET_OTHER_WIDGET, 0 },
        { "XdndDirectSave0", GTK_TARGET_SAME_WIDGET, 1 }
};


typedef struct {
	guint     vfs_merge_id;
	guint     folder_context_open_id;
	guint     folder_context_create_id;
	guint     folder_context_edit_id;
	guint     folder_context_folder_id;
	guint     update_open_menu_id;
	GMenu    *open_with_menu;
	GList    *applications;
	gboolean  can_paste;
	int       drop_pos;
	int       scroll_diff;
	guint     scroll_event;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	if (data->update_open_menu_id > 0) {
		g_source_remove (data->update_open_menu_id);
		data->update_open_menu_id = 0;
	}
	_g_object_unref (data->open_with_menu);
	_g_object_list_unref (data->applications);
	g_free (data);
}


static void
gth_file_list_drag_data_received (GtkWidget        *file_view,
				  GdkDragContext   *context,
				  int               x,
				  int               y,
				  GtkSelectionData *selection_data,
				  guint             info,
				  guint             time,
				  gpointer          user_data)
{
	GthBrowser     *browser = user_data;
	gboolean        success = FALSE;
	char          **uris;
	GList          *selected_files;
	GdkDragAction   action;

	action = gdk_drag_context_get_suggested_action (context);
	if (action == GDK_ACTION_COPY || action == GDK_ACTION_MOVE) {
		success = TRUE;
	}

	if (action == GDK_ACTION_ASK) {
		GdkDragAction actions = _gtk_menu_ask_drag_drop_action (file_view, gdk_drag_context_get_actions (context));
		gdk_drag_status (context, actions, time);
		success = gdk_drag_context_get_selected_action (context) != 0;
	}

	if (gtk_selection_data_get_data_type (selection_data) == XDND_ACTION_DIRECT_SAVE_ATOM) {
		const guchar *data;
		int           format;
		int           length;

		data = gtk_selection_data_get_data (selection_data);
		format = gtk_selection_data_get_format (selection_data);
		length = gtk_selection_data_get_length (selection_data);

		if ((format == 8) && (length == 1) && (data[0] == 'S')) {
			success = TRUE;
		}
		else {
			gdk_property_change (gdk_drag_context_get_dest_window (context),
					     XDND_ACTION_DIRECT_SAVE_ATOM,
					     TEXT_PLAIN_ATOM,
					     8,
					     GDK_PROP_MODE_REPLACE,
					     (const guchar *) "",
					     0);
			success = FALSE;
		}

		gtk_drag_finish (context, success, FALSE, time);
		return;
	}

	gtk_drag_finish (context, success, FALSE, time);
	if (! success)
		return;

	uris = gtk_selection_data_get_uris (selection_data);
	selected_files = _g_file_list_new_from_uriv (uris);
	if (selected_files != NULL) {
		if (gtk_drag_get_source_widget (context) == file_view) {
			GList       *file_data_list;
			GList       *visible_files;
			BrowserData *data;
			GthTask     *task;

			file_data_list = gth_file_store_get_visibles (gth_browser_get_file_store (browser));
			visible_files = gth_file_data_list_to_file_list (file_data_list);

			data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
			task = gth_reorder_task_new (gth_browser_get_location_source (browser),
						     gth_browser_get_location_data (browser),
						     visible_files,
						     selected_files,
						     data->drop_pos);
			gth_browser_exec_task (browser, task, GTH_TASK_FLAGS_DEFAULT);

			g_object_unref (task);
			_g_object_list_unref (visible_files);
			_g_object_list_unref (file_data_list);
		}
		else {
			GthFileSource *file_source;
			gboolean       cancel = FALSE;
			gboolean       move;

			file_source = gth_browser_get_location_source (browser);
			move = gdk_drag_context_get_selected_action (context) == GDK_ACTION_MOVE;
			if (move && ! gth_file_source_can_cut (file_source)) {
				GtkWidget *dialog;
				int        response;

				dialog = _gtk_message_dialog_new (GTK_WINDOW (browser),
								  GTK_DIALOG_MODAL,
								  _GTK_ICON_NAME_DIALOG_QUESTION,
								  _("Could not move the files"),
								  _("Files cannot be moved to the current location, as alternative you can choose to copy them."),
								  _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
								  _GTK_LABEL_COPY, GTK_RESPONSE_OK,
								  NULL);
				response = gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);

				if (response == GTK_RESPONSE_CANCEL)
					cancel = TRUE;

				move = FALSE;
			}

			if (! cancel) {
				GthFileSource *location_source;
				BrowserData   *data;
				GthTask       *task;

				location_source = gth_main_get_file_source (gth_browser_get_location (browser));
				data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
				task = gth_copy_task_new (location_source,
							  gth_browser_get_location_data (browser),
							  move,
							  selected_files,
							  data->drop_pos);
				gth_browser_exec_task (browser, task, GTH_TASK_FLAGS_DEFAULT);

				g_object_unref (task);
				g_object_unref (location_source);
			}
		}
	}

	_g_object_list_unref (selected_files);
	g_strfreev (uris);
}


static gboolean
gth_file_list_drag_drop (GtkWidget      *widget,
			 GdkDragContext *context,
			 gint            x,
			 gint            y,
			 guint           time,
			 gpointer        user_data)
{
	GthBrowser *browser = user_data;
	int         filename_len;
	char       *filename;

	g_signal_stop_emission_by_name (widget, "drag-drop");

	if (gdk_property_get (gdk_drag_context_get_source_window (context),
			      XDND_ACTION_DIRECT_SAVE_ATOM,
			      TEXT_PLAIN_ATOM,
			      0,
			      1024,
			      FALSE,
			      NULL,
			      NULL,
			      &filename_len,
			      (guchar **) &filename)
	    && GTH_IS_FILE_SOURCE_VFS (gth_browser_get_location_source (browser)))
	{
		GFile *file;
		char  *uri;

		filename = g_realloc (filename, filename_len + 1);
		filename[filename_len] = '\0';

		file = _g_file_append_path (gth_browser_get_location (browser), filename);
		uri = g_file_get_uri (file);
		gdk_property_change (gdk_drag_context_get_source_window (context),
				     XDND_ACTION_DIRECT_SAVE_ATOM,
				     TEXT_PLAIN_ATOM,
				     8,
				     GDK_PROP_MODE_REPLACE,
				     (const guchar *) uri,
				     strlen (uri));

		g_free (uri);
		g_object_unref (file);
		g_free (filename);

		gtk_drag_get_data (widget,
		                   context,
		                   XDND_ACTION_DIRECT_SAVE_ATOM,
		                   time);
	}
	else
		gtk_drag_get_data (widget,
				   context,
				   URI_LIST_ATOM,
				   time);

	return TRUE;
}


static gboolean
drag_motion_autoscroll_cb (gpointer user_data)
{
	GthBrowser    *browser = user_data;
	BrowserData   *data;
	GtkAdjustment *adj;
	double         max_value;
	double         value;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	adj = gth_file_list_get_vadjustment (GTH_FILE_LIST (gth_browser_get_file_list (browser)));
	max_value = gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj);
	value = gtk_adjustment_get_value (adj) + data->scroll_diff;
	if (value > max_value)
		value = max_value;
	gtk_adjustment_set_value (adj, value);

	return TRUE;
}


static gboolean
gth_file_list_drag_motion (GtkWidget      *file_view,
			   GdkDragContext *context,
			   gint            x,
			   gint            y,
			   guint           time,
			   gpointer        extra_data)
{
	GthBrowser  *browser = extra_data;
	BrowserData *data;
	GthFileData *location_data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	data->drop_pos = -1;

	if ((gtk_drag_get_source_widget (context) == file_view) && ! gth_file_source_is_reorderable (gth_browser_get_location_source (browser))) {
		gdk_drag_status (context, 0, time);
		return FALSE;
	}

	location_data = gth_browser_get_location_data (browser);
	if (! g_file_info_get_attribute_boolean (location_data->info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE)) {
		gdk_drag_status (context, 0, time);
		return FALSE;
	}

	if (gth_file_source_is_reorderable (gth_browser_get_location_source (browser))) {
		GtkAllocation allocation;

		if (gtk_drag_get_source_widget (context) == file_view)
			gdk_drag_status (context, GDK_ACTION_MOVE, time);
		else
			gdk_drag_status (context, GDK_ACTION_COPY, time);
		gth_file_view_set_drag_dest_pos (GTH_FILE_VIEW (file_view), context, x, y, time, &data->drop_pos);

		gtk_widget_get_allocation (file_view, &allocation);

		if (y < 10)
			data->scroll_diff = - (10 - y);
		else if (y > allocation.height - 10)
			data->scroll_diff = (10 - (allocation.height - y));
		else
			data->scroll_diff = 0;

		if (data->scroll_diff != 0) {
			if (data->scroll_event == 0)
				data->scroll_event = gdk_threads_add_timeout (SCROLL_TIMEOUT, drag_motion_autoscroll_cb, browser);
		}
		else if (data->scroll_event != 0) {
			g_source_remove (data->scroll_event);
			data->scroll_event = 0;
		}
	}
	else if (gdk_drag_context_get_suggested_action (context) == GDK_ACTION_ASK) {
		gdk_drag_status (context, GDK_ACTION_ASK, time);
	}
	else {
		gboolean  source_is_reorderable = FALSE;
		GList    *targets = gdk_drag_context_list_targets (context);
		GList    *scan;

		/* use COPY when dragging a file from a catalog to a directory */

		for (scan = targets; scan; scan = scan->next) {
			GdkAtom target = scan->data;

			if (target == GTHUMB_REORDERABLE_LIST_ATOM) {
				source_is_reorderable = TRUE;
				break;
			}
		}

		gdk_drag_status (context, source_is_reorderable ? GDK_ACTION_COPY : GDK_ACTION_MOVE, time);
	}

	return TRUE;
}


static gboolean
gth_file_list_drag_leave (GtkWidget      *file_view,
			  GdkDragContext *context,
			  guint           time,
			  gpointer        extra_data)
{
	if (gtk_drag_get_source_widget (context) == file_view)
		gth_file_view_set_drag_dest_pos (GTH_FILE_VIEW (file_view), context, -1, -1, time, NULL);

	return TRUE;
}


static void
gth_file_list_drag_end (GtkWidget      *widget,
			GdkDragContext *drag_context,
			gpointer        user_data)
{
	GthBrowser  *browser = user_data;
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	if (data->scroll_event != 0) {
		g_source_remove (data->scroll_event);
		data->scroll_event = 0;
	}
}


/* -- selection_changed -- */


void
gth_browser_activate_open_with_application (GSimpleAction *action,
					    GVariant      *parameter,
					    gpointer       user_data)
{
	GthBrowser          *browser = user_data;
	BrowserData         *data;
	GList               *appinfo_link;
	GAppInfo            *appinfo;
	GList               *items;
	GList               *file_list;
	GList               *uris;
	GList               *scan;
	GdkAppLaunchContext *context;
	GError              *error = NULL;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	appinfo_link = g_list_nth (data->applications, g_variant_get_int32 (parameter));
	g_return_if_fail (appinfo_link != NULL);

	appinfo = appinfo_link->data;
	g_return_if_fail (G_IS_APP_INFO (appinfo));

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	uris = NULL;
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		uris = g_list_prepend (uris, g_file_get_uri (file_data->file));
	}
	uris = g_list_reverse (uris);

	context = gdk_display_get_app_launch_context (gtk_widget_get_display (GTK_WIDGET (browser)));
	gdk_app_launch_context_set_timestamp (context, 0);
	gdk_app_launch_context_set_icon (context, g_app_info_get_icon (appinfo));
	if (! g_app_info_launch_uris (appinfo, uris, G_APP_LAUNCH_CONTEXT (context), &error)) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser),
						    _("Could not perform the operation"),
						    error);
		g_clear_error (&error);
	}

	g_object_unref (context);
	g_list_free (uris);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


static int
sort_app_info_by_display_name (gconstpointer a,
			       gconstpointer b)
{
	GAppInfo *app_info_a = G_APP_INFO (a);
	GAppInfo *app_info_b = G_APP_INFO (b);

	return g_utf8_collate (g_app_info_get_display_name (app_info_a),
			       g_app_info_get_display_name (app_info_b));
}


static void
_gth_browser_update_open_menu (GthBrowser *browser)
{
	BrowserData  *data;
	GList        *items;
	GList        *file_list;
	GList        *scan;
	int           n;
	GHashTable   *used_mime_types;
	GHashTable   *used_apps;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	g_menu_remove_all (data->open_with_menu);
	_g_object_list_unref (data->applications);
	data->applications = NULL;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	data->applications = NULL;
	used_mime_types = g_hash_table_new (g_str_hash, g_str_equal);
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		const char  *mime_type;

		mime_type = gth_file_data_get_mime_type (file_data);
		if ((mime_type == NULL) || g_content_type_is_unknown (mime_type))
			continue;
		if (g_hash_table_lookup (used_mime_types, mime_type) != NULL)
			continue;

		data->applications = g_list_concat (data->applications, g_app_info_get_all_for_type (mime_type));

		g_hash_table_insert (used_mime_types, (gpointer) mime_type, GINT_TO_POINTER (1));
	}
	g_hash_table_destroy (used_mime_types);

	data->applications = g_list_sort (data->applications, sort_app_info_by_display_name);

	used_apps = g_hash_table_new (g_str_hash, g_str_equal);
	for (scan = data->applications, n = 0; scan; scan = scan->next, n++) {
		GAppInfo  *appinfo = scan->data;
		GIcon     *icon;
		GMenuItem *item;

		if (strstr (g_app_info_get_executable (appinfo), "gthumb") != NULL)
			continue;
		if (g_hash_table_lookup (used_apps, g_app_info_get_id (appinfo)) != NULL)
			continue;
		g_hash_table_insert (used_apps, (gpointer) g_app_info_get_id (appinfo), GINT_TO_POINTER (1));

		item = g_menu_item_new (g_app_info_get_display_name (appinfo), NULL);
		g_menu_item_set_action_and_target (item, "win.open-with-application", "i", n);
		icon = g_app_info_get_icon (appinfo);
		if (icon == NULL) {
			icon = g_themed_icon_new ("application-x-executable");
			if (icon != NULL) {
				g_menu_item_set_icon (item, icon);
				g_object_unref (icon);
			}
		}
		else
			g_menu_item_set_icon (item, icon);
		g_menu_append_item (data->open_with_menu, item);

		g_object_unref (item);
	}

	gth_window_enable_action (GTH_WINDOW (browser), "open-with-application", data->applications != NULL);

	g_hash_table_destroy (used_apps);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


static gboolean
update_open_menu_cb (gpointer user_data)
{
	GthBrowser  *browser = user_data;
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_val_if_fail (data != NULL, FALSE);

	if (data->update_open_menu_id > 0) {
		g_source_remove (data->update_open_menu_id);
		data->update_open_menu_id = 0;
	}
	_gth_browser_update_open_menu (GTH_BROWSER (user_data));

	return FALSE;
}


static void
file_selection_changed_cb (GthFileSelection *self,
			   GthBrowser       *browser)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	if (data->update_open_menu_id > 0)
		g_source_remove (data->update_open_menu_id);
	data->update_open_menu_id = g_timeout_add (UPDATE_OPEN_MENU_DELAY, update_open_menu_cb, browser);
}


void
fm__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkWidget   *file_view;
	GMenu       *open_actions;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	data->update_open_menu_id = 0;

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);
	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_EDIT_ACTIONS),
					 fixed_menu_entries_edit,
				         G_N_ELEMENTS (fixed_menu_entries_edit));
	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_FILE_ACTIONS),
					 fixed_menu_entries_file,
				         G_N_ELEMENTS (fixed_menu_entries_file));
	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_DELETE_ACTIONS),
					 fixed_menu_entries_delete,
				         G_N_ELEMENTS (fixed_menu_entries_delete));
	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_FILE_ACTIONS),
					 fixed_menu_entries_file,
					 G_N_ELEMENTS (fixed_menu_entries_file));
	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_DELETE_ACTIONS),
					 fixed_menu_entries_delete,
				         G_N_ELEMENTS (fixed_menu_entries_delete));
	gth_window_add_shortcuts (GTH_WINDOW (browser),
				  shortcuts,
				  G_N_ELEMENTS (shortcuts));

	gth_browser_add_header_bar_button (browser,
					   GTH_BROWSER_HEADER_SECTION_BROWSER_LOCATIONS,
					   "user-home-symbolic",
					   _("Home Folder"),
					   "win.go-home",
					   NULL);

	data->open_with_menu = g_menu_new ();

	open_actions = gth_menu_manager_get_menu (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_OPEN_ACTIONS));
	g_menu_append_submenu (open_actions, _("Open _With"), G_MENU_MODEL (data->open_with_menu));

	open_actions = gth_menu_manager_get_menu (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_OPEN_ACTIONS));
	g_menu_append_submenu (open_actions, _("Open _With"), G_MENU_MODEL (data->open_with_menu));

	gth_window_enable_action (GTH_WINDOW (browser), "edit-paste", FALSE);

	file_view = gth_file_list_get_view (GTH_FILE_LIST (gth_browser_get_file_list (browser)));
	g_signal_connect (file_view,
                          "drag_data_received",
                          G_CALLBACK (gth_file_list_drag_data_received),
                          browser);
	g_signal_connect (file_view,
	                  "drag_drop",
	                  G_CALLBACK (gth_file_list_drag_drop),
	                  browser);
	g_signal_connect (file_view,
			  "drag_motion",
			  G_CALLBACK (gth_file_list_drag_motion),
			  browser);
	g_signal_connect (file_view,
	                  "drag_leave",
	                  G_CALLBACK (gth_file_list_drag_leave),
	                  browser);
	g_signal_connect (file_view,
	                  "drag_end",
	                  G_CALLBACK (gth_file_list_drag_end),
	                  browser);
	g_signal_connect (file_view,
			  "file-selection-changed",
			  G_CALLBACK (file_selection_changed_cb),
			  browser);

	file_view = gth_file_list_get_empty_view (GTH_FILE_LIST (gth_browser_get_file_list (browser)));
	g_signal_connect (file_view,
                          "drag_data_received",
                          G_CALLBACK (gth_file_list_drag_data_received),
                          browser);
	g_signal_connect (file_view,
	                  "drag_drop",
	                  G_CALLBACK (gth_file_list_drag_drop),
	                  browser);
	g_signal_connect (file_view,
			  "drag_motion",
			  G_CALLBACK (gth_file_list_drag_motion),
			  browser);
	g_signal_connect (file_view,
	                  "drag_leave",
	                  G_CALLBACK (gth_file_list_drag_leave),
	                  browser);
	g_signal_connect (file_view,
	                  "drag_end",
	                  G_CALLBACK (gth_file_list_drag_end),
	                  browser);

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


static void
file_manager_update_ui (BrowserData *data,
			GthBrowser  *browser)
{
	if (GTH_IS_FILE_SOURCE_VFS (gth_browser_get_location_source (browser))) {
		if (data->vfs_merge_id == 0)
			data->vfs_merge_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_FILE_ACTIONS),
									 vfs_entries,
									 G_N_ELEMENTS (vfs_entries));
	}
	else {
		gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_FILE_ACTIONS), data->vfs_merge_id);
		data->vfs_merge_id = 0;
	}
}


void
fm__gth_browser_set_current_page_cb (GthBrowser *browser)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	file_manager_update_ui (data, browser);
}


void
fm__gth_browser_load_location_after_cb (GthBrowser   *browser,
					GthFileData  *location_data)
{
	BrowserData    *data;
	GtkWidget      *file_list;
	GtkWidget      *file_view;
	GtkTargetList  *source_target_list;
	GtkTargetEntry *source_targets;
	int             n_source_targets;
	GdkDragAction   source_actions;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	file_manager_update_ui (data, browser);

	source_target_list = gtk_target_list_new (NULL, 0);
	gtk_target_list_add_uri_targets (source_target_list, 0);
	gtk_target_list_add_text_targets (source_target_list, 0);
	source_actions = GDK_ACTION_PRIVATE;

	file_list = gth_browser_get_file_list (browser);
	if (! g_file_info_get_attribute_boolean (location_data->info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE)) {
		file_view = gth_file_list_get_view (GTH_FILE_LIST (file_list));
		gth_file_view_unset_drag_dest (GTH_FILE_VIEW (file_view));
		file_view = gth_file_list_get_empty_view (GTH_FILE_LIST (file_list));
		gtk_drag_dest_unset (file_view);

		source_actions = GDK_ACTION_COPY;
	}
	else if (gth_file_source_is_reorderable (gth_browser_get_location_source (browser))) {
		file_view = gth_file_list_get_view (GTH_FILE_LIST (file_list));
		gth_file_view_enable_drag_dest (GTH_FILE_VIEW (file_view),
						reorderable_drag_dest_targets,
						G_N_ELEMENTS (reorderable_drag_dest_targets),
						GDK_ACTION_COPY | GDK_ACTION_MOVE);
		file_view = gth_file_list_get_empty_view (GTH_FILE_LIST (file_list));
		gtk_drag_dest_set (file_view,
				   0,
				   reorderable_drag_dest_targets,
				   G_N_ELEMENTS (reorderable_drag_dest_targets),
				   GDK_ACTION_COPY | GDK_ACTION_MOVE);

		gtk_target_list_add_table (source_target_list,
					   reorderable_drag_source_targets,
					   G_N_ELEMENTS (reorderable_drag_source_targets));
		source_actions = GDK_ACTION_COPY | GDK_ACTION_MOVE;
	}
	else {
		file_view = gth_file_list_get_view (GTH_FILE_LIST (file_list));
		gth_file_view_enable_drag_dest (GTH_FILE_VIEW (file_view),
						non_reorderable_drag_dest_targets,
						G_N_ELEMENTS (non_reorderable_drag_dest_targets),
						GDK_ACTION_COPY | GDK_ACTION_MOVE);
		file_view = gth_file_list_get_empty_view (GTH_FILE_LIST (file_list));
		gtk_drag_dest_set (file_view,
				   0,
				   non_reorderable_drag_dest_targets,
				   G_N_ELEMENTS (non_reorderable_drag_dest_targets),
				   GDK_ACTION_COPY | GDK_ACTION_MOVE);

		source_actions = GDK_ACTION_MOVE | GDK_ACTION_ASK;
	}

	/* set the drag source targets */

	source_targets = gtk_target_table_new_from_list (source_target_list, &n_source_targets);
	gth_file_view_enable_drag_source (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (file_list))),
					  GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
					  source_targets,
					  n_source_targets,
					  source_actions);

	gtk_target_list_unref (source_target_list);
	gtk_target_table_free (source_targets, n_source_targets);
}


void
fm__gth_browser_folder_tree_popup_before_cb (GthBrowser    *browser,
					     GthFileSource *file_source,
					     GthFileData   *folder)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	if (GTH_IS_FILE_SOURCE_VFS (file_source)) {
		if (data->folder_context_open_id == 0)
			data->folder_context_open_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_OPEN_ACTIONS),
									 folder_context_open_entries,
									 G_N_ELEMENTS (folder_context_open_entries));
		if (data->folder_context_create_id == 0)
			data->folder_context_create_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_CREATE_ACTIONS),
									 folder_context_create_entries,
									 G_N_ELEMENTS (folder_context_create_entries));
		if (data->folder_context_edit_id == 0)
			data->folder_context_edit_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_EDIT_ACTIONS),
									 folder_context_edit_entries,
									 G_N_ELEMENTS (folder_context_edit_entries));
		if (data->folder_context_folder_id == 0)
			data->folder_context_folder_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_FOLDER_ACTIONS),
									 folder_context_folder_entries,
									 G_N_ELEMENTS (folder_context_folder_entries));

		fm__gth_browser_update_sensitivity_cb (browser);
	}
	else {
		if (data->folder_context_open_id != 0)
			gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_OPEN_ACTIONS), data->folder_context_open_id);
		if (data->folder_context_create_id != 0)
			gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_CREATE_ACTIONS), data->folder_context_create_id);
		if (data->folder_context_edit_id != 0)
			gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_EDIT_ACTIONS), data->folder_context_edit_id);
		if (data->folder_context_folder_id != 0)
			gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_FOLDER_ACTIONS), data->folder_context_folder_id);
		data->folder_context_open_id = 0;
		data->folder_context_create_id = 0;
		data->folder_context_edit_id = 0;
		data->folder_context_folder_id = 0;
	}
}


void
fm__gth_browser_folder_tree_drag_data_received_cb (GthBrowser    *browser,
						   GthFileData   *destination,
						   GList         *file_list,
						   GdkDragAction  action)
{
	int            n_files;
	GthFileSource *destination_source;
	GFile         *first_file;
	GthFileSource *file_list_source;
	gboolean       move_files;
	GtkWidget     *dialog;
	GthTask       *task;
	char          *message;
	int            response;

	if (destination == NULL)
		return;

	n_files = g_list_length (file_list);
	if (n_files == 0)
		return;

	if ((action != GDK_ACTION_MOVE) && (action != GDK_ACTION_COPY))
		return;

	destination_source = gth_main_get_file_source (destination->file);
	if (destination_source == NULL)
		return;

	first_file = G_FILE (file_list->data);
	file_list_source = gth_main_get_file_source (first_file);
	if (file_list_source == NULL)
		return;

	if (action == GDK_ACTION_MOVE)
		action |= GDK_ACTION_COPY;

	action = action & gth_file_source_get_drop_actions (destination_source, destination->file, first_file);
	if (action == 0) {
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       "%s",
				       _("Could not perform the operation"));
		return;
	}

	move_files = (action & GDK_ACTION_MOVE) != 0;

	/* ask confirmation */

	response = GTK_RESPONSE_OK;
	if (n_files == 1) {
		GFileInfo  *info;
		char       *filename;

		info = gth_file_source_get_file_info (file_list_source, first_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);
		if (info != NULL)
			filename = g_strdup (g_file_info_get_display_name (info));
		else
			filename = _g_file_get_display_name (first_file);

		if (move_files)
			message = g_strdup_printf (_("Do you want to move “%s” to “%s”?"), filename, g_file_info_get_display_name (destination->info));
		else
			message = g_strdup_printf (_("Do you want to copy “%s” to “%s”?"), filename, g_file_info_get_display_name (destination->info));

		g_free (filename);
		_g_object_unref (info);
	}
	else {
		if (move_files)
			message = g_strdup_printf (_("Do you want to move the dragged files to “%s”?"), g_file_info_get_display_name (destination->info));
		else
			message = g_strdup_printf (_("Do you want to copy the dragged files to “%s”?"), g_file_info_get_display_name (destination->info));
	}
	dialog = _gtk_message_dialog_new (GTK_WINDOW (browser),
					  GTK_DIALOG_MODAL,
					  _GTK_ICON_NAME_DIALOG_QUESTION,
					  message,
					  NULL,
					  _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
					  (move_files ? _("Move") : _("_Copy")), GTK_RESPONSE_OK,
					  NULL);
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (message);

	if (response != GTK_RESPONSE_OK)
		return;

	/* exec task */

	task = gth_copy_task_new (destination_source,
				  destination,
				  move_files,
				  file_list,
				  -1);
	gth_browser_exec_task (browser, task, GTH_TASK_FLAGS_DEFAULT);

	g_object_unref (task);
	g_object_unref (destination_source);
}


void
fm__gth_browser_folder_tree_selection_changed_cb (GthBrowser *browser)
{
	fm__gth_browser_update_sensitivity_cb (browser);
}


static void
clipboard_targets_received_cb (GtkClipboard *clipboard,
			       GdkAtom      *atoms,
                               int           n_atoms,
                               gpointer      user_data)
{
	GthBrowser  *browser = user_data;
	BrowserData *data;
	int          i;
	GthFileData *folder;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	data->can_paste = FALSE;
	for (i = 0; ! data->can_paste && (i < n_atoms); i++)
		if (atoms[i] == GNOME_COPIED_FILES)
			data->can_paste = TRUE;

	gth_window_enable_action (GTH_WINDOW (browser), "edit-paste", data->can_paste);

	folder = gth_browser_get_folder_popup_file_data (browser);
	gth_window_enable_action (GTH_WINDOW (browser), "folder-context-paste-into-folder", (folder != NULL) && data->can_paste && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE));

	_g_object_unref (folder);
	g_object_unref (browser);
}


static void
_gth_browser_update_paste_command_sensitivity (GthBrowser   *browser,
                                               GtkClipboard *clipboard)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	data->can_paste = FALSE;
	gth_window_enable_action (GTH_WINDOW (browser), "edit-paste", data->can_paste);

	if (clipboard == NULL)
		clipboard = gtk_widget_get_clipboard (GTK_WIDGET (browser), GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_request_targets (clipboard,
				       clipboard_targets_received_cb,
				       g_object_ref (browser));
}


void
fm__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	BrowserData   *data;
	GthFileSource *file_source;
	int            n_selected;
	gboolean       sensitive;
	GthFileData   *folder;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	file_source = gth_browser_get_location_source (browser);
	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));

	sensitive = (n_selected > 0) && (file_source != NULL) && gth_file_source_can_cut (file_source);
	gth_window_enable_action (GTH_WINDOW (browser), "edit-cut", sensitive);

	sensitive = (n_selected > 0) && (file_source != NULL);
	gth_window_enable_action (GTH_WINDOW (browser), "edit-copy", sensitive);
	gth_window_enable_action (GTH_WINDOW (browser), "trash", sensitive);
	gth_window_enable_action (GTH_WINDOW (browser), "delete", sensitive);
	gth_window_enable_action (GTH_WINDOW (browser), "duplicate", sensitive);
	gth_window_enable_action (GTH_WINDOW (browser), "move-to-folder", sensitive);
	gth_window_enable_action (GTH_WINDOW (browser), "copy-to-folder", sensitive);
	gth_window_enable_action (GTH_WINDOW (browser), "file-list-rename", n_selected > 0);

	folder = gth_browser_get_folder_popup_file_data (browser);
	gth_window_enable_action (GTH_WINDOW (browser), "folder-context-create", (folder != NULL) && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE));
	gth_window_enable_action (GTH_WINDOW (browser), "folder-context-rename", (folder != NULL) && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME));
	gth_window_enable_action (GTH_WINDOW (browser), "folder-context-delete", (folder != NULL) && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE));
	gth_window_enable_action (GTH_WINDOW (browser), "folder-context-trash", (folder != NULL) && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH));
	gth_window_enable_action (GTH_WINDOW (browser), "folder-context-cut", (folder != NULL) && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE));
	gth_window_enable_action (GTH_WINDOW (browser), "folder-context-move-to", (folder != NULL) && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE));
	gth_window_enable_action (GTH_WINDOW (browser), "rename", ((folder != NULL) && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE)) || (n_selected > 0));
	gth_window_enable_action (GTH_WINDOW (browser), "folder-context-copy", (folder != NULL) && (g_file_info_get_file_type (folder->info) != G_FILE_TYPE_MOUNTABLE));
	gth_window_enable_action (GTH_WINDOW (browser), "folder-context-copy-to", (folder != NULL) && (g_file_info_get_file_type (folder->info) != G_FILE_TYPE_MOUNTABLE));

	_g_object_unref (folder);

	_gth_browser_update_paste_command_sensitivity (browser, NULL);
}


static void
clipboard_owner_change_cb (GtkClipboard *clipboard,
                           GdkEvent     *event,
                           gpointer      user_data)
{
	_gth_browser_update_paste_command_sensitivity ((GthBrowser *) user_data, clipboard);
}


void
fm__gth_browser_realize_cb (GthBrowser *browser)
{
	GtkClipboard *clipboard;

	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	g_signal_connect (clipboard,
	                  "owner_change",
	                  G_CALLBACK (clipboard_owner_change_cb),
	                  browser);
}


void
fm__gth_browser_unrealize_cb (GthBrowser *browser)
{
	GtkClipboard *clipboard;

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (browser), GDK_SELECTION_CLIPBOARD);
	g_signal_handlers_disconnect_by_func (clipboard,
	                                      G_CALLBACK (clipboard_owner_change_cb),
	                                      browser);
}
