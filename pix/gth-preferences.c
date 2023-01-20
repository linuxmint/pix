/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003-2008 Free Software Foundation, Inc.
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

#include <string.h>
#include <math.h>
#include <gio/gio.h>
#include "glib-utils.h"
#include "gth-enum-types.h"
#include "gth-preferences.h"


typedef struct {
	char *wallpaper_filename;
	char *wallpaper_options;
	char *startup_location;
} GthPreferences;


static GthPreferences *Preferences = NULL;


void
gth_pref_initialize (void)
{
	GSettings *settings;

	if (Preferences == NULL)
		Preferences = g_new0 (GthPreferences, 1);

	/* desktop background */

	settings = g_settings_new (GNOME_DESKTOP_BACKGROUND_SCHEMA);
	Preferences->wallpaper_filename = g_settings_get_string(settings, PREF_BACKGROUND_PICTURE_URI);
	Preferences->wallpaper_options = g_settings_get_string(settings, PREF_BACKGROUND_PICTURE_OPTIONS);
	g_object_unref (settings);

	/* startup location */

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	Preferences->startup_location = NULL;
	if (g_settings_get_boolean (settings, PREF_BROWSER_USE_STARTUP_LOCATION)
	    || g_settings_get_boolean (settings, PREF_BROWSER_GO_TO_LAST_LOCATION))
	{
		char *startup_location;

		startup_location = _g_settings_get_uri (settings, PREF_BROWSER_STARTUP_LOCATION);
		if (startup_location == NULL) {
			GFile *file;

			file = g_file_new_for_path (g_get_user_special_dir (G_USER_DIRECTORY_PICTURES));
			startup_location = g_file_get_uri (file);

			g_object_unref (file);
		}
		if (startup_location == NULL)
			startup_location = g_strdup (_g_uri_get_home ());
		gth_pref_set_startup_location (startup_location);

		g_free (startup_location);
	}
	else {
		char *current_dir;
		char *current_uri;

		current_dir = g_get_current_dir ();
		current_uri = g_filename_to_uri (current_dir, NULL, NULL);

		gth_pref_set_startup_location (current_uri);

		g_free (current_uri);
		g_free (current_dir);
	}
	g_object_unref (settings);
}


void
gth_pref_release (void)
{
	if (Preferences == NULL)
		return;

	g_free (Preferences->wallpaper_filename);
	g_free (Preferences->wallpaper_options);
	g_free (Preferences->startup_location);
	g_free (Preferences);
	Preferences = NULL;
}


void
gth_pref_set_startup_location (const char *location)
{
	g_free (Preferences->startup_location);
	Preferences->startup_location = NULL;
	if (location != NULL)
		Preferences->startup_location = g_strdup (location);
}


const char *
gth_pref_get_startup_location (void)
{
	if (Preferences->startup_location != NULL)
		return Preferences->startup_location;
	else
		return _g_uri_get_home ();
}


const char *
gth_pref_get_wallpaper_filename (void)
{
	return Preferences->wallpaper_filename;
}


const char *
gth_pref_get_wallpaper_options (void)
{
	return Preferences->wallpaper_options;
}


void
gth_pref_save_window_geometry (GtkWindow  *window,
                               const char *schema)
{
        GSettings *settings;
        int        width;
        int        height;

        settings = g_settings_new (schema);

        gtk_window_get_size (window, &width, &height);
        g_settings_set_int (settings, "width", width);
        g_settings_set_int (settings, "height", height);

        g_object_unref (settings);
}


void
gth_pref_restore_window_geometry (GtkWindow  *window,
                                  const char *schema)
{
        GSettings *settings;
        int        width;
        int        height;

        settings = g_settings_new (schema);

        width = g_settings_get_int (settings, "width");
        height = g_settings_get_int (settings, "height");
        if ((width != -1) && (height != 1))
                gtk_window_set_default_size (window, width, height);
        gtk_window_present (window);

        g_object_unref (settings);
}
