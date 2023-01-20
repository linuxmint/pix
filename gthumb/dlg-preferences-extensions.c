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
#include <gtk/gtk.h>
#include "dlg-preferences-extensions.h"
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gtk-utils.h"
#include "main.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define DEFAULT_ICON "extension-symbolic"
#define EXTENSION_CATEGORY_ALL "*"
#define EXTENSION_CATEGORY_ENABLED "+"
#define EXTENSION_CATEGORY_DISABLED "-"
#define EXTENSION_CATEGORY_SEPARATOR "---"
#define BROWSER_DATA_KEY "extensions-preference-data"
#define CARDINALITY_FORMAT "(%d)"


enum {
	CATEGORY_ID_COLUMN,
	CATEGORY_NAME_COLUMN,
	CATEGORY_ICON_COLUMN,
	CATEGORY_SEPARATOR_COLUMN,
	CATEGORY_CARDINALITY_COLUMN,
	CATEGORY_COLUMNS
};


typedef struct {
	const char *id;
	const char *name;
	const char *icon;
} ExtensionCategory;


static ExtensionCategory extension_category[] = {
	{ EXTENSION_CATEGORY_ALL, NC_("Extensions", "All"), "folder" },
	{ EXTENSION_CATEGORY_ENABLED, NC_("Extensions", "Enabled"), "folder" },
	{ EXTENSION_CATEGORY_DISABLED, NC_("Extensions", "Disabled"), "folder" },
	{ EXTENSION_CATEGORY_SEPARATOR, NULL, NULL },
	{ "Browser", N_("Browser"), "folder" },
	{ "Viewer", N_("Viewers"), "folder" },
	{ "Metadata", N_("Metadata"), "folder" },
	{ "File-Tool", N_("File tools"), "folder" },
	{ "List-Tool", N_("List tools"), "folder" },
	{ "Importer", N_("Importers"), "folder" },
	{ "Exporter", N_("Exporters"), "folder" },
	{ NULL, NULL, NULL }
};


typedef struct {
	GthBrowser   *browser;
	GtkBuilder   *builder;
	GSettings    *settings;
	GtkWidget    *dialog;
	GtkWidget    *extensions_list;
	GList        *active_extensions;
	char         *current_category;
	gboolean      enabled_disabled_cardinality_changed;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_list_foreach (data->active_extensions, (GFunc) g_free, NULL);
	g_list_free (data->active_extensions);
	g_object_unref (data->settings);
	g_object_unref (data->builder);
	g_free (data->current_category);
	g_free (data);
}


typedef struct {
	BrowserData             *browser_data;
	GthExtensionDescription *description;
	gboolean                 original_status_is_active;
} RowData;


static RowData *
row_data_new (BrowserData		*browser_data,
	      GthExtensionDescription	*description)
{
	RowData *row_data;

	row_data = g_new0 (RowData, 1);
	row_data->browser_data = browser_data;
	row_data->description = g_object_ref (description);
	row_data->original_status_is_active = gth_extension_description_is_active (description);

	return row_data;
}


static void
row_data_free (RowData *row_data)
{
	if (row_data == NULL)
		return;
	g_object_unref (row_data->description);
	g_free (row_data);
}


static gboolean
list_equal (GList *list1,
	    GList *list2)
{
	GList *sscan1;

	if (g_list_length (list1) != g_list_length (list2))
		return FALSE;

	for (sscan1 = list1; sscan1; sscan1 = sscan1->next) {
		char  *name1 = sscan1->data;
		GList *sscan2;

		for (sscan2 = list2; sscan2; sscan2 = sscan2->next) {
			char *name2 = sscan2->data;

			if (strcmp (name1, name2) == 0)
				break;
		}

		if (sscan2 == NULL)
			return FALSE;
	}

	return TRUE;
}


static void
add_category_combobox_columns (GtkWidget   *combo_box,
			       BrowserData *data)
{
	GtkCellRenderer *renderer;

	/* the name column */

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box),
					renderer,
					"text", CATEGORY_NAME_COLUMN,
					NULL);

	/* the cardinality column */

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "size", 8000, NULL);
	gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (combo_box), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box),
					renderer,
					"text", CATEGORY_CARDINALITY_COLUMN,
					NULL);
}


static void
reset_original_extension_status (BrowserData *data)
{
	GList *rows;
	GList *scan;

	rows = gtk_container_get_children (GTK_CONTAINER (data->extensions_list));
	for (scan = rows; scan; scan = scan->next) {
		GObject *row = scan->data;
		RowData *row_data = g_object_get_data (G_OBJECT (row), "extension-row-data");

		if (row_data == NULL)
			continue;

		row_data->original_status_is_active = gth_extension_description_is_active (row_data->description);
	}
}


