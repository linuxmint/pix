/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011-2014 Free Software Foundation, Inc.
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
#include <extensions/image_viewer/image-viewer.h>
#include "gth-file-tool-adjust-contrast.h"
#include "gth-preview-tool.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150
#define PREVIEW_SIZE 0.9
#define HISTOGRAM_CROP_0_5 0.005 /* ignores the 0.5% on each side of the histogram */
#define HISTOGRAM_CROP_1_5 0.015 /* ignores the 1.5% on each side of the histogram */


typedef enum {
	METHOD_STRETCH,
	METHOD_STRETCH_0_5,
	METHOD_STRETCH_1_5,
	METHOD_EQUALIZE_LINEAR,
	METHOD_EQUALIZE_SQUARE_ROOT
} Method;


struct _GthFileToolAdjustContrastPrivate {
	cairo_surface_t    *destination;
	cairo_surface_t    *preview;
	GtkBuilder         *builder;
	GthTask            *image_task;
	GthImageViewerTool *preview_tool;
	guint               apply_event;
	gboolean            apply_to_original;
	gboolean            closing;
	Method		    method;
	Method              last_applied_method;
	gboolean            view_original;
};


G_DEFINE_TYPE_WITH_CODE (GthFileToolAdjustContrast,
			 gth_file_tool_adjust_contrast,
			 GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL,
			 G_ADD_PRIVATE (GthFileToolAdjustContrast))


/* equalize histogram */


typedef struct {
	Method   method;
	long   **value_map;
} EqualizeData;


static void
value_map_free (long **value_map)
{
	int c;

	if (value_map == NULL)
		return;

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		g_free (value_map[c]);
	g_free (value_map);
}


static double
get_histogram_value (GthHistogram        *histogram,
		     GthHistogramChannel  channel,
		     int                  bin,
		     Method               method)
{
	double h = gth_histogram_get_value (histogram, channel, bin);

	switch (method) {
	case METHOD_EQUALIZE_SQUARE_ROOT:
		return (h >= 2) ? sqrt (h) : h;
	case METHOD_EQUALIZE_LINEAR:
		return h;
	default:
		g_assert_not_reached ();
	}

	return 0;
}


static long **
get_value_map_for_equalize (GthHistogram *histogram,
			    Method	  method)
{
	long **value_map;
	int    c, v;

	value_map = g_new (long *, GTH_HISTOGRAM_N_CHANNELS);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		double sum;
		double scale;

		sum = 0.0;
		for (v = 0; v < 255; v++)
			sum += 2 * get_histogram_value (histogram, c, v, method);
		sum += get_histogram_value (histogram, c, 255, method);
		scale = 255 / sum;

		value_map[c] = g_new (long, 256);
		value_map[c][0] = 0;
		sum = get_histogram_value (histogram, c, 0, method);
		for (v = 1; v < 255; v++) {
			double delta = get_histogram_value (histogram, c, v, method);
			sum += delta;
			value_map[c][v] = (int) round (sum * scale);
			sum += delta;
		}
		value_map[c][255] = 255;
	}

	return value_map;
}


static long **
get_value_map_for_stretch (GthHistogram *histogram,
			   Method	 method)
{
	long **value_map;
	int    n_pixels, lower_limit, higher_limit;
	int    c, v;

	n_pixels = gth_histogram_get_n_pixels (histogram);
	switch (method) {
	case METHOD_STRETCH:
		lower_limit = 0;
		higher_limit = n_pixels;
		break;
	case METHOD_STRETCH_0_5:
		lower_limit = n_pixels * HISTOGRAM_CROP_0_5;
		higher_limit = n_pixels * (1 - HISTOGRAM_CROP_0_5);
		break;
	case METHOD_STRETCH_1_5:
		lower_limit = n_pixels * HISTOGRAM_CROP_1_5;
		higher_limit = n_pixels * (1 - HISTOGRAM_CROP_1_5);
		break;
	default:
		g_assert_not_reached ();
	}

	value_map = g_new (long *, GTH_HISTOGRAM_N_CHANNELS);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		guchar min, max;
		double sum, scale;

		min = 0;
		sum = 0;
		for (v = 0; v < 256; v++) {
			sum += gth_histogram_get_value (histogram, c, v);
			if (sum >= lower_limit) {
				min = v;
				break;
			}
		}

		max = 0;
		sum = 0;
		for (v = 0; v < 256; v++) {
			sum += gth_histogram_get_value (histogram, c, v);
			if (sum <= higher_limit)
				max = v;
		}

		scale = 255.0 / (max - min);

		value_map[c] = g_new (long, 256);
		for (v = 0; v <= min; v++)
			value_map[c][v] = 0;
		for (v = min + 1; v < max; v++)
			value_map[c][v] = (int) round (scale * (v - min));
		for (v = max; v <= 255; v++)
			value_map[c][v] = 255;
	}

	return value_map;
}


