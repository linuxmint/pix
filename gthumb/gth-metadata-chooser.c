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
#include <glib/gi18n.h>
#include "glib-utils.h"
#include "gth-main.h"
#include "gth-metadata-chooser.h"


#define ORDER_CHANGED_DELAY 250
#define CATEGORY_SIZE 1000


enum {
	WEIGHT_COLUMN,
	NAME_COLUMN,
	ID_COLUMN,
	SORT_ORDER_COLUMN,
	USED_COLUMN,
	SEPARATOR_COLUMN,
	IS_METADATA_COLUMN,
	N_COLUMNS
};


/* Properties */
enum {
	PROP_0,
	PROP_REORDERABLE
};

/* Signals */
enum {
	CHANGED,
	LAST_SIGNAL
};


struct _GthMetadataChooserPrivate {
	GthMetadataFlags allowed_flags;
	gboolean         reorderable;
	gulong           row_inserted_event;
	gulong           row_deleted_event;
	guint            changed_id;
};


static guint gth_metadata_chooser_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthMetadataChooser,
			 gth_metadata_chooser,
			 GTK_TYPE_TREE_VIEW,
			 G_ADD_PRIVATE (GthMetadataChooser))



static void
ggth_metadata_chooser_set_property (GObject      *object,
				    guint         property_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	GthMetadataChooser *self;

	self = GTH_METADATA_CHOOSER (object);

	switch (property_id) {
	case PROP_REORDERABLE:
		self->priv->reorderable = g_value_get_boolean (value);
		gtk_tree_view_set_reorderable (GTK_TREE_VIEW (self), self->priv->reorderable);
		break;
	default:
		break;
	}
}


