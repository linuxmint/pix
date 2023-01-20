/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2009 Free Software Foundation, Inc.
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
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include "gio-utils.h"
#include "glib-utils.h"
#include "gth-file-source.h"
#include "gth-location-chooser.h"
#include "gth-location-chooser-dialog.h"
#include "gth-main.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"


#define MIN_WIDTH 200


typedef enum {
	ITEM_TYPE_NONE,
	ITEM_TYPE_SEPARATOR,
	ITEM_TYPE_LOCATION,
	ITEM_TYPE_ENTRY_POINT,
	ITEM_TYPE_CHOOSE_LOCATION
} ItemType;

enum {
	ICON_COLUMN,
	NAME_COLUMN,
	URI_COLUMN,
	TYPE_COLUMN,
	ELLIPSIZE_COLUMN,
	N_COLUMNS
};

enum {
	PROP_0,
	PROP_SHOW_ENTRY_POINTS,
	PROP_SHOW_OTHER,
	PROP_SHOW_ROOT,
	PROP_RELIEF
};

enum {
	CHANGED,
	LAST_SIGNAL
};


struct _GthLocationChooserPrivate {
	GtkWidget      *combo;
	GtkTreeStore   *model;
	GFile          *location;
	GthFileSource  *file_source;
	gulong          entry_points_changed_id;
	guint           update_entry_list_id;
	guint           update_location_list_id;
	gboolean        show_entry_points;
	gboolean        show_root;
	gboolean        show_other;
	GtkReliefStyle  relief;
	gboolean        reload;
};


static guint gth_location_chooser_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthLocationChooser,
			 gth_location_chooser,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthLocationChooser))


static void
gth_location_chooser_set_property (GObject      *object,
				   guint         property_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
	GthLocationChooser *self;

	self = GTH_LOCATION_CHOOSER (object);

	switch (property_id) {
	case PROP_SHOW_ENTRY_POINTS:
		gth_location_chooser_set_show_entry_points (self, g_value_get_boolean (value));
		break;
	case PROP_SHOW_ROOT:
		self->priv->show_root = g_value_get_boolean (value);
		break;
	case PROP_SHOW_OTHER:
		self->priv->show_other = g_value_get_boolean (value);
		break;
	case PROP_RELIEF:
		gth_location_chooser_set_relief (self, g_value_get_enum (value));
		break;
	default:
		break;
	}
}


static void
gth_location_chooser_get_property (GObject    *object,
				   guint       property_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
	GthLocationChooser *self;

	self = GTH_LOCATION_CHOOSER (object);

	switch (property_id) {
	case PROP_SHOW_ENTRY_POINTS:
		g_value_set_boolean (value, self->priv->show_entry_points);
		break;
	case PROP_SHOW_ROOT:
		g_value_set_boolean (value, self->priv->show_root);
		break;
	case PROP_SHOW_OTHER:
		g_value_set_boolean (value, self->priv->show_other);
		break;
	case PROP_RELIEF:
		g_value_set_enum (value, self->priv->relief);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_location_chooser_finalize (GObject *object)
{
	GthLocationChooser *self;

	self = GTH_LOCATION_CHOOSER (object);

	if (self->priv->update_location_list_id != 0)
		g_source_remove (self->priv->update_location_list_id);
	if (self->priv->update_entry_list_id != 0)
		g_source_remove (self->priv->update_entry_list_id);
	if (self->priv->entry_points_changed_id != 0)
		g_signal_handler_disconnect (gth_main_get_default_monitor (),
					     self->priv->entry_points_changed_id);
	if (self->priv->file_source != NULL)
		g_object_unref (self->priv->file_source);
	if (self->priv->location != NULL)
		g_object_unref (self->priv->location);

	G_OBJECT_CLASS (gth_location_chooser_parent_class)->finalize (object);
}


static gboolean
get_nth_separator_pos (GthLocationChooser *self,
		       int                 pos,
		       int                *idx)
{
	GtkTreeIter iter;
	gboolean    n_found = 0;

	if (idx != NULL)
		*idx = 0;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->model), &iter))
		return FALSE;

	do {
		int item_type = ITEM_TYPE_NONE;

		gtk_tree_model_get (GTK_TREE_MODEL (self->priv->model),
				    &iter,
				    TYPE_COLUMN, &item_type,
				    -1);
		if (item_type == ITEM_TYPE_SEPARATOR) {
			n_found++;
			if (n_found == pos)
				break;
		}

		if (idx != NULL)
			*idx = *idx + 1;
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->model), &iter));

	return n_found == pos;
}