static void
adjust_contrast_setup (EqualizeData    *equalize_data,
		       cairo_surface_t *source)
{
	GthHistogram *histogram;

	histogram = gth_histogram_new ();
	gth_histogram_calculate_for_image (histogram, source);
	switch (equalize_data->method) {
	case METHOD_STRETCH:
	case METHOD_STRETCH_0_5:
	case METHOD_STRETCH_1_5:
		equalize_data->value_map = get_value_map_for_stretch (histogram, equalize_data->method);
		break;
	case METHOD_EQUALIZE_LINEAR:
	case METHOD_EQUALIZE_SQUARE_ROOT:
		equalize_data->value_map = get_value_map_for_equalize (histogram, equalize_data->method);
		break;
	}

	g_object_unref (histogram);
}


static inline guchar
adjust_contrast_func (EqualizeData *equalize_data,
		      int           n_channel,
		      guchar        value)
{
	return (guchar) equalize_data->value_map[n_channel][value];
}


static gpointer
adjust_contrast_exec (GthAsyncTask *task,
		      gpointer      user_data)
{
	EqualizeData    *equalize_data = user_data;
	cairo_surface_t *source;
	cairo_format_t   format;
	int              width;
	int              height;
	int              source_stride;
	cairo_surface_t *destination;
	int              destination_stride;
	unsigned char   *p_source_line;
	unsigned char   *p_destination_line;
	unsigned char   *p_source;
	unsigned char   *p_destination;
	gboolean         cancelled;
	double           progress;
	int              x, y, temp;
	unsigned char    red, green, blue, alpha;

	/* initialize the extra data */

	source = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	adjust_contrast_setup (equalize_data, source);

	/* convert the image */

	format = cairo_image_surface_get_format (source);
	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	source_stride = cairo_image_surface_get_stride (source);

	destination = cairo_image_surface_create (format, width, height);
	destination_stride = cairo_image_surface_get_stride (destination);
	p_source_line = _cairo_image_surface_flush_and_get_data (source);
	p_destination_line = _cairo_image_surface_flush_and_get_data (destination);
	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled) {
			cairo_surface_destroy (destination);
			cairo_surface_destroy (source);
			return NULL;
		}

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		p_destination = p_destination_line;
		for (x = 0; x < width; x++) {
			CAIRO_GET_RGBA (p_source, red, green, blue, alpha);
			red   = adjust_contrast_func (equalize_data, GTH_HISTOGRAM_CHANNEL_RED, red);
			green = adjust_contrast_func (equalize_data, GTH_HISTOGRAM_CHANNEL_GREEN, green);
			blue  = adjust_contrast_func (equalize_data, GTH_HISTOGRAM_CHANNEL_BLUE, blue);
			CAIRO_SET_RGBA (p_destination, red, green, blue, alpha);

			p_source += 4;
			p_destination += 4;
		}
		p_source_line += source_stride;
		p_destination_line += destination_stride;
	}

	cairo_surface_mark_dirty (destination);
	gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), destination);

	cairo_surface_destroy (destination);
	cairo_surface_destroy (source);

	return NULL;
}


static void
adjust_contrast_data_destroy (gpointer user_data)
{
	EqualizeData *equalize_data = user_data;

	if (equalize_data->value_map != NULL)
		value_map_free (equalize_data->value_map);
	g_free (equalize_data);
}


static void apply_changes (GthFileToolAdjustContrast *self);


static void
image_task_completed_cb (GthTask  *task,
			 GError   *error,
			 gpointer  user_data)
{
	GthFileToolAdjustContrast *self = user_data;
	GthImage		  *destination_image;

	self->priv->image_task = NULL;

	if (self->priv->closing) {
		g_object_unref (task);
		gth_image_viewer_page_tool_reset_image (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
		return;
	}

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			apply_changes (self);
		g_object_unref (task);
		return;
	}

	destination_image = gth_image_task_get_destination (GTH_IMAGE_TASK (task));
	if (destination_image == NULL) {
		g_object_unref (task);
		return;
	}

	cairo_surface_destroy (self->priv->destination);
	self->priv->destination = gth_image_get_cairo_surface (destination_image);
	self->priv->last_applied_method = self->priv->method;

	if (self->priv->apply_to_original) {
		if (self->priv->destination != NULL) {
			GtkWidget     *window;
			GthViewerPage *viewer_page;

			window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
			viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
			gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->destination, TRUE);
		}

		gth_file_tool_hide_options (GTH_FILE_TOOL (self));
	}
	else {
		if (! self->priv->view_original)
			gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->destination);
	}

	g_object_unref (task);
}


