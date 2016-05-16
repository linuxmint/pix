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
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "gth-file-tool-negative.h"


G_DEFINE_TYPE (GthFileToolNegative, gth_file_tool_negative, GTH_TYPE_FILE_TOOL)


typedef struct {
	GtkWidget       *viewer_page;
	cairo_surface_t *source;
	cairo_surface_t *destination;
} NegativeData;


static void
gth_file_tool_negative_update_sensitivity (GthFileTool *base)
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
negative_data_free (gpointer user_data)
{
	NegativeData *negative_data = user_data;

	cairo_surface_destroy (negative_data->destination);
	cairo_surface_destroy (negative_data->source);
	g_free (negative_data);
}


static void
negative_init (GthAsyncTask *task,
	       gpointer      user_data)
{
	gth_task_progress (GTH_TASK (task), _("Applying changes"), NULL, TRUE, 0.0);
}


static gpointer
negative_exec (GthAsyncTask *task,
	       gpointer      user_data)
{
	NegativeData    *negative_data = user_data;
	cairo_format_t   format;
	int              width;
	int              height;
	int              source_stride;
	int              destination_stride;
	unsigned char   *p_source_line;
	unsigned char   *p_destination_line;
	unsigned char   *p_source;
	unsigned char   *p_destination;
	gboolean         cancelled;
	double           progress;
	gboolean         terminated;
	int              x, y;
	unsigned char    red, green, blue, alpha;

	format = cairo_image_surface_get_format (negative_data->source);
	width = cairo_image_surface_get_width (negative_data->source);
	height = cairo_image_surface_get_height (negative_data->source);
	source_stride = cairo_image_surface_get_stride (negative_data->source);

	negative_data->destination = cairo_image_surface_create (format, width, height);
	cairo_surface_flush (negative_data->destination);
	destination_stride = cairo_image_surface_get_stride (negative_data->destination);
	p_source_line = cairo_image_surface_get_data (negative_data->source);
	p_destination_line = cairo_image_surface_get_data (negative_data->destination);
	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled)
			return NULL;

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		p_destination = p_destination_line;
		for (x = 0; x < width; x++) {
			CAIRO_GET_RGBA (p_source, red, green, blue, alpha);
			CAIRO_SET_RGBA (p_destination,
					255 - red,
					255 - green,
					255 - blue,
					alpha);

			p_source += 4;
			p_destination += 4;
		}
		p_source_line += source_stride;
		p_destination_line += destination_stride;
	}

	cairo_surface_mark_dirty (negative_data->destination);
	terminated = TRUE;
	gth_async_task_set_data (task, &terminated, NULL, NULL);

	return NULL;
}


static void
negative_after (GthAsyncTask *task,
		GError       *error,
	        gpointer      user_data)
{
	NegativeData *negative_data = user_data;

	if (error == NULL)
		gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (negative_data->viewer_page),
					         negative_data->destination,
					         TRUE);
}


static void
gth_file_tool_negative_activate (GthFileTool *base)
{
	GtkWidget       *window;
	GtkWidget       *viewer_page;
	GtkWidget       *viewer;
	cairo_surface_t *image;
	NegativeData    *negative_data;
	GthTask         *task;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer));
	if (image == NULL)
		return;

	negative_data = g_new0 (NegativeData, 1);
	negative_data->viewer_page = viewer_page;
	negative_data->source = cairo_surface_reference (image);
	task = gth_async_task_new (negative_init,
				   negative_exec,
				   negative_after,
				   negative_data,
				   negative_data_free);
	gth_browser_exec_task (GTH_BROWSER (window), task, FALSE);

	g_object_unref (task);
}


static void
gth_file_tool_negative_class_init (GthFileToolNegativeClass *klass)
{
	GthFileToolClass *file_tool_class;

	file_tool_class = GTH_FILE_TOOL_CLASS (klass);
	file_tool_class->update_sensitivity = gth_file_tool_negative_update_sensitivity;
	file_tool_class->activate = gth_file_tool_negative_activate;
}


static void
gth_file_tool_negative_init (GthFileToolNegative *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-invert", _("Negative"), _("Negative"), FALSE);
}