static int
get_category_cardinality (BrowserData *data,
			  const char  *category_name)
{
	GList *rows;
	GList *scan;
	int    n;

	n = 0;
	rows = gtk_container_get_children (GTK_CONTAINER (data->extensions_list));
	for (scan = rows; scan; scan = scan->next) {
		GObject *row = scan->data;
		RowData *row_data = g_object_get_data (G_OBJECT (row), "extension-row-data");

		if (row_data == NULL)
			continue;

		if (g_strcmp0 (category_name, EXTENSION_CATEGORY_ALL) == 0)
			n += 1;
		else if (g_strcmp0 (category_name, EXTENSION_CATEGORY_ENABLED) == 0)
			n += row_data->original_status_is_active ? 1 : 0;
		else if (g_strcmp0 (category_name, EXTENSION_CATEGORY_DISABLED) == 0)
			n += row_data->original_status_is_active ? 0 : 1;
		else if (g_strcmp0 (category_name, row_data->description->category) == 0)
			n += 1;
	}

	return n;
}


static void
update_enabled_disabled_cardinality (BrowserData *data)
{
	GtkTreeModel        *tree_model;
	GtkTreeIter          iter;
	GtkTreeRowReference *enabled_iter = NULL;
	GtkTreeRowReference *disabled_iter = NULL;
	GtkTreePath         *path;

	if (! data->enabled_disabled_cardinality_changed)
		return;

	tree_model = GTK_TREE_MODEL (GET_WIDGET ("category_liststore"));
	if (gtk_tree_model_get_iter_first (tree_model, &iter)) {
		do {
			char *category_id;

			gtk_tree_model_get (tree_model,
					    &iter,
					    CATEGORY_ID_COLUMN, &category_id,
					    -1);

			if (g_strcmp0 (category_id, EXTENSION_CATEGORY_ENABLED) == 0) {
				path = gtk_tree_model_get_path (tree_model, &iter);
				enabled_iter = gtk_tree_row_reference_new  (tree_model, path);
				gtk_tree_path_free (path);
			}

			if (g_strcmp0 (category_id, EXTENSION_CATEGORY_DISABLED) == 0) {
				path = gtk_tree_model_get_path (tree_model, &iter);
				disabled_iter = gtk_tree_row_reference_new  (tree_model, path);
				gtk_tree_path_free (path);
			}

			g_free (category_id);
		}
		while (gtk_tree_model_iter_next (tree_model, &iter));
	}

	path = gtk_tree_row_reference_get_path (enabled_iter);
	if (path != NULL) {
		if (gtk_tree_model_get_iter (tree_model, &iter, path)) {
			char *s;

			s = g_strdup_printf (CARDINALITY_FORMAT, get_category_cardinality (data, EXTENSION_CATEGORY_ENABLED));
			gtk_list_store_set (GTK_LIST_STORE (tree_model),
					    &iter,
					    CATEGORY_CARDINALITY_COLUMN, s,
					    -1);

			g_free (s);
		}
		gtk_tree_path_free (path);
	}

	path = gtk_tree_row_reference_get_path (disabled_iter);
	if (path != NULL) {
		if (gtk_tree_model_get_iter (tree_model, &iter, path)) {
			char *s;

			s = g_strdup_printf (CARDINALITY_FORMAT, get_category_cardinality (data, EXTENSION_CATEGORY_DISABLED));
			gtk_list_store_set (GTK_LIST_STORE (tree_model),
					    &iter,
					    CATEGORY_CARDINALITY_COLUMN, s,
					    -1);

			g_free (s);
		}
		gtk_tree_path_free (path);
	}

	gtk_tree_row_reference_free (enabled_iter);
	gtk_tree_row_reference_free (disabled_iter);

	data->enabled_disabled_cardinality_changed = FALSE;
}


static void
category_combobox_changed_cb (GtkComboBox *combo_box,
			      gpointer     user_data)
{
	BrowserData *data = user_data;
	GtkTreeIter iter;

	if (! gtk_combo_box_get_active_iter (combo_box, &iter))
		return;

	reset_original_extension_status (data);
	update_enabled_disabled_cardinality (data);

	g_free (data->current_category);
	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("category_liststore")),
			    &iter,
			    CATEGORY_ID_COLUMN, &data->current_category,
			    -1);
	gtk_list_box_invalidate_filter (GTK_LIST_BOX (data->extensions_list));
}