static GthTask *
get_image_task_for_method (Method method)
{
	EqualizeData *equalize_data;

	equalize_data = g_new (EqualizeData, 1);
	equalize_data->method = method;
	equalize_data->value_map = NULL;

	return gth_image_task_new (_("Applying changes"),
				   NULL,
				   adjust_contrast_exec,
				   NULL,
				   equalize_data,
				   adjust_contrast_data_destroy);
}


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolAdjustContrast *self = user_data;
	GtkWidget                 *window;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	if (self->priv->image_task != NULL) {
		gth_task_cancel (self->priv->image_task);
		return FALSE;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));

	self->priv->image_task = get_image_task_for_method (self->priv->method);
	if (self->priv->apply_to_original)
		gth_image_task_set_source_surface (GTH_IMAGE_TASK (self->priv->image_task), gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self)));
	else
		gth_image_task_set_source_surface (GTH_IMAGE_TASK (self->priv->image_task), self->priv->preview);

	g_signal_connect (self->priv->image_task,
			  "completed",
			  G_CALLBACK (image_task_completed_cb),
			  self);
	gth_browser_exec_task (GTH_BROWSER (window), self->priv->image_task, GTH_TASK_FLAGS_DEFAULT);

	return FALSE;
}


static void
apply_changes (GthFileToolAdjustContrast *self)
{
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}
	self->priv->apply_event = g_timeout_add (APPLY_DELAY, apply_cb, self);
}


static void
gth_file_tool_adjust_contrast_reset_image (GthImageViewerPageTool *base)
{
	GthFileToolAdjustContrast *self = GTH_FILE_TOOL_ADJUST_CONTRAST (base);

	if (self->priv->image_task != NULL) {
		self->priv->closing = TRUE;
		return;
	}

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	gth_image_viewer_page_reset (GTH_IMAGE_VIEWER_PAGE (gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self))));
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
filter_grid_activated_cb (GthFilterGrid	*filter_grid,
			  int            filter_id,
			  gpointer       user_data)
{
	GthFileToolAdjustContrast *self = user_data;

	self->priv->view_original = (filter_id == GTH_FILTER_GRID_NO_FILTER);
	if (self->priv->view_original) {
		gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
	}
	else if (filter_id == self->priv->last_applied_method) {
		gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->destination);
	}
	else {
		self->priv->method = filter_id;
		apply_changes (self);
	}
}


static GtkWidget *
gth_file_tool_adjust_contrast_get_options (GthFileTool *base)
{
	GthFileToolAdjustContrast *self;
	GtkWidget                 *window;
	GthViewerPage             *viewer_page;
	GtkWidget                 *viewer;
	cairo_surface_t           *source;
	GtkWidget                 *options;
	int                        width, height;
	GtkAllocation              allocation;
	GtkWidget                 *filter_grid;

	self = (GthFileToolAdjustContrast *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	_cairo_clear_surface (&self->priv->preview);
	_cairo_clear_surface (&self->priv->destination);

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	source = gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (source == NULL)
		return NULL;

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	gtk_widget_get_allocation (GTK_WIDGET (viewer), &allocation);
	if (scale_keeping_ratio (&width, &height, PREVIEW_SIZE * allocation.width, PREVIEW_SIZE * allocation.height, FALSE))
		self->priv->preview = _cairo_image_surface_scale_fast (source, width, height);
	else
		self->priv->preview = cairo_surface_reference (source);

	self->priv->destination = cairo_surface_reference (self->priv->preview);
	self->priv->apply_to_original = FALSE;
	self->priv->closing = FALSE;

	self->priv->builder = _gtk_builder_new_from_file ("adjust-contrast-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	filter_grid = gth_filter_grid_new ();
	gth_filter_grid_add_filter (GTH_FILTER_GRID (filter_grid),
				    METHOD_STRETCH_0_5,
				    get_image_task_for_method (METHOD_STRETCH_0_5),
				    _("Stretch"),
				    /* xgettext:no-c-format */
				    _("Stretch the histogram after trimming 0.5% from both ends"));
	gth_filter_grid_add_filter (GTH_FILTER_GRID (filter_grid),
				    METHOD_EQUALIZE_SQUARE_ROOT,
				    get_image_task_for_method (METHOD_EQUALIZE_SQUARE_ROOT),
				    _("Equalize"),
				    _("Equalize the histogram using the square root function"));
	gth_filter_grid_add_filter (GTH_FILTER_GRID (filter_grid),
				    METHOD_EQUALIZE_LINEAR,
				    get_image_task_for_method (METHOD_EQUALIZE_LINEAR),
				    _("Uniform"),
				    _("Equalize the histogram using the linear function"));

	g_signal_connect (filter_grid,
			  "activated",
			  G_CALLBACK (filter_grid_activated_cb),
			  self);

	gtk_widget_show (filter_grid);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("filter_grid_box")), filter_grid, TRUE, FALSE, 0);

	self->priv->preview_tool = gth_preview_tool_new ();
	gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->preview_tool);
	gth_filter_grid_activate (GTH_FILTER_GRID (filter_grid), METHOD_STRETCH_0_5);
	gth_filter_grid_generate_previews (GTH_FILTER_GRID (filter_grid), source);

	return options;
}


