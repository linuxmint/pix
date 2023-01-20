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
#include <gthumb.h>
#include "dlg-bookmarks.h"


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GtkWidget  *uri_list;
	char       *last_selected_uri;
	gulong      bookmarks_changed_id;
	gboolean    entry_changed;
} DialogData;


static void
entry_activate_cb (GtkEntry   *entry,
		   DialogData *data);


static void
set_bookmark_data (DialogData *data,
		   const char *name,
		   const char *location)
{
	g_signal_handlers_block_by_func (_gtk_builder_get_widget (data->builder, "entry_name"), entry_activate_cb, data);
	g_signal_handlers_block_by_func (_gtk_builder_get_widget (data->builder, "entry_location"), entry_activate_cb, data);
	gtk_entry_set_text (GTK_ENTRY (_gtk_builder_get_widget (data->builder, "entry_name")), name);
	gtk_entry_set_text (GTK_ENTRY (_gtk_builder_get_widget (data->builder, "entry_location")), location);
	g_signal_handlers_unblock_by_func (_gtk_builder_get_widget (data->builder, "entry_location"), entry_activate_cb, data);
	g_signal_handlers_unblock_by_func (_gtk_builder_get_widget (data->builder, "entry_name"), entry_activate_cb, data);

	data->entry_changed = FALSE;
}


static void
update_dialog_from_bookmark_file (DialogData *data,
				  const char *uri)
{
	GBookmarkFile *bookmarks;
	GFile         *file;
	char          *location;
	char          *name;

	bookmarks = gth_main_get_default_bookmarks ();

	file = g_file_new_for_uri (uri);
	location = g_file_get_parse_name (file);

	name = g_bookmark_file_get_title (bookmarks, uri, NULL);
	if (name == NULL)
		name = g_file_get_basename (file);

	set_bookmark_data (data, name, location);

	g_free (name);
	g_free (location);
	g_object_unref (file);
}


static void
update_current_entry (DialogData *data,
		      gboolean   *update_selected_uri)
{
	const char    *name;
	const char    *location;
	GFile         *file;
	char          *uri;
	GBookmarkFile *bookmarks;

	if (update_selected_uri != NULL)
		*update_selected_uri = TRUE;

	if (data->last_selected_uri == NULL)
		return;

	if (! data->entry_changed)
		return;

	data->entry_changed = FALSE;

	name = gtk_entry_get_text (GTK_ENTRY (_gtk_builder_get_widget (data->builder, "entry_name")));
	location = gtk_entry_get_text (GTK_ENTRY (_gtk_builder_get_widget (data->builder, "entry_location")));
	file = g_file_parse_name (location);
	uri = g_file_get_uri (file);

	bookmarks = gth_main_get_default_bookmarks ();
	gth_uri_list_update_uri (GTH_URI_LIST (data->uri_list), data->last_selected_uri, uri, name);
	gth_uri_list_update_bookmarks (GTH_URI_LIST (data->uri_list), bookmarks);
	gth_main_bookmarks_changed ();

	if (g_strcmp0 (data->last_selected_uri, uri) != 0) {
		g_free (data->last_selected_uri);
		data->last_selected_uri = g_strdup (uri);
		if (update_selected_uri != NULL)
			*update_selected_uri = FALSE;
	}

	g_free (uri);
	g_object_unref (file);
}


static void
uri_list_selection_changed_cb (GtkTreeSelection *treeselection,
                               gpointer          user_data)
{
	DialogData   *data = user_data;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	char         *uri;
	gboolean      update_selected_uri;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->uri_list));
	if (! gtk_tree_selection_get_selected (treeselection,
					       &model,
					       &iter))
	{
		return;
	}

	uri = gth_uri_list_get_uri (GTH_URI_LIST (data->uri_list), &iter);
	if (uri == NULL)
		return;

	update_current_entry (data, &update_selected_uri);
	if (update_selected_uri) {
		g_free (data->last_selected_uri);
		data->last_selected_uri = uri;
	}
	update_dialog_from_bookmark_file (data, uri);
}


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	update_current_entry (data, NULL);
	gth_browser_set_dialog (data->browser, "bookmarks", NULL);
	g_signal_handler_disconnect (gth_main_get_default_monitor (), data->bookmarks_changed_id);

	g_object_unref (data->builder);
	g_free (data);
}


