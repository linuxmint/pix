/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include "gth-file-tool-adjust-contrast.h"


#define HISTOGRAM_CROP 0.005


G_DEFINE_TYPE (GthFileToolAdjustContrast, gth_file_tool_adjust_contrast, GTH_TYPE_FILE_TOOL)


typedef struct {
	GtkWidget        *viewer_page;
	cairo_surface_t  *source;
	int              *lowest;
	int              *highest;
	double           *factor;
} AdjustContrastData;


static void
adjust_contrast_setup (AdjustContrastData *adjust_data)
{
	GthHistogram  *histogram;
	long         **cumulative;
	int            c, v;
	glong          n_pixels;
	double         lower_threshold;
	double         higher_threshold;

	/* histogram */

	histogram = gth_histogram_new ();
	gth_histogram_calculate_for_image (histogram, adjust_data->source);
	cumulative = gth_histogram_get_cumulative (histogram);

	/* lowest and highest values for each channel */

	adjust_data->lowest = g_new (int, GTH_HISTOGRAM_N_CHANNELS);
	adjust_data->highest = g_new (int, GTH_HISTOGRAM_N_CHANNELS);

	n_pixels = cairo_image_surface_get_width (adjust_data->source) * cairo_image_surface_get_height (adjust_data->source);
	lower_threshold = HISTOGRAM_CROP * n_pixels;
	higher_threshold = (1.0 - HISTOGRAM_CROP) * n_pixels;

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		gboolean lowest_set = FALSE;

		for (v = 0; v < 256; v++) {
			if (! lowest_set && (cumulative[c][v] >= lower_threshold)) {
				adjust_data->lowest[c] = v;
				lowest_set = TRUE;
			}

			if (cumulative[c][v] <= higher_threshold)
				adjust_data->highest[c] = v;
		}
	}

	/* stretch factor */

	adjust_data->factor = g_new (double, GTH_HISTOGRAM_N_CHANNELS);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		if (adjust_data->highest[c] != adjust_data->lowest[c])
			adjust_data->factor[c] = 255.0 / ((double) adjust_data->highest[c] - adjust_data->lowest[c]);
		else
			adjust_data->factor[c] = 0.0;
	}

	/**/

	gth_cumulative_histogram_free (cumulative);
	g_object_unref (histogram);
}


static guchar
adjust_contrast_func (AdjustContrastData *adjust_data,
		      int                 n_channel,
		      guchar              value)
{
	if (value <= adjust_data->lowest[n_channel])
		return 0;
	else if (value >= adjust_data->highest[n_channel])
		return 255;
	else
		return (int) (adjust_data->factor[n_channel] * (value - adjust_data->lowest[n_channel]));
}


static gpointer
adjust_contrast_exec (GthAsyncTask *task,
		      gpointer      user_data)
{
	AdjustContrastData *adjust_data = user_data;
	cairo_format_t      format;
	int                 width;
	int                 height;
	int                 source_stride;
	cairo_surface_t    *destination;
	int                 destination_stride;
	unsigned char      *p_source_line;
	unsigned char      *p_destination_line;
	unsigned char      *p_source;
	unsigned char      *p_destination;
	gboolean            cancelled;
	double              progress;
	int                 x, y;
	unsigned char       red, green, blue, alpha;
	GthImage          *destination_image;

	/* initialize some extra data */

	adjust_contrast_setup (adjust_data);

	/* convert the image */

	format = cairo_image_surface_get_format (adjust_data->source);
	width = cairo_image_surface_get_width (adjust_data->source);
	height = cairo_image_surface_get_height (adjust_data->source);
	source_stride = cairo_image_surface_get_stride (adjust_data->source);

	destination = cairo_image_surface_create (format, width, height);
	cairo_surface_flush (destination);
	destination_stride = cairo_image_surface_get_stride (destination);
	p_source_line = cairo_image_surface_get_data (adjust_data->source);
	p_destination_line = cairo_image_surface_get_data (destination);
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
			red   = adjust_contrast_func (adjust_data, GTH_HISTOGRAM_CHANNEL_RED, red);
			green = adjust_contrast_func (adjust_data, GTH_HISTOGRAM_CHANNEL_GREEN, green);
			blue  = adjust_contrast_func (adjust_data, GTH_HISTOGRAM_CHANNEL_BLUE, blue);
			CAIRO_SET_RGBA (p_destination, red, green, blue, alpha);

			p_source += 4;
			p_destination += 4;
		}
		p_source_line += source_stride;
		p_destination_line += destination_stride;
	}

	cairo_surface_mark_dirty (destination);

	destination_image = gth_image_new_for_surface (destination);
	gth_image_task_set_destination (GTH_IMAGE_TASK (task), destination_image);

	_g_object_unref (destination_image);
	cairo_surface_destroy (destination);

	return NULL;
}


