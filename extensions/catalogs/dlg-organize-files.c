/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include "dlg-organize-files.h"
#include "gth-organize-task.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GFile      *folder;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_object_ref (data->folder);
	g_object_unref (data->builder);
	g_free (data);
}


static void
start_button_clicked_cb (GtkWidget  *widget,
			 DialogData *data)
{
	GthTask *task;

	task = gth_organize_task_new (data->browser, data->folder, gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("group_by_combobox"))));
	gth_organize_task_set_recursive (GTH_ORGANIZE_TASK (task), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("include_subfolders_checkbutton"))));
	gth_organize_task_set_create_singletons (GTH_ORGANIZE_TASK (task), ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("ignore_singletons_checkbutton"))));
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_singletons_catalog_checkbutton"))))
		gth_organize_task_set_singletons_catalog (GTH_ORGANIZE_TASK (task), gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("single_catalog_entry"))));
	gth_browser_exec_task (data->browser, task, GTH_TASK_FLAGS_DEFAULT);

	gtk_widget_destroy (data->dialog);
	g_object_unref (task);
}


static void
ignore_singletons_checkbutton_clicked_cb (GtkToggleButton *button,
					  DialogData      *data)
{
	if (gtk_toggle_button_get_active (button)) {
		gtk_widget_set_sensitive (GET_WIDGET ("single_catalog_box"), TRUE);
		gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_singletons_catalog_checkbutton")), FALSE);
	}
	else {
		gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_singletons_catalog_checkbutton")), TRUE);
		gtk_widget_set_sensitive (GET_WIDGET ("single_catalog_box"), FALSE);
	}
}


static void
use_singletons_catalog_checkbutton_clicked_cb (GtkToggleButton *button,
					       DialogData      *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("single_catalog_entry"), gtk_toggle_button_get_active (button));
}


void
dlg_organize_files (GthBrowser *browser,
		    GFile      *folder)
{
	DialogData *data;
	GtkWidget  *info_bar;
	GtkWidget  *info_label;

	g_return_if_fail (folder != NULL);

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->folder = g_file_dup (folder);
	data->builder = _gtk_builder_new_from_file ("organize-files.ui", "catalogs");

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Organize Files"),
				     "transient-for", GTK_WINDOW (browser),
				     "modal", TRUE,
				     "resizable", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
			        _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_GTK_LABEL_EXECUTE, GTK_RESPONSE_OK,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	info_bar = gth_info_bar_new ();
	gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar), GTK_MESSAGE_INFO);
	info_label = gth_info_bar_get_primary_label (GTH_INFO_BAR (info_bar));
	gtk_label_set_ellipsize (GTK_LABEL (info_label), PANGO_ELLIPSIZE_NONE);
	gtk_label_set_line_wrap (GTK_LABEL (info_label), TRUE);
	gtk_label_set_single_line_mode (GTK_LABEL (info_label), FALSE);
	gtk_label_set_text (GTK_LABEL (info_label), _("Files will be organized in catalogs. No file will be moved on disk."));
	gtk_widget_show (info_label);
	gtk_widget_show (info_bar);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("info_alignment")), info_bar);

	{
		GtkListStore *list_store = (GtkListStore *) GET_WIDGET ("group_by_liststore");
		GtkTreeIter   iter;

		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    0, GTH_GROUP_POLICY_DIGITALIZED_DATE,
				    1, _("Date photo was taken"),
				    2, "camera-photo-symbolic",
				    -1);

		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    0, GTH_GROUP_POLICY_MODIFIED_DATE,
				    1, _("File modified date"),
				    2, "change-date-symbolic",
				    -1);

		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    0, GTH_GROUP_POLICY_TAG,
				    1, _("Tag"),
				    2, "tag-symbolic",
				    -1);

		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    0, GTH_GROUP_POLICY_TAG_EMBEDDED,
				    1, _("Tag (embedded)"),
				    2, "tag-symbolic",
				    -1);
	}

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("group_by_combobox")), 0);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CANCEL),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  data->dialog);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK),
			  "clicked",
			  G_CALLBACK (start_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("ignore_singletons_checkbutton")),
			  "clicked",
			  G_CALLBACK (ignore_singletons_checkbutton_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("use_singletons_catalog_checkbutton")),
			  "clicked",
			  G_CALLBACK (use_singletons_catalog_checkbutton_clicked_cb),
			  data);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("include_subfolders_checkbutton")), TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("ignore_singletons_checkbutton")), FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("use_singletons_catalog_checkbutton")), FALSE);
	gtk_widget_set_sensitive (GET_WIDGET ("single_catalog_box"), FALSE);

	/* run dialog. */

	gtk_widget_show (data->dialog);
}
