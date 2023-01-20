/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gth-empty-list.h"
#include "gth-file-list.h"
#include "gth-file-selection.h"
#include "gth-file-store.h"
#include "gth-icon-cache.h"
#include "gth-preferences.h"
#include "gth-thumb-loader.h"
#include "gtk-utils.h"


#define RESTART_LOADING_THUMBS_DELAY 1500
#define N_VIEWAHEAD 50
#define N_CREATEAHEAD 50000
#define MAX_THUMBNAIL_LOADERS 4
#define NO_FILE_MSG (N_("No file"))
#define _FILE_VIEW "file-view"
#define _EMPTY_VIEW "empty-view"
#undef DEBUG_THUMBNAILER


#ifdef DEBUG_THUMBNAILER
static const char *StateName[] = {
	"GTH_THUMBNAIL_STATE_DEFAULT",
	"GTH_THUMBNAIL_STATE_CREATING",
	"GTH_THUMBNAIL_STATE_CREATED",
	"GTH_THUMBNAIL_STATE_LOADING",
	"GTH_THUMBNAIL_STATE_ERROR",
	"GTH_THUMBNAIL_STATE_LOADED",
};
#endif


typedef enum {
	GTH_FILE_LIST_OP_TYPE_SET_FILES,
	GTH_FILE_LIST_OP_TYPE_CLEAR_FILES,
	GTH_FILE_LIST_OP_TYPE_ADD_FILES,
	GTH_FILE_LIST_OP_TYPE_UPDATE_FILES,
	GTH_FILE_LIST_OP_TYPE_UPDATE_EMBLEMS,
	GTH_FILE_LIST_OP_TYPE_DELETE_FILES,
	GTH_FILE_LIST_OP_TYPE_SET_FILTER,
	GTH_FILE_LIST_OP_TYPE_SET_SORT_FUNC,
	GTH_FILE_LIST_OP_TYPE_ENABLE_THUMBS,
	GTH_FILE_LIST_OP_TYPE_RENAME_FILE,
	GTH_FILE_LIST_OP_TYPE_MAKE_FILE_VISIBLE,
	GTH_FILE_LIST_OP_TYPE_RESTORE_STATE
} GthFileListOpType;


typedef struct {
	GthFileListOpType    type;
	GtkTreeModel        *model;
	GthTest             *filter;
	GList               *file_list; /* GthFileData */
	GList               *files; /* GFile */
	GthFileDataCompFunc  cmp_func;
	gboolean             inverse_sort;
	char                *sval;
	int                  ival;
	GFile               *file;
	GthFileData         *file_data;
	int                  position;
	GList               *selected;
	double               vscroll;
} GthFileListOp;


typedef struct {
	GthFileList    *file_list;
	GthThumbLoader *loader;
	GCancellable   *cancellable;
	GthFileData    *file_data;
	gboolean        update_in_view;
	gboolean        started;
	guint           idle_id;
} ThumbnailJob;


typedef enum {
	THUMBNAILER_PHASE_INITIALIZE,
	THUMBNAILER_PHASE_UPDATE_VISIBLE,
	THUMBNAILER_PHASE_UPDATE_DOWNWARD,
	THUMBNAILER_PHASE_UPDATE_UPWARD,
	THUMBNAILER_PHASE_COMPLETED
} ThumbnailerPhase;


typedef struct {
	ThumbnailerPhase phase;
	int              first_visible;
	int              last_visible;
	GtkTreeIter      current;
	int              remaining;
	int              completed;
} ThumbnailerState;


struct _GthFileListPrivate {
	GSettings        *settings;
	GthFileListMode   type;
	GtkAdjustment    *vadj;
	GtkWidget        *notebook;
	GtkWidget        *view;
	GtkWidget        *message;
	GtkWidget        *scrolled_window;
	GthIconCache     *icon_cache;
	GthFileSource    *file_source;
	gboolean          load_thumbs;
	int               thumb_size;
	gboolean          ignore_hidden_thumbs;
	GthThumbLoader   *thumb_loader;
	gboolean          loading_thumbs;
	guint             restart_thumb_update;
	GList            *queue; /* list of GthFileListOp */
	GList            *jobs; /* list of ThumbnailJob */
	gboolean          cancelling;
	ThumbnailerState  thumbnailer_state;
	int               max_loaders;
	int               started_loaders;
};


G_DEFINE_TYPE_WITH_CODE (GthFileList,
			 gth_file_list,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthFileList))


/* OPs */


static void _gth_file_list_exec_next_op (GthFileList *file_list);


static GthFileListOp *
gth_file_list_op_new (GthFileListOpType op_type)
{
	GthFileListOp *op;

	op = g_new0 (GthFileListOp, 1);
	op->type = op_type;

	return op;
}


static void
gth_file_list_op_free (GthFileListOp *op)
{
	switch (op->type) {
	case GTH_FILE_LIST_OP_TYPE_SET_FILES:
		_g_object_list_unref (op->file_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_CLEAR_FILES:
		g_free (op->sval);
		break;
	case GTH_FILE_LIST_OP_TYPE_ADD_FILES:
	case GTH_FILE_LIST_OP_TYPE_UPDATE_FILES:
	case GTH_FILE_LIST_OP_TYPE_UPDATE_EMBLEMS:
		_g_object_list_unref (op->file_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_DELETE_FILES:
		_g_object_list_unref (op->files);
		break;
	case GTH_FILE_LIST_OP_TYPE_SET_FILTER:
		g_object_unref (op->filter);
		break;
	case GTH_FILE_LIST_OP_TYPE_RENAME_FILE:
		g_object_unref (op->file);
		g_object_unref (op->file_data);
		break;
	case GTH_FILE_LIST_OP_TYPE_MAKE_FILE_VISIBLE:
		g_object_unref (op->file);
		break;
	case GTH_FILE_LIST_OP_TYPE_RESTORE_STATE:
		g_list_free_full (op->selected, (GDestroyNotify) gtk_tree_path_free);
		break;
	default:
		break;
	}
	g_free (op);
}


static void
_gth_file_list_clear_queue (GthFileList *file_list)
{
	if (file_list->priv->restart_thumb_update != 0) {
		g_source_remove (file_list->priv->restart_thumb_update);
		file_list->priv->restart_thumb_update = 0;
	}

	g_list_foreach (file_list->priv->queue, (GFunc) gth_file_list_op_free, NULL);
	g_list_free (file_list->priv->queue);
	file_list->priv->queue = NULL;
}


static void
_gth_file_list_remove_op (GthFileList       *file_list,
			  GthFileListOpType  op_type)
{
	GList *scan;

	scan = file_list->priv->queue;
	while (scan != NULL) {
		GthFileListOp *op = scan->data;

		if (op->type != op_type) {
			scan = scan->next;
			continue;
		}

		file_list->priv->queue = g_list_remove_link (file_list->priv->queue, scan);
		gth_file_list_op_free (op);
		g_list_free (scan);

		scan = file_list->priv->queue;
	}
}


static void
_gth_file_list_queue_op (GthFileList   *file_list,
			 GthFileListOp *op)
{
	if ((op->type == GTH_FILE_LIST_OP_TYPE_SET_FILES) || (op->type == GTH_FILE_LIST_OP_TYPE_CLEAR_FILES))
		_gth_file_list_clear_queue (file_list);
	if (op->type == GTH_FILE_LIST_OP_TYPE_SET_FILTER)
		_gth_file_list_remove_op (file_list, GTH_FILE_LIST_OP_TYPE_SET_FILTER);
	file_list->priv->queue = g_list_append (file_list->priv->queue, op);

	if (! file_list->priv->loading_thumbs)
		_gth_file_list_exec_next_op (file_list);
}


/* -- gth_file_list -- */


static void
gth_file_list_finalize (GObject *object)
{
	GthFileList *file_list;

	file_list = GTH_FILE_LIST (object);

	_gth_file_list_clear_queue (file_list);
	_g_object_unref (file_list->priv->thumb_loader);
	if (file_list->priv->icon_cache != NULL)
		gth_icon_cache_free (file_list->priv->icon_cache);
	g_object_unref (file_list->priv->settings);

	G_OBJECT_CLASS (gth_file_list_parent_class)->finalize (object);
}


static GtkSizeRequestMode
gth_file_list_get_request_mode (GtkWidget *widget)
{
	return GTK_SIZE_REQUEST_CONSTANT_SIZE;
}


static void
gth_file_list_get_preferred_width (GtkWidget *widget,
                		   int       *minimum_width,
                		   int       *natural_width)
{
	GthFileList *file_list = GTH_FILE_LIST (widget);
	GtkWidget   *vscrollbar;
	const int    border = 1;

	gtk_widget_get_preferred_width (file_list->priv->view, minimum_width, natural_width);
	if (minimum_width)
		*minimum_width += border * 2;
	if (natural_width)
		*natural_width += border * 2;

	vscrollbar = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (file_list->priv->scrolled_window));
	if (gtk_widget_get_visible (vscrollbar) || (file_list->priv->type == GTH_FILE_LIST_MODE_V_SIDEBAR)) {
		int vscrollbar_minimum_width;
		int vscrollbar_natural_width;
		int scrollbar_spacing;

		gtk_widget_get_preferred_width (vscrollbar, &vscrollbar_minimum_width, &vscrollbar_natural_width);
		gtk_widget_style_get (file_list->priv->scrolled_window,
				      "scrollbar-spacing", &scrollbar_spacing,
				      NULL);

		*minimum_width += vscrollbar_minimum_width + scrollbar_spacing;
		*natural_width += vscrollbar_natural_width + scrollbar_spacing;
	}
}


static void
gth_file_list_get_preferred_height (GtkWidget *widget,
                		    int       *minimum_height,
                		    int       *natural_height)
{
	GthFileList *file_list = GTH_FILE_LIST (widget);
	const int    border = 1;

	gtk_widget_get_preferred_height (file_list->priv->view, minimum_height, natural_height);
	if (minimum_height)
		*minimum_height += border * 2;
	if (natural_height)
		*natural_height += border * 2;
}


static void
gth_file_list_class_init (GthFileListClass *class)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_file_list_finalize;

	widget_class = (GtkWidgetClass*) class;
	widget_class->get_request_mode = gth_file_list_get_request_mode;
	widget_class->get_preferred_width = gth_file_list_get_preferred_width;
	widget_class->get_preferred_height = gth_file_list_get_preferred_height;
}