static void
adjust_contrast_after (GthAsyncTask *task,
		       GError       *error,
		       gpointer      user_data)
{
	AdjustContrastData *adjust_data = user_data;

	g_free (adjust_data->lowest);
	adjust_data->lowest = NULL;

	g_free (adjust_data->highest);
	adjust_data->highest = NULL;
}


static void
adjust_contrast_data_free (gpointer user_data)
{
	AdjustContrastData *adjust_contrast_data = user_data;

	g_object_unref (adjust_contrast_data->viewer_page);
	cairo_surface_destroy (adjust_contrast_data->source);
	g_free (adjust_contrast_data);
}


static void
image_task_completed_cb (GthTask  *task,
			 GError   *error,
			 gpointer  user_data)
{
	GthFileTool     *base = user_data;
	GthImage        *destination_image;
	cairo_surface_t *destination;
	GtkWidget       *window;
	GtkWidget       *viewer_page;

	if (error != NULL) {
		g_object_unref (task);
		return;
	}

	destination_image = gth_image_task_get_destination (GTH_IMAGE_TASK (task));
	if (destination_image == NULL) {
		g_object_unref (task);
		return;
	}

	destination = gth_image_get_cairo_surface (destination_image);

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), destination, TRUE);

	cairo_surface_destroy (destination);
	g_object_unref (task);
}


static void
gth_file_tool_adjust_contrast_activate (GthFileTool *base)
{
	GtkWidget          *window;
	GtkWidget          *viewer_page;
	GtkWidget          *viewer;
	cairo_surface_t    *image;
	AdjustContrastData *adjust_contrast_data;
	GthTask            *task;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer));
	if (image == NULL)
		return;

	adjust_contrast_data = g_new0 (AdjustContrastData, 1);
	adjust_contrast_data->viewer_page = g_object_ref (viewer_page);
	adjust_contrast_data->source = cairo_surface_reference (image);
	adjust_contrast_data->lowest = NULL;
	adjust_contrast_data->highest = NULL;
	adjust_contrast_data->factor = NULL;
	task = gth_image_task_new (_("Applying changes"),
				   NULL,
				   adjust_contrast_exec,
				   adjust_contrast_after,
				   adjust_contrast_data,
				   adjust_contrast_data_free);
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (image_task_completed_cb),
			  base);
	gth_browser_exec_task (GTH_BROWSER (window), task, FALSE);
}


static void
gth_file_tool_adjust_contrast_update_sensitivity (GthFileTool *base)
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
gth_file_tool_adjust_contrast_init (GthFileToolAdjustContrast *self)
{
	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-adjust-contrast", _("Adjust Contrast"), NULL, TRUE);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Automatic contrast adjustment"));
}


static void
gth_file_tool_adjust_contrast_class_init (GthFileToolAdjustContrastClass *klass)
{
	GthFileToolClass *file_tool_class;

	file_tool_class = GTH_FILE_TOOL_CLASS (klass);
	file_tool_class->update_sensitivity = gth_file_tool_adjust_contrast_update_sensitivity;
	file_tool_class->activate = gth_file_tool_adjust_contrast_activate;
}
