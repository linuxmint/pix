/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "oauth-account-chooser-dialog.h"

#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


G_DEFINE_TYPE (OAuthAccountChooserDialog, oauth_account_chooser_dialog, GTK_TYPE_DIALOG)


enum {
	ACCOUNT_DATA_COLUMN,
	ACCOUNT_NAME_COLUMN,
	ACCOUNT_SEPARATOR_COLUMN,
	ACCOUNT_ICON_COLUMN
};


struct _OAuthAccountChooserDialogPrivate {
	GtkBuilder *builder;
};


static void
oauth_account_chooser_dialog_finalize (GObject *object)
{
	OAuthAccountChooserDialog *self;

	self = OAUTH_ACCOUNT_CHOOSER_DIALOG (object);

	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (oauth_account_chooser_dialog_parent_class)->finalize (object);
}


static void
oauth_account_chooser_dialog_class_init (OAuthAccountChooserDialogClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (OAuthAccountChooserDialogPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = oauth_account_chooser_dialog_finalize;
}


static void
account_combobox_changed_cb (GtkComboBox *combobox,
			     gpointer     user_data)
{
	OAuthAccountChooserDialog *self = user_data;
	GtkTreeIter                iter;
	OAuthAccount              *account;

	if (! gtk_combo_box_get_active_iter (combobox, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("account_liststore")), &iter,
			    ACCOUNT_DATA_COLUMN, &account,
			    -1);

	if (account == NULL)
		gtk_dialog_response (GTK_DIALOG (self), OAUTH_ACCOUNT_CHOOSER_RESPONSE_NEW);

	_g_object_unref (account);
}


static gboolean
row_separator_func (GtkTreeModel *model,
		    GtkTreeIter  *iter,
		    gpointer      user_data)
{
	OAuthAccountChooserDialog *self = user_data;
	gboolean                    is_separator;

	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("account_liststore")),
			    iter,
			    ACCOUNT_SEPARATOR_COLUMN, &is_separator,
			    -1);

	return is_separator;
}


static void
oauth_account_chooser_dialog_init (OAuthAccountChooserDialog *self)
{
	GtkWidget *content;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, OAUTH_TYPE_ACCOUNT_CHOOSER_DIALOG, OAuthAccountChooserDialogPrivate);
	self->priv->builder = _gtk_builder_new_from_file ("oauth-account-chooser.ui", "oauth");

	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	content = _gtk_builder_get_widget (self->priv->builder, "account_chooser");
	gtk_container_set_border_width (GTK_CONTAINER (content), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	{
		GtkCellLayout   *cell_layout;
		GtkCellRenderer *renderer;

		cell_layout = GTK_CELL_LAYOUT (GET_WIDGET ("account_combobox"));

		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"icon-name", ACCOUNT_ICON_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, TRUE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"text", ACCOUNT_NAME_COLUMN,
						NULL);
	}
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")),
					      row_separator_func,
					      self,
					      NULL);
	g_signal_connect (GET_WIDGET ("account_combobox"),
			  "changed",
			  G_CALLBACK (account_combobox_changed_cb),
			  self);

	gtk_dialog_add_button (GTK_DIALOG (self),
			       GTK_STOCK_NEW,
			       OAUTH_ACCOUNT_CHOOSER_RESPONSE_NEW);
	gtk_dialog_add_button (GTK_DIALOG (self),
			       GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self),
			       GTK_STOCK_OK,
			       GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
}


static void
oauth_account_chooser_dialog_construct (OAuthAccountChooserDialog *self,
				         GList                      *accounts,
				         OAuthAccount              *default_account)
{
	GtkTreeIter  iter;
	GList       *scan;
	int          active = 0;
	int          idx;

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("account_liststore")));

	for (scan = accounts, idx = 0; scan; scan = scan->next, idx++) {
		OAuthAccount *account = scan->data;

		if ((default_account != NULL) && (g_strcmp0 (account->username, default_account->username) == 0))
			active = idx;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
				    ACCOUNT_DATA_COLUMN, account,
				    ACCOUNT_NAME_COLUMN, account->username,
				    ACCOUNT_SEPARATOR_COLUMN, FALSE,
				    ACCOUNT_ICON_COLUMN, "dialog-password",
				    -1);
	}

	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
			    ACCOUNT_SEPARATOR_COLUMN, TRUE,
			    -1);

	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
			    ACCOUNT_DATA_COLUMN, NULL,
			    ACCOUNT_NAME_COLUMN, _("New authentication..."),
			    ACCOUNT_SEPARATOR_COLUMN, FALSE,
			    ACCOUNT_ICON_COLUMN, GTK_STOCK_NEW,
			    -1);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")), active);
}


GtkWidget *
oauth_account_chooser_dialog_new (GList        *accounts,
				  OAuthAccount *default_account)
{
	OAuthAccountChooserDialog *self;

	self = g_object_new (OAUTH_TYPE_ACCOUNT_CHOOSER_DIALOG, NULL);
	oauth_account_chooser_dialog_construct (self, accounts, default_account);

	return (GtkWidget *) self;
}


OAuthAccount *
oauth_account_chooser_dialog_get_active (OAuthAccountChooserDialog *self)
{
	GtkTreeIter    iter;
	OAuthAccount *account;

	if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")), &iter))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("account_liststore")), &iter,
			    ACCOUNT_DATA_COLUMN, &account,
			    -1);

	return account;
}
