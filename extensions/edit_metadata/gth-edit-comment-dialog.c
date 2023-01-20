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
#include "gth-edit-comment-dialog.h"
#include "gth-edit-metadata-dialog.h"


struct _GthEditCommentDialogPrivate {
	GtkWidget *notebook;
	GtkWidget *save_changed_checkbutton;
};


static void gth_edit_comment_dialog_gth_edit_metadata_dialog_interface_init (GthEditMetadataDialogInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthEditCommentDialog,
			 gth_edit_comment_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthEditCommentDialog)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_EDIT_METADATA_DIALOG,
						gth_edit_comment_dialog_gth_edit_metadata_dialog_interface_init))



static void
gth_edit_comment_dialog_set_file_list (GthEditMetadataDialog *base,
				       GList                 *file_list)
{
	GthEditCommentDialog *self = GTH_EDIT_COMMENT_DIALOG (base);
	int                   n_files;
	GList                *pages;
	GList                *scan;

	/* update the widgets */

	n_files = g_list_length (file_list);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->save_changed_checkbutton), n_files > 1);
	gtk_widget_set_sensitive (self->priv->save_changed_checkbutton, n_files > 1);

	pages = gtk_container_get_children (GTK_CONTAINER (self->priv->notebook));
	for (scan = pages; scan; scan = scan->next)
		gth_edit_comment_page_set_file_list (GTH_EDIT_COMMENT_PAGE (scan->data), file_list);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (self),
					   GTK_RESPONSE_APPLY,
					   n_files > 0);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (self),
					   GTK_RESPONSE_OK,
					   n_files > 0);

	g_list_free (pages);
}


static void
gth_edit_comment_dialog_update_info (GthEditMetadataDialog *base,
				     GList                 *file_list /* GthFileData list */)
{
	GthEditCommentDialog *self = GTH_EDIT_COMMENT_DIALOG (base);
	gboolean              only_modified_fields;
	GList                *pages;
	GList                *scan;

	only_modified_fields = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->save_changed_checkbutton));
	pages = gtk_container_get_children (GTK_CONTAINER (self->priv->notebook));
	for (scan = pages; scan; scan = scan->next) {
		GList *scan_file;

		for (scan_file = file_list; scan_file; scan_file = scan_file->next) {
			GthFileData *file_data = scan_file->data;
			gth_edit_comment_page_update_info (GTH_EDIT_COMMENT_PAGE (scan->data), file_data->info, only_modified_fields);
		}
	}

	g_list_free (pages);
}


static void
gth_edit_comment_dialog_gth_edit_metadata_dialog_interface_init (GthEditMetadataDialogInterface *iface)
{
	iface->set_file_list = gth_edit_comment_dialog_set_file_list;
	iface->update_info = gth_edit_comment_dialog_update_info;
}


static void
gth_edit_comment_dialog_class_init (GthEditCommentDialogClass *klass)
{
	/* void */
}


static void
gth_edit_comment_dialog_init (GthEditCommentDialog *self)
{
	GtkWidget *vbox;
	GArray    *pages;
	int        i;

	self->priv = gth_edit_comment_dialog_get_instance_private (self);

	gtk_window_set_title (GTK_WINDOW (self), _("Comment"));
	gtk_window_set_resizable (GTK_WINDOW (self), TRUE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
	gtk_widget_show (vbox);
	gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), vbox, TRUE, TRUE, 0);

	self->priv->notebook = gtk_notebook_new ();
	gtk_widget_show (self->priv->notebook);
	gtk_box_pack_start (GTK_BOX (vbox), self->priv->notebook, TRUE, TRUE, 0);

	self->priv->save_changed_checkbutton = gtk_check_button_new_with_mnemonic (_("Save only cha_nged fields"));
	gtk_widget_show (self->priv->save_changed_checkbutton);
	gtk_box_pack_start (GTK_BOX (vbox), self->priv->save_changed_checkbutton, FALSE, FALSE, 0);

	pages = gth_main_get_type_set ("edit-comment-dialog-page");
	if (pages == NULL)
		return;

	for (i = 0; i < pages->len; i++) {
		GType      page_type;
		GtkWidget *page;

		page_type = g_array_index (pages, GType, i);
		page = g_object_new (page_type, NULL);
		if (! GTH_IS_EDIT_COMMENT_PAGE (page)) {
			g_object_unref (page);
			continue;
		}

		gtk_widget_show (page);
		gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
					  page,
					  gtk_label_new (gth_edit_comment_page_get_name (GTH_EDIT_COMMENT_PAGE (page))));
	}
}


/* -- gth_edit_comment_dialog_page -- */


G_DEFINE_INTERFACE (GthEditCommentPage, gth_edit_comment_page, 0)


static void
gth_edit_comment_page_default_init (GthEditCommentPageInterface *iface)
{
	/* void */
}


void
gth_edit_comment_page_set_file_list (GthEditCommentPage *self,
				      GList               *file_list)
{
	GTH_EDIT_COMMENT_PAGE_GET_INTERFACE (self)->set_file_list (self, file_list);
}


void
gth_edit_comment_page_update_info (GthEditCommentPage *self,
				    GFileInfo           *info,
				    gboolean             only_modified_fields)
{
	GTH_EDIT_COMMENT_PAGE_GET_INTERFACE (self)->update_info (self, info, only_modified_fields);
}


const char *
gth_edit_comment_page_get_name (GthEditCommentPage *self)
{
	return GTH_EDIT_COMMENT_PAGE_GET_INTERFACE (self)->get_name (self);
}
