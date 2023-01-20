/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 The Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-file-data.h"
#include "gth-vfs-tree.h"
#include "gth-location-chooser.h"
#include "gth-location-chooser-dialog.h"
#include "gth-main.h"


#define MIN_WIDTH 600
#define MIN_HEIGHT 600


struct _GthLocationChooserDialogPrivate {
	GFile     *folder;
	GtkWidget *folder_tree;
	GtkWidget *entry;
	gulong     entry_changed_id;
};


G_DEFINE_TYPE_WITH_CODE (GthLocationChooserDialog,
			 gth_location_chooser_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthLocationChooserDialog))


static void
gth_location_chooser_dialog_finalize (GObject *object)
{
	GthLocationChooserDialog *self;

	self = GTH_LOCATION_CHOOSER_DIALOG (object);

	_g_object_unref (self->priv->folder);

	G_OBJECT_CLASS (gth_location_chooser_dialog_parent_class)->finalize (object);
}


static void
gth_location_chooser_dialog_class_init (GthLocationChooserDialogClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_location_chooser_dialog_finalize;
}


static void
gth_location_chooser_dialog_init (GthLocationChooserDialog *self)
{
	self->priv = gth_location_chooser_dialog_get_instance_private (self);
	self->priv->folder = NULL;
	self->priv->entry_changed_id = 0;
}


static void
_set_folder (GthLocationChooserDialog *self,
	     GFile                    *folder)
{
	if (self->priv->folder != folder) {
		_g_object_unref (self->priv->folder);
		self->priv->folder = _g_object_ref (folder);
	}

	if (self->priv->folder != NULL) {
		g_signal_handler_block (self->priv->entry, self->priv->entry_changed_id);
		gth_location_chooser_set_current (GTH_LOCATION_CHOOSER (self->priv->entry), self->priv->folder);
		g_signal_handler_unblock (self->priv->entry, self->priv->entry_changed_id);
	}

	gtk_dialog_set_response_sensitive (GTK_DIALOG (self), GTK_RESPONSE_OK, (self->priv->folder != NULL));
}


static void
folder_tree_changed_cb (GthVfsTree *tree,
			gpointer    user_data)
{
	GthLocationChooserDialog *self = user_data;
	_set_folder (self, gth_vfs_tree_get_folder (tree));
}


static void
location_entry_changed_cb (GthLocationChooser *entry,
			   gpointer            user_data)
{
	GthLocationChooserDialog *self = user_data;
	GFile                    *folder;

	folder = gth_location_chooser_get_current (entry);
	if (_g_file_equal (folder, gth_folder_tree_get_root (GTH_FOLDER_TREE (self->priv->folder_tree)))) {
		gtk_tree_view_collapse_all (GTK_TREE_VIEW (self->priv->folder_tree));
		_set_folder (self, NULL);
	}
	else
		gth_location_chooser_dialog_set_folder (self, folder);
}


static void
hidden_files_toggled_cb (GtkToggleButton *togglebutton,
			 gpointer         user_data)
{
	GthLocationChooserDialog *self = user_data;

	gth_vfs_tree_set_show_hidden (GTH_VFS_TREE (self->priv->folder_tree), gtk_toggle_button_get_active (togglebutton));
}


static void
_gth_location_chooser_dialog_construct (GthLocationChooserDialog *self)
{
	GtkWidget *vbox;
	GtkWidget *scrolled_window;
	GtkWidget *check_button;

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), vbox, TRUE, TRUE, 0);

	self->priv->entry = g_object_new (GTH_TYPE_LOCATION_CHOOSER,
					  "show-entry-points", FALSE,
					  "show-other", FALSE,
					  "show-root", TRUE,
					  NULL);
	self->priv->entry_changed_id =
		g_signal_connect (self->priv->entry,
				  "changed",
				  G_CALLBACK (location_entry_changed_cb),
				  self);

	gtk_widget_show (self->priv->entry);
	gtk_box_pack_start (GTK_BOX (vbox), self->priv->entry, FALSE, FALSE, 0);

	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	_gtk_dialog_add_to_window_group (GTK_DIALOG (self));

	gtk_dialog_add_button (GTK_DIALOG (self), _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), _GTK_LABEL_OK, GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (self), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (self), GTK_RESPONSE_OK, FALSE);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_IN);
	gtk_widget_set_size_request (scrolled_window, MIN_WIDTH, MIN_HEIGHT);
	gtk_widget_show (scrolled_window);

	self->priv->folder_tree = gth_vfs_tree_new (NULL, NULL);
	g_signal_connect (self->priv->folder_tree,
			  "changed",
			  G_CALLBACK (folder_tree_changed_cb),
			  self);
	gtk_widget_show (self->priv->folder_tree);
	gtk_container_add (GTK_CONTAINER (scrolled_window), self->priv->folder_tree);
	gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

	check_button = gtk_check_button_new_with_label (_("Hidden Files"));
	g_signal_connect (check_button,
			  "toggled",
			  G_CALLBACK (hidden_files_toggled_cb),
			  self);
	gtk_widget_show (check_button);
	gtk_box_pack_start (GTK_BOX (vbox), check_button, TRUE, TRUE, 0);

	_set_folder (self, NULL);
}


GtkWidget *
gth_location_chooser_dialog_new (const char *title,
				 GtkWindow  *parent)
{
	GthLocationChooserDialog *self;

	self = g_object_new (GTH_TYPE_LOCATION_CHOOSER_DIALOG,
			     "title", title,
			     "transient-for", parent,
			     "modal", TRUE,
			     "resizable", TRUE,
			     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			     NULL);
	_gth_location_chooser_dialog_construct (self);

	return (GtkWidget *) self;
}


void
gth_location_chooser_dialog_set_folder (GthLocationChooserDialog *self,
					GFile                    *folder)
{
	g_return_if_fail (GTH_IS_LOCATION_CHOOSER_DIALOG (self));
	g_return_if_fail (folder != NULL);

	gth_vfs_tree_set_folder (GTH_VFS_TREE (self->priv->folder_tree), folder);
}


GFile *
gth_location_chooser_dialog_get_folder (GthLocationChooserDialog *self)
{
	g_return_val_if_fail (GTH_IS_LOCATION_CHOOSER_DIALOG (self), NULL);

	return self->priv->folder;
}
