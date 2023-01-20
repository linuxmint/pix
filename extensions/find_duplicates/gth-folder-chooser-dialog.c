/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include "gth-folder-chooser-dialog.h"

#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


enum {
	FOLDER_FILE_COLUMN,
	FOLDER_NAME_COLUMN,
	FOLDER_SELECTED_COLUMN
};


struct _GthFolderChooserDialogPrivate {
	GtkBuilder *builder;
};


G_DEFINE_TYPE_WITH_CODE (GthFolderChooserDialog,
			 gth_folder_chooser_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthFolderChooserDialog))


static void
gth_folder_chooser_dialog_finalize (GObject *object)
{
	GthFolderChooserDialog *self;

	self = GTH_FOLDER_CHOOSER_DIALOG (object);

	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (gth_folder_chooser_dialog_parent_class)->finalize (object);
}


static void
gth_folder_chooser_dialog_class_init (GthFolderChooserDialogClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_folder_chooser_dialog_finalize;
}


static void
folder_cellrenderertoggle_toggled_cb (GtkCellRendererToggle *cell_renderer,
                		      char                  *path,
                		      gpointer               user_data)
{
	GthFolderChooserDialog *self = user_data;
	GtkTreeModel           *model;
	GtkTreePath            *tree_path;
	GtkTreeIter             iter;
	gboolean                selected;

	model = GTK_TREE_MODEL (GET_WIDGET ("folders_liststore"));
	tree_path = gtk_tree_path_new_from_string (path);
	if (! gtk_tree_model_get_iter (model, &iter, tree_path)) {
		gtk_tree_path_free (tree_path);
		return;
	}

	gtk_tree_model_get (model, &iter, FOLDER_SELECTED_COLUMN, &selected, -1);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, FOLDER_SELECTED_COLUMN, ! selected, -1);

	gtk_tree_path_free (tree_path);
}


static void
gth_folder_chooser_dialog_init (GthFolderChooserDialog *self)
{
	GtkWidget *content;

	self->priv = gth_folder_chooser_dialog_get_instance_private (self);
	self->priv->builder = _gtk_builder_new_from_file ("find-duplicates-choose-folders.ui", "find_duplicates");

	gtk_window_set_resizable (GTK_WINDOW (self), TRUE);

	content = _gtk_builder_get_widget (self->priv->builder, "folder_chooser");
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (GET_WIDGET ("folders_liststore")), FOLDER_NAME_COLUMN, GTK_SORT_ASCENDING);

	g_signal_connect (GET_WIDGET ("folder_cellrenderertoggle"),
			  "toggled",
			  G_CALLBACK (folder_cellrenderertoggle_toggled_cb),
			  self);

	gtk_dialog_add_button (GTK_DIALOG (self), _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self),  _GTK_LABEL_OK, GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (self), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);
}


static void
gth_folder_chooser_dialog_construct (GthFolderChooserDialog *self,
				     GList                  *folders)
{
	GtkTreeIter  iter;
	GList       *scan;
	int          idx;

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("folders_liststore")));

	for (scan = folders, idx = 0; scan; scan = scan->next, idx++) {
		GFile *folder = scan->data;
		char  *display_name;

		display_name = g_file_get_parse_name (folder);
		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("folders_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("folders_liststore")), &iter,
				    FOLDER_FILE_COLUMN, folder,
				    FOLDER_NAME_COLUMN, display_name,
				    FOLDER_SELECTED_COLUMN, FALSE,
				    -1);

		g_free (display_name);
	}
}


GtkWidget *
gth_folder_chooser_dialog_new (GList *folders)
{
	GthFolderChooserDialog *self;

	self = g_object_new (GTH_TYPE_FOLDER_CHOOSER_DIALOG,
			     /*"title", _("Folders"),*/
			     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			     NULL);
	gth_folder_chooser_dialog_construct (self, folders);

	return (GtkWidget *) self;
}


GHashTable *
gth_folder_chooser_dialog_get_selected (GthFolderChooserDialog *self)
{
	GHashTable   *folders;
	GtkTreeModel *model;
	GtkTreeIter   iter;

	folders = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);

	model = GTK_TREE_MODEL (GET_WIDGET ("folders_liststore"));
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			GFile    *folder;
			gboolean  selected;

			gtk_tree_model_get (model, &iter,
					    FOLDER_FILE_COLUMN, &folder,
					    FOLDER_SELECTED_COLUMN, &selected,
					    -1);

			if (selected)
				g_hash_table_insert (folders, g_object_ref (folder), GINT_TO_POINTER (1));

			g_object_unref (folder);
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	return folders;
}
