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
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "actions.h"


#define DESKTOP_BACKGROUND_PROPERTIES_COMMAND "gnome-control-center background"
#define DESKTOP_BACKGROUND_PROPERTIES_UNITY_COMMAND "unity-control-center appearance"
#define DESKTOP_BACKGROUND_SCHEMA "org.gnome.desktop.background"
#define DESKTOP_BACKGROUND_FILE_KEY "picture-uri"
#define DESKTOP_BACKGROUND_STYLE_KEY "picture-options"


typedef enum {
	BACKGROUND_STYLE_NONE,
	BACKGROUND_STYLE_WALLPAPER,
	BACKGROUND_STYLE_CENTERED,
	BACKGROUND_STYLE_SCALED,
	BACKGROUND_STYLE_STRETCHED,
	BACKGROUND_STYLE_ZOOM,
	BACKGROUND_STYLE_SPANNED
} BackgroundStyle;


typedef struct {
	GFile           *file;
	BackgroundStyle  background_style;
} WallpaperStyle;


typedef struct {
	GthBrowser     *browser;
	WallpaperStyle  old_style;
	WallpaperStyle  new_style;
	gulong          response_id;
} WallpaperData;


static void
wallpaper_style_init (WallpaperStyle *style)
{
	style->file = NULL;
	style->background_style = BACKGROUND_STYLE_WALLPAPER;
}


static void
wallpaper_style_init_from_current (WallpaperStyle *style)
{
	GSettings *settings;
	char      *uri;

	settings = g_settings_new (DESKTOP_BACKGROUND_SCHEMA);
	uri = g_settings_get_string (settings, DESKTOP_BACKGROUND_FILE_KEY);
	style->file = (uri != NULL) ? g_file_new_for_uri (uri) : NULL;
	style->background_style = g_settings_get_enum (settings, DESKTOP_BACKGROUND_STYLE_KEY);

	g_free (uri);
	g_object_unref (settings);
}


static void
wallpaper_style_set_as_current (WallpaperStyle *style)
{
	char *uri;

	if (style->file == NULL)
		return;

	uri = g_file_get_uri (style->file);
	if (uri != NULL) {
		GSettings *settings;

		settings = g_settings_new (DESKTOP_BACKGROUND_SCHEMA);
		g_settings_set_string (settings, DESKTOP_BACKGROUND_FILE_KEY, uri);
		g_settings_set_enum (settings, DESKTOP_BACKGROUND_STYLE_KEY, style->background_style);

		g_object_unref (settings);
	}

	g_free (uri);
}


static void
wallpaper_style_free (WallpaperStyle *style)
{
	_g_object_unref (style->file);
	wallpaper_style_init (style);
}


/* -- get_new_wallpaper_file_async -- */


typedef struct {
	GFile  *folder;
	int     max_n;
	GList  *wallpaper_files;
	GRegex *wallpaper_name_regex;
} NewWallpaperData;


static void
new_wallpaper_data_free (NewWallpaperData *data)
{
	_g_object_unref (data->folder);
	_g_string_list_free (data->wallpaper_files);
	g_regex_unref (data->wallpaper_name_regex);
	g_slice_free (NewWallpaperData, data);
}


static void
nw_for_each_file_func (GFile     *file,
		       GFileInfo *info,
		       gpointer   user_data)
{
	GTask             *task = user_data;
	NewWallpaperData  *nw_data = g_task_get_task_data (task);
	const char        *filename;
	char             **tokens;

	filename = g_file_info_get_name (info);
	tokens = g_regex_split (nw_data->wallpaper_name_regex, filename, 0);
	if (g_strv_length (tokens) >= 2) {
		int n = atoi (tokens[1]);
		if (n > nw_data->max_n)
			nw_data->max_n = n;
		nw_data->wallpaper_files = g_list_prepend (nw_data->wallpaper_files, g_strdup (filename));
	}

	g_strfreev (tokens);
}


