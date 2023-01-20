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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gthumb.h>
#include "dlg-personalize-scripts.h"
#include "gth-script.h"
#include "gth-script-editor-dialog.h"
#include "gth-script-file.h"
#include "shortcuts.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define ORDER_CHANGED_DELAY 250


enum {
	COLUMN_SCRIPT,
	COLUMN_NAME,
	COLUMN_SHORTCUT,
	COLUMN_VISIBLE,
	NUM_COLUMNS
};


typedef struct {
	GthBrowser   *browser;
	GtkBuilder   *builder;
	GtkWidget    *dialog;
	GtkWidget    *list_view;
	GtkListStore *list_store;
	gulong        scripts_changed_id;
	guint         list_changed_id;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	if (data->list_changed_id != 0)
		g_source_remove (data->list_changed_id);
	data->list_changed_id = 0;

	gth_browser_set_dialog (data->browser, "personalize_scripts", NULL);
	g_signal_handler_disconnect (gth_script_file_get (), data->scripts_changed_id);

	g_object_unref (data->builder);
	g_free (data);
}


static gboolean
list_view_row_order_changed_cb (gpointer user_data)
{
	DialogData   *data = user_data;
	GtkTreeModel *model = GTK_TREE_MODEL (data->list_store);
	GtkTreeIter   iter;

	if (data->list_changed_id != 0)
		g_source_remove (data->list_changed_id);
	data->list_changed_id = 0;

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		GthScriptFile *script_file;

		script_file = gth_script_file_get ();
		gth_script_file_clear (script_file);

		do {
			GthScript *script;

			gtk_tree_model_get (model, &iter, COLUMN_SCRIPT, &script, -1);
			gth_script_file_add (script_file, script);

			g_object_unref (script);
		}
		while (gtk_tree_model_iter_next (model, &iter));

		gth_script_file_save (script_file, NULL);
	}

	return FALSE;
}


static void
row_deleted_cb (GtkTreeModel *tree_model,
		GtkTreePath  *path,
		gpointer      user_data)
{
	DialogData *data = user_data;

	if (data->list_changed_id != 0)
		g_source_remove (data->list_changed_id);
	data->list_changed_id = g_timeout_add (ORDER_CHANGED_DELAY, list_view_row_order_changed_cb, data);
}


static void
row_inserted_cb (GtkTreeModel *tree_model,
		 GtkTreePath  *path,
		 GtkTreeIter  *iter,
		 gpointer      user_data)
{
	DialogData *data = user_data;

	if (data->list_changed_id != 0)
		g_source_remove (data->list_changed_id);
	data->list_changed_id = g_timeout_add (ORDER_CHANGED_DELAY, list_view_row_order_changed_cb, data);
}


static char *
get_shortcut_label (DialogData *data,
		    GthScript  *script)
{
	GthShortcut *shortcut;

	shortcut = gth_window_get_shortcut (GTH_WINDOW (data->browser),
					    gth_script_get_detailed_action (script));

	return (shortcut != NULL) ? shortcut->label : "";
}


static void
set_script_list (DialogData *data,
		 GList      *script_list)
{
	GList *scan;

	g_signal_handlers_block_by_func (data->list_store, row_inserted_cb, data);

	for (scan = script_list; scan; scan = scan->next) {
		GthScript   *script = scan->data;
		GtkTreeIter  iter;

		gtk_list_store_append (data->list_store, &iter);
		gtk_list_store_set (data->list_store, &iter,
				    COLUMN_SCRIPT, script,
				    COLUMN_NAME, gth_script_get_display_name (script),
				    COLUMN_SHORTCUT, get_shortcut_label (data, script),
				    COLUMN_VISIBLE, gth_script_is_visible (script),
				   -1);
	}

	g_signal_handlers_unblock_by_func (data->list_store, row_inserted_cb, data);
}


static void
update_script_list (DialogData *data)
{
	GthScriptFile *script_file;
	GList         *script_list;

	g_signal_handlers_block_by_func (data->list_store, row_deleted_cb, data);
	gtk_list_store_clear (data->list_store);
	g_signal_handlers_unblock_by_func (data->list_store, row_deleted_cb, data);

	script_file = gth_script_file_get ();
	script_list = gth_script_file_get_scripts (script_file);
	set_script_list (data, script_list);

	_g_object_list_unref (script_list);
}


static void
scripts_changed_cb (GthScriptFile *script_file,
		    DialogData    *data)
{
	update_script_list (data);
}


