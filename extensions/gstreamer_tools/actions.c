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
#include "actions.h"
#include "gth-media-viewer-page.h"
#include "preferences.h"

#define MAX_ATTEMPTS 1024


/* -- gth_browser_activate_video_screenshot -- */


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
	char               *filename;
	char               *text;

	if (error != NULL)
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (save_data->browser), _("Could not save the file"), error);
	else if (save_data->playing_before_screenshot)
		gst_element_set_state (gth_media_viewer_page_get_playbin (page), GST_STATE_PLAYING);

	filename = g_file_get_parse_name (save_data->file_data->file);
	/* Translators: %s is a filename */
	text = g_strdup_printf (_("Image saved as %s"), filename);
	gth_statusbar_set_secondary_text_temp (GTH_STATUSBAR (gth_browser_get_statusbar (save_data->browser)), text);

	g_free (text);
	g_free (filename);
	save_date_free (save_data);
	g_object_unref (task);
}


static GFile *
get_screenshot_file (SaveData  *save_data,
		     GError   **error)
{
	GFile       *file = NULL;
	char        *uri;
	GFile       *folder;
	GthFileData *file_data;
	char        *prefix;
	int          attempt;

	uri = _g_settings_get_uri_or_special_dir (save_data->settings, PREF_GSTREAMER_TOOLS_SCREESHOT_LOCATION, G_USER_DIRECTORY_PICTURES);
	folder = g_file_new_for_uri (uri);
	file_data = gth_media_viewer_page_get_file_data (save_data->page);
	prefix = _g_path_remove_extension (g_file_info_get_display_name (file_data->info));
	if (prefix == NULL)
		prefix = g_strdup (C_("Filename", "Screenshot"));

	for (attempt = 1; (file == NULL) && (attempt < MAX_ATTEMPTS); attempt++) {
		char  *display_name;
		GFile *proposed_file;

		display_name = g_strdup_printf ("%s-%02d.jpeg", prefix, attempt);
		proposed_file = g_file_get_child_for_display_name (folder, display_name, NULL);
		if ((proposed_file != NULL) && ! g_file_query_exists (proposed_file, NULL))
			file = g_object_ref (proposed_file);

		_g_object_unref (proposed_file);
		g_free (display_name);
	}

	if (file == NULL)
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME, "Invalid filename");

	g_free (prefix);
	_g_object_unref (folder);
	g_free (uri);

	return file;
}


static void
screenshot_ready_cb (GdkPixbuf *pixbuf,
		     gpointer   user_data)
{
	SaveData *save_data = user_data;
	GFile    *file;
	GError   *error = NULL;
	GthTask  *task;

	if (pixbuf == NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (save_data->browser), _("Could not take a screenshot"), NULL);
		save_date_free (save_data);
		return;
	}

	save_data->image = gth_image_new_for_pixbuf (pixbuf);

	/* save the image */

	file = get_screenshot_file (save_data, &error);
	if (file == NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (save_data->browser), _("Could not take a screenshot"), error);
		save_date_free (save_data);
		g_clear_error (&error);
		return;
	}

	save_data->file_data = gth_file_data_new (file, NULL);
	gth_file_data_set_mime_type (save_data->file_data, "image/jpeg");
	task = gth_save_image_task_new (save_data->image,
					"image/jpeg",
					save_data->file_data,
					GTH_OVERWRITE_RESPONSE_YES);
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (save_screenshot_task_completed_cb),
			  save_data);
	gth_browser_exec_task (GTH_BROWSER (save_data->browser), task, GTH_TASK_FLAGS_IGNORE_ERROR);
}


void
gth_browser_activate_video_screenshot (GSimpleAction	*action,
				       GVariant		*parameter,
				       gpointer		 user_data)
{
	GthBrowser		*browser = GTH_BROWSER (user_data);
	GthMediaViewerPage	*page;
	GstElement		*playbin;
	SaveData		*save_data;

	page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));
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
	_gst_playbin_get_current_frame (playbin,
					screenshot_ready_cb,
					save_data);
}


void
gth_browser_activate_toggle_play (GSimpleAction	*action,
				  GVariant	*parameter,
				  gpointer	 user_data)
{
	GthBrowser		*browser = GTH_BROWSER (user_data);
	GthMediaViewerPage	*page;

	page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));
	gth_media_viewer_page_toggle_play (page);
}


void
gth_browser_activate_toggle_mute (GSimpleAction	*action,
				  GVariant	*parameter,
				  gpointer	 user_data)
{
	GthBrowser		*browser = GTH_BROWSER (user_data);
	GthMediaViewerPage	*page;

	page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));
	gth_media_viewer_page_toggle_mute (page);
}