static void
gth_metadata_chooser_get_property (GObject    *object,
				   guint       property_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
	GthMetadataChooser *self;

	self = GTH_METADATA_CHOOSER (object);

	switch (property_id) {
	case PROP_REORDERABLE:
		g_value_set_boolean (value, self->priv->reorderable);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_metadata_chooser_class_init (GthMetadataChooserClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->set_property = ggth_metadata_chooser_set_property;
	object_class->get_property = gth_metadata_chooser_get_property;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_REORDERABLE,
					 g_param_spec_boolean ("reorderable",
							       "Reorderable",
							       "Whether the user can reorder the list",
							       FALSE,
							       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/* signals */

	gth_metadata_chooser_signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthMetadataChooserClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}


/* -- gth_metadata_chooser_reorder_list -- */


typedef struct {
	int       pos;
	int       sort_order;
	char     *name;
	char     *id;
	gboolean  used;
	gboolean  separator;
} ItemData;


static void
item_data_free (ItemData *item_data)
{
	g_free (item_data->name);
	g_free (item_data->id);
	g_free (item_data);
}


static int
item_data_compare_func (gconstpointer a,
			gconstpointer b,
			gpointer      user_data)
{
	GthMetadataChooser *self = user_data;
	ItemData           *item_a = (ItemData *) a;
	ItemData           *item_b = (ItemData *) b;

	if (! self->priv->reorderable) {
		if (item_a->sort_order < item_b->sort_order)
			return -1;
		else if (item_a->sort_order > item_b->sort_order)
			return 1;
		else
			return g_strcmp0 (item_a->id, item_b->id);
	}

	/* self->priv->reorderable == TRUE */

	if (item_a->separator) {
		if (item_b->used)
			return 1;
		else
			return -1;
	}

	if (item_b->separator) {
		if (item_a->used)
			return -1;
		else
			return 1;
	}

	if (item_a->used == item_b->used) {
		if (item_a->used) {
			/* keep the user defined order for the used items */
			if (item_a->pos < item_b->pos)
				return -1;
			else if (item_a->pos > item_b->pos)
				return 1;
			else
				return 0;
		}
		else {
			if (item_a->sort_order < item_b->sort_order)
				return -1;
			else if (item_a->sort_order > item_b->sort_order)
				return 1;
			else
				return 0;
		}
	}
	else if (item_a->used)
		return -1;
	else
		return 1;
}


static gboolean
gth_metadata_chooser_reorder_list (GthMetadataChooser *self)
{
	gboolean      changed = FALSE;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GList        *list;
	int           pos;
	int          *new_order;
	GList        *scan;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (! gtk_tree_model_get_iter_first (model, &iter))
		return FALSE;

	list = NULL;
	pos = 0;
	do {
		ItemData *item_data;

		item_data = g_new0 (ItemData, 1);
		item_data->pos = pos;
		gtk_tree_model_get (model, &iter,
				    NAME_COLUMN, &item_data->name,
				    ID_COLUMN, &item_data->id,
				    SORT_ORDER_COLUMN, &item_data->sort_order,
				    USED_COLUMN, &item_data->used,
				    SEPARATOR_COLUMN, &item_data->separator,
				    -1);
		list = g_list_prepend (list, item_data);
		pos++;
	}
	while (gtk_tree_model_iter_next (model, &iter));

	list = g_list_sort_with_data (list, item_data_compare_func, self);
	new_order = g_new (int, g_list_length (list));
	for (pos = 0, scan = list; scan; pos++, scan = scan->next) {
		ItemData *item_data = scan->data;

		if (pos != item_data->pos)
			changed = TRUE;
		new_order[pos] = item_data->pos;
	}
	gtk_list_store_reorder (GTK_LIST_STORE (model), new_order);

	g_free (new_order);
	g_list_foreach (list, (GFunc) item_data_free, NULL);
	g_list_free (list);

	return changed;
}


static void
cell_renderer_toggle_toggled_cb (GtkCellRendererToggle *cell_renderer,
				 char                  *path,
                                 gpointer               user_data)
{
	GthMetadataChooser *self = user_data;
	GtkTreePath        *tree_path;
	GtkTreeModel       *tree_model;
	GtkTreeIter         iter;

	tree_path = gtk_tree_path_new_from_string (path);
	if (tree_path == NULL)
		return;

	tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (gtk_tree_model_get_iter (tree_model, &iter, tree_path)) {
		gboolean used;

		gtk_tree_model_get (tree_model, &iter,
				    USED_COLUMN, &used,
				    -1);
		gtk_list_store_set (GTK_LIST_STORE (tree_model), &iter,
				    USED_COLUMN, ! used,
				    -1);
		gth_metadata_chooser_reorder_list (self);
		g_signal_emit (self, gth_metadata_chooser_signals[CHANGED], 0);
	}

	gtk_tree_path_free (tree_path);
}


static gboolean
row_separator_func (GtkTreeModel *model,
                    GtkTreeIter  *iter,
                    gpointer      data)
{
	gboolean separator;

	gtk_tree_model_get (model, iter, SEPARATOR_COLUMN, &separator, -1);

	return separator;
}



static gboolean
order_changed (gpointer user_data)
{
	GthMetadataChooser *self = user_data;

	if (self->priv->changed_id != 0)
		g_source_remove (self->priv->changed_id);
	self->priv->changed_id = 0;

	gth_metadata_chooser_reorder_list (self);
	g_signal_emit (self, gth_metadata_chooser_signals[CHANGED], 0);

	return FALSE;
}


static void
row_deleted_cb (GtkTreeModel *tree_model,
		GtkTreePath  *path,
		gpointer      user_data)
{
	GthMetadataChooser *self = user_data;

	if (self->priv->changed_id != 0)
		g_source_remove (self->priv->changed_id);
	self->priv->changed_id = gdk_threads_add_timeout (ORDER_CHANGED_DELAY, order_changed, self);
}


static void
row_inserted_cb (GtkTreeModel *tree_model,
		 GtkTreePath  *path,
		 GtkTreeIter  *iter,
		 gpointer      user_data)
{
	GthMetadataChooser *self = user_data;

	if (self->priv->changed_id != 0)
		g_source_remove (self->priv->changed_id);
	self->priv->changed_id = gdk_threads_add_timeout (ORDER_CHANGED_DELAY, order_changed, self);
}


static void
gth_metadata_chooser_init (GthMetadataChooser *self)
{
	GtkListStore      *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer   *renderer;

	self->priv = gth_metadata_chooser_get_instance_private (self);

	/* the list view */

	store = gtk_list_store_new (N_COLUMNS,
				    PANGO_TYPE_WEIGHT,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_INT,
				    G_TYPE_BOOLEAN,
				    G_TYPE_BOOLEAN,
				    G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (GTK_TREE_VIEW (self), GTK_TREE_MODEL (store));
	g_object_unref (store);
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW (self), self->priv->reorderable);
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self), FALSE);
        gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (self),
       					      row_separator_func,
       					      self,
       					      NULL);

        self->priv->row_inserted_event = g_signal_connect (store,
							   "row-inserted",
							   G_CALLBACK (row_inserted_cb),
							   self);
        self->priv->row_deleted_event = g_signal_connect (store,
							  "row-deleted",
							  G_CALLBACK (row_deleted_cb),
							  self);

	/* the checkbox column */

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer,
			  "toggled",
			  G_CALLBACK (cell_renderer_toggle_toggled_cb),
			  self);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "active", USED_COLUMN,
					     "visible", IS_METADATA_COLUMN,
					     NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);

	/* the name column. */

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", NAME_COLUMN,
                                             "weight", WEIGHT_COLUMN,
                                             NULL);
        gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self), column);
}