/* --- */


static int
_get_max_loaders (void)
{
	int n = (int) g_get_num_processors () - 2;
	return CLAMP (n, 1, MAX_THUMBNAIL_LOADERS);
}


static void
gth_file_list_init (GthFileList *file_list)
{
	gtk_widget_set_can_focus (GTK_WIDGET (file_list), FALSE);

	file_list->priv = gth_file_list_get_instance_private (file_list);
	file_list->priv->settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	file_list->priv->type = GTH_FILE_LIST_MODE_NORMAL;
	file_list->priv->vadj = NULL;
	file_list->priv->notebook = NULL;
	file_list->priv->view = NULL;
	file_list->priv->message = NULL;
	file_list->priv->scrolled_window = NULL;
	file_list->priv->icon_cache = NULL;
	file_list->priv->file_source = NULL;
	file_list->priv->load_thumbs = TRUE;
	file_list->priv->thumb_size = g_settings_get_int (file_list->priv->settings, PREF_BROWSER_THUMBNAIL_SIZE);
	file_list->priv->ignore_hidden_thumbs = FALSE;
	file_list->priv->thumb_loader = NULL;
	file_list->priv->loading_thumbs = FALSE;
	file_list->priv->restart_thumb_update = 0;
	file_list->priv->queue = NULL;
	file_list->priv->jobs = NULL;
	file_list->priv->cancelling = FALSE;
	file_list->priv->thumbnailer_state.phase = THUMBNAILER_PHASE_INITIALIZE;
	file_list->priv->started_loaders = 0;
	file_list->priv->max_loaders = _get_max_loaders ();
}


static void _gth_file_list_update_next_thumb (GthFileList *file_list);


static gboolean
restart_thumb_update_cb (gpointer data)
{
	GthFileList *file_list = data;

	if (file_list->priv->restart_thumb_update != 0) {
		g_source_remove (file_list->priv->restart_thumb_update);
		file_list->priv->restart_thumb_update = 0;
	}

	if (file_list->priv->cancelling)
		return FALSE;

	_gth_file_list_update_next_thumb (file_list);

	return FALSE;
}


static void
start_update_next_thumb (GthFileList *file_list)
{
	if (file_list->priv->cancelling)
		return;

	if (file_list->priv->loading_thumbs)
		return;

	if (! file_list->priv->load_thumbs)
		return;

	if (file_list->priv->restart_thumb_update != 0)
		g_source_remove (file_list->priv->restart_thumb_update);
	file_list->priv->restart_thumb_update = g_idle_add (restart_thumb_update_cb, file_list);
}


static void
_gth_file_list_thumbs_completed (GthFileList *file_list)
{
	file_list->priv->loading_thumbs = FALSE;
}


/* ThumbnailJob */


static void
thumbnail_job_free (ThumbnailJob *job)
{
	job->file_list->priv->jobs = g_list_remove (job->file_list->priv->jobs, job);
	job->file_list->priv->started_loaders--;

	_g_object_unref (job->file_data);
	_g_object_unref (job->cancellable);
	_g_object_unref (job->loader);
	_g_object_unref (job->file_list);
	g_free (job);
}


static void
thumbnail_job_cancel (ThumbnailJob *job)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;

	file_store = gth_file_list_get_model (job->file_list);

	if (gth_file_store_find (file_store, job->file_data->file, &iter))
		gth_file_store_set (file_store,
				    &iter,
				    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, GTH_THUMBNAIL_STATE_DEFAULT,
				    -1);

	if (job->started)
		g_cancellable_cancel (job->cancellable);
	else {
		if (job->idle_id != 0) {
			g_source_remove (job->idle_id);
			job->idle_id = 0;
		}
		thumbnail_job_free (job);
	}
}


/* --- */


static gboolean
vadj_changed_cb (GtkAdjustment *adjustment,
		 gpointer       user_data)
{
	GthFileList *file_list = user_data;

	file_list->priv->thumbnailer_state.phase = THUMBNAILER_PHASE_INITIALIZE;
	start_update_next_thumb (GTH_FILE_LIST (user_data));

	return FALSE;
}


static void
file_view_drag_data_get_cb (GtkWidget        *widget,
			    GdkDragContext   *drag_context,
			    GtkSelectionData *data,
			    guint             info,
			    guint             time,
			    gpointer          user_data)
{
	GthFileList  *file_list = user_data;
	GList        *items;
	GList        *files;
	GList        *scan;
	int           n_uris;
	char        **uris;
	int           i;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (file_list->priv->view));
	files = gth_file_list_get_files (file_list, items);
	n_uris = g_list_length (files);
	uris = g_new (char *, n_uris + 1);
	for (scan = files, i = 0; scan; scan = scan->next, i++) {
		GthFileData *file_data = scan->data;
		uris[i] = g_file_get_uri (file_data->file);
	}
	uris[i] = NULL;

	gtk_selection_data_set_uris (data, uris);

	g_strfreev (uris);
	_g_object_list_unref (files);
	_gtk_tree_path_list_free (items);
}


static void
_gth_file_list_update_orientation (GthFileList *file_list)
{
	GtkPolicyType hscrollbar_policy;
	GtkPolicyType vscrollbar_policy;

	hscrollbar_policy = GTK_POLICY_AUTOMATIC;
	vscrollbar_policy = GTK_POLICY_AUTOMATIC;

	if (file_list->priv->type == GTH_FILE_LIST_MODE_V_SIDEBAR) {
		gtk_orientable_set_orientation (GTK_ORIENTABLE (file_list), GTK_ORIENTATION_VERTICAL);
		vscrollbar_policy = GTK_POLICY_ALWAYS;
	}
	else if (file_list->priv->type == GTH_FILE_LIST_MODE_H_SIDEBAR)
		gtk_orientable_set_orientation (GTK_ORIENTABLE (file_list), GTK_ORIENTATION_HORIZONTAL);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (file_list->priv->scrolled_window),
					hscrollbar_policy,
					vscrollbar_policy);
}