static gboolean
get_iter_from_current_file_entries (GthLocationChooser *self,
				    GFile              *file,
				    GtkTreeIter        *iter)
{
	gboolean  found = FALSE;
	char     *uri;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->model), iter))
		return FALSE;

	uri = g_file_get_uri (file);
	do {
		int   item_type = ITEM_TYPE_NONE;
		char *list_uri;

		gtk_tree_model_get (GTK_TREE_MODEL (self->priv->model),
				    iter,
				    TYPE_COLUMN, &item_type,
				    URI_COLUMN, &list_uri,
				    -1);
		if (item_type == ITEM_TYPE_SEPARATOR)
			break;
		if (_g_str_equal (uri, list_uri)) {
			found = TRUE;
			g_free (list_uri);
			break;
		}
		g_free (list_uri);
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->model), iter));

	g_free (uri);

	return found;
}


static void
combo_changed_cb (GtkComboBox *widget,
		  gpointer     user_data)
{
	GthLocationChooser *self = user_data;
	GtkTreeIter         iter;
	char               *uri = NULL;
	int                 item_type = ITEM_TYPE_NONE;

	if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (self->priv->combo), &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (self->priv->model),
			    &iter,
			    TYPE_COLUMN, &item_type,
			    URI_COLUMN, &uri,
			    -1);

	if (item_type == ITEM_TYPE_CHOOSE_LOCATION) {
		GtkWidget *dialog;

		dialog = gth_location_chooser_dialog_new (_("Location"), _gtk_widget_get_toplevel_if_window (GTK_WIDGET (widget)));
		if (self->priv->location != NULL)
			gth_location_chooser_dialog_set_folder (GTH_LOCATION_CHOOSER_DIALOG (dialog), self->priv->location);

		switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
		case GTK_RESPONSE_OK:
			gth_location_chooser_set_current (self, gth_location_chooser_dialog_get_folder (GTH_LOCATION_CHOOSER_DIALOG (dialog)));
			break;

		default:
			/* reset the previous value. */
			gth_location_chooser_set_current (self, self->priv->location);
			break;
		}

		gtk_widget_destroy (dialog);
	}
	else if (uri != NULL) {
		GFile *file;

		file = g_file_new_for_uri (uri);
		gth_location_chooser_set_current (self, file);

		g_object_unref (file);
	}
}


static void
add_file_source_entries (GthLocationChooser *self,
			 GFile              *file,
			 const char         *name,
			 GIcon              *icon,
			 int                 position,
			 gboolean            update_active_iter,
			 int                 iter_type)
{
	GtkTreeIter  iter;
	char        *uri;

	uri = g_file_get_uri (file);

	gtk_tree_store_insert (self->priv->model, &iter, NULL, position);
	gtk_tree_store_set (self->priv->model, &iter,
			    TYPE_COLUMN, iter_type,
			    ICON_COLUMN, icon,
			    NAME_COLUMN, name,
			    URI_COLUMN, uri,
			    ELLIPSIZE_COLUMN, PANGO_ELLIPSIZE_END,
			    -1);

	g_free (uri);

	if (update_active_iter && g_file_equal (self->priv->location, file)) {
		g_signal_handlers_block_by_func (self->priv->combo, combo_changed_cb, self);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->priv->combo), &iter);
		g_signal_handlers_unblock_by_func (self->priv->combo, combo_changed_cb, self);
	}
}


static void
clear_items_from_separator (GthLocationChooser *self,
			    int                 nth_separator,
			    gboolean            stop_at_next_separator)
{
	int first_position;
	int i;

	if (! get_nth_separator_pos (self, nth_separator, &first_position))
		return;

	for (i = first_position + 1; TRUE; i++) {
		GtkTreePath *path;
		GtkTreeIter  iter;

		path = gtk_tree_path_new_from_indices (first_position + 1, -1);
		if (gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->model), &iter, path)) {
			if (stop_at_next_separator) {
				int item_type = ITEM_TYPE_NONE;

				gtk_tree_model_get (GTK_TREE_MODEL (self->priv->model),
						    &iter,
						    TYPE_COLUMN, &item_type,
						    -1);
				if (item_type == ITEM_TYPE_SEPARATOR)
					break;
			}
			gtk_tree_store_remove (self->priv->model, &iter);
		}
		else
			break;

		gtk_tree_path_free (path);
	}
}


