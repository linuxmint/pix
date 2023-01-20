/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2014 The Free Software Foundation, Inc.
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
#include "gth-curve-preset-editor-dialog.h"


#define ORDER_CHANGED_DELAY 250


enum {
	COLUMN_ID,
	COLUMN_NAME,
	COLUMN_ICON_NAME
};


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


struct _GthCurvePresetEditorDialogPrivate {
	GtkBuilder	*builder;
	GthCurvePreset	*preset;
	guint            changed_id;
};


G_DEFINE_TYPE_WITH_CODE (GthCurvePresetEditorDialog,
			 gth_curve_preset_editor_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthCurvePresetEditorDialog))


static void
gth_curve_preset_editor_dialog_finalize (GObject *object)
{
	GthCurvePresetEditorDialog *self;

	self = GTH_CURVE_PRESET_EDITOR_DIALOG (object);

	if (self->priv->changed_id != 0)
		g_source_remove (self->priv->changed_id);
	g_object_unref (self->priv->builder);
	g_object_unref (self->priv->preset);

	G_OBJECT_CLASS (gth_curve_preset_editor_dialog_parent_class)->finalize (object);
}


static void
gth_curve_preset_editor_dialog_class_init (GthCurvePresetEditorDialogClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_curve_preset_editor_dialog_finalize;
}


static void
gth_curve_preset_editor_dialog_init (GthCurvePresetEditorDialog *self)
{
	self->priv = gth_curve_preset_editor_dialog_get_instance_private (self);
	self->priv->changed_id = 0;
}


static void
preset_name_edited_cb (GtkCellRendererText *renderer,
	               gchar               *path,
	               gchar               *new_text,
	               gpointer             user_data)
{
	GthCurvePresetEditorDialog	*self = user_data;
	GtkListStore			*list_store;
	GtkTreePath			*tree_path;
	GtkTreeIter			 iter;
	int				 id;

	list_store = (GtkListStore *) gtk_builder_get_object (self->priv->builder, "preset_liststore");
	tree_path = gtk_tree_path_new_from_string (path);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store),
				       &iter,
				       tree_path))
	{
		gtk_tree_path_free (tree_path);
		return;
	}
	gtk_tree_path_free (tree_path);

	gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, COLUMN_ID, &id, -1);
	gtk_list_store_set (list_store, &iter, COLUMN_NAME, new_text, -1);
	gth_curve_preset_rename (self->priv->preset, id, new_text);
}


static void
delete_toolbutton_clicked_cb (GtkButton *button,
			      gpointer   user_data)
{
	GthCurvePresetEditorDialog	*self = user_data;
	GtkWidget			*tree_view;
	GtkTreeSelection		*tree_selection;
	GtkTreeModel			*tree_model;
	GtkTreeIter			 iter;
	int				 id;

	tree_view = (GtkWidget *) gtk_builder_get_object (self->priv->builder, "preset_treeview");
	tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));

	if (! gtk_tree_selection_get_selected (tree_selection, &tree_model, &iter))
		return;

	gtk_tree_model_get (tree_model, &iter, COLUMN_ID, &id, -1);
	gtk_list_store_remove (GTK_LIST_STORE (tree_model), &iter);
	gth_curve_preset_remove (self->priv->preset, id);
}


static gboolean
order_changed (gpointer user_data)
{
	GthCurvePresetEditorDialog	*self = user_data;
	GList				*id_list;
	GtkTreeModel			*tree_model;
	GtkTreeIter			 iter;

	if (self->priv->changed_id != 0)
		g_source_remove (self->priv->changed_id);
	self->priv->changed_id = 0;

	id_list = NULL;
	tree_model = (GtkTreeModel *) gtk_builder_get_object (self->priv->builder, "preset_liststore");
	if (gtk_tree_model_get_iter_first (tree_model, &iter)) {
		do {
			int id;
			gtk_tree_model_get (tree_model, &iter, COLUMN_ID, &id, -1);
			id_list = g_list_prepend (id_list, GINT_TO_POINTER (id));
		}
		while (gtk_tree_model_iter_next (tree_model, &iter));
	}
	id_list = g_list_reverse (id_list);
	gth_curve_preset_change_order (self->priv->preset, id_list);

	g_list_free (id_list);

	return FALSE;
}


static void
row_deleted_cb (GtkTreeModel *tree_model,
		GtkTreePath  *path,
		gpointer      user_data)
{
	GthCurvePresetEditorDialog *self = user_data;

	if (self->priv->changed_id != 0)
		g_source_remove (self->priv->changed_id);
	self->priv->changed_id = gdk_threads_add_timeout (ORDER_CHANGED_DELAY, order_changed, self);
}


static void
row_inserted_cb (GtkTreeModel *tree_model,
		 GtkTreePath  *path,
		 GtkTreeIter  *iter,
		 gpointer      user_data)
{
	GthCurvePresetEditorDialog *self = user_data;

	if (self->priv->changed_id != 0)
		g_source_remove (self->priv->changed_id);
	self->priv->changed_id = gdk_threads_add_timeout (ORDER_CHANGED_DELAY, order_changed, self);
}


static void
gth_curve_preset_editor_dialog_construct (GthCurvePresetEditorDialog	*self,
					  GtkWindow			*parent,
					  GthCurvePreset		*preset)
{
	GtkWidget    *button;
	GtkWidget    *content;
	GtkListStore *list_store;
	int           n, i;
	GtkTreeIter   iter;

    	self->priv->builder = _gtk_builder_new_from_file ("curve-preset-editor.ui", "file_tools");
    	content = _gtk_builder_get_widget (self->priv->builder, "curve_preset_editor");
  	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	button = gtk_dialog_add_button (GTK_DIALOG (self), _GTK_LABEL_CLOSE, GTK_RESPONSE_CLOSE);
	g_signal_connect_swapped (button,
			          "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  self);

  	g_signal_connect (gtk_builder_get_object (self->priv->builder, "preset_name_cellrenderertext"),
  			  "edited",
			  G_CALLBACK (preset_name_edited_cb),
			  self);

  	self->priv->preset = g_object_ref (preset);

  	list_store = (GtkListStore *) gtk_builder_get_object (self->priv->builder, "preset_liststore");
  	n = gth_curve_preset_get_size (self->priv->preset);
  	for (i = 0; i < n; i++) {
  		const char *name;
  		int         id;

  		gth_curve_preset_get_nth (self->priv->preset, i, &id, &name, NULL);
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    COLUMN_ID, id,
				    COLUMN_NAME, name,
				    COLUMN_ICON_NAME, "curves-symbolic",
				    -1);
  	}

	g_signal_connect (list_store,
			  "row-deleted",
			  G_CALLBACK (row_deleted_cb),
			  self);
	g_signal_connect (list_store,
			  "row-inserted",
			  G_CALLBACK (row_inserted_cb),
			  self);

  	g_signal_connect (gtk_builder_get_object (self->priv->builder, "delete_toolbutton"),
  			  "clicked",
			  G_CALLBACK (delete_toolbutton_clicked_cb),
			  self);
}


GtkWidget *
gth_curve_preset_editor_dialog_new (GtkWindow		*parent,
				    GthCurvePreset	*preset)
{
	GthCurvePresetEditorDialog *self;

	g_return_val_if_fail (preset != NULL, NULL);

	self = g_object_new (GTH_TYPE_CURVE_PRESET_EDITOR_DIALOG,
			     "title", _("Presets"),
			     "transient-for", parent,
			     "resizable", TRUE,
			     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			     NULL);
	gth_curve_preset_editor_dialog_construct (self, parent, preset);

	return (GtkWidget *) self;
}