static void
gth_file_list_construct (GthFileList     *file_list,
			 GtkWidget       *file_view,
			 GthFileListMode  list_type,
			 gboolean         enable_drag_drop)
{
	GthFileStore *model;

	file_list->priv->thumb_loader = gth_thumb_loader_new (file_list->priv->thumb_size);
	file_list->priv->icon_cache = gth_icon_cache_new (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (file_list))), file_list->priv->thumb_size / 2);

	/* the main notebook */

	file_list->priv->notebook = gtk_stack_new ();
	gtk_widget_set_can_focus (file_list->priv->notebook, FALSE);

	/* the message pane */

	file_list->priv->message = gth_empty_list_new (_("Loading…"));

	/* the file view */

	file_list->priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_can_focus (file_list->priv->scrolled_window, FALSE);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (file_list->priv->scrolled_window), GTK_SHADOW_ETCHED_IN);

	file_list->priv->vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (file_list->priv->scrolled_window));
	g_signal_connect (G_OBJECT (file_list->priv->vadj),
			  "changed",
			  G_CALLBACK (vadj_changed_cb),
			  file_list);
	g_signal_connect (G_OBJECT (file_list->priv->vadj),
			  "value-changed",
			  G_CALLBACK (vadj_changed_cb),
			  file_list);

	file_list->priv->view = file_view;
	g_signal_connect (G_OBJECT (file_list->priv->view),
			  "drag-data-get",
			  G_CALLBACK (file_view_drag_data_get_cb),
			  file_list);

	model = gth_file_store_new ();
	gth_file_list_set_model (file_list, model);
	g_object_unref (model);

	if (enable_drag_drop)
		gth_file_list_enable_drag_source (file_list, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);

	gth_file_list_set_mode (file_list, list_type);

	/* pack the widgets together */

	gtk_widget_show (file_list->priv->view);
	gtk_container_add (GTK_CONTAINER (file_list->priv->scrolled_window), file_list->priv->view);

	gtk_widget_show (file_list->priv->scrolled_window);
	gtk_stack_add_named (GTK_STACK (file_list->priv->notebook), file_list->priv->scrolled_window, _FILE_VIEW);

	gtk_widget_show (file_list->priv->message);
	gtk_stack_add_named (GTK_STACK (file_list->priv->notebook), file_list->priv->message, _EMPTY_VIEW);

	gtk_widget_show (file_list->priv->notebook);
	gtk_box_pack_start (GTK_BOX (file_list), file_list->priv->notebook, TRUE, TRUE, 0);

	gtk_stack_set_visible_child_name (GTK_STACK (file_list->priv->notebook), _EMPTY_VIEW);
}


GtkWidget *
gth_file_list_new (GtkWidget       *file_view,
		   GthFileListMode  list_type,
		   gboolean         enable_drag_drop)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (g_object_new (GTH_TYPE_FILE_LIST, NULL));
	gth_file_list_construct (GTH_FILE_LIST (widget), file_view, list_type, enable_drag_drop);

	return widget;
}


/* -- gth_file_list_set_model -- */


static void
_gth_file_list_update_pane (GthFileList *file_list)
{
	GthFileStore *file_store;

	file_store = gth_file_list_get_model (file_list);

	if (gth_file_store_n_visibles (file_store) > 0) {
		gtk_stack_set_visible_child_name (GTK_STACK (file_list->priv->notebook), _FILE_VIEW);
	}
	else {
		gth_empty_list_set_text (GTH_EMPTY_LIST (file_list->priv->message), _(NO_FILE_MSG));
		gtk_stack_set_visible_child_name (GTK_STACK (file_list->priv->notebook), _EMPTY_VIEW);
	}
}


static void
_after_visibility_change (GthFileList  *file_list)
{
	file_list->priv->thumbnailer_state.phase = THUMBNAILER_PHASE_INITIALIZE;
}


static void
file_store_visibility_changed_cb (GthFileStore *file_store,
				  GthFileList  *file_list)
{
	_after_visibility_change (file_list);
	_gth_file_list_update_pane (file_list);
}


static void
file_store_rows_reordered_cb (GtkTreeModel *tree_model,
			      GtkTreePath  *path,
			      GtkTreeIter  *iter,
			      gpointer      new_order,
			      GthFileList  *file_list)
{
	_after_visibility_change (file_list);
}


void
gth_file_list_set_model (GthFileList  *file_list,
			 GthFileStore *model)
{
	g_return_if_fail (model != NULL);

	if (file_list->priv->view != NULL) {
		GtkTreeModel *model = gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
		if (model != NULL)
			g_signal_handlers_disconnect_by_data (model, file_list);
	}

	gth_file_view_set_model (GTH_FILE_VIEW (file_list->priv->view),
				 GTK_TREE_MODEL (model));
	g_signal_connect (model,
			  "visibility-changed",
			  G_CALLBACK (file_store_visibility_changed_cb),
			  file_list);
	g_signal_connect (model,
			  "rows-reordered",
			  G_CALLBACK (file_store_rows_reordered_cb),
			  file_list);
}


GthFileStore *
gth_file_list_get_model (GthFileList *file_list)
{
	return GTH_FILE_STORE (gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view)));
}


void
gth_file_list_set_mode (GthFileList     *file_list,
			GthFileListMode  list_type)
{
	g_return_if_fail (GTH_IS_FILE_LIST (file_list));

	file_list->priv->type = list_type;

	if ((file_list->priv->type == GTH_FILE_LIST_MODE_SELECTOR) || (file_list->priv->type == GTH_FILE_LIST_MODE_NO_SELECTION))
		gth_file_selection_set_selection_mode (GTH_FILE_SELECTION (file_list->priv->view), GTK_SELECTION_NONE);
	else if ((file_list->priv->type == GTH_FILE_LIST_MODE_H_SIDEBAR) || (file_list->priv->type == GTH_FILE_LIST_MODE_V_SIDEBAR))
		gth_file_selection_set_selection_mode (GTH_FILE_SELECTION (file_list->priv->view), GTK_SELECTION_SINGLE);
	else
		gth_file_selection_set_selection_mode (GTH_FILE_SELECTION (file_list->priv->view), GTK_SELECTION_MULTIPLE);

	_gth_file_list_update_orientation (file_list);
}


GthFileListMode
gth_file_list_get_mode (GthFileList *file_list)
{
	return file_list->priv->type;
}


typedef struct {
	GthFileList *file_list;
	DataFunc     done_func;
	gpointer     user_data;
} CancelData;


static void
cancel_data_free (CancelData *cancel_data)
{
	g_object_unref (cancel_data->file_list);
	g_free (cancel_data);
}


static gboolean
wait_for_jobs_to_finish (gpointer user_data)
{
	CancelData *cancel_data = user_data;

	if (cancel_data->file_list->priv->jobs == NULL) {
		cancel_data->file_list->priv->cancelling = FALSE;
		if (cancel_data->done_func != NULL)
			cancel_data->done_func (cancel_data->user_data);

		cancel_data_free (cancel_data);

		return G_SOURCE_REMOVE;
	}

	return G_SOURCE_CONTINUE;
}


void
gth_file_list_cancel (GthFileList    *file_list,
		      DataFunc        done_func,
		      gpointer        user_data)
{
	CancelData *cancel_data;
	GList      *list;
	GList      *scan;

	file_list->priv->cancelling = TRUE;
	_gth_file_list_clear_queue (file_list);

	cancel_data = g_new0 (CancelData, 1);
	cancel_data->file_list = g_object_ref (file_list);
	cancel_data->done_func = done_func;
	cancel_data->user_data = user_data;

	list = g_list_copy (file_list->priv->jobs);
	for (scan = list; scan; scan = scan->next) {
		ThumbnailJob *job = scan->data;
		thumbnail_job_cancel (job);
	}
	g_list_free (list);

	g_idle_add (wait_for_jobs_to_finish, cancel_data);
}