static void
remove_cb (GtkWidget  *widget,
	   DialogData *data)
{
	char          *uri;
	GBookmarkFile *bookmarks;
	GError        *error = NULL;

	uri = gth_uri_list_get_selected (GTH_URI_LIST (data->uri_list));
	if (uri == NULL)
		return;

	bookmarks = gth_main_get_default_bookmarks ();
	if (g_bookmark_file_remove_item (bookmarks, uri, &error)) {
		gth_uri_list_remove_uri (GTH_URI_LIST (data->uri_list), uri);
		gth_main_bookmarks_changed ();
	}
	else {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not remove the bookmark"), error);
		g_clear_error (&error);
	}

	g_free (uri);
}


static void
go_to_cb (GtkWidget  *widget,
	  DialogData *data)
{
	char *uri;

	uri = gth_uri_list_get_selected (GTH_URI_LIST (data->uri_list));
	if (uri != NULL) {
		GFile *location;

		location = g_file_new_for_uri (uri);
		gth_browser_go_to (data->browser, location, NULL);

		g_object_unref (location);
		g_free (uri);
	}
}


static void
bookmarks_changed_cb (GthMonitor *monitor,
		      DialogData *data)
{
	GBookmarkFile    *bookmarks;
	char             *uri;
	GtkTreeSelection *selection;
	gboolean          selected;

	if (data->entry_changed)
		return;

	uri = gth_uri_list_get_selected (GTH_URI_LIST (data->uri_list));

	g_free (data->last_selected_uri);
	data->last_selected_uri = NULL; /* do no update the entry */

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->uri_list));
	g_signal_handlers_block_by_func (selection, uri_list_selection_changed_cb, data);
	bookmarks = gth_main_get_default_bookmarks ();
	gth_uri_list_set_bookmarks (GTH_URI_LIST (data->uri_list), bookmarks);
	g_signal_handlers_unblock_by_func (selection, uri_list_selection_changed_cb, data);

	selected = FALSE;
	if (uri != NULL)
		selected = gth_uri_list_select_uri (GTH_URI_LIST (data->uri_list), uri);

	if (! selected) {
		/* select the last one */

		char **uris;
		char  *last_uri;
		int    i;

		uris = g_bookmark_file_get_uris (bookmarks, NULL);
		last_uri = NULL;
		for (i = 0; uris[i] != NULL; i++)
			last_uri = uris[i];
		if (last_uri != NULL)
			gth_uri_list_select_uri (GTH_URI_LIST (data->uri_list), last_uri);
		else
			set_bookmark_data (data, "", "");

		g_strfreev (uris);
	}

	g_free (uri);
}


static void
uri_list_order_changed_cb (GthUriList *uri_list,
		           DialogData *data)
{
	GBookmarkFile *bookmarks;

	bookmarks = gth_main_get_default_bookmarks ();
	gth_uri_list_update_bookmarks (GTH_URI_LIST (data->uri_list), bookmarks);
	gth_main_bookmarks_changed ();
}


static void
uri_list_row_activated_cb (GtkTreeView       *tree_view,
                           GtkTreePath       *path,
                           GtkTreeViewColumn *column,
                           gpointer           user_data)
{
	DialogData   *data = user_data;
	GtkTreeModel *tree_model;
	GtkTreeIter   iter;
	char         *uri;
	GFile        *location;

	tree_model = gtk_tree_view_get_model (tree_view);
	if (! gtk_tree_model_get_iter (tree_model, &iter, path))
		return;

	uri = gth_uri_list_get_uri (GTH_URI_LIST (tree_view), &iter);
	if (uri == NULL)
		return;

	location = g_file_new_for_uri (uri);
	gth_browser_go_to (data->browser, location, NULL);

	g_object_unref (location);
	g_free (uri);
}


