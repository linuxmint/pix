/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2008 Free Software Foundation, Inc.
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
#include "glib-utils.h"
#include "gth-enum-types.h"
#include "gth-marshal.h"
#include "gth-monitor.h"


#define UPDATE_DIR_DELAY 500


enum {
	ICON_THEME_CHANGED,
	BOOKMARKS_CHANGED,
	SHORTCUTS_CHANGED,
	FILTERS_CHANGED,
	TAGS_CHANGED,
	FOLDER_CONTENT_CHANGED,
	FILE_RENAMED,
	METADATA_CHANGED,
	EMBLEMS_CHANGED,
	ENTRY_POINTS_CHANGED,
	ORDER_CHANGED,
	LAST_SIGNAL
};


struct _GthMonitorPrivate {
	gboolean    active;
	GHashTable *paused_files;
};


static guint monitor_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthMonitor,
			 gth_monitor,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthMonitor))


static void
gth_monitor_finalize (GObject *object)
{
	GthMonitor *self = GTH_MONITOR (object);

	g_hash_table_unref (self->priv->paused_files);

	G_OBJECT_CLASS (gth_monitor_parent_class)->finalize (object);
}


static void
gth_monitor_init (GthMonitor *self)
{
	self->priv = gth_monitor_get_instance_private (self);
	self->priv->active = TRUE;
	self->priv->paused_files = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);
}


