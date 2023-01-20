/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2010 Free Software Foundation, Inc.
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
#include <glib/gprintf.h>
#include <gio/gdesktopappinfo.h>
#ifdef HAVE_GSTREAMER
#  include <gst/gst.h>
#endif
#ifdef HAVE_CLUTTER
#  include <clutter/clutter.h>
#  include <clutter-gtk/clutter-gtk.h>
#endif
#include "glib-utils.h"
#include "gth-application.h"
#include "gth-browser.h"
#include "gth-browser-actions-callbacks.h"
#include "gth-file-data.h"
#include "gth-file-source-vfs.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gtk-utils.h"
#include "main.h"
#include "main-migrate.h"


static char **  remaining_args = NULL;
static gboolean version = FALSE;


static const GOptionEntry options[] = {
	{ "new-window", 'n', 0, G_OPTION_ARG_NONE, &NewWindow,
	  N_("Open a new window"),
	  0 },

	{ "fullscreen", 'f', 0, G_OPTION_ARG_NONE, &StartInFullscreen,
	  N_("Start in fullscreen mode"),
	  0 },

	{ "slideshow", 's', 0, G_OPTION_ARG_NONE, &StartSlideshow,
	  N_("Automatically start a presentation"),
	  0 },

	{ "import-photos", 'i', 0, G_OPTION_ARG_NONE, &ImportPhotos,
	  N_("Automatically import digital camera photos"),
	  0 },

	{ "version", 'v', 0, G_OPTION_ARG_NONE, &version,
	  N_("Show version"), NULL },

	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &remaining_args,
	  NULL,
	  NULL },

	{ NULL }
};


typedef GtkApplication      GthApplication;
typedef GtkApplicationClass GthApplicationClass;

GType gth_application_get_type	(void);


G_DEFINE_TYPE (GthApplication, gth_application, GTK_TYPE_APPLICATION);


static void
gth_application_finalize (GObject *object)
{
	gth_main_release ();
	gth_pref_release ();

        G_OBJECT_CLASS (gth_application_parent_class)->finalize (object);
}


static void
gth_application_init (GthApplication *app)
{
	GDesktopAppInfo *app_info;

	app_info = g_desktop_app_info_new ("org.gnome.gThumb.desktop");
	if (app_info == NULL) {
		/* manually set name and icon */

		g_set_application_name (_("gThumb"));
		gtk_window_set_default_icon_name ("gthumb");

		return;
	}

	if (g_desktop_app_info_has_key (app_info, "Name")) {
		char *app_name;

		app_name = g_desktop_app_info_get_string (app_info, "Name");
		g_set_application_name (app_name);

		g_free (app_name);
	}

	if (g_desktop_app_info_has_key (app_info, "Icon")) {
		char *icon;

		icon = g_desktop_app_info_get_string (app_info, "Icon");
		if (g_path_is_absolute (icon))
			gtk_window_set_default_icon_from_file (icon, NULL);
		else
			gtk_window_set_default_icon_name (icon);

		g_free (icon);
	}

	g_object_unref (app_info);
}


static void
migrate_data (void)
{
	migrate_catalogs_from_2_10 ();
}


static void
gth_application_startup (GApplication *application)
{
	G_APPLICATION_CLASS (gth_application_parent_class)->startup (application);

	g_object_set (gtk_settings_get_default (), "gtk-application-prefer-dark-theme", TRUE, NULL);

	gth_pref_initialize ();
	migrate_data ();
	gth_main_initialize ();
	gth_main_register_default_hooks ();
	gth_main_register_file_source (GTH_TYPE_FILE_SOURCE_VFS);
	gth_main_register_default_sort_types ();
	gth_main_register_default_tests ();
	gth_main_register_default_types ();
	gth_main_register_default_metadata ();
	gth_main_activate_extensions ();
	gth_hook_invoke ("initialize", NULL);
}


static GOptionContext *
gth_application_create_option_context (void)
{
	GOptionContext *context;
	static gsize    initialized = FALSE;

	context = g_option_context_new (N_("— Image browser and viewer"));
	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
	g_option_context_set_ignore_unknown_options (context, TRUE);
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);

	if (g_once_init_enter (&initialized)) {
		g_option_context_add_group (context, gtk_get_option_group (TRUE));
#ifdef HAVE_CLUTTER
		g_option_context_add_group (context, clutter_get_option_group_without_init ());
		g_option_context_add_group (context, gtk_clutter_get_option_group ());
#endif
#ifdef HAVE_GSTREAMER
		g_option_context_add_group (context, gst_init_get_option_group ());
#endif
		g_once_init_leave (&initialized, TRUE);
	}

	return context;
}


static void
open_browser_window (GFile    *location,
		     GFile    *file_to_select,
		     gboolean  force_new_window)
{
	gboolean   reuse_active_window;
	GtkWidget *window;

	if (! force_new_window) {
		GSettings *settings;

		settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
		reuse_active_window = g_settings_get_boolean (settings, PREF_BROWSER_REUSE_ACTIVE_WINDOW);
		g_object_unref (settings);
	}
	else
		reuse_active_window = FALSE;

	window = NULL;

	if (reuse_active_window) {
		GList *windows = gtk_application_get_windows (Main_Application);
		if (windows != NULL)
			window = windows->data;
	}

	if (window == NULL)
		window = gth_browser_new (location, file_to_select);
	else if (file_to_select != NULL)
		gth_browser_go_to (GTH_BROWSER (window), location, file_to_select);
	else
		gth_browser_load_location (GTH_BROWSER (window), location);

	if (! StartSlideshow)
		gtk_window_present (GTK_WINDOW (window));
}