static void
delete_section_by_type (GthLocationChooser *self,
			ItemType            type_to_delete)
{
	GtkTreeIter iter;
	gboolean    valid;
	gboolean    prev_matched;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->model), &iter))
		return;

	prev_matched = FALSE;
	do {
		int item_type = ITEM_TYPE_NONE;

		gtk_tree_model_get (GTK_TREE_MODEL (self->priv->model),
				    &iter,
				    TYPE_COLUMN, &item_type,
				    -1);

		if (item_type == type_to_delete) {
			valid = gtk_tree_store_remove (self->priv->model, &iter);
			prev_matched = TRUE;
		}
		else {
			if (prev_matched && (item_type == ITEM_TYPE_SEPARATOR))
				valid = gtk_tree_store_remove (self->priv->model, &iter);
			else
				valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->model), &iter);
			prev_matched = FALSE;
		}
	}
	while (valid);
}


static gboolean
get_section_end_by_type (GthLocationChooser *self,
			 ItemType            item_to_search,
			 int                *p_pos)
{
	GtkTreeIter iter;
	int         pos;
	gboolean    valid;
	gboolean    prev_matched;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->model), &iter))
		return FALSE;

	pos = 0;
	valid = FALSE;
	prev_matched = FALSE;
	do {
		int item_type = ITEM_TYPE_NONE;

		gtk_tree_model_get (GTK_TREE_MODEL (self->priv->model),
				    &iter,
				    TYPE_COLUMN, &item_type,
				    -1);

		if (item_type == item_to_search) {
			*p_pos = pos;
			valid = TRUE;
			prev_matched = TRUE;
		}
		else {
			if ((item_type == ITEM_TYPE_SEPARATOR) && prev_matched)
				*p_pos = pos;
			prev_matched = FALSE;
		}

		pos++;
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->model), &iter));

	return valid;
}


static void
update_entry_point_list (GthLocationChooser *self)
{
	int    position;
	GList *entry_points;
	GList *scan;

	self->priv->update_entry_list_id = 0;

	delete_section_by_type (self, ITEM_TYPE_ENTRY_POINT);

	if (get_section_end_by_type (self, ITEM_TYPE_LOCATION, &position))
		position = position + 1;
	else
		position = 0;

	entry_points = gth_main_get_all_entry_points ();
	for (scan = entry_points; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		add_file_source_entries (self,
					 file_data->file,
					 g_file_info_get_display_name (file_data->info),
					 g_file_info_get_symbolic_icon (file_data->info),
					 position++,
					 FALSE,
					 ITEM_TYPE_ENTRY_POINT);
	}

	if (self->priv->show_other) {
		GtkTreeIter iter;

		gtk_tree_store_insert (self->priv->model, &iter, NULL, position);
		gtk_tree_store_set (self->priv->model, &iter,
				    TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
				    -1);
	}

	_g_object_list_unref (entry_points);
}


static gboolean
row_separator_func (GtkTreeModel *model,
		    GtkTreeIter  *iter,
		    gpointer      data)
{
	int item_type = ITEM_TYPE_NONE;

	gtk_tree_model_get (model,
			    iter,
			    TYPE_COLUMN, &item_type,
			    -1);

	return (item_type == ITEM_TYPE_SEPARATOR);
}


static void
entry_points_changed_cb (GthMonitor         *monitor,
			 GthLocationChooser *self)
{
	if (! self->priv->show_entry_points)
		return;
	if (self->priv->update_entry_list_id != 0)
		return;
	self->priv->update_entry_list_id = call_when_idle ((DataFunc) update_entry_point_list, self);
}


