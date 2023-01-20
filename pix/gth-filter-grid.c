/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2014 The Free Software Foundation, Inc.
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
#include "cairo-utils.h"
#include "cairo-scale.h"
#include "glib-utils.h"
#include "gth-filter-grid.h"
#include "gth-image-task.h"


#define MAX_N_COLUMNS 10
#define DEFAULT_N_COLUMNS 2
#define PREVIEW_SIZE 110
#define COLUMN_SPACING 20
#define ROW_SPACING 20
#define FILTER_ID_KEY "filter_id"


/* Properties */
enum {
	PROP_0,
	PROP_COLUMNS
};


/* Signals */
enum {
	ACTIVATED,
	LAST_SIGNAL
};


typedef struct {
	int	 filter_id;
	GthTask	*image_task;
} PreviewTask;


typedef struct {
	GthFilterGrid   *self;
	GList           *tasks;
	GList           *current_task;
	GCancellable    *cancellable;
	cairo_surface_t *original;
	GthTask         *resize_task;
} GeneratePreviewData;


typedef struct {
	GtkWidget *cell;
	GtkWidget *button;
	GtkWidget *preview;
	GtkWidget *label;
	GthTask   *task;
} CellData;


static void
cell_data_free (CellData *cell_data)
{
	if (cell_data == NULL)
		return;

	_g_object_unref (cell_data->task);
	g_free (cell_data);
}


struct _GthFilterGridPrivate {
	GtkWidget		*grid;
	int			 n_columns;
	int			 current_column;
	int			 current_row;
	GList                   *filter_ids;
	GHashTable		*cell_data;
	GtkWidget		*active_button;
	GeneratePreviewData	*gp_data;
	int                      next_filter_id;
};


static guint gth_filter_grid_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthFilterGrid,
			 gth_filter_grid,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthFilterGrid))


static void
_gth_filter_grid_set_n_columns (GthFilterGrid *self,
				int            n_columns)
{
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);

	self->priv->grid = gtk_grid_new ();
	gtk_grid_set_column_spacing (GTK_GRID (self->priv->grid), COLUMN_SPACING);
	gtk_grid_set_row_spacing (GTK_GRID (self->priv->grid), ROW_SPACING);
	gtk_widget_show (self->priv->grid);
	gtk_box_pack_start (GTK_BOX (self), self->priv->grid, TRUE, FALSE, 0);
	gtk_widget_set_margin_top (self->priv->grid, COLUMN_SPACING);
	gtk_widget_set_margin_bottom (self->priv->grid, COLUMN_SPACING);

	self->priv->n_columns = n_columns;
	self->priv->current_column = 0;
	self->priv->current_row = 0;
}


static void
gth_filter_grid_set_property (GObject      *object,
			      guint         property_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GthFilterGrid *self;

        self = GTH_FILTER_GRID (object);

	switch (property_id) {
	case PROP_COLUMNS:
		_gth_filter_grid_set_n_columns (self, g_value_get_int (value));
		break;
	default:
		break;
	}
}


