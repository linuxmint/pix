/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2020 Free Software Foundation, Inc.
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
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-file-data.h"
#include "gth-file-selection-info.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


struct _GthFileSelectionInfoPrivate {
	GtkBuilder *builder;
	GthBrowser *browser;
};


G_DEFINE_TYPE_WITH_CODE (GthFileSelectionInfo,
			 gth_file_selection_info,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthFileSelectionInfo))


static void
gth_file_selection_info_finalize (GObject *object)
{
	GthFileSelectionInfo *self;

	self = GTH_FILE_SELECTION_INFO (object);
	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (gth_file_selection_info_parent_class)->finalize (object);
}


static void
gth_file_selection_info_class_init (GthFileSelectionInfoClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_file_selection_info_finalize;
}


static GthBrowser *
_get_browser (GthFileSelectionInfo *self)
{
	GtkWindow *dialog;

	if (self->priv->browser != NULL)
		return self->priv->browser;

	dialog = _gtk_widget_get_toplevel_if_window (GTK_WIDGET (self));
	if (dialog != NULL)
		self->priv->browser = (GthBrowser *) gtk_window_get_transient_for (dialog);

	return self->priv->browser;
}


static void
next_button_clicked_cb (GtkButton *button,
			gpointer   user_data)
{
	GthFileSelectionInfo *self = user_data;

	self->priv->browser = _get_browser (self);
	if (self->priv->browser != NULL)
		gth_browser_show_next_image (self->priv->browser, FALSE, FALSE);
}


static void
prev_button_clicked_cb (GtkButton *button,
			gpointer   user_data)
{
	GthFileSelectionInfo *self = user_data;

	self->priv->browser = _get_browser (self);
	if (self->priv->browser != NULL)
		gth_browser_show_prev_image (self->priv->browser, FALSE, FALSE);
}


static void
gth_file_selection_info_init (GthFileSelectionInfo *self)
{
	self->priv = gth_file_selection_info_get_instance_private (self);
	self->priv->builder = _gtk_builder_new_from_file ("file-selection-info.ui", NULL);
	self->priv->browser = NULL;

	gtk_box_pack_start (GTK_BOX (self), GET_WIDGET ("content"), TRUE, TRUE, 0);
	g_signal_connect (GET_WIDGET ("next_button"), "clicked", G_CALLBACK (next_button_clicked_cb), self);
	g_signal_connect (GET_WIDGET ("prev_button"), "clicked", G_CALLBACK (prev_button_clicked_cb), self);
}


GtkWidget *
gth_file_selection_info_new (void)
{
	GthFileSelectionInfo *self;

	self = g_object_new (GTH_TYPE_FILE_SELECTION_INFO, NULL);

	return GTK_WIDGET (self);
}


void
gth_file_selection_info_set_file_list (GthFileSelectionInfo	*self,
				       GList			*file_list)
{
	char *title;

	if ((file_list != NULL) && (file_list->next == NULL)) {
		GthFileData *file_data = file_list->data;
		title = g_strdup (g_file_info_get_display_name (file_data->info));
	}
	else {
		int n_files = g_list_length (file_list);
		title = g_strdup_printf (g_dngettext (NULL, "%d file", "%d files", n_files), n_files);
	}

	gtk_label_set_label (GTK_LABEL (GET_WIDGET ("info_label")), title);

	g_free (title);
}


void
gth_file_selection_info_set_visible (GthFileSelectionInfo *self,
				     gboolean              visible)
{
	gtk_revealer_set_reveal_child (GTK_REVEALER (GET_WIDGET ("content")), visible);
}
