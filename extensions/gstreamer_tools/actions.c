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
#include <gst/gst.h>
#include <gthumb.h>
#include <extensions/gstreamer_utils/gstreamer-utils.h>
#include "gth-media-viewer-page.h"
#include "preferences.h"

#define MAX_ATTEMPTS 1024


/* -- media_viewer_activate_action_screenshot -- */


typedef struct {
	GthBrowser         *browser;
	GSettings          *settings;
	GthMediaViewerPage *page;
	gboolean            playing_before_screenshot;
	GthImage           *image;
	GthFileData        *file_data;
} SaveData;


static void
save_date_free (SaveData *save_data)
{
	_g_object_unref (save_data->file_data);
	_g_object_unref (save_data->image);
	_g_object_unref (save_data->settings);
	g_free (save_data);
}


static void
save_screenshot_task_completed_cb (GthTask  *task,
				   GError   *error,
				   gpointer  user_data)
{
	SaveData           *save_data = user_data;
	GthMediaViewerPage *page = save_data->page;

	if (error != NULL)
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (save_data->browser), _("Could not save the file"), error);
	else if (save_data->playing_before_screenshot)
		gst_element_set_state (gth_media_viewer_page_get_playbin (page), GST_STATE_PLAYING);

	save_date_free (save_data);
	g_object_unref (task);
}


static void
save_as_response_cb (GtkDialog  *file_sel,
		     int         response,
		     SaveData   *save_data)
{
	GFile      *file;
	const char *mime_type;
	GFile      *folder;
	char       *folder_uri;
	GthTask    *task;

	if (response != GTK_RESPONSE_OK) {
		GthMediaViewerPage *page = save_data->page;

		if (save_data->playing_before_screenshot)
			gst_element_set_state (gth_media_viewer_page_get_playbin (page), GST_STATE_PLAYING);
		save_date_free (save_data);
		gtk_widget_destroy (GTK_WIDGET (file_sel));
		return;
	}

	if (! gth_file_chooser_dialog_get_file (GTH_FILE_CHOOSER_DIALOG (file_sel), &file, &mime_type))
		return;

	folder = g_file_get_parent (file);
	folder_uri = g_file_get_uri (folder);
	g_settings_set_string (save_data->settings, PREF_GSTREAMER_TOOLS_SCREESHOT_LOCATION, folder_uri);

	save_data->file_data = gth_file_data_new (file, NULL);
	gth_file_data_set_mime_type (save_data->file_data, mime_type);
	task = gth_save_image_task_new (save_data->image,
					mime_type,
					save_data->file_data,
					GTH_OVERWRITE_RESPONSE_YES);
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (save_screenshot_task_completed_cb),
			  save_data);
	gth_browser_exec_task (GTH_BROWSER (save_data->browser), task, FALSE);

	gtk_widget_destroy (GTK_WIDGET (file_sel));

	g_free (folder_uri);
	g_object_unref (folder);
	g_object_unref (file);
}


static void
screenshot_ready_cb (GdkPixbuf *pixbuf,
		     gpointer   user_data)
{
	SaveData  *save_data = user_data;
	GtkWidget *file_sel;

	if (pixbuf == NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (save_data->browser), _("Could not take a screenshot"), NULL);
		save_date_free (save_data);
		return;
	}

	save_data->image = gth_image_new_for_pixbuf (pixbuf);
	file_sel = gth_file_chooser_dialog_new (_("Save Image"), GTK_WINDOW (save_data->browser), "image-saver");
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);

	{
		char        *last_uri;
		GFile       *last_folder;
		GthFileData *file_data;
		char        *prefix;
		char        *display_name;
		int          attempt;

		last_uri = g_settings_get_string (save_data->settings, PREF_GSTREAMER_TOOLS_SCREESHOT_LOCATION);
		if ((last_uri == NULL) || (strcmp (last_uri, "~") == 0) || (strcmp (last_uri, "file://~") == 0)) {
			const char *dir;

			dir = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
			if (dir != NULL)
				last_folder = g_file_new_for_path (dir);
			else
				last_folder = g_file_new_for_uri (get_home_uri ());
		}
		else
			last_folder = g_file_new_for_uri (last_uri);
		gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (file_sel), last_folder, NULL);

		file_data = gth_media_viewer_page_get_file_data (save_data->page);
		prefix = _g_utf8_remove_extension (g_file_info_get_display_name (file_data->info));
		if (prefix == NULL)
			prefix = g_strdup (C_("Filename", "Screenshot"));
		display_name = NULL;
		for (attempt = 1; attempt < MAX_ATTEMPTS; attempt++) {
			GFile *proposed_file;

			g_free (display_name);

			display_name = g_strdup_printf ("%s-%02d.jpeg", prefix, attempt);
			proposed_file = g_file_get_child_for_display_name (last_folder, display_name, NULL);
			if ((proposed_file != NULL) && ! g_file_query_exists (proposed_file, NULL)) {
				g_object_unref (proposed_file);
				break;
			}
		}

		if (display_name != NULL) {
			gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_sel), display_name);
			g_free (display_name);
		}

		g_free (prefix);
		g_object_unref (last_folder);
		g_free (last_uri);
	}

	g_signal_connect (GTK_DIALOG (file_sel),
			  "response",
			  G_CALLBACK (save_as_response_cb),
			  save_data);

	gtk_widget_show (file_sel);
}


void
media_viewer_activate_action_screenshot (GtkAction          *action,
				         GthMediaViewerPage *page)
{
	GstElement *playbin;
	SaveData   *save_data;
	int         video_fps_n;
	int         video_fps_d;

	playbin = gth_media_viewer_page_get_playbin (page);
	if (playbin == NULL)
		return;

	save_data = g_new0 (SaveData, 1);
	save_data->browser = gth_media_viewer_page_get_browser (page);
	save_data->settings = g_settings_new (GTHUMB_GSTREAMER_TOOLS_SCHEMA);
	save_data->page = page;
	save_data->playing_before_screenshot = gth_media_viewer_page_is_playing (page);

	if (save_data->playing_before_screenshot)
		gst_element_set_state (playbin, GST_STATE_PAUSED);
	gth_media_viewer_page_get_video_fps (page, &video_fps_n, &video_fps_d);
	_gst_playbin_get_current_frame (playbin,
					video_fps_n,
					video_fps_d,
					screenshot_ready_cb,
					save_data);
}