void
gth_browser_activate_play_faster (GSimpleAction	*action,
				  GVariant	*parameter,
				  gpointer	 user_data)
{
	GthBrowser		*browser = GTH_BROWSER (user_data);
	GthMediaViewerPage	*page;

	page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));
	gth_media_viewer_page_play_faster (page);
}


void
gth_browser_activate_play_slower (GSimpleAction	*action,
				  GVariant	*parameter,
				  gpointer	 user_data)
{
	GthBrowser		*browser = GTH_BROWSER (user_data);
	GthMediaViewerPage	*page;

	page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));
	gth_media_viewer_page_play_slower (page);
}


void
gth_browser_activate_video_zoom_fit (GSimpleAction	*action,
				     GVariant		*state,
				     gpointer		 user_data)
{
	GthBrowser	   *browser = GTH_BROWSER (user_data);
	GthMediaViewerPage *page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));;

	g_simple_action_set_state (action, state);
	gth_media_viewer_page_set_fit_if_larger (page, g_variant_get_boolean (state));
}


void
gth_browser_activate_next_video_frame (GSimpleAction	*action,
				       GVariant		*state,
				       gpointer		 user_data)
{
	GthBrowser	   *browser = GTH_BROWSER (user_data);
	GthMediaViewerPage *page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));;

	gth_media_viewer_page_next_frame (page);
}


void
gth_browser_activate_skip_forward_smallest (GSimpleAction	*action,
					    GVariant		*state,
					    gpointer		 user_data)
{
	GthBrowser	   *browser = GTH_BROWSER (user_data);
	GthMediaViewerPage *page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));;

	gth_media_viewer_page_skip (page, 1);
}


void
gth_browser_activate_skip_forward_smaller (GSimpleAction	*action,
					   GVariant		*state,
					   gpointer		 user_data)
{
	GthBrowser	   *browser = GTH_BROWSER (user_data);
	GthMediaViewerPage *page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));;

	gth_media_viewer_page_skip (page, 5);
}


void
gth_browser_activate_skip_forward_small (GSimpleAction	*action,
					 GVariant		*state,
					 gpointer		 user_data)
{
	GthBrowser	   *browser = GTH_BROWSER (user_data);
	GthMediaViewerPage *page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));;

	gth_media_viewer_page_skip (page, 10);
}


void
gth_browser_activate_skip_forward_big (GSimpleAction	*action,
				       GVariant		*state,
				       gpointer		 user_data)
{
	GthBrowser	   *browser = GTH_BROWSER (user_data);
	GthMediaViewerPage *page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));;

	gth_media_viewer_page_skip (page, 60);
}


void
gth_browser_activate_skip_forward_bigger (GSimpleAction	*action,
					  GVariant		*state,
					  gpointer		 user_data)
{
	GthBrowser	   *browser = GTH_BROWSER (user_data);
	GthMediaViewerPage *page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));;

	gth_media_viewer_page_skip (page, 60 * 5);
}


void
gth_browser_activate_skip_back_smallest (GSimpleAction	*action,
					 GVariant		*state,
					 gpointer		 user_data)
{
	GthBrowser	   *browser = GTH_BROWSER (user_data);
	GthMediaViewerPage *page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));;

	gth_media_viewer_page_skip (page, -1);
}


void
gth_browser_activate_skip_back_smaller (GSimpleAction	*action,
					GVariant		*state,
					gpointer		 user_data)
{
	GthBrowser	   *browser = GTH_BROWSER (user_data);
	GthMediaViewerPage *page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));;

	gth_media_viewer_page_skip (page, -5);
}


void
gth_browser_activate_skip_back_small (GSimpleAction	*action,
				      GVariant		*state,
				      gpointer		 user_data)
{
	GthBrowser	   *browser = GTH_BROWSER (user_data);
	GthMediaViewerPage *page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));;

	gth_media_viewer_page_skip (page, -10);
}


void
gth_browser_activate_skip_back_big (GSimpleAction	*action,
				    GVariant		*state,
				    gpointer		 user_data)
{
	GthBrowser	   *browser = GTH_BROWSER (user_data);
	GthMediaViewerPage *page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));;

	gth_media_viewer_page_skip (page, -60);
}


void
gth_browser_activate_skip_back_bigger (GSimpleAction	*action,
				       GVariant		*state,
				       gpointer		 user_data)
{
	GthBrowser	   *browser = GTH_BROWSER (user_data);
	GthMediaViewerPage *page = GTH_MEDIA_VIEWER_PAGE (gth_browser_get_viewer_page (browser));;

	gth_media_viewer_page_skip (page, -60 * 5);
}
