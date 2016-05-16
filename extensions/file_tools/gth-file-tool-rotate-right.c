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
#include <math.h>
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "gth-file-tool-rotate-right.h"


G_DEFINE_TYPE (GthFileToolRotateRight, gth_file_tool_rotate_right, GTH_TYPE_FILE_TOOL)


static void
gth_file_tool_rotate_right_activate (GthFileTool *base)
{
	GtkWidget       *window;
	GtkWidget       *viewer_page;
	GtkWidget       *viewer;
	cairo_surface_t *old_image;
	cairo_surface_t *new_image;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	old_image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer));
	if (old_image == NULL)
		return;

	new_image = _cairo_image_surface_transform (old_image, GTH_TRANSFORM_ROTATE_90);
	gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), new_image, TRUE);

	cairo_surface_destroy (new_image);
}


static void
gth_file_tool_rotate_right_update_sensitivity (GthFileTool *base)
{
	GtkWidget *window;
	GtkWidget *viewer_page;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		gtk_widget_set_sensitive (GTK_WIDGET (base), FALSE);
	else
		gtk_widget_set_sensitive (GTK_WIDGET (base), TRUE);
}


static void
gth_file_tool_rotate_right_class_init (GthFileToolRotateRightClass *klass)
{
	GthFileToolClass *file_tool_class;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->update_sensitivity = gth_file_tool_rotate_right_update_sensitivity;
	file_tool_class->activate = gth_file_tool_rotate_right_activate;
}


static void
gth_file_tool_rotate_right_init (GthFileToolRotateRight *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-rotate-90", _("Rotate Right"), NULL, TRUE);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Rotate the image by 90 degrees clockwise"));
}