GthThumbLoader *
gth_file_list_get_thumb_loader (GthFileList *file_list)
{
	return file_list->priv->thumb_loader;
}


static void
gfl_clear_list (GthFileList *file_list,
		const char  *message)
{
	GthFileStore *file_store;

	gth_file_selection_unselect_all (GTH_FILE_SELECTION (file_list->priv->view));

	file_store = gth_file_list_get_model (file_list);
	gth_file_store_clear (file_store);

	gth_empty_list_set_text (GTH_EMPTY_LIST (file_list->priv->message), message);
	gtk_stack_set_visible_child_name (GTK_STACK (file_list->priv->notebook), _EMPTY_VIEW);
}


void
gth_file_list_clear (GthFileList *file_list,
		     const char  *message)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_CLEAR_FILES);
	op->sval = g_strdup (message != NULL ? message : _(NO_FILE_MSG));
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_add_files (GthFileList *file_list,
	       GList       *files,
	       int          position)
{
	GthFileStore *file_store;
	GList        *scan;
	char         *cache_base_uri;

	performance (DEBUG_INFO, "gfl_add_files start");

	file_store = gth_file_list_get_model (file_list);

	cache_base_uri = g_strconcat (_g_uri_get_home (), "/.thumbnails", NULL);
	for (scan = files; scan; scan = scan->next) {
		GthFileData     *file_data = scan->data;
		char            *uri;
		gboolean         is_thumbnail;
		GIcon           *icon;
		cairo_surface_t *image = NULL;

		if (g_file_info_get_file_type (file_data->info) != G_FILE_TYPE_REGULAR)
			continue;

		uri = g_file_get_uri (file_data->file);
		/* files in the .thumbnails directory are already thumbnails,
		 * set them as created. */
		is_thumbnail = _g_uri_is_parent (cache_base_uri, uri);
		icon = g_file_info_get_symbolic_icon (file_data->info);
		image = gth_icon_cache_get_surface (file_list->priv->icon_cache, icon);
		gth_file_store_queue_add (file_store,
					  file_data,
					  image,
					  TRUE,
					  is_thumbnail ? GTH_THUMBNAIL_STATE_CREATED : GTH_THUMBNAIL_STATE_DEFAULT);

		cairo_surface_destroy (image);
		g_free (uri);
	}
	g_free (cache_base_uri);

	gth_file_store_exec_add (file_store, position);

	performance (DEBUG_INFO, "gfl_add_files end");
}


void
gth_file_list_add_files (GthFileList *file_list,
			 GList       *files,
			 int          position)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_ADD_FILES);
	op->file_list = _g_object_list_ref (files);
	op->position = position;
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_delete_files (GthFileList *file_list,
		  GList       *files)
{
	GthFileStore *file_store;
	GList        *scan;

	file_store = gth_file_list_get_model (file_list);

	for (scan = files; scan; scan = scan->next) {
		GFile       *file = scan->data;
		GtkTreeIter  iter;

		if (gth_file_store_find (file_store, file, &iter))
			gth_file_store_queue_remove (file_store, &iter);
	}
	gth_file_store_exec_remove (file_store);
}


void
gth_file_list_delete_files (GthFileList *file_list,
			    GList       *files)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_DELETE_FILES);
	op->files = _g_object_list_ref (files);
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_update_files (GthFileList *file_list,
		  GList       *files)
{
	GthFileStore *file_store;
	GList        *scan;

	file_store = gth_file_list_get_model (file_list);
	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GtkTreeIter  iter;

		if (gth_file_store_find (file_store, file_data->file, &iter))
			gth_file_store_queue_set (file_store,
						  &iter,
						  GTH_FILE_STORE_FILE_DATA_COLUMN, file_data,
						  GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, GTH_THUMBNAIL_STATE_DEFAULT,
						  -1);
	}
	gth_file_store_exec_set (file_store);
}


void
gth_file_list_update_files (GthFileList *file_list,
			    GList       *files)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_UPDATE_FILES);
	op->file_list = _g_object_list_ref (files);
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_update_emblems (GthFileList *file_list,
		    GList       *files)
{
	GthFileStore *file_store;
	GList        *scan;

	file_store = gth_file_list_get_model (file_list);
	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GtkTreeIter  iter;

		if (gth_file_store_find (file_store, file_data->file, &iter))
			gth_file_store_queue_set (file_store,
						  &iter,
						  GTH_FILE_STORE_EMBLEMS_COLUMN, g_file_info_get_attribute_object (file_data->info, GTH_FILE_ATTRIBUTE_EMBLEMS),
						  -1);
	}
	gth_file_store_exec_set (file_store);
}


void
gth_file_list_update_emblems (GthFileList *file_list,
			      GList       *files /* GthFileData */)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_UPDATE_EMBLEMS);
	op->file_list = _g_object_list_ref (files);
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_rename_file (GthFileList *file_list,
		 GFile       *file,
		 GthFileData *file_data)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;

	file_store = gth_file_list_get_model (file_list);
	if (gth_file_store_find (file_store, file, &iter))
		gth_file_store_set (file_store,
				    &iter,
				    GTH_FILE_STORE_FILE_DATA_COLUMN, file_data,
				    -1);
}


void
gth_file_list_rename_file (GthFileList *file_list,
			   GFile       *file,
			   GthFileData *file_data)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_RENAME_FILE);
	op->file = g_object_ref (file);
	op->file_data = g_object_ref (file_data);
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_set_files (GthFileList *file_list,
	       GList       *files)
{
	gth_thumb_loader_set_save_thumbnails (file_list->priv->thumb_loader, g_settings_get_boolean (file_list->priv->settings, PREF_BROWSER_SAVE_THUMBNAILS));
	gth_thumb_loader_set_max_file_size (file_list->priv->thumb_loader, g_settings_get_int (file_list->priv->settings, PREF_BROWSER_THUMBNAIL_LIMIT));
	gth_file_selection_unselect_all (GTH_FILE_SELECTION (file_list->priv->view));

	gth_file_view_set_vscroll (GTH_FILE_VIEW (file_list->priv->view), 0);
	gth_file_store_clear ((GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view)));
	gfl_add_files (file_list, files, -1);
}


void
gth_file_list_set_files (GthFileList *file_list,
			 GList       *files)
{
	GthFileListOp *op;

	if (files != NULL) {
		op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_SET_FILES);
		op->file_list = _g_object_list_ref (files);
		_gth_file_list_queue_op (file_list, op);
	}
	else {
		op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_CLEAR_FILES);
		op->sval = g_strdup (_(NO_FILE_MSG));
		_gth_file_list_queue_op (file_list, op);
	}
}


GList *
gth_file_list_get_files (GthFileList *file_list,
			 GList       *items)
{
	GList        *list = NULL;
	GthFileStore *file_store;
	GList        *scan;

	file_store = gth_file_list_get_model (file_list);
	for (scan = items; scan; scan = scan->next) {
		GtkTreePath *tree_path = scan->data;
		GtkTreeIter  iter;
		GthFileData *file_data;

		if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (file_store), &iter, tree_path))
			continue;
		file_data = gth_file_store_get_file (file_store, &iter);
		if (file_data != NULL)
			list = g_list_prepend (list, g_object_ref (file_data));
	}

	return g_list_reverse (list);
}


static void
gfl_set_filter (GthFileList *file_list,
		GthTest     *filter)
{
	GthFileStore *file_store;

	file_store = gth_file_list_get_model (file_list);
	if (file_store != NULL)
		gth_file_store_set_filter (file_store, filter);
}


