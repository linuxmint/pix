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
#include "gth-file-tool-sharpen.h"
#include "cairo-blur.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150
#define DEFAULT_RADIUS 2.0
#define DEFAULT_AMOUNT 50.0
#define DEFAULT_THRESHOLD 0.0


struct _GthFileToolSharpenPrivate {
	GtkBuilder      *builder;
	GtkAdjustment   *radius_adj;
	GtkAdjustment   *amount_adj;
	GtkAdjustment   *threshold_adj;
	GtkWidget       *preview;
	guint            apply_event;
	gboolean         show_preview;
	cairo_surface_t *preview_source;
	gboolean         image_centered;
};


G_DEFINE_TYPE_WITH_CODE (GthFileToolSharpen,
			 gth_file_tool_sharpen,
			 GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL,
			 G_ADD_PRIVATE (GthFileToolSharpen))


typedef struct {
	int    radius;
	double amount;
	int    threshold;
} SharpenData;


static SharpenData *
sharpen_data_new (GthFileToolSharpen *self)
{
	SharpenData *sharpen_data;

	sharpen_data = g_new0 (SharpenData, 1);
	sharpen_data->radius = gtk_adjustment_get_value (self->priv->radius_adj);
	sharpen_data->amount = - gtk_adjustment_get_value (self->priv->amount_adj) / 100.0;
	sharpen_data->threshold = gtk_adjustment_get_value (self->priv->threshold_adj);

	return sharpen_data;
}


static void
sharpen_data_free (gpointer user_data)
{
	SharpenData *sharpen_data = user_data;
	g_free (sharpen_data);
}


static gpointer
sharpen_exec (GthAsyncTask *task,
	      gpointer      user_data)
{
	SharpenData     *sharpen_data = user_data;
	cairo_surface_t *source;
	cairo_surface_t *destination;

	source = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	destination = _cairo_image_surface_copy (source);
	_cairo_image_surface_sharpen (destination,
				      sharpen_data->radius,
				      sharpen_data->amount,
				      sharpen_data->threshold,
				      task);
	gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), destination);

	cairo_surface_destroy (destination);
	cairo_surface_destroy (source);

	return NULL;
}


static void
reset_button_clicked_cb (GtkButton          *button,
			 GthFileToolSharpen *self)
{
	gtk_adjustment_set_value (self->priv->radius_adj, DEFAULT_RADIUS);
	gtk_adjustment_set_value (self->priv->amount_adj, DEFAULT_AMOUNT);
	gtk_adjustment_set_value (self->priv->threshold_adj, DEFAULT_THRESHOLD);
}


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolSharpen *self = user_data;
	cairo_surface_t    *source;
	GthImageViewer     *preview;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	source = self->priv->preview_source;
	preview = GTH_IMAGE_VIEWER (self->priv->preview);
	if (self->priv->show_preview) {
		SharpenData     *sharpen_data;
		int              x, y, w ,h;
		cairo_surface_t *destination;
		cairo_surface_t *preview_surface;

		sharpen_data = sharpen_data_new (self);
		gth_image_viewer_get_scroll_offset (preview, &x, &y);
		w = MIN (gtk_adjustment_get_page_size (preview->hadj), cairo_image_surface_get_width (source));
		h = MIN (gtk_adjustment_get_page_size (preview->vadj), cairo_image_surface_get_height (source));

		if ((w < 0) || (h < 0))
			return FALSE;

		destination = _cairo_image_surface_copy (source);
		_cairo_image_surface_copy_metadata (source, destination);

		preview_surface = _cairo_image_surface_copy_subsurface (destination, x, y, w, h);
		if (preview_surface != NULL) {
			cairo_t *cr;

			_cairo_image_surface_sharpen (preview_surface,
						      sharpen_data->radius,
						      sharpen_data->amount,
						      sharpen_data->threshold,
						      NULL);

			cr = cairo_create (destination);
			cairo_set_source_surface (cr, preview_surface, x, y);
			cairo_rectangle (cr, x, y, w, h);
			cairo_fill (cr);
			cairo_destroy (cr);
		}

		gth_image_viewer_set_surface (preview, destination, -1, -1);

		cairo_surface_destroy (preview_surface);
		cairo_surface_destroy (destination);
		sharpen_data_free (sharpen_data);
	}
	else
		gth_image_viewer_set_surface (preview, source, -1, -1);

	return FALSE;
}