static void
entry_activate_cb (GtkEntry   *entry,
		   DialogData *data)
{
	update_current_entry (data, NULL);
}


static void
entry_changed_cb (GtkEditable *editable,
		  DialogData  *data)
{
	data->entry_changed = TRUE;
}


void
dlg_bookmarks (GthBrowser *browser)
{
	DialogData        *data;
	GtkWidget         *bm_list_container;
	GtkWidget         *bm_bookmarks_label;
	GtkWidget         *bm_remove_button;
	GtkWidget         *bm_go_to_button;
	GBookmarkFile     *bookmarks;
	GtkTreeSelection  *selection;

	if (gth_browser_get_dialog (browser, "bookmarks") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "bookmarks")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = gtk_builder_new_from_resource ("/org/gnome/gThumb/bookmarks/data/ui/bookmarks.ui");
	data->last_selected_uri = NULL;
	data->entry_changed = FALSE;

	/* Get the widgets. */

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Bookmarks"),
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_OK, GTK_RESPONSE_CLOSE,
				NULL);

	gth_browser_set_dialog (browser, "bookmarks", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	bm_list_container = _gtk_builder_get_widget (data->builder, "bm_list_container");
	bm_bookmarks_label = _gtk_builder_get_widget (data->builder, "bm_bookmarks_label");
	bm_remove_button = _gtk_builder_get_widget (data->builder, "bm_remove_button");
	bm_go_to_button = _gtk_builder_get_widget (data->builder, "bm_go_to_button");

	data->uri_list = gth_uri_list_new ();
	gtk_widget_show (data->uri_list);
	gtk_widget_set_vexpand (data->uri_list, TRUE);
	gtk_container_add (GTK_CONTAINER (bm_list_container), data->uri_list);
	gtk_label_set_mnemonic_widget (GTK_LABEL (bm_bookmarks_label), data->uri_list);

	/* Set widgets data. */

	bookmarks = gth_main_get_default_bookmarks ();
	gth_uri_list_set_bookmarks (GTH_URI_LIST (data->uri_list), bookmarks);

	data->bookmarks_changed_id = g_signal_connect (gth_main_get_default_monitor (),
				                       "bookmarks-changed",
				                       G_CALLBACK (bookmarks_changed_cb),
				                       data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CLOSE),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (bm_remove_button),
			  "clicked",
			  G_CALLBACK (remove_cb),
			  data);
	g_signal_connect (G_OBJECT (bm_go_to_button),
			  "clicked",
			  G_CALLBACK (go_to_cb),
			  data);
	g_signal_connect (G_OBJECT (data->uri_list),
			  "order-changed",
			  G_CALLBACK (uri_list_order_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->uri_list),
			  "row-activated",
			  G_CALLBACK (uri_list_row_activated_cb),
			  data);
	g_signal_connect (_gtk_builder_get_widget (data->builder, "entry_location"),
			  "activate",
			  G_CALLBACK (entry_activate_cb),
			  data);
	g_signal_connect (_gtk_builder_get_widget (data->builder, "entry_name"),
			  "activate",
			  G_CALLBACK (entry_activate_cb),
			  data);
	g_signal_connect (_gtk_builder_get_widget (data->builder, "entry_location"),
			  "changed",
			  G_CALLBACK (entry_changed_cb),
			  data);
	g_signal_connect (_gtk_builder_get_widget (data->builder, "entry_name"),
			  "changed",
			  G_CALLBACK (entry_changed_cb),
			  data);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->uri_list));
	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (uri_list_selection_changed_cb),
			  data);

	/* run dialog. */

	gtk_widget_show (data->dialog);
}
