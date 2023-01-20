/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "gth-image-viewer-page-tool.h"


struct _GthImageViewerPageToolPrivate {
	cairo_surface_t *source;
	GthTask         *image_task;
};


G_DEFINE_TYPE_WITH_CODE (GthImageViewerPageTool,
			 gth_image_viewer_page_tool,
			 GTH_TYPE_FILE_TOOL,
			 G_ADD_PRIVATE (GthImageViewerPageTool))


static void
original_image_task_completed_cb (GthTask  *task,
				  GError   *error,
				  gpointer  user_data)
{
	GthImageViewerPageTool *self = user_data;

	self->priv->image_task = NULL;

	if (gth_file_tool_is_cancelled (GTH_FILE_TOOL (self))) {
		gth_image_viewer_page_tool_reset_image (self);
		g_object_unref (task);
		return;
	}

	if (error != NULL) {
		g_object_unref (task);
		return;
	}

	self->priv->source = gth_original_image_task_get_image (task);
	if (self->priv->source != NULL)
		GTH_IMAGE_VIEWER_PAGE_TOOL_GET_CLASS (self)->modify_image (self);

	g_object_unref (task);
}


static void
gth_image_viewer_page_tool_activate (GthFileTool *base)
{
	GthImageViewerPageTool *self = (GthImageViewerPageTool *) base;
	GtkWidget              *window;
	GthViewerPage          *viewer_page;

	/* load the original image */

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	self->priv->image_task = gth_original_image_task_new (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	g_signal_connect (self->priv->image_task,
			  "completed",
			  G_CALLBACK (original_image_task_completed_cb),
			  self);
	gth_browser_exec_task (GTH_BROWSER (gth_file_tool_get_window (GTH_FILE_TOOL (self))),
			       self->priv->image_task,
			       GTH_TASK_FLAGS_DEFAULT);
}


static void
gth_image_viewer_page_tool_cancel (GthFileTool *base)
{
	GthImageViewerPageTool *self = (GthImageViewerPageTool *) base;

	if (self->priv->image_task != NULL) {
		gth_task_cancel (self->priv->image_task);
		return;
	}

	GTH_IMAGE_VIEWER_PAGE_TOOL_GET_CLASS (self)->reset_image (self);
}


static void
gth_image_viewer_page_tool_update_sensitivity (GthFileTool *base)
{
	GthViewerPage *viewer_page;

	viewer_page = gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (base));
	gtk_widget_set_sensitive (GTK_WIDGET (base), viewer_page != NULL);
}


static void
base_modify_image (GthImageViewerPageTool *self)
{
	gth_file_tool_show_options (GTH_FILE_TOOL (self));
}


static void
base_reset_image (GthImageViewerPageTool *self)
{
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
gth_image_viewer_page_tool_finalize (GObject *object)
{
	GthImageViewerPageTool *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_VIEWER_PAGE_TOOL (object));

	self = (GthImageViewerPageTool *) object;

	cairo_surface_destroy (self->priv->source);

	/* Chain up */
	G_OBJECT_CLASS (gth_image_viewer_page_tool_parent_class)->finalize (object);
}


static void
gth_image_viewer_page_tool_class_init (GthImageViewerPageToolClass *klass)
{
	GObjectClass     *gobject_class;
	GthFileToolClass *file_tool_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_image_viewer_page_tool_finalize;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->activate = gth_image_viewer_page_tool_activate;
	file_tool_class->cancel = gth_image_viewer_page_tool_cancel;
	file_tool_class->update_sensitivity = gth_image_viewer_page_tool_update_sensitivity;

	klass->modify_image = base_modify_image;
	klass->reset_image = base_reset_image;
}


static void
gth_image_viewer_page_tool_init (GthImageViewerPageTool *self)
{
	self->priv = gth_image_viewer_page_tool_get_instance_private (self);
	self->priv->source = NULL;
	self->priv->image_task = NULL;
}


cairo_surface_t *
gth_image_viewer_page_tool_get_source (GthImageViewerPageTool *self)
{
	return self->priv->source;
}


GthTask *
gth_image_viewer_page_tool_get_task (GthImageViewerPageTool *self)
{
	return self->priv->image_task;
}


GthViewerPage *
gth_image_viewer_page_tool_get_page (GthImageViewerPageTool *self)
{
	GtkWidget     *window;
	GthViewerPage *viewer_page;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	return viewer_page;
}


void
gth_image_viewer_page_tool_reset_image (GthImageViewerPageTool *self)
{
	GTH_IMAGE_VIEWER_PAGE_TOOL_GET_CLASS (self)->reset_image (self);
}
