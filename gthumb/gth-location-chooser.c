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
#include "glib-utils.h"
#include "gth-file-source.h"
#include "gth-icon-cache.h"
#include "gth-location-chooser.h"
#include "gth-main.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"


#define MIN_WIDTH 200


G_DEFINE_TYPE (GthLocationChooser, gth_location_chooser, GTK_TYPE_BOX)


enum {
	ITEM_TYPE_NONE,
	ITEM_TYPE_SEPARATOR,
	ITEM_TYPE_LOCATION,
	ITEM_TYPE_ENTRY_POINT
};

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
        PROP_RELIEF
};

enum {
	CHANGED,
	LAST_SIGNAL
};

struct _GthLocationChooserPrivate
{
	GtkWidget      *combo;
	GtkWidget      *arrow;
	GtkTreeStore   *model;
	GFile          *location;
	GthIconCache   *icon_cache;
	GthFileSource  *file_source;
	gulong          entry_points_changed_id;
	guint           update_entry_list_id;
	guint           update_location_list_id;
	gboolean        show_entry_points;
	GtkReliefStyle  relief;
};


static guint gth_location_chooser_signals[LAST_SIGNAL] = { 0 };



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


static void
gth_location_chooser_grab_focus (GtkWidget *widget)
{
	GthLocationChooser *self = GTH_LOCATION_CHOOSER (widget);

	gtk_widget_grab_focus (self->priv->combo);
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

	if (uri != NULL) {
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
	GdkPixbuf   *pixbuf;
	char        *uri;

	pixbuf = gth_icon_cache_get_pixbuf (self->priv->icon_cache, icon);
	uri = g_file_get_uri (file);

	gtk_tree_store_insert (self->priv->model, &iter, NULL, position);
	gtk_tree_store_set (self->priv->model, &iter,
			    TYPE_COLUMN, iter_type,
			    ICON_COLUMN, pixbuf,
			    NAME_COLUMN, name,
			    URI_COLUMN, uri,
			    ELLIPSIZE_COLUMN, PANGO_ELLIPSIZE_END,
			    -1);

	g_free (uri);
	g_object_unref (pixbuf);

	if (update_active_iter && g_file_equal (self->priv->location, file)) {
		g_signal_handlers_block_by_func (self->priv->combo, combo_changed_cb, self);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->priv->combo), &iter);
		g_signal_handlers_unblock_by_func (self->priv->combo, combo_changed_cb, self);
	}
}


static void
clear_entry_point_list (GthLocationChooser *self)
{
	int first_position;
	int i;

	if (! get_nth_separator_pos (self, 1, &first_position))
		return;

	for (i = first_position + 1; TRUE; i++) {
		GtkTreePath *path;
		GtkTreeIter  iter;

		path = gtk_tree_path_new_from_indices (first_position + 1, -1);
		if (gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->model), &iter, path))
			gtk_tree_store_remove (self->priv->model, &iter);
		else
			break;

		gtk_tree_path_free (path);
	}
}


static void
update_entry_point_list (GthLocationChooser *self)
{
	int    first_position;
	int    position;
	GList *entry_points;
	GList *scan;

	self->priv->update_entry_list_id = 0;

	clear_entry_point_list (self);

	if (! get_nth_separator_pos (self, 1, &first_position)) {
		GtkTreeIter  iter;
		GtkTreePath *path;

		gtk_tree_store_append (self->priv->model, &iter, NULL);
		gtk_tree_store_set (self->priv->model, &iter,
				    TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
				    -1);

		path = gtk_tree_model_get_path (GTK_TREE_MODEL (self->priv->model), &iter);
		if (path == NULL)
			return;
		first_position = gtk_tree_path_get_indices(path)[0];

		gtk_tree_path_free (path);
	}

	position = first_position + 1;
	entry_points = gth_main_get_all_entry_points ();
	for (scan = entry_points; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		add_file_source_entries (self,
					 file_data->file,
					 g_file_info_get_display_name (file_data->info),
					 g_file_info_get_icon (file_data->info),
					 position++,
					 FALSE,
					 ITEM_TYPE_ENTRY_POINT);
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


static gboolean
delete_current_file_entries (GthLocationChooser *self)
{
	gboolean    found = FALSE;
	GtkTreeIter iter;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->model), &iter))
		return FALSE;

	do {
		int item_type = ITEM_TYPE_NONE;

		gtk_tree_model_get (GTK_TREE_MODEL (self->priv->model),
				    &iter,
				    TYPE_COLUMN, &item_type,
				    -1);
		if (item_type == ITEM_TYPE_SEPARATOR)
			break;
	}
	while (gtk_tree_store_remove (self->priv->model, &iter));

	return found;
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
		if (same_uri (uri, list_uri)) {
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
update_location_list (gpointer user_data)
{
	GthLocationChooser *self = user_data;
	GtkTreeIter         iter;

	self->priv->update_location_list_id = 0;

	if (self->priv->location == NULL)
		return;

	if (get_iter_from_current_file_entries (self, self->priv->location, &iter)) {
		g_signal_handlers_block_by_func (self->priv->combo, combo_changed_cb, self);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->priv->combo), &iter);
		g_signal_handlers_unblock_by_func (self->priv->combo, combo_changed_cb, self);
	}
	else {
		GList *list;
		GList *scan;
		int    position = 0;

		delete_current_file_entries (self);

		list = gth_file_source_get_current_list (self->priv->file_source, self->priv->location);
		for (scan = list; scan; scan = scan->next) {
			GFile     *file = scan->data;
			GFileInfo *info;

			info = gth_file_source_get_file_info (self->priv->file_source, file, GFILE_DISPLAY_ATTRIBUTES);
			if (info == NULL)
				continue;
			add_file_source_entries (self,
						 file,
						 g_file_info_get_display_name (info),
						 g_file_info_get_icon (info),
						 position++,
						 TRUE,
						 ITEM_TYPE_LOCATION);

			g_object_unref (info);
		}

		_g_object_list_unref (list);
	}
}