GtkWidget *
gth_metadata_chooser_new (GthMetadataFlags allowed_flags,
			  gboolean         reorderable)
{
	GthMetadataChooser *self;

	self = g_object_new (GTH_TYPE_METADATA_CHOOSER, "reorderable", reorderable, NULL);
	self->priv->allowed_flags = allowed_flags;
	gth_metadata_chooser_set_selection (self, "");

	return (GtkWidget *) self;
}


void
gth_metadata_chooser_set_selection (GthMetadataChooser *self,
				    char               *ids)
{
	GtkListStore  *store;
	char         **attributes_v;
	char         **ids_v;
	int            i;
	GtkTreeIter    iter;
	GHashTable    *category_root;

	store = (GtkListStore *) gtk_tree_view_get_model (GTK_TREE_VIEW (self));

	g_signal_handler_block (store, self->priv->row_inserted_event);
	g_signal_handler_block (store, self->priv->row_deleted_event);

	gtk_list_store_clear (store);

	attributes_v = gth_main_get_metadata_attributes ("*");
	ids_v = g_strsplit (ids, ",", -1);

	if (self->priv->reorderable) {
		for (i = 0; ids_v[i] != NULL; i++) {
			int                  idx;
			GthMetadataInfo     *info;
			const char          *name;
			GthMetadataCategory *category;

			idx = _g_strv_find (attributes_v, ids_v[i]);
			if (idx < 0)
				continue;

			info = gth_main_get_metadata_info (attributes_v[idx]);
			if ((info == NULL) || ((info->flags & self->priv->allowed_flags) == 0))
				continue;

			if (info->display_name != NULL)
				name = _(info->display_name);
			else
				name = info->id;

			category = gth_main_get_metadata_category (info->category);

			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
					WEIGHT_COLUMN, PANGO_WEIGHT_NORMAL,
					NAME_COLUMN, name,
					ID_COLUMN, info->id,
					SORT_ORDER_COLUMN, (category->sort_order * CATEGORY_SIZE) + info->sort_order,
					USED_COLUMN, TRUE,
					SEPARATOR_COLUMN, FALSE,
					IS_METADATA_COLUMN, TRUE,
					-1);
		}

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				SEPARATOR_COLUMN, TRUE,
				-1);
	}

	category_root = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, NULL);
	for (i = 0; attributes_v[i] != NULL; i++) {
		gboolean             used;
		GtkTreeIter          iter;
		GthMetadataInfo     *info;
		const char          *name;
		GthMetadataCategory *category;

		used = _g_strv_find (ids_v, attributes_v[i]) >= 0;
		if (self->priv->reorderable && used)
			continue;

		info = gth_main_get_metadata_info (attributes_v[i]);
		if ((info == NULL) || ((info->flags & self->priv->allowed_flags) == 0))
			continue;

		name = info->display_name;
		if (name == NULL)
			name = info->id;

		category = gth_main_get_metadata_category (info->category);

		if (g_hash_table_lookup (category_root, category->id) == NULL) {
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
					    WEIGHT_COLUMN, PANGO_WEIGHT_BOLD,
					    NAME_COLUMN, _(category->display_name),
					    ID_COLUMN, category->id,
					    SORT_ORDER_COLUMN, category->sort_order * CATEGORY_SIZE,
					    USED_COLUMN, FALSE,
					    SEPARATOR_COLUMN, FALSE,
					    IS_METADATA_COLUMN, FALSE,
					    -1);

			g_hash_table_insert (category_root, g_strdup (info->category), GINT_TO_POINTER (1));
		}

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    WEIGHT_COLUMN, PANGO_WEIGHT_NORMAL,
				    NAME_COLUMN, _(name),
				    ID_COLUMN, info->id,
				    SORT_ORDER_COLUMN, (category->sort_order * CATEGORY_SIZE) + info->sort_order,
				    USED_COLUMN, used,
				    SEPARATOR_COLUMN, FALSE,
				    IS_METADATA_COLUMN, TRUE,
				    -1);
	}
	gth_metadata_chooser_reorder_list (self);

	g_signal_handler_unblock (store, self->priv->row_inserted_event);
	g_signal_handler_unblock (store, self->priv->row_deleted_event);

	g_hash_table_destroy (category_root);
	g_strfreev (attributes_v);
	g_strfreev (ids_v);
}


char *
gth_metadata_chooser_get_selection (GthMetadataChooser *self)
{
	GString      *selection;
	GtkTreeModel *model;
	GtkTreeIter   iter;

	selection = g_string_new ("");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			gboolean  used;
			char     *id;

			gtk_tree_model_get (model, &iter,
					    ID_COLUMN, &id,
					    USED_COLUMN, &used,
					    -1);

			if (used) {
				if (selection->len > 0)
					g_string_append (selection, ",");
				g_string_append (selection, id);
			}

			g_free (id);
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	if (selection->len == 0)
		g_string_append (selection, "none");

	return g_string_free (selection, FALSE);
}