void
gth_file_list_set_filter (GthFileList *file_list,
			  GthTest     *filter)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_SET_FILTER);
	if (filter != NULL)
		op->filter = g_object_ref (filter);
	else
		op->filter = gth_test_new ();
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_set_sort_func (GthFileList         *file_list,
		   GthFileDataCompFunc  cmp_func,
		   gboolean             inverse_sort)
{
	GthFileStore *file_store;

	file_store = gth_file_list_get_model (file_list);
	if (file_store != NULL)
		gth_file_store_set_sort_func (file_store, cmp_func, inverse_sort);
}


void
gth_file_list_set_sort_func (GthFileList         *file_list,
			     GthFileDataCompFunc  cmp_func,
			     gboolean             inverse_sort)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_SET_SORT_FUNC);
	op->cmp_func = cmp_func;
	op->inverse_sort = inverse_sort;
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_enable_thumbs (GthFileList *file_list,
		   gboolean     enable)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;

	gth_thumb_loader_set_save_thumbnails (file_list->priv->thumb_loader,
					      g_settings_get_boolean (file_list->priv->settings, PREF_BROWSER_SAVE_THUMBNAILS));
	gth_thumb_loader_set_max_file_size (file_list->priv->thumb_loader,
					    g_settings_get_int (file_list->priv->settings, PREF_BROWSER_THUMBNAIL_LIMIT));

	file_list->priv->load_thumbs = enable;

	file_store = gth_file_list_get_model (file_list);
	if (gth_file_store_get_first (file_store, &iter)) {
		do {
			GthFileData     *file_data;
			GIcon           *icon;
			cairo_surface_t *image;

			file_data = gth_file_store_get_file (file_store, &iter);

			icon = g_file_info_get_symbolic_icon (file_data->info);
			image = gth_icon_cache_get_surface (file_list->priv->icon_cache, icon);
			gth_file_store_queue_set (file_store,
						  &iter,
						  GTH_FILE_STORE_THUMBNAIL_COLUMN, image,
						  GTH_FILE_STORE_IS_ICON_COLUMN, TRUE,
						  GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, GTH_THUMBNAIL_STATE_DEFAULT,
						  -1);

			cairo_surface_destroy (image);
		}
		while (gth_file_store_get_next (file_store, &iter));

		gth_file_store_exec_set (file_store);
	}

	if (enable)
		file_list->priv->thumbnailer_state.phase = THUMBNAILER_PHASE_INITIALIZE;
	start_update_next_thumb (file_list);
}


void
gth_file_list_enable_thumbs (GthFileList *file_list,
			     gboolean     enable)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_ENABLE_THUMBS);
	op->ival = enable;
	_gth_file_list_queue_op (file_list, op);
}


void
gth_file_list_set_ignore_hidden (GthFileList *file_list,
				 gboolean      value)
{
	file_list->priv->ignore_hidden_thumbs = value;
}


void
gth_file_list_set_thumb_size (GthFileList *file_list,
			      int          size)
{
	file_list->priv->thumb_size = size;

	gth_thumb_loader_set_requested_size (file_list->priv->thumb_loader, size);
	gth_thumb_loader_set_save_thumbnails (file_list->priv->thumb_loader,  g_settings_get_boolean (file_list->priv->settings, PREF_BROWSER_SAVE_THUMBNAILS));
	gth_thumb_loader_set_max_file_size (file_list->priv->thumb_loader, g_settings_get_int (file_list->priv->settings, PREF_BROWSER_THUMBNAIL_LIMIT));

	gth_icon_cache_free (file_list->priv->icon_cache);
	file_list->priv->icon_cache = gth_icon_cache_new (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (file_list))), size / 2);
	gth_icon_cache_set_fallback (file_list->priv->icon_cache, g_themed_icon_new ("text-x-generic-symbolic"));

	gth_file_view_set_thumbnail_size (GTH_FILE_VIEW (file_list->priv->view), file_list->priv->thumb_size);

	_gth_file_list_update_orientation (file_list);
}


void
gth_file_list_set_caption (GthFileList *file_list,
			   const char  *attributes)
{
	gth_file_view_set_caption (GTH_FILE_VIEW (file_list->priv->view), attributes);
}


static void
gfl_make_file_visible (GthFileList *file_list,
		       GFile       *file)
{
	int pos;

	pos = gth_file_store_get_pos ((GthFileStore *) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view)), file);
	if (pos >= 0) {
		gth_file_selection_unselect_all (GTH_FILE_SELECTION (file_list->priv->view));
		gth_file_selection_select (GTH_FILE_SELECTION (file_list->priv->view), pos);
		gth_file_view_set_cursor (GTH_FILE_VIEW (file_list->priv->view), pos);
	}
}


void
gth_file_list_make_file_visible (GthFileList *file_list,
				 GFile       *file)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_MAKE_FILE_VISIBLE);
	op->file = g_object_ref (file);
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_restore_state (GthFileList *file_list,
		   GList       *selected,
		   double       vscroll)
{
	GList *scan;

	for (scan = selected; scan; scan = scan->next) {
		GtkTreePath *path = scan->data;
		int          pos;

		pos = gtk_tree_path_get_indices (path)[0];
		gth_file_selection_select (GTH_FILE_SELECTION (file_list->priv->view), pos);
	}

	gth_file_view_set_vscroll (GTH_FILE_VIEW (file_list->priv->view), vscroll);
}


void
gth_file_list_restore_state (GthFileList *file_list,
			     GList       *selected,
			     double       vscroll)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_RESTORE_STATE);
	op->selected = g_list_copy_deep (selected, (GCopyFunc) gtk_tree_path_copy, NULL);
	op->vscroll = vscroll;
	_gth_file_list_queue_op (file_list, op);
}


GtkWidget *
gth_file_list_get_view (GthFileList *file_list)
{
	return file_list->priv->view;
}


GtkWidget *
gth_file_list_get_empty_view (GthFileList *file_list)
{
	return file_list->priv->message;
}


GtkAdjustment *
gth_file_list_get_vadjustment (GthFileList *file_list)
{
	return file_list->priv->vadj;
}


/* thumbs */


