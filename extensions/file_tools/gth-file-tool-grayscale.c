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
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "gth-file-tool-grayscale.h"
#include "gth-preview-tool.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150
#define PREVIEW_SIZE 0.9


G_DEFINE_TYPE (GthFileToolGrayscale, gth_file_tool_grayscale, GTH_TYPE_FILE_TOOL)


typedef enum {
	METHOD_BRIGHTNESS,
	METHOD_SATURATION,
	METHOD_AVARAGE
} Method;


struct _GthFileToolGrayscalePrivate {
	cairo_surface_t    *source;
	cairo_surface_t    *destination;
	cairo_surface_t    *preview;
	GtkBuilder         *builder;
	GthTask            *image_task;
	GthImageViewerTool *preview_tool;
	guint               apply_event;
	gboolean            apply_to_original;
	gboolean            closing;
};


typedef struct {
	GthFileToolGrayscale *self;
	GtkWidget            *viewer_page;
	cairo_surface_t      *source;
	Method                method;
} GrayscaleData;


static void
grayscale_data_free (gpointer user_data)
{
	GrayscaleData *grayscale_data = user_data;

	g_object_unref (grayscale_data->viewer_page);
	cairo_surface_destroy (grayscale_data->source);
	g_free (grayscale_data);
}


static gpointer
grayscale_exec (GthAsyncTask *task,
	         gpointer      user_data)
{
	GrayscaleData   *grayscale_data = user_data;
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
	int              x, y;
	unsigned char    red, green, blue, alpha;
	unsigned char    min, max, value;
	cairo_surface_t *destination;
	GthImage        *destination_image;

	format = cairo_image_surface_get_format (grayscale_data->source);
	width = cairo_image_surface_get_width (grayscale_data->source);
	height = cairo_image_surface_get_height (grayscale_data->source);
	source_stride = cairo_image_surface_get_stride (grayscale_data->source);

	destination = cairo_image_surface_create (format, width, height);
	cairo_surface_flush (destination);
	destination_stride = cairo_image_surface_get_stride (destination);
	p_source_line = cairo_image_surface_get_data (grayscale_data->source);
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

	destination_image = gth_image_new_for_surface (destination);
	gth_image_task_set_destination (GTH_IMAGE_TASK (task), destination_image);

	_g_object_unref (destination_image);
	cairo_surface_destroy (destination);

	return NULL;
}


static void apply_changes (GthFileToolGrayscale *self);
static void gth_file_tool_grayscale_cancel (GthFileTool *file_tool);


static void
image_task_completed_cb (GthTask  *task,
			 GError   *error,
			 gpointer  user_data)
{
	GthFileToolGrayscale *self = user_data;
	GthImage              *destination_image;

	self->priv->image_task = NULL;

	if (self->priv->closing) {
		g_object_unref (task);
		gth_file_tool_grayscale_cancel (GTH_FILE_TOOL (self));
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

	if (self->priv->apply_to_original) {
		if (self->priv->destination != NULL) {
			GtkWidget *window;
			GtkWidget *viewer_page;

			window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
			viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
			gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->destination, TRUE);
		}

		gth_file_tool_hide_options (GTH_FILE_TOOL (self));
	}
	else {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("preview_checkbutton"))))
			gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->destination);
	}

	g_object_unref (task);
}


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolGrayscale *self = user_data;
	GtkWidget            *window;
	GrayscaleData        *grayscale_data;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	if (self->priv->image_task != NULL) {
		gth_task_cancel (self->priv->image_task);
		return FALSE;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));

	grayscale_data = g_new0 (GrayscaleData, 1);
	grayscale_data->viewer_page = g_object_ref (gth_browser_get_viewer_page (GTH_BROWSER (window)));
	grayscale_data->source = cairo_surface_reference (self->priv->apply_to_original ? self->priv->source : self->priv->preview);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("brightness_radiobutton"))))
		grayscale_data->method = METHOD_BRIGHTNESS;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("saturation_radiobutton"))))
		grayscale_data->method = METHOD_SATURATION;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("average_radiobutton"))))
		grayscale_data->method = METHOD_AVARAGE;

	self->priv->image_task = gth_image_task_new (_("Applying changes"),
						     NULL,
						     grayscale_exec,
						     NULL,
						     grayscale_data,
						     grayscale_data_free);
	g_signal_connect (self->priv->image_task,
			  "completed",
			  G_CALLBACK (image_task_completed_cb),
			  self);
	gth_browser_exec_task (GTH_BROWSER (window), self->priv->image_task, FALSE);

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
gth_file_tool_grayscale_update_sensitivity (GthFileTool *base)
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
gth_file_tool_grayscale_activate (GthFileTool *base)
{
	gth_file_tool_show_options (base);
}


static void
gth_file_tool_grayscale_cancel (GthFileTool *file_tool)
{
	GthFileToolGrayscale *self = GTH_FILE_TOOL_GRAYSCALE (file_tool);
	GtkWidget             *window;
	GtkWidget             *viewer_page;

	if (self->priv->image_task != NULL) {
		self->priv->closing = TRUE;
		return;
	}

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_reset (GTH_IMAGE_VIEWER_PAGE (viewer_page));
}