static void
gth_filter_grid_get_property (GObject    *object,
		              guint       property_id,
		              GValue     *value,
		              GParamSpec *pspec)
{
	GthFilterGrid *self;

        self = GTH_FILTER_GRID (object);

	switch (property_id) {
	case PROP_COLUMNS:
		g_value_set_int (value, self->priv->n_columns);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void generate_preview_data_cancel (GeneratePreviewData *data);


static void
gth_filter_grid_finalize (GObject *obj)
{
	GthFilterGrid *self;

	self = GTH_FILTER_GRID (obj);
	if (self->priv->gp_data != NULL) {
		generate_preview_data_cancel (self->priv->gp_data);
		self->priv->gp_data = NULL;
	}
	g_hash_table_destroy (self->priv->cell_data);
	g_list_free (self->priv->filter_ids);

	G_OBJECT_CLASS (gth_filter_grid_parent_class)->finalize (obj);
}


static void
gth_filter_grid_class_init (GthFilterGridClass *klass)
{
	GObjectClass   *object_class;

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_filter_grid_set_property;
	object_class->get_property = gth_filter_grid_get_property;
	object_class->finalize = gth_filter_grid_finalize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_COLUMNS,
					 g_param_spec_int ("columns",
                                                           "Columns",
                                                           "Number of columns",
                                                           1,
                                                           MAX_N_COLUMNS,
                                                           DEFAULT_N_COLUMNS,
                                                           G_PARAM_WRITABLE | G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	/* signals */

	gth_filter_grid_signals[ACTIVATED] =
                g_signal_new ("activated",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthFilterGridClass, activated),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);
}


static void
gth_filter_grid_init (GthFilterGrid *self)
{
	self->priv = gth_filter_grid_get_instance_private (self);
	self->priv->n_columns = DEFAULT_N_COLUMNS;
	self->priv->filter_ids = NULL;
	self->priv->cell_data = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) cell_data_free);
	self->priv->active_button = NULL;
	self->priv->gp_data = NULL;
	self->priv->next_filter_id = 0;
}


GtkWidget *
gth_filter_grid_new (void)
{
	return (GtkWidget *) g_object_new (GTH_TYPE_FILTER_GRID, NULL);
}


static void
button_toggled_cb (GtkWidget *toggle_button,
	           gpointer   user_data)
{
	GthFilterGrid *self = user_data;
	int            filter_id = GTH_FILTER_GRID_NO_FILTER;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle_button))) {
		if (self->priv->active_button != toggle_button) {
			if (self->priv->active_button != NULL) {
				_g_signal_handlers_block_by_data (self->priv->active_button, self);
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->active_button), FALSE);
				_g_signal_handlers_unblock_by_data (self->priv->active_button, self);
			}
			self->priv->active_button = toggle_button;
		}
		filter_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (toggle_button), FILTER_ID_KEY));
	}
	else if (self->priv->active_button == toggle_button) {
		self->priv->active_button = NULL;
	}

	g_signal_emit (self,
		       gth_filter_grid_signals[ACTIVATED],
		       0,
		       filter_id);
}


static void
_gth_filter_grid_append_cell (GthFilterGrid *self,
			      GtkWidget     *cell)
{
	gtk_grid_attach (GTK_GRID (self->priv->grid),
			 cell,
			 self->priv->current_column,
			 self->priv->current_row,
			 1,
			 1);

	self->priv->current_column++;
	if (self->priv->current_column >= self->priv->n_columns) {
		self->priv->current_column = 0;
		self->priv->current_row++;
	}
}


static CellData *
_gth_filter_grid_add_filter (GthFilterGrid	*self,
			   int			*filter_id_p,
			   cairo_surface_t	*preview,
			   const char		*label_text,
			   const char		*tooltip)
{
	CellData  *cell_data;
	int        filter_id;
	GtkWidget *cell;
	GtkWidget *button;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *button_content;

	cell_data = g_new0 (CellData, 1);

	if (*filter_id_p == GTH_FILTER_GRID_NEW_FILTER_ID)
		*filter_id_p = self->priv->next_filter_id++;
	filter_id = *filter_id_p;

	cell_data->cell = cell = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

	cell_data->button = button = gtk_toggle_button_new ();
	g_object_set_data_full (G_OBJECT (button), FILTER_ID_KEY, GINT_TO_POINTER (filter_id), NULL);
	gtk_style_context_add_class (gtk_widget_get_style_context (button), "filter-preview");
	gtk_widget_set_tooltip_text (button, tooltip);
	g_signal_connect (button, "toggled", G_CALLBACK (button_toggled_cb), self);

	cell_data->preview = image = gtk_image_new_from_surface (preview);
	gtk_widget_set_size_request (image, PREVIEW_SIZE, PREVIEW_SIZE);

	cell_data->label = label = gtk_label_new_with_mnemonic (label_text);
	gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);

	button_content = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

	gtk_box_pack_start (GTK_BOX (button_content), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_content), label, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (button), button_content);
	gtk_box_pack_start (GTK_BOX (cell), button, FALSE, FALSE, 0);
	gtk_widget_show_all (cell);
	_gth_filter_grid_append_cell (self, cell);

	self->priv->filter_ids = g_list_append (self->priv->filter_ids, GINT_TO_POINTER (filter_id));
	g_hash_table_insert (self->priv->cell_data, GINT_TO_POINTER (filter_id), cell_data);

	return cell_data;
}