static void
thumbnail_job_ready_cb (GObject      *source_object,
			GAsyncResult *result,
			gpointer      user_data)
{
	ThumbnailJob    *job = user_data;
	GthFileList     *file_list = job->file_list;
	gboolean         success;
	cairo_surface_t *image = NULL;
	GError          *error = NULL;
	GthFileStore    *file_store;
	GtkTreeIter      iter;

	success = gth_thumb_loader_load_finish (GTH_THUMB_LOADER (source_object),
						result,
						&image,
						&error);
	job->started = FALSE;
	file_store = gth_file_list_get_model (file_list);

	if ((! success && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
	    || file_list->priv->cancelling)
	{
		if (! file_list->priv->cancelling && gth_file_store_find (file_store, job->file_data->file, &iter))
			gth_file_store_set (file_store,
					    &iter,
					    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, GTH_THUMBNAIL_STATE_DEFAULT,
					    -1);

#ifdef DEBUG_THUMBNAILER
		g_print ("<CANCELLED: %s\n", g_file_get_uri (job->file_data->file));
		g_print (" success: %d\n", success);
		g_print (" file_list->priv->cancelling: %d\n", file_list->priv->cancelling);
#endif
		cairo_surface_destroy (image);
		thumbnail_job_free (job);
		return;
	}

	if (! gth_file_store_find (file_store, job->file_data->file, &iter)) {
#ifdef DEBUG_THUMBNAILER
		g_print ("<INVALID: %s\n", g_file_get_uri (job->file_data->file));
#endif
	}
	else if (! job->update_in_view && success) {
#ifdef DEBUG_THUMBNAILER
		g_print ("<CREATED: %s\n", g_file_get_uri (job->file_data->file));
#endif
		gth_file_store_set (file_store,
				    &iter,
				    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, GTH_THUMBNAIL_STATE_CREATED,
				    -1);
	}
	else if (success) {
#ifdef DEBUG_THUMBNAILER
		g_print ("<LOADED: %s\n", g_file_get_uri (job->file_data->file));
#endif
		gth_file_store_set (file_store,
				    &iter,
				    GTH_FILE_STORE_THUMBNAIL_COLUMN, image,
				    GTH_FILE_STORE_IS_ICON_COLUMN, FALSE,
				    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, GTH_THUMBNAIL_STATE_LOADED,
				    -1);
	}
	else {
#ifdef DEBUG_THUMBNAILER
		g_print ("<ERROR: %s\n", g_file_get_uri (job->file_data->file));
#endif

		GIcon           *icon;
		cairo_surface_t *icon_image;

		icon = g_file_info_get_symbolic_icon (job->file_data->info);
		icon_image = gth_icon_cache_get_surface (file_list->priv->icon_cache, icon);
		gth_file_store_set (file_store,
				    &iter,
				    GTH_FILE_STORE_THUMBNAIL_COLUMN, icon_image,
				    GTH_FILE_STORE_IS_ICON_COLUMN, TRUE,
				    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, GTH_THUMBNAIL_STATE_ERROR,
				    -1);

		cairo_surface_destroy (icon_image);
	}

	cairo_surface_destroy (image);
	thumbnail_job_free (job);

	_gth_file_list_update_next_thumb (file_list);
}


static gboolean
_gth_file_list_update_thumb (gpointer user_data)
{
	ThumbnailJob *job = user_data;
	GthFileList  *file_list = job->file_list;
	GthFileStore *file_store;
	GtkTreeIter   iter;

	job->idle_id = 0;

	if (file_list->priv->cancelling) {
#ifdef DEBUG_THUMBNAILER
		g_print ("CANCELLING\n");
#endif
		thumbnail_job_free (job);
		return G_SOURCE_REMOVE;
	}

	file_store = gth_file_list_get_model (file_list);

	if (! gth_file_store_find (file_store, job->file_data->file, &iter)) {
#ifdef DEBUG_THUMBNAILER
		g_print ("=INVALID: %s\n", g_file_get_uri (job->file_data->file));
#endif

		thumbnail_job_free (job);
		_gth_file_list_update_next_thumb (file_list);
		return G_SOURCE_REMOVE;
	}

	if (! job->update_in_view) {
		if (gth_thumb_loader_has_valid_thumbnail (job->loader, job->file_data)) {
#ifdef DEBUG_THUMBNAILER
			g_print ("=CREATED: %s\n", g_file_get_uri (job->file_data->file));
#endif

			gth_file_store_set (file_store,
					    &iter,
					    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, GTH_THUMBNAIL_STATE_CREATED,
					    -1);
			thumbnail_job_free (job);
			job = NULL;
		}
		else if (gth_thumb_loader_has_failed_thumbnail (job->loader, job->file_data)) {
#ifdef DEBUG_THUMBNAILER
			g_print ("=FAILED: %s\n", g_file_get_uri (job->file_data->file));
#endif

			gth_file_store_set (file_store,
					    &iter,
					    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, GTH_THUMBNAIL_STATE_ERROR,
					    -1);
			thumbnail_job_free (job);
			job = NULL;
		}

		if (job == NULL) {
			_gth_file_list_update_next_thumb (file_list);
			return G_SOURCE_REMOVE;
		}
	}

	job->started = TRUE;
	gth_thumb_loader_load (job->loader,
			       job->file_data,
			       job->cancellable,
			       thumbnail_job_ready_cb,
			       job);

	return G_SOURCE_REMOVE;
}


static gboolean
can_create_file_thumbnail (GthFileData *file_data,
			   GTimeVal    *current_time,
			   gboolean    *young_file_found)
{

	time_t   time_diff;
	gboolean young_file;

	/* Check for files that are exactly 0 or 1 seconds old; they may still be changing. */
	time_diff = current_time->tv_sec - gth_file_data_get_mtime (file_data);
	young_file = (time_diff <= 1) && (time_diff >= 0);

	if (young_file)
		*young_file_found = TRUE;

	return ! young_file;
}


static gboolean
_thumbnail_loaded (GthThumbnailState state)
{
	return (state == GTH_THUMBNAIL_STATE_LOADING)
		|| (state == GTH_THUMBNAIL_STATE_LOADED)
		|| (state == GTH_THUMBNAIL_STATE_ERROR);
}


static gboolean
_thumbnail_created (GthThumbnailState state)
{
	return state != GTH_THUMBNAIL_STATE_DEFAULT;
}


static gboolean
_gth_file_list_thumbnailer_iterate (GthFileList *file_list,
				    GTimeVal    *current_time,
				    gboolean    *young_file_found)
{
	GthFileStore *file_store;
	GthFileData  *file_data;

	file_store = gth_file_list_get_model (file_list);

	switch (file_list->priv->thumbnailer_state.phase) {
	case THUMBNAILER_PHASE_INITIALIZE:
		file_list->priv->thumbnailer_state.first_visible = gth_file_view_get_first_visible (GTH_FILE_VIEW (file_list->priv->view));
		file_list->priv->thumbnailer_state.last_visible = gth_file_view_get_last_visible (GTH_FILE_VIEW (file_list->priv->view));

		if ((file_list->priv->thumbnailer_state.first_visible < 0)
		    || ! gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (file_store),
							&file_list->priv->thumbnailer_state.current,
							NULL,
							file_list->priv->thumbnailer_state.first_visible))
		{
			file_list->priv->thumbnailer_state.phase = THUMBNAILER_PHASE_COMPLETED;
			return FALSE;
		}

		/* pass to the 'update visible files' phase. */
		file_list->priv->thumbnailer_state.phase = THUMBNAILER_PHASE_UPDATE_VISIBLE;
		file_list->priv->thumbnailer_state.remaining = file_list->priv->thumbnailer_state.last_visible - file_list->priv->thumbnailer_state.first_visible + 1;
		file_list->priv->thumbnailer_state.completed = 0;
		break;

	case THUMBNAILER_PHASE_UPDATE_VISIBLE:
		/* Find a non-loaded thumbnail among the visible files. */

		while (file_list->priv->thumbnailer_state.remaining > 0) {
			GthThumbnailState state;

			gtk_tree_model_get (GTK_TREE_MODEL (file_store),
					    &file_list->priv->thumbnailer_state.current,
					    GTH_FILE_STORE_FILE_DATA_COLUMN, &file_data,
					    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, &state,
					    -1);

#ifdef DEBUG_THUMBNAILER
			g_print ("  UPDATE_VISIBLE: %s\n", g_file_get_uri (file_data->file));
			g_print ("    STATE: %s\n", StateName[state]);
#endif

			if (! _thumbnail_loaded (state)
			    && can_create_file_thumbnail (file_data, current_time, young_file_found))
			{
				/* found a thumbnail to load */
				g_object_unref (file_data);
				return FALSE;
			}

			g_object_unref (file_data);

			if (! gtk_tree_model_iter_next (GTK_TREE_MODEL (file_store),
							&file_list->priv->thumbnailer_state.current))
			{
				break;
			}
			file_list->priv->thumbnailer_state.remaining--;
			file_list->priv->thumbnailer_state.completed++;
		}

		/* No thumbnail to load among the visible images, pass to the
		 * next phase.  Start from the one after the last visible image. */
		if (! gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (file_store),
						     &file_list->priv->thumbnailer_state.current,
						     NULL,
						     file_list->priv->thumbnailer_state.last_visible + 1))
		{
			file_list->priv->thumbnailer_state.phase = THUMBNAILER_PHASE_COMPLETED;
			return FALSE;
		}
		file_list->priv->thumbnailer_state.phase = THUMBNAILER_PHASE_UPDATE_DOWNWARD;
		file_list->priv->thumbnailer_state.remaining = N_CREATEAHEAD;
		file_list->priv->thumbnailer_state.completed = 0;
		break;

	case THUMBNAILER_PHASE_UPDATE_DOWNWARD:
		while (file_list->priv->thumbnailer_state.remaining > 0) {
			GthThumbnailState state;
			gboolean          requested_action_performed;

			gtk_tree_model_get (GTK_TREE_MODEL (file_store),
					    &file_list->priv->thumbnailer_state.current,
					    GTH_FILE_STORE_FILE_DATA_COLUMN, &file_data,
					    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, &state,
					    -1);

#ifdef DEBUG_THUMBNAILER
			g_print ("  DOWNWARD: %s\n", g_file_get_uri (file_data->file));
			g_print ("    STATE: %s\n", StateName[state]);
#endif

			if (file_list->priv->thumbnailer_state.completed < N_VIEWAHEAD)
				/* requested action: load the thumbnail */
				requested_action_performed = _thumbnail_loaded (state);
			else
				/* requested action: create the thumbnail */
				requested_action_performed = _thumbnail_created (state);

			if (! requested_action_performed
			    && can_create_file_thumbnail (file_data, current_time, young_file_found))
			{
				/* found a thumbnail to load */
				g_object_unref (file_data);
				return FALSE;
			}

			g_object_unref (file_data);

			if (! gtk_tree_model_iter_next (GTK_TREE_MODEL (file_store),
							&file_list->priv->thumbnailer_state.current))
			{
				break;
			}
			file_list->priv->thumbnailer_state.remaining--;
			file_list->priv->thumbnailer_state.completed++;
		}

		/* No thumbnail to load, pass to the next phase. Start from the
		 * one before the first visible upward to the first one. */
		if ((file_list->priv->thumbnailer_state.first_visible == 0)
		    || ! gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (file_store),
							&file_list->priv->thumbnailer_state.current,
							NULL,
							file_list->priv->thumbnailer_state.first_visible - 1))
		{
			file_list->priv->thumbnailer_state.phase = THUMBNAILER_PHASE_COMPLETED;
			return FALSE;
		}
		file_list->priv->thumbnailer_state.phase = THUMBNAILER_PHASE_UPDATE_UPWARD;
		file_list->priv->thumbnailer_state.remaining = N_CREATEAHEAD;
		file_list->priv->thumbnailer_state.completed = 0;
		break;

	case THUMBNAILER_PHASE_UPDATE_UPWARD:
		while (file_list->priv->thumbnailer_state.remaining > 0) {
			GthThumbnailState state;
			gboolean          requested_action_performed;

			gtk_tree_model_get (GTK_TREE_MODEL (file_store),
					    &file_list->priv->thumbnailer_state.current,
					    GTH_FILE_STORE_FILE_DATA_COLUMN, &file_data,
					    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, &state,
					    -1);

#ifdef DEBUG_THUMBNAILER
			g_print ("  UPWARD: %s\n", g_file_get_uri (file_data->file));
			g_print ("    STATE: %s\n", StateName[state]);
#endif

			if (file_list->priv->thumbnailer_state.completed < N_VIEWAHEAD)
				/* requested action: load the thumbnail */
				requested_action_performed = _thumbnail_loaded (state);
			else
				/* requested action: create the thumbnail */
				requested_action_performed = _thumbnail_created (state);

			if (! requested_action_performed
			    && can_create_file_thumbnail (file_data, current_time, young_file_found))
			{
				/* found a thumbnail to load */
				g_object_unref (file_data);
				return FALSE;
			}

			g_object_unref (file_data);

			if (! gtk_tree_model_iter_previous (GTK_TREE_MODEL (file_store),
							    &file_list->priv->thumbnailer_state.current))
			{
				break;
			}
			file_list->priv->thumbnailer_state.remaining--;
			file_list->priv->thumbnailer_state.completed++;
		}

		/* No thumbnail to load, terminate the process. */
		file_list->priv->thumbnailer_state.phase = THUMBNAILER_PHASE_COMPLETED;
		break;

	case THUMBNAILER_PHASE_COMPLETED:
		return FALSE;
	}

	return TRUE;
}