static void
gth_monitor_class_init (GthMonitorClass *class)
{
	GObjectClass  *gobject_class;

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_monitor_finalize;

	monitor_signals[ICON_THEME_CHANGED] =
		g_signal_new ("icon-theme-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, icon_theme_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	monitor_signals[BOOKMARKS_CHANGED] =
		g_signal_new ("bookmarks-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, bookmarks_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	monitor_signals[SHORTCUTS_CHANGED] =
		g_signal_new ("shortcuts-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, shortcuts_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	monitor_signals[FILTERS_CHANGED] =
		g_signal_new ("filters-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, filters_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	monitor_signals[TAGS_CHANGED] =
		g_signal_new ("tags-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, tags_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	monitor_signals[FOLDER_CONTENT_CHANGED] =
		g_signal_new ("folder-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, folder_changed),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_BOXED_INT_ENUM,
			      G_TYPE_NONE,
			      4,
			      G_TYPE_OBJECT,
			      G_TYPE_OBJECT_LIST,
			      G_TYPE_INT,
			      GTH_TYPE_MONITOR_EVENT);
	monitor_signals[FILE_RENAMED] =
		g_signal_new ("file-renamed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, file_renamed),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_OBJECT,
			      G_TYPE_OBJECT);
	monitor_signals[METADATA_CHANGED] =
		g_signal_new ("metadata-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, metadata_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_OBJECT);
	monitor_signals[EMBLEMS_CHANGED] =
		g_signal_new ("emblems-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, emblems_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOXED,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_OBJECT_LIST);
	monitor_signals[ENTRY_POINTS_CHANGED] =
		g_signal_new ("entry-points-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, entry_points_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	monitor_signals[ORDER_CHANGED] =
		g_signal_new ("order-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, order_changed),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_POINTER,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_OBJECT,
			      G_TYPE_POINTER);
}


GthMonitor *
gth_monitor_new (void)
{
	return (GthMonitor*) g_object_new (GTH_TYPE_MONITOR, NULL);
}


void
gth_monitor_pause (GthMonitor *self,
		   GFile      *file)
{
	int n;

	g_return_if_fail (file != NULL);

	n = GPOINTER_TO_INT (g_hash_table_lookup (self->priv->paused_files, file));
	n += 1;
	g_hash_table_insert (self->priv->paused_files, g_object_ref (file), GINT_TO_POINTER (n));
}


void
gth_monitor_resume (GthMonitor *self,
		    GFile      *file)
{
	int n;

	g_return_if_fail (file != NULL);

	n = GPOINTER_TO_INT (g_hash_table_lookup (self->priv->paused_files, file));
	if (n == 0)
		return;
	n -= 1;
	if (n > 0)
		g_hash_table_insert (self->priv->paused_files, g_object_ref (file), GINT_TO_POINTER (n));
	else
		g_hash_table_remove (self->priv->paused_files, file);
}


void
gth_monitor_icon_theme_changed (GthMonitor *self)
{
	g_return_if_fail (GTH_IS_MONITOR (self));

	g_signal_emit (G_OBJECT (self),
		       monitor_signals[ICON_THEME_CHANGED],
		       0);
}


void
gth_monitor_bookmarks_changed (GthMonitor *self)
{
	g_return_if_fail (GTH_IS_MONITOR (self));

	g_signal_emit (G_OBJECT (self),
		       monitor_signals[BOOKMARKS_CHANGED],
		       0);
}


void
gth_monitor_shortcuts_changed (GthMonitor *self)
{
	g_return_if_fail (GTH_IS_MONITOR (self));

	g_signal_emit (G_OBJECT (self),
		       monitor_signals[SHORTCUTS_CHANGED],
		       0);
}


void
gth_monitor_filters_changed (GthMonitor *self)
{
	g_return_if_fail (GTH_IS_MONITOR (self));

	g_signal_emit (G_OBJECT (self),
		       monitor_signals[FILTERS_CHANGED],
		       0);
}


void
gth_monitor_tags_changed (GthMonitor *self)
{
	g_return_if_fail (GTH_IS_MONITOR (self));

	g_signal_emit (G_OBJECT (self),
		       monitor_signals[TAGS_CHANGED],
		       0);
}


void
gth_monitor_folder_changed (GthMonitor      *self,
			    GFile           *parent,
			    GList           *list,
			    GthMonitorEvent  event)
{
	g_return_if_fail (GTH_IS_MONITOR (self));

	if (g_hash_table_lookup (self->priv->paused_files, parent) != NULL)
		return;

	g_signal_emit (G_OBJECT (self),
		       monitor_signals[FOLDER_CONTENT_CHANGED],
		       0,
		       parent,
		       list,
		       -1,
		       event);
}


void
gth_monitor_files_created_with_pos (GthMonitor *self,
				    GFile      *parent,
				    GList      *list, /* GFile list */
				    int         position)
{
	g_return_if_fail (GTH_IS_MONITOR (self));

	if (g_hash_table_lookup (self->priv->paused_files, parent) != NULL)
		return;

	g_signal_emit (G_OBJECT (self),
		       monitor_signals[FOLDER_CONTENT_CHANGED],
		       0,
		       parent,
		       list,
		       position,
		       GTH_MONITOR_EVENT_CREATED);
}


void
gth_monitor_file_renamed (GthMonitor *self,
			  GFile      *file,
			  GFile      *new_file)
{
	g_return_if_fail (GTH_IS_MONITOR (self));

	if (g_hash_table_lookup (self->priv->paused_files, file) != NULL)
		return;

	g_signal_emit (G_OBJECT (self),
		       monitor_signals[FILE_RENAMED],
		       0,
		       file,
		       new_file);
}


/* -- gth_monitor_files_deleted -- */


static void
parent_table_value_free (gpointer key,
			 gpointer value,
			 gpointer user_data)
{
	_g_object_list_unref (value);
}


void
gth_monitor_files_deleted (GthMonitor *monitor,
			   GList      *list /* GFile list */)
{
	GHashTable *parent_table;
	GList      *scan;
	GList      *parent_list;

	parent_table = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);
	for (scan = list; scan; scan = scan->next) {
		GFile *file = scan->data;
		GFile *parent;
		GList *children;

		parent = g_file_get_parent (file);
		if (parent == NULL)
			return;

		children = g_hash_table_lookup (parent_table, parent);
		children = g_list_prepend (children, g_object_ref (file));
		g_hash_table_replace (parent_table, g_object_ref (parent), children);

		g_object_unref (parent);
	}

	parent_list = g_hash_table_get_keys (parent_table);
	for (scan = parent_list; scan; scan = scan->next) {
		GFile *parent = scan->data;
		GList *children;

		children = g_hash_table_lookup (parent_table, parent);
		if (children != NULL)
			gth_monitor_folder_changed (monitor,
						    parent,
						    children,
						    GTH_MONITOR_EVENT_DELETED);
	}

	g_list_free (parent_list);
	g_hash_table_foreach (parent_table, parent_table_value_free, NULL);
	g_hash_table_unref (parent_table);
}


void
gth_monitor_metadata_changed (GthMonitor  *self,
			      GthFileData *file_data)
{
	g_return_if_fail (GTH_IS_MONITOR (self));

	if (g_hash_table_lookup (self->priv->paused_files, file_data->file) != NULL)
		return;

	g_signal_emit (G_OBJECT (self),
		       monitor_signals[METADATA_CHANGED],
		       0,
		       file_data);
}


void
gth_monitor_emblems_changed (GthMonitor *self,
			     GList      *list /* GFile list */)
{
	GList *changed_list = NULL;
	GList *scan;

	/* ignore paused files */
	for (scan = list; scan; scan = scan->next) {
		GFile *file = scan->data;

		if (g_hash_table_lookup (self->priv->paused_files, file) != NULL)
			continue;

		changed_list = g_list_prepend (changed_list, g_object_ref (file));
	}
	changed_list = g_list_reverse (changed_list);

	if (changed_list == NULL)
		return;

	g_signal_emit (G_OBJECT (self),
		       monitor_signals[EMBLEMS_CHANGED],
		       0,
		       changed_list);

	_g_object_list_unref (changed_list);
}


void
gth_monitor_entry_points_changed (GthMonitor *self)
{
	g_return_if_fail (GTH_IS_MONITOR (self));

	g_signal_emit (G_OBJECT (self),
		       monitor_signals[ENTRY_POINTS_CHANGED],
		       0);
}


void
gth_monitor_order_changed (GthMonitor *self,
			   GFile      *file,
			   int        *new_order)
{
	g_return_if_fail (GTH_IS_MONITOR (self));

	if (g_hash_table_lookup (self->priv->paused_files, file) != NULL)
		return;

	g_signal_emit (G_OBJECT (self),
		       monitor_signals[ORDER_CHANGED],
		       0,
		       file,
		       new_order);
}
