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
#include <gtk/gtk.h>
#ifdef HAVE_GSTREAMER
#  include <gst/gst.h>
#endif
#ifdef HAVE_CLUTTER
#  include <clutter/clutter.h>
#  include <clutter-gtk/clutter-gtk.h>
#endif
#ifdef USE_SMCLIENT
#  include "eggsmclient.h"
#endif
#include "eggdesktopfile.h"
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-browser-actions-callbacks.h"
#include "gth-file-data.h"
#include "gth-file-source-vfs.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gth-window-actions-callbacks.h"
#include "main-migrate.h"


GtkApplication     *Main_Application = NULL;
gboolean            NewWindow = FALSE;
gboolean            StartInFullscreen = FALSE;
gboolean            StartSlideshow = FALSE;
gboolean            ImportPhotos = FALSE;
static char       **remaining_args;
static const char  *program_argv0; /* argv[0] from main(); used as the command to restart the program */
static gboolean     Restart = FALSE;
static gboolean     version = FALSE;


static const GOptionEntry options[] = {
	{ "new-window", 'n', 0, G_OPTION_ARG_NONE, &NewWindow,
	  N_("Open a new window"),
	  0 },

	{ "fullscreen", 'f', 0, G_OPTION_ARG_NONE, &StartInFullscreen,
	  N_("Start in fullscreen mode"),
	  0 },

	{ "slideshow", 's', 0, G_OPTION_ARG_NONE, &StartSlideshow,
	  N_("Automatically start a slideshow"),
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
	gth_hook_invoke ("import-photos", window, location, NULL);
}


static void
migrate_data (void)
{
	migrate_catalogs_from_2_10 ();
}


/* -- session management -- */


#ifdef USE_SMCLIENT


static void
client_save_state (EggSMClient *client,
		   GKeyFile    *state,
		   gpointer     user_data)
{
	const char *argv[2] = { NULL };
	GList      *scan;
	guint       i;

	argv[0] = program_argv0;
	argv[1] = NULL;
	egg_sm_client_set_restart_command (client, 1, argv);

	i = 0;
	for (scan = gtk_application_get_windows (Main_Application); scan; scan = scan->next) {
		GtkWidget   *window = scan->data;
		GFile       *location;
		char        *key;
		char        *uri;
		GthFileData *focused_file = NULL;

		focused_file = gth_browser_get_current_file (GTH_BROWSER (window));
		if (focused_file == NULL)
			location = gth_browser_get_location (GTH_BROWSER (window));
		else
			location = focused_file->file;

		if (location == NULL)
			continue;

		key = g_strdup_printf ("location%d", ++i);
		uri = g_file_get_uri (location);
		g_key_file_set_string (state, "Session", key, uri);

		g_free (uri);
		g_free (key);
		g_object_unref (location);
	}

	g_key_file_set_integer (state, "Session", "locations", i);
}


/* quit_requested handler for the master client */


static GList *client_window = NULL;


static void modified_file_saved_cb (GthBrowser  *browser,
				    gboolean     cancelled,
				    gpointer     user_data);


static void
check_whether_to_save (EggSMClient *client)
{
	for (/* void */; client_window; client_window = client_window->next) {
		GtkWidget *window = client_window->data;

		if (gth_browser_get_file_modified (GTH_BROWSER (window))) {
			gth_browser_ask_whether_to_save (GTH_BROWSER (window),
							 modified_file_saved_cb,
							 client);
			return;
		}
	}

	egg_sm_client_will_quit (client, TRUE);
}


static void
modified_file_saved_cb (GthBrowser *browser,
			gboolean    cancelled,
			gpointer    user_data)
{
	EggSMClient *client = user_data;

	if (cancelled) {
		egg_sm_client_will_quit (client, FALSE);
	}
	else {
		client_window = client_window->next;
		check_whether_to_save (client);
	}
}


static void
client_quit_requested_cb (EggSMClient *client,
			  gpointer     data)
{
	client_window = gtk_application_get_windows (Main_Application);
	check_whether_to_save (client);
}


static void
client_quit_cb (EggSMClient *client,
		gpointer     data)
{
	gtk_main_quit ();
}


static void
restore_session (EggSMClient *client)
{
	GKeyFile *state = NULL;
	guint     i;

	state = egg_sm_client_get_state_file (client);

	i = g_key_file_get_integer (state, "Session", "locations", NULL);
	g_assert (i > 0);
	for (; i > 0; i--) {
		GtkWidget *window;
		char      *key;
		char      *location;
		GFile     *file;

		key = g_strdup_printf ("location%d", i);
		location = g_key_file_get_string (state, "Session", key, NULL);
		g_free (key);

		g_assert (location != NULL);

		file = g_file_new_for_uri (location);
		window = gth_browser_new (file, NULL);
		gtk_widget_show (window);

		g_object_unref (file);
		g_free (location);
	}
}


#endif


/* -- main application -- */


typedef GtkApplication      GthApplication;
typedef GtkApplicationClass GthApplicationClass;

static gpointer gth_application_parent_class;


G_DEFINE_TYPE (GthApplication, gth_application, GTK_TYPE_APPLICATION)


static void
gth_application_finalize (GObject *object)
{
        G_OBJECT_CLASS (gth_application_parent_class)->finalize (object);
}


static void
gth_application_init (GthApplication *app)
{
#ifdef GDK_WINDOWING_X11
	egg_set_desktop_file (GTHUMB_APPLICATIONS_DIR "/gthumb.desktop");
#else
	/* manually set name and icon */
	g_set_application_name (_("gThumb"));
	gtk_window_set_default_icon_name ("gthumb");
#endif
}


static void
activate_new_window (GSimpleAction *action,
		     GVariant      *parameter,
		     gpointer       user_data)
{
        GApplication *application = user_data;
        GList        *windows;

        windows = gtk_application_get_windows (GTK_APPLICATION (application));
        if (windows != NULL)
        	gth_browser_activate_action_file_new_window (NULL, windows->data);
}


static void
activate_preferences (GSimpleAction *action,
		      GVariant      *parameter,
		      gpointer       user_data)
{
        GApplication *application = user_data;
        GList        *windows;

        windows = gtk_application_get_windows (GTK_APPLICATION (application));
        if (windows != NULL)
        	gth_browser_activate_action_edit_preferences (NULL, windows->data);
}


static void
activate_help (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
        GApplication *application = user_data;
        GList        *windows;

        windows = gtk_application_get_windows (GTK_APPLICATION (application));
        if (windows != NULL)
        	gth_browser_activate_action_help_help (NULL, windows->data);
}


static void
activate_about (GSimpleAction *action,
		GVariant      *parameter,
		gpointer       user_data)
{
        GApplication *application = user_data;
        GList        *windows;

        windows = gtk_application_get_windows (GTK_APPLICATION (application));
        if (windows != NULL)
        	gth_browser_activate_action_help_about (NULL, windows->data);
}


static void
activate_quit (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
        GApplication *application = user_data;
        GList        *windows;

        windows = gtk_application_get_windows (GTK_APPLICATION (application));
        if (windows != NULL)
        	gth_window_activate_action_file_quit_application (NULL, windows->data);
}


static const GActionEntry app_menu_entries[] = {
	{ "new-window",  activate_new_window },
	{ "preferences",  activate_preferences },
	{ "help",  activate_help },
	{ "about", activate_about },
	{ "quit",  activate_quit }
};


static void
_gth_application_initialize_app_menu (GApplication *application)
{
	gboolean    show_app_menu;
	GtkBuilder *builder;
	GError     *error = NULL;

	g_object_get (gtk_settings_get_default (),
		      "gtk-shell-shows-app-menu", &show_app_menu,
		      NULL);
	if (! show_app_menu)
		return;

	g_action_map_add_action_entries (G_ACTION_MAP (application),
					 app_menu_entries,
					 G_N_ELEMENTS (app_menu_entries),
					 application);

	builder = gtk_builder_new ();
	gtk_builder_add_from_string (builder,
			"<interface>"
			"  <menu id='app-menu'>"
			"    <section>"
			"      <item>"
			"        <attribute name='label' translatable='yes'>New _Window</attribute>"
			"        <attribute name='action'>app.new-window</attribute>"
			"      </item>"
			"    </section>"
			"    <section>"
			"      <item>"
			"        <attribute name='label' translatable='yes'>_Preferences</attribute>"
			"        <attribute name='action'>app.preferences</attribute>"
			"      </item>"
			"    </section>"
			"    <section>"
			"      <item>"
			"        <attribute name='label' translatable='yes'>_Help</attribute>"
			"        <attribute name='action'>app.help</attribute>"
			"      </item>"
			"      <item>"
			"        <attribute name='label' translatable='yes'>_About gThumb</attribute>"
			"        <attribute name='action'>app.about</attribute>"
			"      </item>"
			"      <item>"
			"        <attribute name='label' translatable='yes'>_Quit</attribute>"
			"        <attribute name='action'>app.quit</attribute>"
			"      </item>"
			"    </section>"
			"  </menu>"
			"</interface>", -1, &error);
	gtk_application_set_app_menu (GTK_APPLICATION (application),
				      G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu")));

	g_object_unref (builder);
}


static void
gth_application_startup (GApplication *application)
{
	G_APPLICATION_CLASS (gth_application_parent_class)->startup (application);

	_gth_application_initialize_app_menu (application);
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

	context = g_option_context_new (N_("- Image browser and viewer"));
	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
	g_option_context_set_ignore_unknown_options (context, TRUE);
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);

	if (g_once_init_enter (&initialized)) {
		g_option_context_add_group (context, gtk_get_option_group (TRUE));
#ifdef USE_SMCLIENT
		g_option_context_add_group (context, egg_sm_client_get_option_group ());
#endif
#ifdef HAVE_CLUTTER
		g_option_context_add_group (context, cogl_get_option_group ());
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

	/* restore the session */

#ifdef USE_SMCLIENT
	{
		EggSMClient *client;

		client = egg_sm_client_get ();
		g_signal_connect (client,
				  "save_state",
				  G_CALLBACK (client_save_state),
				  NULL);
		g_signal_connect (client,
				  "quit_requested",
				  G_CALLBACK (client_quit_requested_cb),
				  NULL);
		g_signal_connect (client,
				  "quit",
				  G_CALLBACK (client_quit_cb),
				  NULL);
		if (egg_sm_client_is_resumed (client)) {
			restore_session (client);
			return 0;
		}
	}
#endif

	/* exec the command line */

	if (ImportPhotos) {
		GFile *location = NULL;

		if (remaining_args != NULL)
			location = g_file_new_for_commandline_arg (remaining_args[0]);
		import_photos_from_location (location);
		gdk_notify_startup_complete ();

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
		gdk_notify_startup_complete ();

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
		file_type = _g_file_get_standard_type (location);
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

	gdk_notify_startup_complete ();

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


static GtkApplication *
gth_application_new (void)
{
        return g_object_new (gth_application_get_type (),
                             "application-id", "org.gnome.Gthumb",
                             "register-session", TRUE, /* required to call gtk_application_inhibit */
                             "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
                             NULL);
}


/* -- main -- */


int
main (int argc, char *argv[])
{
	int status;

	program_argv0 = argv[0];

	g_type_init ();

	/* text domain */

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* run the main application */

	Main_Application = gth_application_new ();
	status = g_application_run (G_APPLICATION (Main_Application), argc, argv);

	gth_main_release ();
	gth_pref_release ();
	g_object_unref (Main_Application);

	/* restart if requested by the user */

	if (Restart)
		g_spawn_command_line_async (program_argv0, NULL);

	return status;
}


void
gth_quit (gboolean restart)
{
	GList *windows;
	GList *scan;

	windows = g_list_copy (gtk_application_get_windows (Main_Application));
	for (scan = windows; scan; scan = scan->next)
		gth_window_close (GTH_WINDOW (scan->data));
	g_list_free (windows);

	Restart = restart;
}