static void
_gth_file_list_update_next_thumb (GthFileList *file_list)
{
	GTimeVal      current_time;
	gboolean      young_file_found;
	ThumbnailJob *job;
	GthFileStore *file_store;

	/* give priority to any other operation, the thumbnailer will restart
	 * again soon after the operation terminates. */
	if (file_list->priv->queue != NULL) {
		file_list->priv->loading_thumbs = FALSE;
		_gth_file_list_exec_next_op (file_list);
		return;
	}

	if (file_list->priv->cancelling)
		return;

	if (! file_list->priv->load_thumbs) {
		_gth_file_list_thumbs_completed (file_list);
		return;
	}

	if (file_list->priv->started_loaders >= file_list->priv->max_loaders)
		return;

	file_list->priv->loading_thumbs = TRUE;

	/* find the new thumbnail to load */

	g_get_current_time (&current_time);
	young_file_found = FALSE;
	while (_gth_file_list_thumbnailer_iterate (file_list, &current_time, &young_file_found))
		/* void */;

	if (file_list->priv->thumbnailer_state.phase == THUMBNAILER_PHASE_COMPLETED) {
		_gth_file_list_thumbs_completed (file_list);
		if (young_file_found) {
			if (file_list->priv->restart_thumb_update != 0)
				g_source_remove (file_list->priv->restart_thumb_update);
			file_list->priv->restart_thumb_update = g_timeout_add (RESTART_LOADING_THUMBS_DELAY,
									       restart_thumb_update_cb,
									       file_list);
		}
		return;
	}

	job = g_new0 (ThumbnailJob, 1);
	job->file_list = g_object_ref (file_list);
	job->loader = gth_thumb_loader_copy (file_list->priv->thumb_loader);
	job->cancellable = g_cancellable_new ();
	job->update_in_view = file_list->priv->thumbnailer_state.completed < N_VIEWAHEAD;
	gtk_tree_model_get (GTK_TREE_MODEL (gth_file_list_get_model (file_list)),
			    &file_list->priv->thumbnailer_state.current,
			    GTH_FILE_STORE_FILE_DATA_COLUMN, &job->file_data,
			    -1);

#ifdef DEBUG_THUMBNAILER
	g_print ("JOB: %s\n", g_file_get_uri (job->file_data->file));
#endif

	file_store = gth_file_list_get_model (file_list);

	if (job->update_in_view) {
		GIcon           *icon;
		cairo_surface_t *icon_image;

#ifdef DEBUG_THUMBNAILER
		g_print (">LOADING: %s\n", g_file_get_uri (job->file_data->file));
#endif

		icon = g_themed_icon_new ("content-loading-symbolic");
		icon_image = gth_icon_cache_get_surface (file_list->priv->icon_cache, icon);
		gth_file_store_set (file_store,
				    &file_list->priv->thumbnailer_state.current,
				    GTH_FILE_STORE_THUMBNAIL_COLUMN, icon_image,
				    GTH_FILE_STORE_IS_ICON_COLUMN, TRUE,
				    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, GTH_THUMBNAIL_STATE_LOADING,
				    -1);

		cairo_surface_destroy (icon_image);
		g_object_unref (icon);
	}
	else {
#ifdef DEBUG_THUMBNAILER
		g_print (">CREATING: %s\n", g_file_get_uri (job->file_data->file));
#endif

		gth_file_store_set (file_store,
				    &file_list->priv->thumbnailer_state.current,
				    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, GTH_THUMBNAIL_STATE_CREATING,
				    -1);
	}

	file_list->priv->jobs = g_list_prepend (file_list->priv->jobs, job);
	file_list->priv->started_loaders++;
	job->idle_id = g_idle_add (_gth_file_list_update_thumb, job);

#ifdef DEBUG_THUMBNAILER
	g_print ("STARTED LOADERS: %d\n", file_list->priv->started_loaders);
	g_print ("JOBS: %d\n", g_list_length (file_list->priv->jobs));
#endif

	_gth_file_list_update_next_thumb (file_list);
}


