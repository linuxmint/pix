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
#include "oauth-account.h"
#include "oauth-account-manager-dialog.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


enum {
	ACCOUNT_DATA_COLUMN,
	ACCOUNT_NAME_COLUMN,
};


struct _OAuthAccountManagerDialogPrivate {
	GtkBuilder *builder;
};


G_DEFINE_TYPE_WITH_CODE (OAuthAccountManagerDialog,
			 oauth_account_manager_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (OAuthAccountManagerDialog))


static void
oauth_account_manager_dialog_finalize (GObject *object)
{
	OAuthAccountManagerDialog *self;

	self = OAUTH_ACCOUNT_MANAGER_DIALOG (object);

	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (oauth_account_manager_dialog_parent_class)->finalize (object);
}


static void
oauth_account_manager_dialog_class_init (OAuthAccountManagerDialogClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->finalize = oauth_account_manager_dialog_finalize;
}


static void
add_button_clicked_cb (GtkWidget *button,
		       gpointer   user_data)
{
	OAuthAccountManagerDialog *self = user_data;
	gtk_dialog_response (GTK_DIALOG (self), OAUTH_ACCOUNT_CHOOSER_RESPONSE_NEW);
}


static void
delete_button_clicked_cb (GtkWidget *button,
		          gpointer   user_data)
{
	OAuthAccountManagerDialog *self = user_data;
	GtkTreeModel              *tree_model;
	GtkTreeIter                iter;

	if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (GET_WIDGET ("account_treeview"))),
					     &tree_model,
					     &iter))
	{
		gtk_list_store_remove (GTK_LIST_STORE (tree_model), &iter);
	}
}


static void
text_renderer_edited_cb (GtkCellRendererText *renderer,
			 char                *path,
			 char                *new_text,
			 gpointer             user_data)
{
	OAuthAccountManagerDialog *self = user_data;
	GtkTreePath               *tree_path;
	GtkTreeIter                iter;
	OAuthAccount              *account;

	tree_path = gtk_tree_path_new_from_string (path);
	tree_path = gtk_tree_path_new_from_string (path);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (GET_WIDGET ("accounts_liststore")),
				       &iter,
				       tree_path))
	{
		gtk_tree_path_free (tree_path);
		return;
	}
	gtk_tree_path_free (tree_path);

	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("accounts_liststore")), &iter,
			    ACCOUNT_DATA_COLUMN, &account,
			    -1);
	g_object_set (account, "name", new_text, NULL);

	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("accounts_liststore")), &iter,
			    ACCOUNT_DATA_COLUMN, account,
			    ACCOUNT_NAME_COLUMN, new_text,
			    -1);

	g_object_unref (account);
}


static void
oauth_account_manager_dialog_init (OAuthAccountManagerDialog *self)
{
	GtkWidget *content;

	self->priv = oauth_account_manager_dialog_get_instance_private (self);
	self->priv->builder = _gtk_builder_new_from_file ("oauth-account-manager.ui", "oauth");

	content = _gtk_builder_get_widget (self->priv->builder, "account_manager");
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	gtk_dialog_add_buttons (GTK_DIALOG (self),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_GTK_LABEL_SAVE, GTK_RESPONSE_OK,
				NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (self), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	g_object_set (GET_WIDGET ("account_cellrenderertext"), "editable", TRUE, NULL);
        g_signal_connect (GET_WIDGET ("account_cellrenderertext"),
                          "edited",
                          G_CALLBACK (text_renderer_edited_cb),
                          self);

	g_signal_connect (GET_WIDGET ("add_button"),
			  "clicked",
			  G_CALLBACK (add_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("delete_button"),
			  "clicked",
			  G_CALLBACK (delete_button_clicked_cb),
			  self);
}


static void
oauth_account_manager_dialog_construct (OAuthAccountManagerDialog *self,
				        GList                     *accounts)
{
	GtkListStore *list_store;
	GtkTreeIter   iter;
	GList        *scan;

	list_store = GTK_LIST_STORE (GET_WIDGET ("accounts_liststore"));
	gtk_list_store_clear (list_store);
	for (scan = accounts; scan; scan = scan->next) {
		OAuthAccount *account = scan->data;

		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    ACCOUNT_DATA_COLUMN, account,
				    ACCOUNT_NAME_COLUMN, account->name,
				    -1);
	}
}


GtkWidget *
oauth_account_manager_dialog_new (GList *accounts)
{
	OAuthAccountManagerDialog *self;

	self = g_object_new (OAUTH_TYPE_ACCOUNT_MANAGER_DIALOG,
			     "resizable", FALSE,
			     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			     NULL);
	oauth_account_manager_dialog_construct (self, accounts);

	return (GtkWidget *) self;
}


GList *
oauth_account_manager_dialog_get_accounts (OAuthAccountManagerDialog *self)
{
	GList        *accounts;
	GtkTreeModel *tree_model;
	GtkTreeIter   iter;

	tree_model = (GtkTreeModel *) GET_WIDGET ("accounts_liststore");
	if (! gtk_tree_model_get_iter_first (tree_model, &iter))
		return NULL;

	accounts = NULL;
	do {
		OAuthAccount *account;

		gtk_tree_model_get (tree_model, &iter,
				    ACCOUNT_DATA_COLUMN, &account,
				    -1);
		accounts = g_list_prepend (accounts, account);
	}
	while (gtk_tree_model_iter_next (tree_model, &iter));

	return g_list_reverse (accounts);
}