static gboolean
category_view_separator_func (GtkTreeModel *model,
			      GtkTreeIter  *iter,
			      gpointer      data)
{
	gboolean separator;

	gtk_tree_model_get (model, iter, CATEGORY_SEPARATOR_COLUMN, &separator, -1);
	return separator;
}


static void
change_switch_state (gpointer user_data)
{
	gtk_switch_set_active (GTK_SWITCH (user_data), ! gtk_switch_get_active (GTK_SWITCH (user_data)));
}


static void
extension_switch_activated_cb (GObject    *gobject,
			       GParamSpec *pspec,
			       gpointer    user_data)
{
	RowData			*row_data = user_data;
	BrowserData		*browser_data = row_data->browser_data;
	GthExtensionDescription *description = row_data->description;
	GError                  *error = NULL;

	if (gtk_switch_get_active (GTK_SWITCH (gobject))) {
		if (! gth_extension_manager_activate (gth_main_get_default_extension_manager (), description->id, &error))
			_gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser_data->dialog), _("Could not activate the extension"), error);
		else
			browser_data->enabled_disabled_cardinality_changed = TRUE;
	}
	else {
		if (! gth_extension_manager_deactivate (gth_main_get_default_extension_manager (), description->id, &error))
			_gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser_data->dialog), _("Could not deactivate the extension"), error);
		else
			browser_data->enabled_disabled_cardinality_changed = TRUE;
	}

	if (error != NULL) {
		/* reset to the previous state */
		call_when_idle (change_switch_state, gobject);
	}

	g_clear_error (&error);
}


static void
extension_information_button_clicked_cb (GtkButton *button,
					 gpointer   user_data)
{
	RowData			*row_data = user_data;
	BrowserData		*browser_data = row_data->browser_data;
	GthExtensionDescription *description = row_data->description;
	GtkWidget               *dialog;

	dialog = gtk_about_dialog_new ();
	if (description->name != NULL)
		g_object_set (dialog, "program-name", description->name, NULL);
	if (description->description != NULL)
		g_object_set (dialog, "comments", description->description, NULL);
	if (description->version != NULL)
		g_object_set (dialog, "version", description->version, NULL);
	if (description->authors != NULL)
		g_object_set (dialog, "authors", description->authors, NULL);
	if (description->copyright != NULL)
		g_object_set (dialog, "copyright", description->copyright, NULL);
	if (description->icon_name != NULL)
		g_object_set (dialog, "logo-icon-name", description->icon_name, NULL);
	else
		g_object_set (dialog, "logo-icon-name", DEFAULT_ICON, NULL);
	if (description->url != NULL)
		g_object_set (dialog, "website", description->url, NULL);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (browser_data->dialog));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}


static void
extension_preferences_button_clicked_cb (GtkButton *button,
					 gpointer   user_data)
{
	RowData			*row_data = user_data;
	BrowserData		*browser_data = row_data->browser_data;
	GthExtensionDescription *description = row_data->description;

	gth_extension_configure (gth_extension_description_get_extension (description),
				 GTK_WINDOW (browser_data->dialog));
}