static void
_gth_file_list_exec_next_op (GthFileList *file_list)
{
	GList         *first;
	GthFileListOp *op;
	gboolean       exec_next_op = TRUE;

	if (file_list->priv->queue == NULL) {
		start_update_next_thumb (file_list);
		return;
	}

	first = file_list->priv->queue;
	file_list->priv->queue = g_list_remove_link (file_list->priv->queue, first);

	op = first->data;

	switch (op->type) {
	case GTH_FILE_LIST_OP_TYPE_SET_FILES:
		gfl_set_files (file_list, op->file_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_ADD_FILES:
		gfl_add_files (file_list, op->file_list, op->position);
		break;
	case GTH_FILE_LIST_OP_TYPE_DELETE_FILES:
		gfl_delete_files (file_list, op->files);
		break;
	case GTH_FILE_LIST_OP_TYPE_UPDATE_FILES:
		gfl_update_files (file_list, op->file_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_UPDATE_EMBLEMS:
		gfl_update_emblems (file_list, op->file_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_ENABLE_THUMBS:
		gfl_enable_thumbs (file_list, op->ival);
		exec_next_op = FALSE;
		break;
	case GTH_FILE_LIST_OP_TYPE_CLEAR_FILES:
		gfl_clear_list (file_list, op->sval);
		break;
	case GTH_FILE_LIST_OP_TYPE_SET_FILTER:
		gfl_set_filter (file_list, op->filter);
		break;
	case GTH_FILE_LIST_OP_TYPE_SET_SORT_FUNC:
		gfl_set_sort_func (file_list, op->cmp_func, op->inverse_sort);
		break;
	case GTH_FILE_LIST_OP_TYPE_RENAME_FILE:
		gfl_rename_file (file_list, op->file, op->file_data);
		break;
	case GTH_FILE_LIST_OP_TYPE_MAKE_FILE_VISIBLE:
		gfl_make_file_visible (file_list, op->file);
		break;
	case GTH_FILE_LIST_OP_TYPE_RESTORE_STATE:
		gfl_restore_state (file_list, op->selected, op->vscroll);
		break;
	default:
		exec_next_op = FALSE;
		break;
	}

	gth_file_list_op_free (op);
	g_list_free (first);

	if (exec_next_op)
		_gth_file_list_exec_next_op (file_list);
}


int
gth_file_list_first_file (GthFileList *file_list,
			  gboolean     skip_broken,
			  gboolean     only_selected)
{
	GthFileStore *file_store;
	int           pos;
	GtkTreeIter   iter;

	file_store = gth_file_list_get_model (file_list);
	pos = 0;
	if (! gth_file_store_get_nth_visible (file_store, pos, &iter))
		return -1;

	do {
		GthThumbnailState state;
		gboolean          _continue = FALSE;

		gtk_tree_model_get (GTK_TREE_MODEL (file_store),
				    &iter,
				    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, &state,
				    -1);

		if (skip_broken && (state == GTH_THUMBNAIL_STATE_ERROR))
			_continue = TRUE;

		if (only_selected && ! gth_file_selection_is_selected (GTH_FILE_SELECTION (file_list->priv->view), pos))
			_continue = TRUE;

		if (! _continue)
			return pos;

		pos++;
	}
	while (gth_file_store_get_next_visible (file_store, &iter));

	return -1;
}


int
gth_file_list_last_file (GthFileList *file_list,
			 gboolean     skip_broken,
			 gboolean     only_selected)
{
	GthFileStore *file_store;
	int           pos;
	GtkTreeIter   iter;

	file_store = gth_file_list_get_model (file_list);
	pos = gth_file_store_n_visibles (file_store) - 1;
	if (pos < 0)
		return -1;

	if (! gth_file_store_get_nth_visible (file_store, pos, &iter))
		return -1;

	do {
		GthThumbnailState state;
		gboolean          _continue = FALSE;

		gtk_tree_model_get (GTK_TREE_MODEL (file_store),
				    &iter,
				    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, &state,
				    -1);

		if (skip_broken && (state == GTH_THUMBNAIL_STATE_ERROR))
			_continue = TRUE;

		if (only_selected && ! gth_file_selection_is_selected (GTH_FILE_SELECTION (file_list->priv->view), pos))
			_continue = TRUE;

		if (! _continue)
			return pos;

		pos--;
	}
	while (gth_file_store_get_prev_visible (file_store, &iter));

	return -1;
}


int
gth_file_list_next_file (GthFileList *file_list,
			 int          pos,
			 gboolean     skip_broken,
			 gboolean     only_selected,
			 gboolean     wrap)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;
	gboolean      valid;

	file_store = gth_file_list_get_model (file_list);

	pos++;
	if (pos >= 0) {
		valid = gth_file_store_get_nth_visible (file_store, pos, &iter);
		if (! valid && wrap) {
			pos = 0;
			valid = gth_file_store_get_nth_visible (file_store, pos, &iter);
		}
	}
	else
		valid = FALSE;

	if (! valid)
		return -1;

	do {
		GthThumbnailState  state;
		gboolean           _continue = FALSE;

		gtk_tree_model_get (GTK_TREE_MODEL (file_store),
				    &iter,
				    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, &state,
				    -1);

		if (skip_broken && (state == GTH_THUMBNAIL_STATE_ERROR))
			_continue = TRUE;

		if (only_selected && ! gth_file_selection_is_selected (GTH_FILE_SELECTION (file_list->priv->view), pos))
			_continue = TRUE;

		if (! _continue)
			return pos;

		pos++;
	}
	while (gth_file_store_get_next_visible (file_store, &iter));

	return -1;
}


int
gth_file_list_prev_file (GthFileList *file_list,
			 int          pos,
			 gboolean     skip_broken,
			 gboolean     only_selected,
			 gboolean     wrap)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;
	gboolean      valid;

	file_store = gth_file_list_get_model (file_list);

	pos--;
	if (pos >= 0)
		valid = gth_file_store_get_nth_visible (file_store, pos, &iter);
	else if (wrap) {
		pos = gth_file_store_n_visibles (file_store) - 1;
		if (pos >= 0)
			valid = gth_file_store_get_nth_visible (file_store, pos, &iter);
		else
			valid = FALSE;
	}
	else
		valid = FALSE;

	if (! valid)
		return -1;

	do {
		GthThumbnailState  state;
		gboolean           _continue = FALSE;

		gtk_tree_model_get (GTK_TREE_MODEL (file_store),
				    &iter,
				    GTH_FILE_STORE_THUMBNAIL_STATE_COLUMN, &state,
				    -1);

		if (skip_broken && (state == GTH_THUMBNAIL_STATE_ERROR))
			_continue = TRUE;

		if (only_selected && ! gth_file_selection_is_selected (GTH_FILE_SELECTION (file_list->priv->view), pos))
			_continue = TRUE;

		if (! _continue)
			return pos;

		pos--;
	}
	while (gth_file_store_get_prev_visible (file_store, &iter));

	return -1;
}


void
gth_file_list_enable_drag_source (GthFileList     *file_list,
				  GdkDragAction    actions)
{
	GtkTargetList  *target_list;
	GtkTargetEntry *targets;
	int             n_targets;

	target_list = gtk_target_list_new (NULL, 0);
	gtk_target_list_add_uri_targets (target_list, 0);
	gtk_target_list_add_text_targets (target_list, 0);
	targets = gtk_target_table_new_from_list (target_list, &n_targets);
	gth_file_view_enable_drag_source (GTH_FILE_VIEW (file_list->priv->view),
					  GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
					  targets,
					  n_targets,
					  actions);

	gtk_target_list_unref (target_list);
	gtk_target_table_free (targets, n_targets);
}


void
gth_file_list_unset_drag_source (GthFileList *file_list)
{
	gth_file_view_unset_drag_source (GTH_FILE_VIEW (file_list->priv->view));
}


void
gth_file_list_focus (GthFileList *file_list)
{
	GtkWidget *child;

	child = gtk_stack_get_visible_child (GTK_STACK (file_list->priv->notebook));
	if (GTK_IS_BIN (child))
		child = gtk_bin_get_child (GTK_BIN (child));
	gtk_widget_grab_focus ((child != NULL) ? child : GTK_WIDGET (file_list));
}