static void
cell_renderer_toggle_toggled_cb (GtkCellRendererToggle *cell_renderer,
				 char                  *path,
                                 gpointer               user_data)
{
	DialogData  *data = user_data;
	GtkTreePath *tpath;
	GtkTreeIter  iter;
	gboolean     visible;

	tpath = gtk_tree_path_new_from_string (path);
	if (tpath == NULL)
		return;

	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (data->list_store), &iter, tpath)) {
		GthScript     *script;
		GthScriptFile *script_file;

		gtk_tree_model_get (GTK_TREE_MODEL (data->list_store), &iter,
				    COLUMN_SCRIPT, &script,
				    COLUMN_VISIBLE, &visible,
				    -1);
		visible = ! visible;
		g_object_set (script, "visible", visible, NULL);

		script_file = gth_script_file_get ();
		g_signal_handlers_block_by_func (script_file, scripts_changed_cb, data);
		gth_script_file_add (script_file, script);
		gth_script_file_save (script_file, NULL);
		g_signal_handlers_unblock_by_func (script_file, scripts_changed_cb, data);

		gtk_list_store_set (data->list_store, &iter,
				    COLUMN_VISIBLE, visible,
				    -1);

		g_object_unref (script);
	}

	gtk_tree_path_free (tpath);
}


static void
add_columns (GtkTreeView *treeview,
	     DialogData  *data)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	/* the name column. */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Script"));

	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", COLUMN_NAME,
                                             NULL);

        gtk_tree_view_column_set_expand (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        /* the shortcut column */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Shortcut"));

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "xalign", 0.5, NULL);
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", COLUMN_SHORTCUT,
                                             NULL);

        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	/* the checkbox column */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Show"));

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer,
			  "toggled",
			  G_CALLBACK (cell_renderer_toggle_toggled_cb),
			  data);

	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "active", COLUMN_VISIBLE,
					     NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


static gboolean
get_iter_for_shortcut (DialogData  *data,
		       GthShortcut *shortcut,
		       GtkTreeIter *iter)
{
	GtkTreeModel *model = GTK_TREE_MODEL (data->list_store);
	gboolean      found = FALSE;
	if (! gtk_tree_model_get_iter_first (model, iter))
		return FALSE;

	do {
		GthScript *script;

		gtk_tree_model_get (model, iter, COLUMN_SCRIPT, &script, -1);
		found = g_strcmp0 (shortcut->detailed_action, gth_script_get_detailed_action (script)) == 0;

		g_object_unref (script);
	}
	while (! found && gtk_tree_model_iter_next (model, iter));

	return found;
}


static gboolean
get_iter_script (DialogData  *data,
	         GthScript   *script,
	         GtkTreeIter *iter)
{
	GtkTreeModel *model = GTK_TREE_MODEL (data->list_store);
	gboolean      found = FALSE;
	const char   *script_id;

	script_id = gth_script_get_id (script);
	if (! gtk_tree_model_get_iter_first (model, iter))
		return FALSE;

	do {
		GthScript *list_script;

		gtk_tree_model_get (model, iter, COLUMN_SCRIPT, &list_script, -1);
		found = g_strcmp0 (script_id, gth_script_get_id (list_script)) == 0;

		g_object_unref (list_script);
	}
	while (! found && gtk_tree_model_iter_next (model, iter));

	return found;
}