static void
import_photos_from_location (GFile *location)
{
	GtkWidget *window;

	window = gth_browser_new (NULL, NULL);
	gth_browser_set_close_with_task (GTH_BROWSER (window), TRUE);
	gth_hook_invoke ("import-photos", window, location, NULL);
}


static int
gth_application_command_line (GApplication            *application,
                              GApplicationCommandLine *command_line)
{
	char           **argv;
	int              argc;
	GOptionContext  *context;
	GError          *error = NULL;
	const char      *arg;
	int              i;
	GList           *files;
	GList           *dirs;
	GFile           *location;
	gboolean         singleton;
	GList           *scan;

	argv = g_application_command_line_get_arguments (command_line, &argc);

	/* parse command line options */

	context = gth_application_create_option_context ();
	if (! g_option_context_parse (context, &argc, &argv, &error)) {
		g_critical ("Failed to parse arguments: %s", error->message);
		g_error_free (error);
		g_option_context_free (context);
		return EXIT_FAILURE;
	}
	g_option_context_free (context);
	g_strfreev (argv);

	gdk_notify_startup_complete ();

	/* exec the command line */

	if (ImportPhotos) {
		GFile *location = NULL;

		if (remaining_args != NULL)
			location = g_file_new_for_commandline_arg (remaining_args[0]);
		import_photos_from_location (location);

		ImportPhotos = FALSE;

		return 0;
	}

	if (remaining_args == NULL) { /* No location specified. */
		GFile     *location;
		GSettings *settings;
		char      *file_to_select_uri;
		GFile     *file_to_select;

		location = g_file_new_for_uri (gth_pref_get_startup_location ());
		settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
		file_to_select_uri = _g_settings_get_uri (settings, PREF_BROWSER_STARTUP_CURRENT_FILE);
		if (file_to_select_uri != NULL)
			file_to_select = g_file_new_for_uri (file_to_select_uri);
		else
			file_to_select = NULL;

		open_browser_window (location, file_to_select, TRUE);

		_g_object_unref (file_to_select);
		g_free (file_to_select_uri);
		g_object_unref (settings);
		g_object_unref (location);

		return 0;
	}

	/* At least a location was specified */

	files = NULL;
	dirs = NULL;
	for (i = 0; (arg = remaining_args[i]) != NULL; i++) {
		GFile     *location;
		GFileType  file_type;

		location = g_file_new_for_commandline_arg (arg);
		file_type = _g_file_query_standard_type (location);
		if (file_type == G_FILE_TYPE_REGULAR)
			files = g_list_prepend (files, location);
		else
			dirs = g_list_prepend (dirs, location);
	}
	files = g_list_reverse (files);
	dirs = g_list_reverse (dirs);

	location = gth_hook_invoke_get ("command-line-files", files);
	if (location != NULL) {
		open_browser_window (location, NULL, FALSE);
		g_object_unref (location);
	}
	else {
		/* Open each file in a new window */

		singleton = (files != NULL) && (files->next == NULL);
		for (scan = files; scan; scan = scan->next)
			open_browser_window ((GFile *) scan->data, NULL, ! singleton);
	}

	/* Open each dir in a new window */

	for (scan = dirs; scan; scan = scan->next)
		open_browser_window ((GFile *) scan->data, NULL, TRUE);

	_g_object_list_unref (dirs);
	_g_object_list_unref (files);

	return 0;
}


static gboolean
gth_application_local_command_line (GApplication   *application,
                                    char         ***arguments,
                                    int            *exit_status)
{
        char           **local_argv;
        int              local_argc;
        GOptionContext  *context;
        GError          *error = NULL;
        gboolean         handled_locally = FALSE;

        *exit_status = 0;

        local_argv = g_strdupv (*arguments);
        local_argc = g_strv_length (local_argv);
        context = gth_application_create_option_context ();
	if (! g_option_context_parse (context, &local_argc, &local_argv, &error)) {
		*exit_status = EXIT_FAILURE;
		g_critical ("Failed to parse arguments: %s", error->message);
                g_clear_error (&error);
                handled_locally = TRUE;
	}
	g_strfreev (local_argv);

	/* substitute the dot with the local current folder */

	local_argv = *arguments;
	if ((local_argv != NULL) && (local_argv[0] != NULL)) {
		int i;

		for (i = 1; local_argv[i] != NULL; i++) {
			if (strstr (local_argv[i], ".") != NULL) {
				GFile *location = g_file_new_for_commandline_arg (local_argv[i]);
				g_free (local_argv[i]);
				local_argv[i] = g_file_get_uri (location);
				g_object_unref (location);
			}
		}
	}

	if (version) {
		g_printf ("%s %s, Copyright © 2001-2010 Free Software Foundation, Inc.\n", PACKAGE_NAME, PACKAGE_VERSION);
		handled_locally = TRUE;
	}

	g_option_context_free (context);

        return handled_locally;
}


static void
gth_application_class_init (GthApplicationClass *klass)
{
	GObjectClass      *object_class;
	GApplicationClass *application_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_application_finalize;

	application_class = G_APPLICATION_CLASS (klass);
	application_class->startup = gth_application_startup;
	application_class->command_line = gth_application_command_line;
	application_class->local_command_line = gth_application_local_command_line;
}


GtkApplication *
gth_application_new (void)
{
        return g_object_new (gth_application_get_type (),
                             "application-id", "org.gnome.gThumb",
                             "register-session", TRUE, /* required to call gtk_application_inhibit */
                             "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
                             NULL);
}
