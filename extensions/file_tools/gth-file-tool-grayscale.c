/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include <extensions/image_viewer/image-viewer.h>
#include "gth-file-tool-grayscale.h"
#include "gth-preview-tool.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150
#define PREVIEW_SIZE 0.9


typedef enum {
	METHOD_BRIGHTNESS,
	METHOD_SATURATION,
	METHOD_AVARAGE
} Method;


struct _GthFileToolGrayscalePrivate {
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


G_DEFINE_TYPE_WITH_CODE (GthFileToolGrayscale,
			 gth_file_tool_grayscale,
			 GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL,
			 G_ADD_PRIVATE (GthFileToolGrayscale))


typedef struct {
	Method method;
} GrayscaleData;


static void
grayscale_data_free (gpointer user_data)
{
	GrayscaleData *grayscale_data = user_data;
	g_free (grayscale_data);
}


static gpointer
grayscale_exec (GthAsyncTask *task,
	        gpointer      user_data)
{
	GrayscaleData   *grayscale_data = user_data;
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
	unsigned char    min, max, value;

	source = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
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

			switch (grayscale_data->method) {
			case METHOD_BRIGHTNESS:
				value = (0.2125 * red + 0.7154 * green + 0.072 * blue);
				break;

			case METHOD_SATURATION:
				max = MAX (MAX (red, green), blue);
				min = MIN (MIN (red, green), blue);
				value = (max + min) / 2;
				break;

			case METHOD_AVARAGE:
				value = (0.3333 * red + 0.3333 * green + 0.3333 * blue);
				break;

			default:
				g_assert_not_reached ();
			}

			CAIRO_SET_RGBA (p_destination,
					value,
					value,
					value,
					alpha);

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


static void apply_changes (GthFileToolGrayscale *self);


static void
image_task_completed_cb (GthTask  *task,
			 GError   *error,
			 gpointer  user_data)
{
	GthFileToolGrayscale *self = user_data;
	GthImage             *destination_image;

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
	GrayscaleData *grayscale_data;

	grayscale_data = g_new0 (GrayscaleData, 1);
	grayscale_data->method = method;

	return gth_image_task_new (_("Applying changes"),
				   NULL,
				   grayscale_exec,
				   NULL,
				   grayscale_data,
				   grayscale_data_free);
}


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolGrayscale *self = user_data;
	GtkWidget            *window;

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
apply_changes (GthFileToolGrayscale *self)
{
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}
	self->priv->apply_event = g_timeout_add (APPLY_DELAY, apply_cb, self);
}


static void
gth_file_tool_grayscale_reset_image (GthImageViewerPageTool *base)
{
	GthFileToolGrayscale *self = GTH_FILE_TOOL_GRAYSCALE (base);

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
	GthFileToolGrayscale *self = user_data;

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
gth_file_tool_grayscale_get_options (GthFileTool *base)
{
	GthFileToolGrayscale *self;
	GtkWidget            *window;
	GthViewerPage        *viewer_page;
	GtkWidget            *viewer;
	cairo_surface_t      *source;
	GtkWidget            *options;
	int                   width, height;
	GtkAllocation         allocation;
	GtkWidget            *filter_grid;

	self = (GthFileToolGrayscale *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	cairo_surface_destroy (self->priv->destination);
	cairo_surface_destroy (self->priv->preview);

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

	self->priv->builder = _gtk_builder_new_from_file ("grayscale-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	filter_grid = gth_filter_grid_new ();
	gth_filter_grid_add_filter (GTH_FILTER_GRID (filter_grid),
				    METHOD_BRIGHTNESS,
				    get_image_task_for_method (METHOD_BRIGHTNESS),
				    _("_Brightness"),
				    NULL);
	gth_filter_grid_add_filter (GTH_FILTER_GRID (filter_grid),
				    METHOD_SATURATION,
				    get_image_task_for_method (METHOD_SATURATION),
				    _("_Saturation"),
				    NULL);
	gth_filter_grid_add_filter (GTH_FILTER_GRID (filter_grid),
				    METHOD_AVARAGE,
				    get_image_task_for_method (METHOD_AVARAGE),
				    _("_Average"),
				    NULL);

	g_signal_connect (filter_grid,
			  "activated",
			  G_CALLBACK (filter_grid_activated_cb),
			  self);

	gtk_widget_show (filter_grid);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("filter_grid_box")), filter_grid, TRUE, FALSE, 0);

	self->priv->preview_tool = gth_preview_tool_new ();
	gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->preview_tool);
	gth_filter_grid_activate (GTH_FILTER_GRID (filter_grid), METHOD_BRIGHTNESS);
	gth_filter_grid_generate_previews (GTH_FILTER_GRID (filter_grid), source);

	return options;
}


static void
gth_file_tool_grayscale_destroy_options (GthFileTool *base)
{
	GthFileToolGrayscale *self;
	GtkWidget            *window;
	GthViewerPage        *viewer_page;

	self = (GthFileToolGrayscale *) base;

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
gth_file_tool_grayscale_apply_options(GthFileTool *base)
{
	GthFileToolGrayscale *self;

	self = (GthFileToolGrayscale *) base;

	if (! self->priv->view_original) {
		self->priv->apply_to_original = TRUE;
		apply_changes (self);
	}
}


static void
gth_file_tool_grayscale_finalize (GObject *object)
{
	GthFileToolGrayscale *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_GRAYSCALE (object));

	self = (GthFileToolGrayscale *) object;

	_g_clear_object (&self->priv->builder);
	_cairo_clear_surface (&self->priv->preview);
	_cairo_clear_surface (&self->priv->destination);

	G_OBJECT_CLASS (gth_file_tool_grayscale_parent_class)->finalize (object);
}


static void
gth_file_tool_grayscale_class_init (GthFileToolGrayscaleClass *klass)
{
	GObjectClass                *gobject_class;
	GthFileToolClass            *file_tool_class;
	GthImageViewerPageToolClass *image_viewer_page_tool_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_grayscale_finalize;

	file_tool_class = GTH_FILE_TOOL_CLASS (klass);
	file_tool_class->get_options = gth_file_tool_grayscale_get_options;
	file_tool_class->destroy_options = gth_file_tool_grayscale_destroy_options;
	file_tool_class->apply_options = gth_file_tool_grayscale_apply_options;

	image_viewer_page_tool_class = (GthImageViewerPageToolClass *) klass;
	image_viewer_page_tool_class->reset_image = gth_file_tool_grayscale_reset_image;
}


static void
gth_file_tool_grayscale_init (GthFileToolGrayscale *self)
{
	self->priv = gth_file_tool_grayscale_get_instance_private (self);
	self->priv->preview = NULL;
	self->priv->destination = NULL;
	self->priv->builder = NULL;
	self->priv->method = GTH_FILTER_GRID_NO_FILTER;
	self->priv->last_applied_method = GTH_FILTER_GRID_NO_FILTER;
	self->priv->view_original = TRUE;

	gth_file_tool_construct (GTH_FILE_TOOL (self),
				 "image-grayscale-symbolic",
				 _("Grayscale"),
				 GTH_TOOLBOX_SECTION_COLORS);
}