static void
update_location_list (gpointer user_data)
{
	GthLocationChooser *self = user_data;
	GtkTreeIter         iter;

	self->priv->update_location_list_id = 0;

	if (self->priv->location == NULL)
		return;

	if (! self->priv->reload && get_iter_from_current_file_entries (self, self->priv->location, &iter)) {
		g_signal_handlers_block_by_func (self->priv->combo, combo_changed_cb, self);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->priv->combo), &iter);
		g_signal_handlers_unblock_by_func (self->priv->combo, combo_changed_cb, self);
	}
	else {
		GList *list;
		GList *scan;
		int    position = 0;

		delete_section_by_type (self, ITEM_TYPE_LOCATION);

		list = gth_file_source_get_current_list (self->priv->file_source, self->priv->location);
		for (scan = list; scan; scan = scan->next) {
			GFile     *file = scan->data;
			GFileInfo *info;

			info = _g_file_get_info_for_display (file);
			if (info == NULL)
				continue;

			add_file_source_entries (self,
						 file,
						 g_file_info_get_display_name (info),
						 g_file_info_get_symbolic_icon (info),
						 position++,
						 TRUE,
						 ITEM_TYPE_LOCATION);

			g_object_unref (info);
		}

		if (self->priv->show_root) {
			GIcon *icon;

			icon = g_themed_icon_new ("computer-symbolic");
			gtk_tree_store_insert (self->priv->model, &iter, NULL, position++);
			gtk_tree_store_set (self->priv->model, &iter,
					    TYPE_COLUMN, ITEM_TYPE_LOCATION,
					    ICON_COLUMN, icon,
					    NAME_COLUMN, _("Locations"),
					    URI_COLUMN, "gthumb-vfs:///",
					    -1);

			_g_object_unref (icon);
		}

		if (self->priv->show_other || self->priv->show_entry_points) {
			GtkTreeIter iter;

			gtk_tree_store_insert (self->priv->model, &iter, NULL, position);
			gtk_tree_store_set (self->priv->model, &iter,
					    TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
					    -1);
		}

		_g_object_list_unref (list);
	}

	self->priv->reload = FALSE;
}


static void
current_location_changed (GthLocationChooser *self)
{
	if (self->priv->update_location_list_id != 0)
		g_source_remove (self->priv->update_location_list_id);
	self->priv->update_location_list_id = call_when_idle ((DataFunc) update_location_list, self);
}


static void
gth_location_chooser_realize (GtkWidget *widget)
{
	GthLocationChooser *self = GTH_LOCATION_CHOOSER (widget);

	GTK_WIDGET_CLASS (gth_location_chooser_parent_class)->realize (widget);

	if (self->priv->show_other) {
		GtkTreeIter iter;

		gtk_tree_store_append (self->priv->model, &iter, NULL);
		gtk_tree_store_set (self->priv->model, &iter,
				    TYPE_COLUMN, ITEM_TYPE_CHOOSE_LOCATION,
				    NAME_COLUMN, _("Other…"),
				    ELLIPSIZE_COLUMN, FALSE,
				    -1);
	}
	entry_points_changed_cb (NULL, self);
	current_location_changed (self);
}