static void
gth_file_tool_adjust_contrast_destroy_options (GthFileTool *base)
{
	GthFileToolAdjustContrast *self;
	GtkWidget                 *window;
	GthViewerPage             *viewer_page;

	self = (GthFileToolAdjustContrast *) base;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_reset_viewer_tool (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_viewer_page_update_sensitivity (viewer_page);

	_g_clear_object (&self->priv->builder);

	_cairo_clear_surface (&self->priv->preview);
	_cairo_clear_surface (&self->priv->destination);
	self->priv->method = GTH_FILTER_GRID_NO_FILTER;
	self->priv->last_applied_method = GTH_FILTER_GRID_NO_FILTER;
	self->priv->view_original = TRUE;
}


static void
gth_file_tool_adjust_contrast_apply_options (GthFileTool *base)
{
	GthFileToolAdjustContrast *self;

	self = (GthFileToolAdjustContrast *) base;
	if (! self->priv->view_original) {
		self->priv->apply_to_original = TRUE;
		apply_changes (self);
	}
}


static void
gth_file_tool_adjust_contrast_finalize (GObject *object)
{
	GthFileToolAdjustContrast *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_ADJUST_CONTRAST (object));

	self = (GthFileToolAdjustContrast *) object;

	_g_clear_object (&self->priv->builder);
	_cairo_clear_surface (&self->priv->preview);
	_cairo_clear_surface (&self->priv->destination);

	G_OBJECT_CLASS (gth_file_tool_adjust_contrast_parent_class)->finalize (object);
}


static void
gth_file_tool_adjust_contrast_class_init (GthFileToolAdjustContrastClass *klass)
{
	GObjectClass                *gobject_class;
	GthFileToolClass            *file_tool_class;
	GthImageViewerPageToolClass *image_viewer_page_tool_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_adjust_contrast_finalize;

	file_tool_class = GTH_FILE_TOOL_CLASS (klass);
	file_tool_class->get_options = gth_file_tool_adjust_contrast_get_options;
	file_tool_class->destroy_options = gth_file_tool_adjust_contrast_destroy_options;
	file_tool_class->apply_options = gth_file_tool_adjust_contrast_apply_options;

	image_viewer_page_tool_class = (GthImageViewerPageToolClass *) klass;
	image_viewer_page_tool_class->reset_image = gth_file_tool_adjust_contrast_reset_image;
}


static void
gth_file_tool_adjust_contrast_init (GthFileToolAdjustContrast *self)
{
	self->priv = gth_file_tool_adjust_contrast_get_instance_private (self);
	self->priv->preview = NULL;
	self->priv->destination = NULL;
	self->priv->builder = NULL;
	self->priv->method = GTH_FILTER_GRID_NO_FILTER;
	self->priv->last_applied_method = GTH_FILTER_GRID_NO_FILTER;
	self->priv->view_original = TRUE;

	gth_file_tool_construct (GTH_FILE_TOOL (self),
				 "image-adjust-contrast-symbolic",
				 _("Adjust Contrast"),
				 GTH_TOOLBOX_SECTION_COLORS);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Automatic contrast adjustment"));
}
