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
#include "gth-file-view.h"


#define DEFAULT_THUMBNAIL_SIZE 128


enum {
	CURSOR_CHANGED,
	FILE_ACTIVATED,
	LAST_SIGNAL
};


static guint gth_file_view_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_INTERFACE (GthFileView, gth_file_view, 0)


static void
gth_file_view_default_init (GthFileViewInterface *iface)
{
	/* signals */

	gth_file_view_signals[CURSOR_CHANGED] =
		g_signal_new ("cursor-changed",
			      GTH_TYPE_FILE_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFileViewInterface, cursor_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1,
			      G_TYPE_INT);

	gth_file_view_signals[FILE_ACTIVATED] =
		g_signal_new ("file-activated",
			      GTH_TYPE_FILE_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFileViewInterface, file_activated),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOXED,
			      G_TYPE_NONE, 1,
			      GTK_TYPE_TREE_PATH);

	/* properties */

	g_object_interface_install_property (iface,
					     g_param_spec_string ("caption",
							     	  "Caption",
							     	  "The file attributes to view in the caption",
							     	  "none",
							     	  G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	g_object_interface_install_property (iface,
					     g_param_spec_object ("model",
							     	  "Data Store",
							     	  "The data to view",
							     	  GTK_TYPE_TREE_MODEL,
							     	  G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	g_object_interface_install_property (iface,
					     g_param_spec_int ("thumbnail-size",
							       "Thumbnail size",
							       "The max width and height of the thumbnails",
							       0,
							       G_MAXINT32,
							       DEFAULT_THUMBNAIL_SIZE,
							       G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}


void
gth_file_view_set_model (GthFileView  *self,
		         GtkTreeModel *model)
{
	g_return_if_fail (GTH_IS_FILE_VIEW (self));

	g_object_set (self, "model", model, NULL);
}


GtkTreeModel *
gth_file_view_get_model (GthFileView *self)
{
	GtkTreeModel *model;

	g_return_val_if_fail (GTH_IS_FILE_VIEW (self), NULL);

	g_object_get (self, "model", &model, NULL);

	return model;
}


void
gth_file_view_set_caption (GthFileView *self,
			   const char  *attributes)
{
	g_return_if_fail (GTH_IS_FILE_VIEW (self));

	g_object_set (self, "caption", attributes, NULL);
}


char *
gth_file_view_get_caption (GthFileView *self)
{
	char *attributes;

	g_return_val_if_fail (GTH_IS_FILE_VIEW (self), NULL);

	g_object_get (self, "caption", &attributes, NULL);

	return attributes;
}


void
gth_file_view_set_thumbnail_size (GthFileView *self,
				  int          value)
{
	g_return_if_fail (GTH_IS_FILE_VIEW (self));

	g_object_set (self, "thumbnail-size", value, NULL);
}


gboolean
gth_file_view_get_thumbnail_size (GthFileView *self)
{
	int value;

	g_return_val_if_fail (GTH_IS_FILE_VIEW (self), FALSE);

	g_object_get (self, "thumbnail-size", &value, NULL);

	return value;
}


void
gth_file_view_scroll_to (GthFileView *self,
			 int          pos,
			 double       yalign)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->scroll_to (self, pos, yalign);
}


void
gth_file_view_set_vscroll (GthFileView *self,
			   double       vscroll)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->set_vscroll (self, vscroll);
}


GthVisibility
gth_file_view_get_visibility (GthFileView *self,
			      int          pos)
{
	return GTH_FILE_VIEW_GET_INTERFACE (self)->get_visibility (self, pos);
}


int
gth_file_view_get_at_position (GthFileView *self,
			       int          x,
			       int          y)
{
	return GTH_FILE_VIEW_GET_INTERFACE (self)->get_at_position (self, x, y);
}


int
gth_file_view_get_first_visible (GthFileView *self)
{
	return GTH_FILE_VIEW_GET_INTERFACE (self)->get_first_visible (self);
}


int
gth_file_view_get_last_visible (GthFileView *self)
{
	return GTH_FILE_VIEW_GET_INTERFACE (self)->get_last_visible (self);
}


void
gth_file_view_activated (GthFileView *self,
			 int          pos)
{
	GtkTreePath *path;

	g_return_if_fail (GTH_IS_FILE_VIEW (self));

	path = gtk_tree_path_new_from_indices (pos, -1);
	g_signal_emit (self, gth_file_view_signals[FILE_ACTIVATED], 0, path);

	gtk_tree_path_free (path);
}


void
gth_file_view_set_cursor (GthFileView *self,
			  int          pos)
{
	g_return_if_fail (GTH_IS_FILE_VIEW (self));

	g_signal_emit (self, gth_file_view_signals[CURSOR_CHANGED], 0, pos);
}


int
gth_file_view_get_cursor (GthFileView *self)
{
	return GTH_FILE_VIEW_GET_INTERFACE (self)->get_cursor (self);
}


void
gth_file_view_enable_drag_source (GthFileView          *self,
				  GdkModifierType       start_button_mask,
				  const GtkTargetEntry *targets,
				  int                   n_targets,
				  GdkDragAction         actions)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->enable_drag_source (self, start_button_mask, targets, n_targets, actions);
}


void
gth_file_view_unset_drag_source (GthFileView *self)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->unset_drag_source (self);
}


void
gth_file_view_enable_drag_dest (GthFileView          *self,
				const GtkTargetEntry *targets,
				int                   n_targets,
				GdkDragAction         actions)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->enable_drag_dest (self, targets, n_targets, actions);
}


void
gth_file_view_unset_drag_dest (GthFileView *self)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->unset_drag_dest (self);
}


void
gth_file_view_set_drag_dest_pos (GthFileView    *self,
				 GdkDragContext *context,
			         int             x,
			         int             y,
			         guint           time,
		                 int            *pos)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->set_drag_dest_pos (self, context, x, y, time, pos);
}


void
gth_file_view_get_drag_dest_pos (GthFileView *self,
				 int         *pos)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->get_drag_dest_pos (self, pos);
}