void
gth_filter_grid_add_filter (GthFilterGrid	*self,
			    int		 	 filter_id,
			    GthTask		*task,
			    const char		*label,
			    const char		*tooltip)
{
	CellData *cell_data;

	cell_data = _gth_filter_grid_add_filter (self, &filter_id, NULL, label, tooltip);
	if (task != NULL)
		cell_data->task = task;
}


void
gth_filter_grid_add_filter_with_preview (GthFilterGrid	*self,
					 int			 filter_id,
					 cairo_surface_t	*preview,
					 const char		*label,
					 const char		*tooltip)
{
	_gth_filter_grid_add_filter (self, &filter_id, preview, label, tooltip);

}


void
gth_filter_grid_set_filter_preview (GthFilterGrid	*self,
				    int			 filter_id,
				    cairo_surface_t	*preview)
{
	CellData *cell_data;

	cell_data = g_hash_table_lookup (self->priv->cell_data, GINT_TO_POINTER (filter_id));
	g_return_if_fail (cell_data != NULL);

	gtk_image_set_from_surface (GTK_IMAGE (cell_data->preview), preview);
}


void
gth_filter_grid_activate (GthFilterGrid	*self,
			  int		 filter_id)
{
	if (filter_id == GTH_FILTER_GRID_NO_FILTER) {
		if (self->priv->active_button != NULL)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->active_button), FALSE);
		self->priv->active_button = NULL;

		g_signal_emit (self,
			       gth_filter_grid_signals[ACTIVATED],
			       0,
			       GTH_FILTER_GRID_NO_FILTER);
	}
	else {
		CellData *cell_data;

		cell_data = g_hash_table_lookup (self->priv->cell_data, GINT_TO_POINTER (filter_id));
		g_return_if_fail (cell_data != NULL);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cell_data->button), TRUE);
	}
}


/* -- gth_filter_grid_generate_previews -- */


static void
preview_task_free (PreviewTask *task)
{
	if (task == NULL)
		return;
	_g_object_unref (task->image_task);
	g_free (task);
}


static void
generate_preview_data_free (GeneratePreviewData *data)
{
	if (data->self != NULL) {
		if (data->self->priv->gp_data == data)
			data->self->priv->gp_data = NULL;
		g_object_remove_weak_pointer (G_OBJECT (data->self), (gpointer *) &data->self);
	}
	if (data->original != NULL)
		cairo_surface_destroy (data->original);
	g_list_free_full (data->tasks, (GDestroyNotify) preview_task_free);
	_g_object_unref (data->cancellable);
	_g_object_unref (data->resize_task);
	g_free (data);
}


static void
generate_preview_data_cancel (GeneratePreviewData *data)
{
	if (data->cancellable != NULL)
		g_cancellable_cancel (data->cancellable);
}


static void generate_preview (GeneratePreviewData *data);


static void
image_preview_completed_cb (GthTask    *task,
		       	    GError     *error,
		       	    gpointer    user_data)
{
	GeneratePreviewData *data = user_data;
	PreviewTask         *current_task;
	cairo_surface_t     *preview;

	current_task = (PreviewTask *) data->current_task->data;
	g_return_if_fail (task == current_task->image_task);

	_g_signal_handlers_disconnect_by_data (task, data);

	if ((error != NULL) || (data->self == NULL)) {
		generate_preview_data_free (data);
		return;
	}

	preview = gth_image_task_get_destination_surface (GTH_IMAGE_TASK (task));
	if (preview != NULL) {
		gth_filter_grid_set_filter_preview (data->self, current_task->filter_id, preview);
		cairo_surface_destroy (preview);
	}

	data->current_task = g_list_next (data->current_task);
	generate_preview (data);
}