static void
script_editor_dialog__response_cb (GtkDialog *dialog,
			           int        response,
			           gpointer   user_data)
{
	DialogData    *data = user_data;
	GthScript     *script;
	GError        *error = NULL;
	GPtrArray     *shortcuts_v;
	GthScriptFile *script_file;
	gboolean       new_script;
	GthShortcut   *shortcut;
	GtkTreeIter    iter;
	gboolean       change_list;

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (dialog));
		return;
	}

	script = gth_script_editor_dialog_get_script (GTH_SCRIPT_EDITOR_DIALOG (dialog), &error);
	if (script == NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (dialog), _("Could not save the script"), error);
		g_clear_error (&error);
		return;
	}

	/* update the shortcuts */

	shortcuts_v = _g_ptr_array_dup (gth_window_get_shortcuts (GTH_WINDOW (data->browser)),
					(GCopyFunc) gth_shortcut_dup,
					(GDestroyNotify) gth_shortcut_free);

	/* If another shortcut has the same accelerator, reset the accelerator
	 * for that shortcut. */

	shortcut = gth_shortcut_array_find_by_accel (shortcuts_v,
						     GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER,
						     GTH_SHORTCUT_VIEWER_CONTEXT_ANY,
						     gth_script_get_accelerator (script));
	if (shortcut != NULL) {
		if (g_strcmp0 (shortcut->detailed_action, gth_script_get_detailed_action (script)) != 0) {
			if (get_iter_for_shortcut (data, shortcut, &iter))
				gtk_list_store_set (data->list_store, &iter,
						    COLUMN_SHORTCUT, "",
						    -1);
			gth_shortcut_set_key (shortcut, 0, 0);
		}
	}

	/* update the script shortcut */

	shortcut = gth_shortcut_array_find_by_action (shortcuts_v, gth_script_get_detailed_action (script));
	if (shortcut != NULL)
		g_ptr_array_remove (shortcuts_v, shortcut);

	shortcut = gth_script_create_shortcut (script);
	g_ptr_array_add (shortcuts_v, shortcut);

	/* save the script */

	script_file = gth_script_file_get ();
	new_script = ! gth_script_file_has_script (script_file, script);

	g_signal_handlers_block_by_func (script_file, scripts_changed_cb, data);
	gth_script_file_add (script_file, script);
	gth_script_file_save (script_file, NULL);
	g_signal_handlers_unblock_by_func (script_file, scripts_changed_cb, data);

	gth_main_shortcuts_changed (shortcuts_v);

	/* update the script list */

	if (new_script) {
		g_signal_handlers_block_by_func (data->list_store, row_inserted_cb, data);
		gtk_list_store_append (data->list_store, &iter);
		g_signal_handlers_unblock_by_func (data->list_store, row_inserted_cb, data);
		change_list = TRUE;
	}
	else
		change_list = get_iter_script (data, script, &iter);

	if (change_list)
		gtk_list_store_set (data->list_store, &iter,
				    COLUMN_SCRIPT, script,
				    COLUMN_NAME, gth_script_get_display_name (script),
				    COLUMN_SHORTCUT, shortcut->label,
				    COLUMN_VISIBLE, gth_script_is_visible (script),
				    -1);

	gtk_widget_destroy (GTK_WIDGET (dialog));

	g_ptr_array_unref (shortcuts_v);
	g_object_unref (script);
}


static void
new_script_cb (GtkButton  *button,
	       DialogData *data)
{
	GtkWidget *dialog;

	dialog = gth_script_editor_dialog_new (_("New Command"), GTH_WINDOW (data->browser), GTK_WINDOW (data->dialog));
	g_signal_connect (dialog, "response",
			  G_CALLBACK (script_editor_dialog__response_cb),
			  data);
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
edit_script_cb (GtkButton  *button,
	        DialogData *data)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model = GTK_TREE_MODEL (data->list_store);
	GtkTreeIter       iter;
	GthScript        *script;
	GtkWidget        *dialog;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->list_view));
	if (! gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, COLUMN_SCRIPT, &script, -1);
	if (script == NULL)
		return;

	dialog = gth_script_editor_dialog_new (_("Edit Command"), GTH_WINDOW (data->browser), GTK_WINDOW (data->dialog));
	gth_script_editor_dialog_set_script (GTH_SCRIPT_EDITOR_DIALOG (dialog), script);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (script_editor_dialog__response_cb),
			  data);
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));

	g_object_unref (script);
}


