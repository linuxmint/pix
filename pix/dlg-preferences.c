/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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
#include "dlg-preferences.h"
#include "gth-browser.h"
#include "gth-enum-types.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gtk-utils.h"
#include "glib-utils.h"
#include "typedefs.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
} DialogData;


static void
apply_changes (DialogData *data)
{
	gth_hook_invoke ("dlg-preferences-apply", data->dialog, data->browser, data->builder);
}


static void
destroy_cb (GtkWidget *widget,
	    DialogData *data)
{
	apply_changes (data);
	gth_browser_set_dialog (data->browser, "preferences", NULL);

	g_object_unref (data->builder);
	g_free (data);
}


static void
list_box_row_activated_cb (GtkListBox    *box,
			   GtkListBoxRow *row,
			   gpointer       user_data)
{
	DialogData *data = user_data;
	int         page_num;

	page_num = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (row), "gth.page_num"));
	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET("notebook")), page_num);
}


void
dlg_preferences (GthBrowser *browser)
{
	DialogData *data;
	GtkWidget  *notebook;
	GList      *children;
	GList      *scan;
	int         page_num;
	GtkWidget  *list_box;

	if (gth_browser_get_dialog (browser, "preferences") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "preferences")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("preferences.ui", NULL);
	data->dialog = GET_WIDGET ("preferences_dialog");

	gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))), 0);
	gth_browser_set_dialog (browser, "preferences", data->dialog);
	gth_hook_invoke ("dlg-preferences-construct", data->dialog, data->browser, data->builder);

	/* widgets */

	list_box = GET_WIDGET ("tabs_listbox");
	notebook = GET_WIDGET ("notebook");
	children = gtk_container_get_children (GTK_CONTAINER (notebook));
	page_num = 0;
	for (scan = children; scan; scan = scan->next) {
		GtkWidget   *child = scan->data;
		const char  *name;
		GtkWidget   *row;
		GtkWidget   *box;
		GtkWidget   *label;

		name = gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (notebook), child);
		if (name == NULL)
			continue;

		row = gtk_list_box_row_new ();
		g_object_set_data (G_OBJECT (row), "gth.page_num", GINT_TO_POINTER (page_num));
		gtk_widget_show (row);
		gtk_container_add (GTK_CONTAINER (list_box), row);

		box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_container_set_border_width (GTK_CONTAINER (box), 10);
		gtk_widget_show (box);
		gtk_container_add (GTK_CONTAINER (row), box);

		label = gtk_label_new (name);
		gtk_label_set_ellipsize (GTK_LABEL(label), PANGO_ELLIPSIZE_END);
		gtk_widget_show (label);
		gtk_container_add (GTK_CONTAINER (box), label);

		page_num += 1;
	}

	/* Set the signals handlers. */

	g_signal_connect (data->dialog,
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (list_box,
			  "row-activated",
			  G_CALLBACK (list_box_row_activated_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_widget_show (data->dialog);
}