static void
nw_done_func (GError     *error,
	      gpointer    user_data)
{
	GTask            *task = user_data;
	NewWallpaperData *nw_data = g_task_get_task_data (task);
	GList            *scan;
	char             *display_name;
	GFile            *proposed_file;

	if (error != NULL) {
		g_task_return_error (task, error);
		g_object_unref (task);
		return;
	}

	/* delete the old wallpapers */
	for (scan = nw_data->wallpaper_files; scan; scan = scan->next) {
		char  *name = scan->data;
		GFile *file;

		file = g_file_get_child (nw_data->folder, name);
		g_file_delete (file, NULL, NULL);

		g_object_unref (file);
	}

	/* use a unique name to force a reload */
	display_name = g_strdup_printf ("wallpaper%d.jpeg", nw_data->max_n + 1);
	proposed_file = g_file_get_child_for_display_name (nw_data->folder, display_name, NULL);
	g_task_return_pointer (task, proposed_file, g_object_unref);

	g_free (display_name);
	g_object_unref (task);
}


static void
get_new_wallpaper_file_async (GCancellable        *cancellable,
			      GAsyncReadyCallback  callback,
			      gpointer             user_data)
{
	NewWallpaperData *nw_data;
	GTask            *task;

	nw_data = g_slice_new (NewWallpaperData);
	nw_data->folder = gth_user_dir_get_dir_for_write (GTH_DIR_DATA, GTHUMB_DIR, NULL);
	nw_data->max_n = 0;
	nw_data->wallpaper_files = NULL;
	nw_data->wallpaper_name_regex = g_regex_new ("wallpaper([0-9]+).jpeg", 0, 0, NULL);

	task = g_task_new (NULL, cancellable, callback, user_data);
	g_task_set_task_data (task, nw_data, (GDestroyNotify) new_wallpaper_data_free);

	_g_directory_foreach_child (nw_data->folder,
				   FALSE,
				   FALSE,
				   GFILE_NAME_TYPE_ATTRIBUTES,
				   cancellable,
				   NULL,
				   nw_for_each_file_func,
				   nw_done_func,
				   task);
}


static GFile *
get_new_wallpaper_file_finish (GAsyncResult  *result,
                               GError       **error)
{
	return g_task_propagate_pointer (G_TASK (result), error);
}


static WallpaperData *
wallpaper_data_new (GthBrowser *browser,
		    GFile      *wallpaper_file)
{
	WallpaperData *wdata;

	wdata = g_new0 (WallpaperData, 1);
	wdata->browser = browser;
	wallpaper_style_init_from_current (&wdata->old_style);
	wallpaper_style_init (&wdata->new_style);
	wdata->new_style.file = g_object_ref (wallpaper_file);

	return wdata;
}


static void
wallpaper_data_free (WallpaperData *wdata)
{
	g_signal_handler_disconnect (gth_browser_get_infobar (wdata->browser), wdata->response_id);
	wallpaper_style_free (&wdata->old_style);
	wallpaper_style_free (&wdata->new_style);
	g_free (wdata);
}


enum {
	_RESPONSE_PREFERENCES = 1,
	_RESPONSE_UNDO
};


static void
infobar_response_cb (GtkInfoBar *info_bar,
		     int         response_id,
		     gpointer    user_data)
{
	WallpaperData *wdata = user_data;
	gchar         *path;
	const gchar   *control_center_command;
	GError        *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (wdata->browser));

	switch (response_id) {
	case _RESPONSE_PREFERENCES:
		path = g_find_program_in_path ("unity-control-center");
		if (path && g_strcmp0 (g_getenv ("XDG_CURRENT_DESKTOP"), "Unity") == 0)
			control_center_command = DESKTOP_BACKGROUND_PROPERTIES_UNITY_COMMAND;
		else
			control_center_command = DESKTOP_BACKGROUND_PROPERTIES_COMMAND;
		g_free (path);
		if (! g_spawn_command_line_async (control_center_command, &error)) {
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (wdata->browser), _("Could not show the desktop background properties"), error);
			g_clear_error (&error);
		}
		break;

	case _RESPONSE_UNDO:
		wallpaper_style_set_as_current (&wdata->old_style);
		break;
	}

	gtk_widget_hide (GTK_WIDGET (info_bar));
	wallpaper_data_free (wdata);
}