static void
value_changed_cb (GtkAdjustment      *adj,
		  GthFileToolSharpen *self)
{
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}
	self->priv->apply_event = g_timeout_add (APPLY_DELAY, apply_cb, self);
}


static void
preview_checkbutton_toggled_cb (GtkToggleButton    *toggle_button,
				GthFileToolSharpen *self)
{
	self->priv->show_preview = gtk_toggle_button_get_active (toggle_button);
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}
	apply_cb (self);
}


static void
preview_size_allocate_cb (GtkWidget    *widget,
	                  GdkRectangle *allocation,
	                  gpointer      user_data)
{
	GthFileToolSharpen *self = user_data;

	if (self->priv->image_centered)
		return;

	gth_image_viewer_scroll_to_center (GTH_IMAGE_VIEWER (self->priv->preview));
	self->priv->image_centered = TRUE;
}


static GtkWidget *
gth_file_tool_sharpen_get_options (GthFileTool *base)
{
	GthFileToolSharpen *self;
	cairo_surface_t    *source;
	GtkWidget          *options;
	GtkWidget          *image_navigator;

	self = (GthFileToolSharpen *) base;

	_cairo_clear_surface (&self->priv->preview_source);

	source = gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (source == NULL)
		return NULL;

	self->priv->preview_source = cairo_surface_reference (source);
	self->priv->builder = _gtk_builder_new_from_file ("sharpen-options.ui", "file_tools");
	self->priv->image_centered = FALSE;
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	self->priv->preview = gth_image_viewer_new ();
	gth_image_viewer_set_reset_scrollbars (GTH_IMAGE_VIEWER (self->priv->preview), FALSE);
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->preview), GTH_FIT_NONE);
	gth_image_viewer_set_zoom_change (GTH_IMAGE_VIEWER (self->priv->preview), GTH_ZOOM_CHANGE_KEEP_PREV);
	gth_image_viewer_set_zoom (GTH_IMAGE_VIEWER (self->priv->preview), 1.0);
	gth_image_viewer_set_zoom_enabled (GTH_IMAGE_VIEWER (self->priv->preview), FALSE);
	gth_image_viewer_set_surface (GTH_IMAGE_VIEWER (self->priv->preview), self->priv->preview_source, -1, -1);
	image_navigator = gth_image_navigator_new (GTH_IMAGE_VIEWER (self->priv->preview));
	gtk_widget_show_all (image_navigator);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("preview_hbox")), image_navigator, TRUE, TRUE, 0);

	self->priv->amount_adj = gth_color_scale_label_new (GET_WIDGET ("amount_hbox"),
						            GTK_LABEL (GET_WIDGET ("amount_label")),
						            GTH_COLOR_SCALE_DEFAULT,
						            DEFAULT_AMOUNT, 0.0, 500.0, 1.0, 1.0, "%.0f");
	self->priv->radius_adj = gth_color_scale_label_new (GET_WIDGET ("radius_hbox"),
						            GTK_LABEL (GET_WIDGET ("radius_label")),
						            GTH_COLOR_SCALE_DEFAULT,
						            DEFAULT_RADIUS, 0.0, 10.0, 1.0, 1.0, "%.0f");
	self->priv->threshold_adj = gth_color_scale_label_new (GET_WIDGET ("threshold_hbox"),
							       GTK_LABEL (GET_WIDGET ("threshold_label")),
							       GTH_COLOR_SCALE_DEFAULT,
							       DEFAULT_THRESHOLD, 0.0, 255.0, 1.0, 1.0, "%.0f");

	g_signal_connect (G_OBJECT (self->priv->radius_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->amount_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->threshold_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (GTH_IMAGE_VIEWER (self->priv->preview)->hadj,
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (GTH_IMAGE_VIEWER (self->priv->preview)->vadj,
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("preview_checkbutton"),
			  "clicked",
			  G_CALLBACK (preview_checkbutton_toggled_cb),
			  self);
	g_signal_connect (self->priv->preview,
			  "size-allocate",
			  G_CALLBACK (preview_size_allocate_cb),
			  self);

	return options;
}


static void
gth_file_tool_sharpen_destroy_options (GthFileTool *base)
{
	GthFileToolSharpen *self;

	self = (GthFileToolSharpen *) base;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	_cairo_clear_surface (&self->priv->preview_source);
	_g_clear_object (&self->priv->builder);
}


static void
gth_file_tool_sharpen_apply_options (GthFileTool *base)
{
	GthFileToolSharpen *self;
	GthViewerPage      *viewer_page;
	SharpenData        *sharpen_data;
	GthTask            *task;

	self = (GthFileToolSharpen *) base;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	viewer_page = gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (viewer_page == NULL)
		return;

	sharpen_data = sharpen_data_new (self);
	task = gth_image_viewer_task_new (GTH_IMAGE_VIEWER_PAGE (viewer_page),
					  _("Sharpening image"),
					  NULL,
					  sharpen_exec,
					  NULL,
					  sharpen_data,
					  sharpen_data_free);
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (gth_image_viewer_task_set_destination),
			  NULL);
	gth_browser_exec_task (GTH_BROWSER (gth_file_tool_get_window (GTH_FILE_TOOL (self))), task, GTH_TASK_FLAGS_DEFAULT);

	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
gth_file_tool_sharpen_populate_headerbar (GthFileTool *base,
					  GthBrowser  *browser)
{
	GthFileToolSharpen *self;
	GtkWidget          *button;

	self = (GthFileToolSharpen *) base;

	/* reset button */

	button = gth_browser_add_header_bar_button (browser,
						    GTH_BROWSER_HEADER_SECTION_EDITOR_COMMANDS,
						    "edit-undo-symbolic",
						    _("Reset"),
						    NULL,
						    NULL);
	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (reset_button_clicked_cb),
			  self);
}


static void
gth_file_tool_sharpen_reset_image (GthImageViewerPageTool *base)
{
	GthFileToolSharpen *self = (GthFileToolSharpen *) base;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	gth_image_viewer_page_reset (GTH_IMAGE_VIEWER_PAGE (gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self))));
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
gth_file_tool_sharpen_finalize (GObject *object)
{
	GthFileToolSharpen *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_SHARPEN (object));

	self = (GthFileToolSharpen *) object;
	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (gth_file_tool_sharpen_parent_class)->finalize (object);
}


