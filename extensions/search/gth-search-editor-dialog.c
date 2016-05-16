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
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-search-editor.h"
#include "gth-search-editor-dialog.h"


G_DEFINE_TYPE (GthSearchEditorDialog, gth_search_editor_dialog, GTK_TYPE_DIALOG)


struct _GthSearchEditorDialogPrivate {
	GtkWidget  *search_editor;
};


static void
gth_search_editor_dialog_finalize (GObject *object)
{
	GthSearchEditorDialog *dialog;

	dialog = GTH_SEARCH_EDITOR_DIALOG (object);

	if (dialog->priv != NULL) {
		g_free (dialog->priv);
		dialog->priv = NULL;
	}

	G_OBJECT_CLASS (gth_search_editor_dialog_parent_class)->finalize (object);
}

static void
gth_search_editor_dialog_class_init (GthSearchEditorDialogClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_search_editor_dialog_finalize;
}


static void
gth_search_editor_dialog_init (GthSearchEditorDialog *dialog)
{
	dialog->priv = g_new0 (GthSearchEditorDialogPrivate, 1);
}


static void
gth_search_editor_dialog_construct (GthSearchEditorDialog *self,
				    const char            *title,
				    GthSearch             *search,
			            GtkWindow             *parent)
{
	if (title != NULL)
    		gtk_window_set_title (GTK_WINDOW (self), title);
  	if (parent != NULL)
    		gtk_window_set_transient_for (GTK_WINDOW (self), parent);
    	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

   	self->priv->search_editor = gth_search_editor_new (search);
    	gtk_container_set_border_width (GTK_CONTAINER (self->priv->search_editor), 5);
    	gtk_widget_show (self->priv->search_editor);
  	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), self->priv->search_editor, TRUE, TRUE, 0);
}


GtkWidget *
gth_search_editor_dialog_new (const char *title,
			      GthSearch  *search,
			      GtkWindow  *parent)
{
	GthSearchEditorDialog *self;

	self = g_object_new (GTH_TYPE_SEARCH_EDITOR_DIALOG, NULL);
	gth_search_editor_dialog_construct (self, title, search, parent);

	return (GtkWidget *) self;
}


void
gth_search_editor_dialog_set_search (GthSearchEditorDialog *self,
				     GthSearch             *search)
{
	gth_search_editor_set_search (GTH_SEARCH_EDITOR (self->priv->search_editor), search);
}


GthSearch *
gth_search_editor_dialog_get_search (GthSearchEditorDialog  *self,
				     GError                **error)
{
	return gth_search_editor_get_search (GTH_SEARCH_EDITOR (self->priv->search_editor), error);
}