static void
generate_preview (GeneratePreviewData *data)
{
	PreviewTask *task;

	if (data->current_task == NULL) {
		generate_preview_data_free (data);
		return;
	}

	task = (PreviewTask *) data->current_task->data;
	g_signal_connect (task->image_task,
			  "completed",
			  G_CALLBACK (image_preview_completed_cb),
			  data);
	gth_image_task_set_source_surface (GTH_IMAGE_TASK (task->image_task), data->original);

	g_cancellable_reset (data->cancellable);
	gth_task_exec (task->image_task, data->cancellable);
}


static void
resize_task_completed_cb (GthTask  *task,
			  GError   *error,
			  gpointer  user_data)
{
	GeneratePreviewData *data = user_data;

	if ((error != NULL) || (data->self == NULL)) {
		generate_preview_data_free (data);
		return;
	}

	data->original = gth_image_task_get_destination_surface (GTH_IMAGE_TASK (task));
	if (data->original == NULL) {
		generate_preview_data_free (data);
		return;
	}

	data->current_task = data->tasks;
	generate_preview (data);
}


static gpointer
resize_task_exec (GthAsyncTask *task,
		  gpointer      user_data)
{
	cairo_surface_t *source;
	cairo_surface_t *destination;

	source = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	destination = _cairo_image_surface_scale_squared (source,
						          PREVIEW_SIZE,
						          SCALE_FILTER_GOOD,
						          task);
	_cairo_image_surface_clear_metadata (destination);
	gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), destination);

	cairo_surface_destroy (destination);
	cairo_surface_destroy (source);

	return NULL;
}


static void
generate_previews (GthFilterGrid	*self,
		   cairo_surface_t	*image,
		   GList		*tasks)
{
	GeneratePreviewData *data;

	if (self->priv->gp_data != NULL)
		generate_preview_data_cancel (self->priv->gp_data);

	data = g_new (GeneratePreviewData, 1);
	data->self = self;
	data->tasks = tasks;
	data->cancellable = g_cancellable_new ();;
	data->original = NULL;

	g_object_add_weak_pointer (G_OBJECT (self), (gpointer *) &data->self);
	self->priv->gp_data = data;

	/* resize the original image */

	data->resize_task = gth_image_task_new (_("Resizing images"),
						NULL,
						resize_task_exec,
						NULL,
						data,
						NULL);
	gth_image_task_set_source_surface (GTH_IMAGE_TASK (data->resize_task), image);
	g_signal_connect (data->resize_task,
			  "completed",
			  G_CALLBACK (resize_task_completed_cb),
			  data);

	gth_task_exec (data->resize_task, data->cancellable);
}


void
gth_filter_grid_generate_previews (GthFilterGrid	*self,
				   cairo_surface_t	*image)
{
	GList *tasks;
	GList *scan;

	/* collect the (filter id, task) pairs */

	tasks = NULL;
	for (scan = self->priv->filter_ids; scan; scan = scan->next) {
		int		 filter_id = GPOINTER_TO_INT (scan->data);
		CellData	*cell_data;
		PreviewTask	*task_data;

		g_return_if_fail (filter_id >= 0);

		cell_data = g_hash_table_lookup (self->priv->cell_data, GINT_TO_POINTER (filter_id));
		g_return_if_fail (cell_data != NULL);

		if (cell_data->task == NULL)
			continue;

		task_data = g_new0 (PreviewTask, 1);
		task_data->filter_id = filter_id;
		task_data->image_task = g_object_ref (cell_data->task);
		tasks = g_list_prepend (tasks, task_data);
	}
	tasks = g_list_reverse (tasks);

	generate_previews (self, image, tasks);
}