static void
gth_location_chooser_class_init (GthLocationChooserClass *klass)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_location_chooser_set_property;
	object_class->get_property = gth_location_chooser_get_property;
	object_class->finalize = gth_location_chooser_finalize;

	widget_class = (GtkWidgetClass *) klass;
	widget_class->realize = gth_location_chooser_realize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_SHOW_ENTRY_POINTS,
					 g_param_spec_boolean ("show-entry-points",
							       "Show entry points",
							       "Whether to show the entry points in the list",
							       TRUE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_SHOW_ROOT,
					 g_param_spec_boolean ("show-root",
							       "Show the VFS root",
							       "Whether to show the VFS root in the list",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_SHOW_OTHER,
					 g_param_spec_boolean ("show-other",
							       "Show the Other... entry",
							       "Whether to show a special entry to choose a location from a dialog",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_RELIEF,
					 g_param_spec_enum ("relief",
							    "Border relief",
							    "The border relief style",
							    GTK_TYPE_RELIEF_STYLE,
							    GTK_RELIEF_NORMAL,
							    G_PARAM_READWRITE));

	/* signals */

	gth_location_chooser_signals[CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthLocationChooserClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
gth_location_chooser_init (GthLocationChooser *self)
{
	GtkCellRenderer *renderer;

	gtk_widget_set_can_focus (GTK_WIDGET (self), FALSE);
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);

	self->priv = gth_location_chooser_get_instance_private (self);
	self->priv->entry_points_changed_id = 0;
	self->priv->show_entry_points = TRUE;
	self->priv->show_root = FALSE;
	self->priv->show_other = TRUE;
	self->priv->relief = GTK_RELIEF_NORMAL;
	self->priv->reload = FALSE;

	self->priv->model = gtk_tree_store_new (N_COLUMNS,
						G_TYPE_ICON,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_INT,
						PANGO_TYPE_ELLIPSIZE_MODE);
	self->priv->combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (self->priv->model));
	g_object_unref (self->priv->model);
	g_signal_connect (self->priv->combo,
			  "changed",
			  G_CALLBACK (combo_changed_cb),
			  self);
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (self->priv->combo),
					      row_separator_func,
					      self,
					      NULL);
	gtk_widget_set_size_request (self->priv->combo, MIN_WIDTH, -1);

	/* icon column */

	renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_set (renderer,
		      "follow-state", TRUE,
		      NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->priv->combo),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (self->priv->combo),
					 renderer,
					 "gicon", ICON_COLUMN,
					 NULL);

	/* path column */

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->priv->combo),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (self->priv->combo),
					renderer,
					"text", NAME_COLUMN,
					"ellipsize", ELLIPSIZE_COLUMN,
					NULL);

	/**/

	gtk_widget_show (self->priv->combo);
	gtk_box_pack_start (GTK_BOX (self), self->priv->combo, TRUE, TRUE, 0);
}


GtkWidget *
gth_location_chooser_new (void)
{
	return GTK_WIDGET (g_object_new (GTH_TYPE_LOCATION_CHOOSER, NULL));
}


/* -- gth_location_chooser_set_relief -- */


void
gth_location_chooser_set_relief (GthLocationChooser *self,
				 GtkReliefStyle      value)
{
	if (self->priv->relief == value)
		return;

	self->priv->relief = value;
	g_object_notify (G_OBJECT (self), "relief");
}


GtkReliefStyle
gth_location_chooser_get_relief (GthLocationChooser *self)
{
	return self->priv->relief;
}


void
gth_location_chooser_set_show_entry_points (GthLocationChooser *self,
					    gboolean            value)
{
	self->priv->show_entry_points = value;

	if (self->priv->show_entry_points) {
		if (self->priv->entry_points_changed_id == 0)
			self->priv->entry_points_changed_id =
					g_signal_connect (gth_main_get_default_monitor (),
							  "entry-points-changed",
							  G_CALLBACK (entry_points_changed_cb),
							  self);
		entry_points_changed_cb (NULL, self);
	}
	else {
		if (self->priv->entry_points_changed_id != 0) {
			g_source_remove (self->priv->entry_points_changed_id);
			self->priv->entry_points_changed_id = 0;
		}
		clear_items_from_separator (self, 1, TRUE);
	}

	g_object_notify (G_OBJECT (self), "show-entry-points");
}


gboolean
gth_location_chooser_get_show_entry_points (GthLocationChooser *self)
{
	return self->priv->show_entry_points;
}


void
gth_location_chooser_set_current (GthLocationChooser *self,
				  GFile              *file)
{
	if (file == NULL)
		return;

	if (file != self->priv->location) {
		if (self->priv->file_source != NULL)
			g_object_unref (self->priv->file_source);
		self->priv->file_source = gth_main_get_file_source (file);

		if (self->priv->file_source == NULL)
			return;

		if (self->priv->location != NULL) {
			g_object_unref (self->priv->location);
			self->priv->location = NULL;
		}

		if (file != NULL)
			self->priv->location = g_file_dup (file);
	}

	if (gtk_widget_get_realized (GTK_WIDGET (self)))
		current_location_changed (self);

	g_signal_emit (G_OBJECT (self), gth_location_chooser_signals[CHANGED], 0);
}


GFile *
gth_location_chooser_get_current (GthLocationChooser *self)
{
	return self->priv->location;
}

void
gth_location_chooser_reload (GthLocationChooser *self)
{
	if (self->priv->location == NULL)
		return;

	self->priv->reload = TRUE;

	if (gtk_widget_get_realized (GTK_WIDGET (self)))
		current_location_changed (self);
}