static void
ok_button_clicked_cb (GtkButton *button,
		      gpointer   user_data)
{
	GthFileToolGrayscale *self = user_data;

	self->priv->apply_to_original = TRUE;
	apply_changes (self);
}


static void
preview_checkbutton_toggled_cb (GtkToggleButton *togglebutton,
				gpointer         user_data)
{
	GthFileToolGrayscale *self = user_data;

	if (gtk_toggle_button_get_active (togglebutton))
		gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->destination);
	else
		gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
}


static void
method_changed_cb (GtkButton *button,
		   gpointer   user_data)
{
	GthFileToolGrayscale *self = user_data;

	apply_changes (self);
}


static GtkWidget *
gth_file_tool_grayscale_get_options (GthFileTool *base)
{
	GthFileToolGrayscale *self;
	GtkWidget            *window;
	GtkWidget            *viewer_page;
	GtkWidget            *viewer;
	GtkWidget            *options;
	int                   width, height;
	GtkAllocation         allocation;

	self = (GthFileToolGrayscale *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	cairo_surface_destroy (self->priv->source);
	cairo_surface_destroy (self->priv->destination);
	cairo_surface_destroy (self->priv->preview);

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	self->priv->source = cairo_surface_reference (gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer)));
	if (self->priv->source == NULL)
		return NULL;

	width = cairo_image_surface_get_width (self->priv->source);
	height = cairo_image_surface_get_height (self->priv->source);
	gtk_widget_get_allocation (GTK_WIDGET (viewer), &allocation);
	if (scale_keeping_ratio (&width, &height, PREVIEW_SIZE * allocation.width, PREVIEW_SIZE * allocation.height, FALSE))
		self->priv->preview = _cairo_image_surface_scale_bilinear (self->priv->source, width, height);
	else
		self->priv->preview = cairo_surface_reference (self->priv->source);

	self->priv->destination = cairo_surface_reference (self->priv->preview);
	self->priv->apply_to_original = FALSE;
	self->priv->closing = FALSE;

	self->priv->builder = _gtk_builder_new_from_file ("grayscale-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	g_signal_connect (GET_WIDGET ("ok_button"),
			  "clicked",
			  G_CALLBACK (ok_button_clicked_cb),
			  self);
	g_signal_connect_swapped (GET_WIDGET ("cancel_button"),
				  "clicked",
				  G_CALLBACK (gth_file_tool_cancel),
				  self);
	g_signal_connect (GET_WIDGET ("brightness_radiobutton"),
			  "clicked",
			  G_CALLBACK (method_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("saturation_radiobutton"),
			  "clicked",
			  G_CALLBACK (method_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("average_radiobutton"),
			  "clicked",
			  G_CALLBACK (method_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("preview_checkbutton"),
			  "toggled",
			  G_CALLBACK (preview_checkbutton_toggled_cb),
			  self);

	self->priv->preview_tool = gth_preview_tool_new ();
	gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->preview_tool);
	apply_changes (self);

	return options;
}


static void
gth_file_tool_grayscale_destroy_options (GthFileTool *base)
{
	GthFileToolGrayscale *self;
	GtkWidget             *window;
	GtkWidget             *viewer_page;
	GtkWidget             *viewer;

	self = (GthFileToolGrayscale *) base;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), NULL);
	gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (viewer_page));

	_cairo_clear_surface (&self->priv->preview);
	_cairo_clear_surface (&self->priv->source);
	_cairo_clear_surface (&self->priv->destination);
	_g_clear_object (&self->priv->builder);
}


static void
gth_file_tool_grayscale_finalize (GObject *object)
{
	GthFileToolGrayscale *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_GRAYSCALE (object));

	self = (GthFileToolGrayscale *) object;

	cairo_surface_destroy (self->priv->preview);
	cairo_surface_destroy (self->priv->source);
	cairo_surface_destroy (self->priv->destination);
	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (gth_file_tool_grayscale_parent_class)->finalize (object);
}


static void
gth_file_tool_grayscale_class_init (GthFileToolGrayscaleClass *klass)
{
	GObjectClass     *gobject_class;
	GthFileToolClass *file_tool_class;

	g_type_class_add_private (klass, sizeof (GthFileToolGrayscalePrivate));

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_grayscale_finalize;

	file_tool_class = GTH_FILE_TOOL_CLASS (klass);
	file_tool_class->update_sensitivity = gth_file_tool_grayscale_update_sensitivity;
	file_tool_class->activate = gth_file_tool_grayscale_activate;
	file_tool_class->cancel = gth_file_tool_grayscale_cancel;
	file_tool_class->get_options = gth_file_tool_grayscale_get_options;
	file_tool_class->destroy_options = gth_file_tool_grayscale_destroy_options;
}


static void
gth_file_tool_grayscale_init (GthFileToolGrayscale *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_TOOL_GRAYSCALE, GthFileToolGrayscalePrivate);
	self->priv->preview = NULL;
	self->priv->source = NULL;
	self->priv->destination = NULL;
	self->priv->builder = NULL;

	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-grayscale", _("Grayscale..."), _("Grayscale"), FALSE);
}