static void
current_location_changed (GthLocationChooser *self)
{
	if (self->priv->update_location_list_id != 0)
		return;
	self->priv->update_location_list_id = call_when_idle ((DataFunc) update_location_list, self);
}


static void
gth_location_chooser_realize (GtkWidget *widget)
{
	GthLocationChooser *self = GTH_LOCATION_CHOOSER (widget);

	GTK_WIDGET_CLASS (gth_location_chooser_parent_class)->realize (widget);
	self->priv->icon_cache = gth_icon_cache_new (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (self))),
						     _gtk_widget_lookup_for_size (GTK_WIDGET (self), GTK_ICON_SIZE_MENU));
	entry_points_changed_cb (NULL, self);
	current_location_changed (self);
}


static void
gth_location_chooser_unrealize (GtkWidget *widget)
{
	GthLocationChooser *self = GTH_LOCATION_CHOOSER (widget);

	gth_icon_cache_free (self->priv->icon_cache);
	self->priv->icon_cache = NULL;

	GTK_WIDGET_CLASS (gth_location_chooser_parent_class)->unrealize (widget);
}


static void
gth_location_chooser_class_init (GthLocationChooserClass *klass)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	g_type_class_add_private (klass, sizeof (GthLocationChooserPrivate));

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_location_chooser_set_property;
	object_class->get_property = gth_location_chooser_get_property;
	object_class->finalize = gth_location_chooser_finalize;

	widget_class = (GtkWidgetClass *) klass;
	widget_class->grab_focus = gth_location_chooser_grab_focus;
	widget_class->realize = gth_location_chooser_realize;
	widget_class->unrealize = gth_location_chooser_unrealize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_SHOW_ENTRY_POINTS,
					 g_param_spec_boolean ("show-entry-points",
                                                               "Show entry points",
                                                               "Whether to show the entry points in the list",
                                                               TRUE,
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
get_combo_box_button (GtkWidget *widget,
		      gpointer   data)
{
	GtkWidget **p_child = data;

	if (GTK_IS_BUTTON (widget))
		*p_child = widget;
}

static void
gth_location_chooser_init (GthLocationChooser *self)
{
	GtkCellRenderer *renderer;

	gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_LOCATION_CHOOSER, GthLocationChooserPrivate);
	self->priv->icon_cache = NULL;
	self->priv->entry_points_changed_id = 0;
	self->priv->arrow = NULL;
	self->priv->show_entry_points = TRUE;
	self->priv->relief = GTK_RELIEF_NORMAL;

	self->priv->model = gtk_tree_store_new (N_COLUMNS,
						GDK_TYPE_PIXBUF,
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
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->priv->combo),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (self->priv->combo),
					 renderer,
					 "pixbuf", ICON_COLUMN,
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


static void
get_combo_box_arrow (GtkWidget *widget,
		      gpointer   data)
{
	GtkWidget **p_child = data;

	if (GTK_IS_ARROW (widget))
		*p_child = widget;
}


static gboolean
show_combo_box_arrow (GthLocationChooser *self)
{
	if (self->priv->relief == GTK_RELIEF_NONE)
		gtk_widget_show (self->priv->arrow);

	return FALSE;
}


static gboolean
hide_combo_box_arrow (GthLocationChooser *self)
{
	if (self->priv->relief == GTK_RELIEF_NONE)
		gtk_widget_hide (self->priv->arrow);

	return FALSE;
}


void
gth_location_chooser_set_relief (GthLocationChooser *self,
				 GtkReliefStyle      value)
{
	GtkWidget *button;

	if (self->priv->relief == value)
		return;

	self->priv->relief = value;

	button = NULL;
	gtk_container_forall (GTK_CONTAINER (self->priv->combo), get_combo_box_button, &button);
	if (button != NULL) {
		gtk_button_set_relief (GTK_BUTTON (button), self->priv->relief);

		/* show the arrow only when the pointer is over the combo_box */

		if (self->priv->arrow == NULL) {
			gtk_container_forall (GTK_CONTAINER (gtk_bin_get_child (GTK_BIN (button))), get_combo_box_arrow, &self->priv->arrow);
			g_signal_connect_swapped (button,
					  	  "enter-notify-event",
					  	  G_CALLBACK (show_combo_box_arrow),
					  	  self);
			g_signal_connect_swapped (button,
					  	  "leave-notify-event",
					  	  G_CALLBACK (hide_combo_box_arrow),
					  	  self);
		}

		gtk_widget_set_visible (self->priv->arrow, self->priv->relief != GTK_RELIEF_NONE);
	}

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
		if (self->priv->entry_points_changed_id != 0)
			g_source_remove (self->priv->entry_points_changed_id);
		clear_entry_point_list (self);
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


	if (self->priv->file_source != NULL)
		g_object_unref (self->priv->file_source);
	self->priv->file_source = gth_main_get_file_source (file);

	if (self->priv->file_source == NULL)
		return;

	if ((self->priv->location != NULL) && g_file_equal (file, self->priv->location))
		return;

	if (self->priv->location != NULL)
		g_object_unref (self->priv->location);
	self->priv->location = g_file_dup (file);

	if (gtk_widget_get_realized (GTK_WIDGET (self)))
		current_location_changed (self);

	g_signal_emit (G_OBJECT (self), gth_location_chooser_signals[CHANGED], 0);
}


GFile *
gth_location_chooser_get_current (GthLocationChooser *self)
{
	return self->priv->location;
}