static void
delete_script_cb (GtkButton  *button,
	          DialogData *data)
{
	GtkWidget        *d;
	int               result;
	GtkTreeSelection *selection;
	GtkTreeModel     *model = GTK_TREE_MODEL (data->list_store);
	GtkTreeIter       iter;
	GthScript        *script;
	GPtrArray        *shortcuts_v;
	GthShortcut      *shortcut;
	GthScriptFile    *script_file;

	d = _gtk_message_dialog_new (GTK_WINDOW (data->dialog),
				     GTK_DIALOG_MODAL,
				     _GTK_ICON_NAME_DIALOG_QUESTION,
				     _("Are you sure you want to delete the selected command?"),
				     NULL,
				     _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				     _GTK_LABEL_DELETE, GTK_RESPONSE_OK,
				     NULL);
	result = gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_destroy (d);
	if (result != GTK_RESPONSE_OK)
		return;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->list_view));
	if (! gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, COLUMN_SCRIPT, &script, -1);
	if (script == NULL)
		return;

	/* update the shortcuts */

	shortcuts_v = _g_ptr_array_dup (gth_window_get_shortcuts (GTH_WINDOW (data->browser)),
					(GCopyFunc) gth_shortcut_dup,
					(GDestroyNotify) gth_shortcut_free);

	shortcut = gth_shortcut_array_find_by_action (shortcuts_v, gth_script_get_detailed_action (script));
	if (shortcut != NULL)
		g_ptr_array_remove (shortcuts_v, shortcut);

	/* update the script file */

	script_file = gth_script_file_get ();
	g_signal_handlers_block_by_func (script_file, scripts_changed_cb, data);
	gth_script_file_remove (script_file, script);
	gth_script_file_save (script_file, NULL);
	g_signal_handlers_unblock_by_func (script_file, scripts_changed_cb, data);

	gth_main_shortcuts_changed (shortcuts_v);

	/* update the script list */

	g_signal_handlers_block_by_func (data->list_store, row_deleted_cb, data);
	gtk_list_store_remove (data->list_store, &iter);
	g_signal_handlers_unblock_by_func (data->list_store, row_deleted_cb, data);

	g_object_unref (script);
}


static void
update_sensitivity (DialogData *data)
{
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	gboolean          script_selected;

	model = GTK_TREE_MODEL (data->list_store);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->list_view));
	script_selected =  gtk_tree_selection_get_selected (selection, &model, &iter);

	gtk_widget_set_sensitive (GET_WIDGET ("edit_button"), script_selected);
	gtk_widget_set_sensitive (GET_WIDGET ("delete_button"), script_selected);
}


static void
list_view_selection_changed_cb (GtkTreeSelection *treeselection,
				gpointer          user_data)
{
	update_sensitivity ((DialogData *) user_data);
}


static void
list_view_row_activated_cb (GtkTreeView       *tree_view,
                            GtkTreePath       *path,
                            GtkTreeViewColumn *column,
                            gpointer           user_data)
{
	edit_script_cb (NULL, user_data);
}


void
dlg_personalize_scripts (GthBrowser *browser)
{
	DialogData *data;

	if (gth_browser_get_dialog (browser, "personalize_scripts") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "personalize_scripts")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = gtk_builder_new_from_resource ("/org/gnome/gThumb/list_tools/data/ui/personalize-scripts.ui");

	/* Get the widgets. */

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Commands"),
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

	gth_browser_set_dialog (browser, "personalize_scripts", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/**/

	data->list_store = gtk_list_store_new (NUM_COLUMNS,
					       G_TYPE_OBJECT,
					       G_TYPE_STRING,
					       G_TYPE_STRING,
					       G_TYPE_BOOLEAN);
	data->list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (data->list_store));
	g_object_unref (data->list_store);
        gtk_tree_view_set_reorderable (GTK_TREE_VIEW (data->list_view), TRUE);
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->list_view), TRUE);

	add_columns (GTK_TREE_VIEW (data->list_view), data);

	gtk_widget_show (data->list_view);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("scripts_scrolledwindow")), data->list_view);

	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("scripts_label")), data->list_view);
	gtk_label_set_use_underline (GTK_LABEL (GET_WIDGET ("scripts_label")), TRUE);

	update_script_list (data);
	update_sensitivity (data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CLOSE),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (GET_WIDGET ("new_button")),
			  "clicked",
			  G_CALLBACK (new_script_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("edit_button")),
			  "clicked",
			  G_CALLBACK (edit_script_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("delete_button")),
			  "clicked",
			  G_CALLBACK (delete_script_cb),
			  data);
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (data->list_view)),
			  "changed",
			  G_CALLBACK (list_view_selection_changed_cb),
			  data);
	g_signal_connect (GTK_TREE_VIEW (data->list_view),
			  "row-activated",
			  G_CALLBACK (list_view_row_activated_cb),
			  data);
	g_signal_connect (data->list_store,
			  "row-deleted",
			  G_CALLBACK (row_deleted_cb),
			  data);
	g_signal_connect (data->list_store,
			  "row-inserted",
			  G_CALLBACK (row_inserted_cb),
			  data);

	data->scripts_changed_id = g_signal_connect (gth_script_file_get (),
				                     "changed",
				                     G_CALLBACK (scripts_changed_cb),
				                     data);

	/* run dialog. */

	gtk_widget_show (data->dialog);
}