static void
wallpaper_data_set__step2 (WallpaperData *wdata)
{
	GtkWidget *infobar;

	wallpaper_style_set_as_current (&wdata->new_style);

	infobar = gth_browser_get_infobar (wdata->browser);
	gth_info_bar_set_icon_name (GTH_INFO_BAR (infobar), "dialog-information-symbolic", GTK_ICON_SIZE_DIALOG);

	{
		char *name;
		char *msg;

		name = _g_file_get_display_name (wdata->new_style.file);
		msg = g_strdup_printf ("The image \"%s\" has been set as desktop background", name);
		gth_info_bar_set_primary_text (GTH_INFO_BAR (infobar), msg);

		g_free (msg);
		g_free (name);
	}

	_gtk_info_bar_clear_action_area (GTK_INFO_BAR (infobar));
	gtk_orientable_set_orientation (GTK_ORIENTABLE (gtk_info_bar_get_action_area (GTK_INFO_BAR (infobar))), GTK_ORIENTATION_HORIZONTAL);
	gtk_info_bar_set_message_type (GTK_INFO_BAR (infobar), GTK_MESSAGE_INFO);
	gtk_info_bar_add_buttons (GTK_INFO_BAR (infobar),
				  _("_Preferences"), _RESPONSE_PREFERENCES,
				  _("_Undo"), _RESPONSE_UNDO,
				  _GTK_LABEL_CLOSE, GTK_RESPONSE_CLOSE,
				  NULL);
	gtk_info_bar_set_response_sensitive (GTK_INFO_BAR (infobar),
					     _RESPONSE_UNDO,
					     wdata->old_style.file != NULL);
	wdata->response_id = g_signal_connect (infobar,
			  	  	       "response",
			  	  	       G_CALLBACK (infobar_response_cb),
			  	  	       wdata);

	gtk_widget_show (infobar);
}


static void
wallpaper_metadata_ready_cb (GObject      *source_object,
			     GAsyncResult *result,
			     gpointer      user_data)
{
	WallpaperData *wdata = user_data;
	GList         *file_list;
	GError        *error = NULL;
	GthFileData   *file_data;
	int            image_width;
	int            image_height;
	GdkRectangle   monitor_geometry;

	file_list = _g_query_metadata_finish (result, &error);
	if (error != NULL) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (wdata->browser), _("Could not set the desktop background"), error);
		wallpaper_data_free (wdata);
		return;
	}

	/* WALLPAPER handles most of the cases correctly */

	wdata->new_style.background_style = BACKGROUND_STYLE_WALLPAPER;

	/* use ZOOM if the image has a size similar to the monitor's */

#if GTK_CHECK_VERSION(3, 22, 0)

	gdk_monitor_get_geometry (gdk_display_get_monitor_at_window (gtk_widget_get_display (GTK_WIDGET (wdata->browser)), gtk_widget_get_window (GTK_WIDGET (wdata->browser))), &monitor_geometry);

#else

	gdk_screen_get_monitor_geometry (gtk_widget_get_screen (GTK_WIDGET (wdata->browser)), 0, &monitor_geometry);

#endif

	file_data = file_list->data;
	image_width = g_file_info_get_attribute_int32 (file_data->info, "image::width");
	image_height = g_file_info_get_attribute_int32 (file_data->info, "image::height");

	if ((image_width >= monitor_geometry.width / 2) && (image_height >= monitor_geometry.height / 2))
		wdata->new_style.background_style = BACKGROUND_STYLE_ZOOM;

	wallpaper_data_set__step2 (wdata);
}


static void
wallpaper_data_set (WallpaperData *wdata)
{
	GList *file_list;

	file_list = g_list_append (NULL, gth_file_data_new (wdata->new_style.file, NULL));
	_g_query_metadata_async (file_list,
			         "image::width,image::height",
			         NULL,
			         wallpaper_metadata_ready_cb,
			         wdata);

	_g_object_list_unref (file_list);
}