static GtkWidget *
create_extensions_row (GthExtensionDescription	*description,
		       BrowserData		*browser_data)
{
	GtkWidget    *row;
	GtkWidget    *row_box;
	RowData      *row_data;
	GtkWidget    *button;
	GtkWidget    *label_box;
	GtkWidget    *label;
	GthExtension *extension;

	row = gtk_list_box_row_new ();
	row_data = row_data_new (browser_data, description);
	g_object_set_data_full (G_OBJECT (row), "extension-row-data", row_data, (GDestroyNotify) row_data_free);

	row_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_container_set_border_width (GTK_CONTAINER (row_box), 10);
	gtk_container_add (GTK_CONTAINER (row), row_box);

	button = gtk_switch_new ();
	gtk_switch_set_active (GTK_SWITCH (button), gth_extension_description_is_active (description));
	gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
	g_signal_connect (button,
			  "notify::active",
			  G_CALLBACK (extension_switch_activated_cb),
			  row_data);
	gtk_box_pack_start (GTK_BOX (row_box), button, FALSE, FALSE, 3);

	label_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

	label = gtk_label_new (description->name);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
	gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
	gtk_style_context_add_class (gtk_widget_get_style_context (label), "extension-name");
	gtk_box_pack_start (GTK_BOX (label_box), label, FALSE, FALSE, 0);

	label = gtk_label_new (description->description);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
	gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
	gtk_style_context_add_class (gtk_widget_get_style_context (label), "extension-description");
	gtk_box_pack_start (GTK_BOX (label_box), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (row_box), label_box, TRUE, TRUE, 3);

	extension = gth_extension_description_get_extension (description);
	if ((extension != NULL) && gth_extension_is_configurable (extension)) {
		button = gtk_button_new_from_icon_name ("emblem-system-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_widget_set_tooltip_text (button, _("Preferences"));
		gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
		g_signal_connect (button,
				  "clicked",
				  G_CALLBACK (extension_preferences_button_clicked_cb),
				  row_data);
		gtk_box_pack_start (GTK_BOX (row_box), button, FALSE, FALSE, 0);
	}
	if ((extension != NULL) && (g_strcmp0 (description->authors[0], _("gThumb Development Team")) != 0)) {
		button = gtk_button_new_from_icon_name ("dialog-information-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
		g_signal_connect (button,
				  "clicked",
				  G_CALLBACK (extension_information_button_clicked_cb),
				  row_data);
		gtk_box_pack_start (GTK_BOX (row_box), button, FALSE, FALSE, 0);
	}

	gtk_widget_show_all (row);

	return row;
}


static gboolean
extensions_list_filter_func (GtkListBoxRow *row,
			     gpointer       user_data)
{
	BrowserData *browser_data = user_data;
	RowData     *row_data;
	gboolean     visible;

	if ((browser_data->current_category == NULL) || g_strcmp0 (browser_data->current_category, EXTENSION_CATEGORY_ALL) == 0)
		return TRUE;

	row_data = g_object_get_data (G_OBJECT (row), "extension-row-data");
	if (row_data == NULL)
		return FALSE;

	visible = FALSE;
	if ((g_strcmp0 (browser_data->current_category, EXTENSION_CATEGORY_ENABLED) == 0) && row_data->original_status_is_active)
		visible = TRUE;
	else if ((g_strcmp0 (browser_data->current_category, EXTENSION_CATEGORY_DISABLED) == 0) && ! row_data->original_status_is_active)
		visible = TRUE;
	else
		visible = g_strcmp0 (row_data->description->category, browser_data->current_category) == 0;

	return visible;
}


static int
sort_extensions_by_name (gconstpointer a,
			 gconstpointer b)
{
	const GthExtensionDescription *description_a = a;
	const GthExtensionDescription *description_b = b;

	return g_strcmp0 (description_a->name, description_b->name);
}


void
extensions__dlg_preferences_construct_cb (GtkWidget  *dialog,
					  GthBrowser *browser,
					  GtkBuilder *dialog_builder)
{
	BrowserData          *data;
	GthExtensionManager  *manager;
	GList                *extensions;
	GList                *descriptions;
	GList                *scan;
	char                **all_active_extensions;
	int                   i;
	GtkWidget            *label;
	GtkWidget            *page;

	data = g_new0 (BrowserData, 1);
	data->builder = _gtk_builder_new_from_file ("extensions-preferences.ui", NULL);
	data->settings = g_settings_new (GTHUMB_GENERAL_SCHEMA);
	data->dialog = dialog;
	data->enabled_disabled_cardinality_changed = FALSE;

	/* save the active extensions to decide if a restart is needed */

	manager = gth_main_get_default_extension_manager ();
	data->active_extensions = NULL;
	all_active_extensions = g_settings_get_strv (data->settings, PREF_GENERAL_ACTIVE_EXTENSIONS);
	for (i = 0; all_active_extensions[i] != NULL; i++) {
		char                    *name = all_active_extensions[i];
		GthExtensionDescription *description;

		description = gth_extension_manager_get_description (manager, name);
		if ((description == NULL) || description->mandatory || description->hidden)
			continue;

		data->active_extensions = g_list_prepend (data->active_extensions, g_strdup (name));
	}
	data->active_extensions = g_list_reverse (data->active_extensions);
	g_strfreev (all_active_extensions);

	/* the extensions list */

	data->extensions_list = gtk_list_box_new ();
	gtk_list_box_set_selection_mode (GTK_LIST_BOX (data->extensions_list), GTK_SELECTION_NONE);
	gtk_list_box_set_filter_func (GTK_LIST_BOX (data->extensions_list),
				      extensions_list_filter_func,
				      data,
				      NULL);

	extensions = gth_extension_manager_get_extensions (manager);
	descriptions = NULL;
	for (scan = extensions; scan; scan = scan->next) {
		const char              *name = scan->data;
		GthExtensionDescription *description;

		description = gth_extension_manager_get_description (manager, name);
		if ((description == NULL) || description->mandatory || description->hidden)
			continue;

		descriptions = g_list_prepend (descriptions, description);
	}
	descriptions = g_list_sort (descriptions, sort_extensions_by_name);

	for (scan = descriptions; scan; scan = scan->next) {
		GthExtensionDescription *description = scan->data;
		gtk_container_add (GTK_CONTAINER (data->extensions_list), create_extensions_row (description, data));
	}

	gtk_widget_show (data->extensions_list);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("extensions_list_scrolledwindow")), data->extensions_list);

	g_list_free (descriptions);
	g_list_free (extensions);

	/* the category combobox */

	add_category_combobox_columns (GET_WIDGET ("category_combobox"), data);

	data->current_category = g_strdup (EXTENSION_CATEGORY_ALL);
	for (i = 0; extension_category[i].id != NULL; i++) {
		GtkTreeIter  iter;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("category_liststore")), &iter);
		if (strcmp (extension_category[i].id, EXTENSION_CATEGORY_SEPARATOR) == 0)
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("category_liststore")),
					    &iter,
					    CATEGORY_SEPARATOR_COLUMN, TRUE,
					    -1);
		else {
			char *cardinality;

			cardinality = g_strdup_printf (CARDINALITY_FORMAT, get_category_cardinality (data, extension_category[i].id));
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("category_liststore")),
					    &iter,
					    CATEGORY_NAME_COLUMN, g_dpgettext2 (NULL, "Extensions", extension_category[i].name),
					    CATEGORY_ID_COLUMN, extension_category[i].id,
					    /* CATEGORY_ICON_COLUMN, extension_category[i].icon, */
					    CATEGORY_SEPARATOR_COLUMN, FALSE,
					    CATEGORY_CARDINALITY_COLUMN, cardinality,
					    -1);

			g_free (cardinality);
		}
	}
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (GET_WIDGET ("category_combobox")),
					      category_view_separator_func,
					      data,
					      NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("category_combobox")), 0);


	/* Set the signals handlers. */

	g_signal_connect (GET_WIDGET ("category_combobox"),
			  "changed",
			  G_CALLBACK (category_combobox_changed_cb),
			  data);

	/* add the page to the preferences dialog */

	label = gtk_label_new (_("Extensions"));
	gtk_widget_show (label);

	page = _gtk_builder_get_widget (data->builder, "preferences_page");
	gtk_widget_show (page);
	gtk_notebook_append_page (GTK_NOTEBOOK (_gtk_builder_get_widget (dialog_builder, "notebook")), page, label);

	g_object_set_data_full (G_OBJECT (dialog), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


void
extensions__dlg_preferences_apply (GtkWidget  *dialog,
		  	  	   GthBrowser *browser,
		  	  	   GtkBuilder *dialog_builder)
{
	BrowserData         *data;
	GList               *active_extensions;
	GthExtensionManager *manager;
	GList               *extension_names;
	GList               *scan;

	data = g_object_get_data (G_OBJECT (dialog), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	active_extensions = NULL;
	manager = gth_main_get_default_extension_manager ();
	extension_names = gth_extension_manager_get_extensions (manager);
	for (scan = extension_names; scan; scan = scan->next) {
		char                    *extension_name = scan->data;
		GthExtensionDescription *description;

		description = gth_extension_manager_get_description (manager, extension_name);
		if ((description == NULL) || description->mandatory || description->hidden)
			continue;

		if (gth_extension_description_is_active (description))
			active_extensions = g_list_prepend (active_extensions, g_strdup (extension_name));
	}
	active_extensions = g_list_reverse (active_extensions);
	_g_settings_set_string_list (data->settings, PREF_GENERAL_ACTIVE_EXTENSIONS, active_extensions);

	if (! list_equal (active_extensions, data->active_extensions)) {
		GtkWidget *dialog;
		int        response;

		dialog = _gtk_message_dialog_new (GTK_WINDOW (data->dialog),
						  GTK_DIALOG_MODAL,
						  _GTK_ICON_NAME_DIALOG_WARNING,
						  _("Restart required"),
						  _("You need to restart gthumb for these changes to take effect"),
						  _("_Continue"), GTK_RESPONSE_CANCEL,
						  _("_Restart"), GTK_RESPONSE_OK,
						  NULL);
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (response == GTK_RESPONSE_OK)
			gth_quit (TRUE);
	}

	g_list_foreach (active_extensions, (GFunc) g_free, NULL);
	g_list_free (active_extensions);
	g_list_free (extension_names);
}
