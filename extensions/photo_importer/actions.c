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
#include "dlg-photo-importer.h"


void
gth_browser_activate_action_import_from_device (GtkAction  *action,
						GthBrowser *browser)
{
	dlg_photo_importer_from_device (browser, NULL);
}


static void
folder_chooser_response_cb (GtkDialog *dialog,
			    int        response_id,
			    gpointer   user_data)
{
	GthBrowser *browser = user_data;
	GFile      *folder;

	if (response_id != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (dialog));
		return;
	}

	folder = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
	if (folder != NULL) {
		dlg_photo_importer_from_folder (browser, folder);
		g_object_unref (folder);
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}


void
gth_browser_activate_action_import_from_folder (GtkAction  *action,
						GthBrowser *browser)
{
	GtkWidget *chooser;
	GFile     *folder;

	chooser = gtk_file_chooser_dialog_new (_("Choose a folder"),
					       GTK_WINDOW (browser),
					       GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					       _("Import"), GTK_RESPONSE_OK,
					       NULL);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), FALSE);

	folder = NULL;
	if (GTH_IS_FILE_SOURCE_VFS (gth_browser_get_location_source (browser)))
		folder = _g_object_ref (gth_browser_get_location (browser));
	if (folder == NULL)
		folder = g_file_new_for_uri (get_home_uri ());
	gtk_file_chooser_set_file (GTK_FILE_CHOOSER (chooser), folder, NULL);

	g_signal_connect (chooser,
			  "response",
			  G_CALLBACK (folder_chooser_response_cb),
			  browser);
	gtk_widget_show (chooser);

	_g_object_unref (folder);
}
