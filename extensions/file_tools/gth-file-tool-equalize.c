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
#include "gth-file-tool-equalize.h"


G_DEFINE_TYPE (GthFileToolEqualize, gth_file_tool_equalize, GTH_TYPE_FILE_TOOL)


typedef struct {
	GtkWidget        *viewer_page;
	cairo_surface_t  *source;
	cairo_surface_t  *destination;
	long            **cumulative;
	double            factor;
} EqualizeData;


static void
equalize_before (GthAsyncTask *task,
	         gpointer      user_data)
{
	gth_task_progress (GTH_TASK (task), _("Equalizing image histogram"), NULL, TRUE, 0.0);
}


static void
equalize_histogram_setup (EqualizeData *equalize_data)
{
	GthHistogram *histogram;

	histogram = gth_histogram_new ();
	gth_histogram_calculate_for_image (histogram, equalize_data->source);
	equalize_data->cumulative = gth_histogram_get_cumulative (histogram);
	equalize_data->factor = 255.0 / (cairo_image_surface_get_width (equalize_data->source) * cairo_image_surface_get_height (equalize_data->source));

	g_object_unref (histogram);
}


static guchar
equalize_func (EqualizeData *equalize_data,
	       int           n_channel,
	       guchar        value)
{
	return (guchar) (equalize_data->factor * equalize_data->cumulative[n_channel][value]);
}


static gpointer
equalize_exec (GthAsyncTask *task,
	       gpointer      user_data)
{
	EqualizeData    *equalize_data = user_data;
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

	/* initialize the extra data */

	equalize_histogram_setup (equalize_data);

	/* convert the image */

	format = cairo_image_surface_get_format (equalize_data->source);
	width = cairo_image_surface_get_width (equalize_data->source);
	height = cairo_image_surface_get_height (equalize_data->source);
	source_stride = cairo_image_surface_get_stride (equalize_data->source);

	equalize_data->destination = cairo_image_surface_create (format, width, height);
	cairo_surface_flush (equalize_data->destination);
	destination_stride = cairo_image_surface_get_stride (equalize_data->destination);
	p_source_line = cairo_image_surface_get_data (equalize_data->source);
	p_destination_line = cairo_image_surface_get_data (equalize_data->destination);
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
			red   = equalize_func (equalize_data, GTH_HISTOGRAM_CHANNEL_RED, red);
			green = equalize_func (equalize_data, GTH_HISTOGRAM_CHANNEL_GREEN, green);
			blue  = equalize_func (equalize_data, GTH_HISTOGRAM_CHANNEL_BLUE, blue);
			CAIRO_SET_RGBA (p_destination, red, green, blue, alpha);

			p_source += 4;
			p_destination += 4;
		}
		p_source_line += source_stride;
		p_destination_line += destination_stride;
	}

	cairo_surface_mark_dirty (equalize_data->destination);
	terminated = TRUE;
	gth_async_task_set_data (task, &terminated, NULL, NULL);

	return NULL;
}


static void
equalize_after (GthAsyncTask *task,
	        GError       *error,
	        gpointer      user_data)
{
	EqualizeData *equalize_data = user_data;

	if (error == NULL)
		gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (equalize_data->viewer_page), equalize_data->destination, TRUE);

	gth_cumulative_histogram_free (equalize_data->cumulative);
	equalize_data->cumulative = NULL;
}


static void
equalize_destroy_data (gpointer user_data)
{
	EqualizeData *equalize_data = user_data;

	g_object_unref (equalize_data->viewer_page);
	cairo_surface_destroy (equalize_data->destination);
	cairo_surface_destroy (equalize_data->source);
	g_free (equalize_data);
}


static void
gth_file_tool_equalize_activate (GthFileTool *base)
{
	GtkWidget       *window;
	GtkWidget       *viewer_page;
	GtkWidget       *viewer;
	cairo_surface_t *image;
	EqualizeData    *equalize_data;
	GthTask         *task;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer));
	if (image == NULL)
		return;

	equalize_data = g_new0 (EqualizeData, 1);
	equalize_data->viewer_page = g_object_ref (viewer_page);
	equalize_data->source = cairo_surface_reference (image);
	task = gth_async_task_new (equalize_before,
				   equalize_exec,
				   equalize_after,
				   equalize_data,
				   equalize_destroy_data);
	gth_browser_exec_task (GTH_BROWSER (window), task, FALSE);

	g_object_unref (task);
}


static void
gth_file_tool_equalize_update_sensitivity (GthFileTool *base)
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
gth_file_tool_equalize_class_init (GthFileToolEqualizeClass *klass)
{
	GthFileToolClass *file_tool_class;

	file_tool_class = GTH_FILE_TOOL_CLASS (klass);
	file_tool_class->update_sensitivity = gth_file_tool_equalize_update_sensitivity;
	file_tool_class->activate = gth_file_tool_equalize_activate;
}


static void
gth_file_tool_equalize_init (GthFileToolEqualize *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), "histogram", _("Equalize"), NULL, FALSE);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Equalize image histogram"));
}