static void
save_wallpaper_task_completed_cb (GthTask  *task,
				  GError   *error,
				  gpointer  user_data)
{
	WallpaperData *wdata = user_data;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (wdata->browser), _("Could not set the desktop background"), error);
		wallpaper_data_free (wdata);
	}
	else
		wallpaper_data_set (wdata);

	_g_object_unref (task);
}


static void
copy_wallpaper_ready_cb (GObject      *source_object,
			 GAsyncResult *res,
			 gpointer      user_data)
{
	WallpaperData *wdata = user_data;
	GError        *error = NULL;

	if (! g_file_copy_finish (G_FILE (source_object), res, &error)) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (wdata->browser), _("Could not set the desktop background"), error);
		g_clear_error (&error);
		wallpaper_data_free (wdata);
		return;
	}

	wallpaper_data_set (wdata);
}


static void
wallpaper_file_read_cb (GObject      *source_object,
		        GAsyncResult *res,
			gpointer      user_data)
{
	GthBrowser     *browser = GTH_BROWSER (user_data);
	GFile          *wallpaper_file;
	GError        **error = NULL;
	WallpaperData  *wdata;
	gboolean        saving_wallpaper = FALSE;
	GList          *items;
	GList          *file_list;
	GthFileData    *file_data;
	const char     *mime_type;

	wallpaper_file = get_new_wallpaper_file_finish (res, error);
	if (wallpaper_file == NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser),  _("Could not set the desktop background"), *error);
		g_clear_error (error);
		return;
	}

	wdata = wallpaper_data_new (browser, wallpaper_file);
	g_object_unref (wallpaper_file);

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	file_data = (file_list != NULL) ? file_list->data : NULL;
	mime_type = (file_data != NULL) ? gth_file_data_get_mime_type (file_data) : NULL;

	if (gth_main_extension_is_active ("image_viewer")
	    && (gth_browser_get_file_modified (browser) || ! _gdk_pixbuf_mime_type_is_readable (mime_type)))
	{
		GthViewerPage *viewer_page;

		viewer_page = gth_browser_get_viewer_page (browser);
		if (viewer_page != NULL) {
			GthImage *image;
			GthTask  *task;

			if (gth_image_viewer_page_get_is_modified (GTH_IMAGE_VIEWER_PAGE (viewer_page)))
				image = gth_image_new_for_surface (gth_image_viewer_page_get_modified_image (GTH_IMAGE_VIEWER_PAGE (viewer_page)));
			else
				image = gth_image_new_for_surface (gth_image_viewer_page_get_current_image (GTH_IMAGE_VIEWER_PAGE (viewer_page)));
			file_data = gth_file_data_new (wdata->new_style.file, NULL);
			task = gth_save_image_task_new (image,
							"image/jpeg",
							file_data,
							GTH_OVERWRITE_RESPONSE_YES);
			g_signal_connect (task,
					  "completed",
					  G_CALLBACK (save_wallpaper_task_completed_cb),
					  wdata);
			gth_browser_exec_task (GTH_BROWSER (browser), task, GTH_TASK_FLAGS_IGNORE_ERROR);

			saving_wallpaper = TRUE;

			g_object_unref (image);
		}
	}

	if (saving_wallpaper)
		return;

	if (file_data == NULL)
		return;

	if (g_file_is_native (file_data->file)) {
		_g_object_unref (wdata->new_style.file);
		wdata->new_style.file = g_file_dup (file_data->file);
		wallpaper_data_set (wdata);
	}
	else
		g_file_copy_async (file_data->file,
				   wdata->new_style.file,
				   G_FILE_COPY_OVERWRITE,
				   G_PRIORITY_DEFAULT,
				   NULL,
				   NULL,
				   NULL,
				   copy_wallpaper_ready_cb,
				   wdata);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_set_desktop_background (GSimpleAction *action,
					     GVariant      *parameter,
					     gpointer       user_data)
{
	get_new_wallpaper_file_async (NULL, wallpaper_file_read_cb, user_data);
}