static void
gth_file_tool_sharpen_class_init (GthFileToolSharpenClass *klass)
{
	GObjectClass                *gobject_class;
	GthFileToolClass            *file_tool_class;
	GthImageViewerPageToolClass *image_viewer_page_tool_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_sharpen_finalize;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->get_options = gth_file_tool_sharpen_get_options;
	file_tool_class->destroy_options = gth_file_tool_sharpen_destroy_options;
	file_tool_class->apply_options = gth_file_tool_sharpen_apply_options;
	file_tool_class->populate_headerbar = gth_file_tool_sharpen_populate_headerbar;

	image_viewer_page_tool_class = (GthImageViewerPageToolClass *) klass;
	image_viewer_page_tool_class->reset_image = gth_file_tool_sharpen_reset_image;
}


static void
gth_file_tool_sharpen_init (GthFileToolSharpen *self)
{
	self->priv = gth_file_tool_sharpen_get_instance_private (self);
	self->priv->builder = NULL;
	self->priv->show_preview = TRUE;
	self->priv->preview_source = NULL;

	gth_file_tool_construct (GTH_FILE_TOOL (self),
				 "image-sharpen-symbolic",
				 _("Enhance Focus"),
				 GTH_TOOLBOX_SECTION_COLORS);
	gth_file_tool_set_zoomable (GTH_FILE_TOOL (self), TRUE);
}
