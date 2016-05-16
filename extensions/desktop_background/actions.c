/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Pix
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
#include <pix.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>


#define DESKTOP_BACKGROUND_PROPERTIES_COMMAND "gnome-control-center background"
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


static GFile *
get_wallpaper_file_n (int n)
{
	char  *name;
	GFile *file;

	name = g_strdup_printf ("wallpaper%d.jpeg", n);
	file = gth_user_dir_get_file_for_write (GTH_DIR_DATA, PIX_DIR, name, NULL);

	g_free (name);

	return file;
}


static GFile *
get_new_wallpaper_file (void)
{
	GFile *wallpaper_file;

	wallpaper_file = get_wallpaper_file_n (1);
	if (g_file_query_exists (wallpaper_file, NULL)) {
		/* Use a new filename to force an update. */

		g_object_unref (wallpaper_file);

		wallpaper_file = get_wallpaper_file_n (2);
		if (g_file_query_exists (wallpaper_file, NULL))
			g_file_delete (wallpaper_file, NULL, NULL);
	}

	return wallpaper_file;
}


static WallpaperData *
wallpaper_data_new (GthBrowser *browser)
{
	WallpaperData *wdata;

	wdata = g_new0 (WallpaperData, 1);
	wdata->browser = browser;
	wallpaper_style_init_from_current (&wdata->old_style);
	wallpaper_style_init (&wdata->new_style);
	wdata->new_style.file = get_new_wallpaper_file ();

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
			control_center_command = "unity-control-center appearance";
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
wallpaper_data_set (WallpaperData *wdata)
{
	GtkWidget *infobar;

	wallpaper_style_set_as_current (&wdata->new_style);

	infobar = gth_browser_get_infobar (wdata->browser);
	gth_info_bar_set_icon (GTH_INFO_BAR (infobar), GTK_STOCK_DIALOG_INFO);

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
				  GTK_STOCK_PREFERENCES, _RESPONSE_PREFERENCES,
				  GTK_STOCK_UNDO, _RESPONSE_UNDO,
				  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
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
save_wallpaper_task_completed_cb (GthTask  *task,
				  GError   *error,
				  gpointer  user_data)
{
	WallpaperData *wdata = user_data;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (wdata->browser), _("Could not set the desktop background"), error);
		wallpaper_data_free (wdata);
		return;
	}

	wallpaper_data_set (wdata);
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


void
gth_browser_activate_action_tool_desktop_background (GtkAction  *action,
					             GthBrowser *browser)
{
	WallpaperData *wdata;
	gboolean       saving_wallpaper = FALSE;
	GList         *items;
	GList         *file_list;
	GthFileData   *file_data;
	const char    *mime_type;

	wdata = wallpaper_data_new (browser);

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	file_data = (file_list != NULL) ? file_list->data : NULL;
	mime_type = (file_data != NULL) ? gth_file_data_get_mime_type (file_data) : NULL;

	if (gth_main_extension_is_active ("image_viewer")
	    && (gth_browser_get_file_modified (browser) || ! _gdk_pixbuf_mime_type_is_readable (mime_type)))
	{
		GtkWidget *viewer_page;

		viewer_page = gth_browser_get_viewer_page (browser);
		if (viewer_page != NULL) {
			GthImage *image;
			GthTask  *task;

			image = gth_image_new_for_surface (gth_image_viewer_page_get_image (GTH_IMAGE_VIEWER_PAGE (viewer_page)));
			file_data = gth_file_data_new (wdata->new_style.file, NULL);
			task = gth_save_image_task_new (image,
							"image/jpeg",
							file_data,
							GTH_OVERWRITE_RESPONSE_YES);
			g_signal_connect (task,
					  "completed",
					  G_CALLBACK (save_wallpaper_task_completed_cb),
					  wdata);
			gth_browser_exec_task (GTH_BROWSER (browser), task, FALSE);

			saving_wallpaper = TRUE;

			_g_object_unref (task);
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
