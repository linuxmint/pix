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
#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#endif
#include "gth-application.h"
#include "gth-main.h"
#include "gth-window.h"
#include "main.h"


GtkApplication * Main_Application = NULL;
gboolean         NewWindow = FALSE;
gboolean         StartInFullscreen = FALSE;
gboolean         StartSlideshow = FALSE;
gboolean         ImportPhotos = FALSE;
static gboolean  Restart = FALSE;


int
main (int argc, char *argv[])
{
	const char *program_argv0;
	int         status;

	program_argv0 = argv[0];

	/* text domain */

	bindtextdomain (GETTEXT_PACKAGE, GTHUMB_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

#if defined (HAVE_GSTREAMER) && defined (GDK_WINDOWING_X11)
	/* https://discourse.gnome.org/t/how-to-correctly-init-gtk-app-with-gstreamer/5105/3 */
	XInitThreads();
#endif

	/* run the main application */

	Main_Application = gth_application_new ();
	status = g_application_run (G_APPLICATION (Main_Application), argc, argv);
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

	Restart = restart;

	windows = g_list_copy (gtk_application_get_windows (Main_Application));
	for (scan = windows; scan; scan = scan->next)
		gth_window_close (GTH_WINDOW (scan->data));
	g_list_free (windows);
}