void
gth_filter_grid_generate_preview (GthFilterGrid		*self,
				  int			 filter_id,
				  cairo_surface_t	*image)
{
	CellData	*cell_data;
	PreviewTask	*task_data;
	GList		*tasks;

	g_return_if_fail (filter_id >= 0);

	cell_data = g_hash_table_lookup (self->priv->cell_data, GINT_TO_POINTER (filter_id));
	g_return_if_fail (cell_data != NULL);

	if (cell_data->task == NULL)
		return;

	task_data = g_new0 (PreviewTask, 1);
	task_data->filter_id = filter_id;
	task_data->image_task = g_object_ref (cell_data->task);
	tasks = g_list_prepend (NULL, task_data);

	generate_previews (self, image, tasks);
}


GthTask *
gth_filter_grid_get_task (GthFilterGrid	*self,
			  int		 filter_id)
{
	CellData *cell_data;

	cell_data = g_hash_table_lookup (self->priv->cell_data, GINT_TO_POINTER (filter_id));
	g_return_val_if_fail (cell_data != NULL, NULL);

	return _g_object_ref (cell_data->task);
}


void
gth_filter_grid_rename_filter (GthFilterGrid	*self,
			       int		 filter_id,
			       const char	*new_name)
{
	CellData *cell_data;

	cell_data = g_hash_table_lookup (self->priv->cell_data, GINT_TO_POINTER (filter_id));
	g_return_if_fail (cell_data != NULL);

	gtk_label_set_text (GTK_LABEL (cell_data->label), new_name);
}


static void
_gth_filter_grid_reflow (GthFilterGrid *self)
{
	GList *children;
	GList *scan;

	self->priv->current_row = 0;
	self->priv->current_column = 0;

	children = NULL;
	for (scan = self->priv->filter_ids; scan; scan = scan->next)
		children = g_list_prepend (children, g_hash_table_lookup (self->priv->cell_data, scan->data));
	children = g_list_reverse (children);

	for (scan = children; scan; scan = scan->next) {
		CellData *cell_data = scan->data;

		g_object_ref (G_OBJECT (cell_data->cell));
		gtk_container_remove (GTK_CONTAINER (self->priv->grid), cell_data->cell);
	}

	for (scan = children; scan; scan = scan->next) {
		CellData *cell_data = scan->data;

		_gth_filter_grid_append_cell (self, GTK_WIDGET (cell_data->cell));
		g_object_unref (G_OBJECT (cell_data->cell));
	}

	g_list_free (children);
}


void
gth_filter_grid_remove_filter (GthFilterGrid	*self,
			       int		 filter_id)
{
	CellData *cell_data;
	GList    *scan, *link;

	cell_data = g_hash_table_lookup (self->priv->cell_data, GINT_TO_POINTER (filter_id));
	g_return_if_fail (cell_data != NULL);

	/* update filter_ids */

	link = NULL;
	for (scan = self->priv->filter_ids; scan; scan = scan->next) {
		if (GPOINTER_TO_INT (scan->data) == filter_id) {
			link = scan;
			break;
		}
	}
	if (link != NULL) {
		self->priv->filter_ids = g_list_remove_link (self->priv->filter_ids, link);
		g_list_free (link);
	}

	/* update active_button */

	if (cell_data->button == self->priv->active_button)
		self->priv->active_button = NULL;

	/* remove the cell */

	gtk_container_remove (GTK_CONTAINER (self->priv->grid), cell_data->cell);
	cell_data->cell = NULL;
	_gth_filter_grid_reflow (self);
	g_hash_table_remove (self->priv->cell_data, GINT_TO_POINTER (filter_id));
}


void
gth_filter_grid_change_order (GthFilterGrid *self,
			      GList	    *id_list)
{
	g_list_free (self->priv->filter_ids);
	self->priv->filter_ids = g_list_copy (id_list);

	_gth_filter_grid_reflow (self);
}
