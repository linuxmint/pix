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
#include "gth-file-tool-adjust-colors.h"
#include "gth-preview-tool.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define APPLY_DELAY 150
#define PREVIEW_SIZE 0.9


struct _GthFileToolAdjustColorsPrivate {
	cairo_surface_t    *destination;
	cairo_surface_t    *preview;
	GtkBuilder         *builder;
	GtkAdjustment      *gamma_adj;
	GtkAdjustment      *brightness_adj;
	GtkAdjustment      *contrast_adj;
	GtkAdjustment      *saturation_adj;
	GtkAdjustment      *cyan_red_adj;
	GtkAdjustment      *magenta_green_adj;
	GtkAdjustment      *yellow_blue_adj;
	GtkWidget          *histogram_view;
	GthHistogram       *histogram;
	GthTask            *image_task;
	guint               apply_event;
	GthImageViewerTool *preview_tool;
	gboolean            apply_to_original;
	gboolean            closing;
};


G_DEFINE_TYPE_WITH_CODE (GthFileToolAdjustColors,
			 gth_file_tool_adjust_colors,
			 GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL,
			 G_ADD_PRIVATE (GthFileToolAdjustColors))


typedef struct {
	GthFileToolAdjustColors *self;
	GthViewerPage           *viewer_page;
	double                   gamma;
	double                   brightness;
	double                   contrast;
	double                   saturation;
	double                   color_level[3];
	PixbufCache             *cache;
	double                   midtone_distance[256];
} AdjustData;


static void
adjust_colors_before (GthAsyncTask *task,
		      gpointer      user_data)
{
	AdjustData *adjust_data = user_data;
	int         i;

	adjust_data->cache = pixbuf_cache_new ();
	for (i = 0; i < 256; i++)
		adjust_data->midtone_distance[i] = 0.667 * (1 - SQR (((double) i - 127.0) / 127.0));
}


static guchar
gamma_correction (int    original,
		  double gamma)
{
	double inten;

	inten = (double) original / 255.0;

	if (gamma != 0.0) {
		if (inten >= 0.0)
			inten =  pow ( inten, (1.0 / gamma));
		else
			inten = -pow (-inten, (1.0 / gamma));
	}

	return CLAMP (inten * 255.0, 0, 255);
}


static gpointer
adjust_colors_exec (GthAsyncTask *task,
		    gpointer      user_data)
{
	AdjustData      *adjust_data = user_data;
	cairo_surface_t *source;
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
	int              x, y, temp;
	int              values[4];
	int              channel;
	int              value;
	double           saturation;
	cairo_surface_t *destination;

	if (adjust_data->saturation < 0)
		saturation = tan (adjust_data->saturation * G_PI_2);
	else
		saturation = adjust_data->saturation;

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
		if (cancelled)
			return NULL;

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		p_destination = p_destination_line;
		for (x = 0; x < width; x++) {
			CAIRO_GET_RGBA (p_source, values[0], values[1], values[2], values[3]);

			/* gamma correction / brightness / contrast */

			for (channel = 0; channel < 3; channel++) {
				value = values[channel];

				if (! pixbuf_cache_get (adjust_data->cache, channel + 1, &value)) {
					int tmp;

					if (adjust_data->gamma != 0.0)
						value = gamma_correction (value, adjust_data->gamma);

					if (adjust_data->brightness > 0)
						tmp = interpolate_value (value, 0, adjust_data->brightness);
					else
						tmp = interpolate_value (value, 255, - adjust_data->brightness);
					value = CLAMP (tmp, 0, 255);

					if (adjust_data->contrast < 0)
						tmp = interpolate_value (value, 127, tan (adjust_data->contrast * G_PI_2));
					else
						tmp = interpolate_value (value, 127, adjust_data->contrast);
					value = CLAMP (tmp, 0, 255);

					tmp = value + adjust_data->color_level[channel] * adjust_data->midtone_distance[value];
					value = CLAMP (tmp, 0, 255);

					pixbuf_cache_set (adjust_data->cache, channel + 1, values[channel], value);
				}

				values[channel] = value;
			}

			/* saturation */

			if (adjust_data->saturation != 0.0) {
				guchar min, max, lightness;
				int    tmp;

				max = MAX (MAX (values[0], values[1]), values[2]);
				min = MIN (MIN (values[0], values[1]), values[2]);
				lightness = (max + min) / 2;

				tmp = interpolate_value (values[0], lightness, saturation);
				values[0] = CLAMP (tmp, 0, 255);

				tmp = interpolate_value (values[1], lightness, saturation);
				values[1] = CLAMP (tmp, 0, 255);

				tmp = interpolate_value (values[2], lightness, saturation);
				values[2] = CLAMP (tmp, 0, 255);
			}

			CAIRO_SET_RGBA (p_destination, values[0], values[1], values[2], values[3]);

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
adjust_data_free (gpointer user_data)
{
	AdjustData *adjust_data = user_data;

	pixbuf_cache_free (adjust_data->cache);
	g_object_unref (adjust_data->viewer_page);
	g_free (adjust_data);
}


static void
reset_button_clicked_cb (GtkButton *button,
		  	 gpointer   user_data)
{
	GthFileToolAdjustColors *self = user_data;

	gtk_adjustment_set_value (self->priv->gamma_adj, 0.0);
	gtk_adjustment_set_value (self->priv->brightness_adj, 0.0);
	gtk_adjustment_set_value (self->priv->contrast_adj, 0.0);
	gtk_adjustment_set_value (self->priv->saturation_adj, 0.0);
	gtk_adjustment_set_value (self->priv->cyan_red_adj, 0.0);
	gtk_adjustment_set_value (self->priv->magenta_green_adj, 0.0);
	gtk_adjustment_set_value (self->priv->yellow_blue_adj, 0.0);
}


static void apply_changes (GthFileToolAdjustColors *self);


static void
image_task_completed_cb (GthTask  *task,
			 GError   *error,
			 gpointer  user_data)
{
	GthFileToolAdjustColors *self = user_data;
	GthImage                *destination_image;

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

	if (self->priv->apply_to_original) {
		if (self->priv->destination != NULL) {
			GthViewerPage *viewer_page;

			viewer_page = gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
			gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->destination, TRUE);
		}

		gth_file_tool_hide_options (GTH_FILE_TOOL (self));
	}
	else {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("preview_checkbutton"))))
			gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->destination);
		gth_histogram_calculate_for_image (self->priv->histogram, self->priv->destination);
	}

	g_object_unref (task);
}


static gboolean
apply_cb (gpointer user_data)
{
	GthFileToolAdjustColors *self = user_data;
	GtkWidget               *window;
	AdjustData              *adjust_data;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	if (self->priv->image_task != NULL) {
		gth_task_cancel (self->priv->image_task);
		return FALSE;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));

	adjust_data = g_new0 (AdjustData, 1);
	adjust_data->self = self;
	adjust_data->viewer_page = g_object_ref (gth_browser_get_viewer_page (GTH_BROWSER (window)));
	adjust_data->gamma = pow (10, - (gtk_adjustment_get_value (self->priv->gamma_adj) / 100.0));
	adjust_data->brightness = gtk_adjustment_get_value (self->priv->brightness_adj) / 100.0 * -1.0;
	adjust_data->contrast = gtk_adjustment_get_value (self->priv->contrast_adj) / 100.0 * -1.0;
	adjust_data->saturation = gtk_adjustment_get_value (self->priv->saturation_adj) / 100.0 * -1.0;
	adjust_data->color_level[0] = gtk_adjustment_get_value (self->priv->cyan_red_adj);
	adjust_data->color_level[1] = gtk_adjustment_get_value (self->priv->magenta_green_adj);
	adjust_data->color_level[2] = gtk_adjustment_get_value (self->priv->yellow_blue_adj);

	self->priv->image_task = gth_image_task_new (_("Applying changes"),
						     adjust_colors_before,
						     adjust_colors_exec,
						     NULL,
						     adjust_data,
						     adjust_data_free);
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
apply_changes (GthFileToolAdjustColors *self)
{
	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}
	self->priv->apply_event = g_timeout_add (APPLY_DELAY, apply_cb, self);
}


static void
value_changed_cb (GtkAdjustment *adj,
		  gpointer       user_data)
{
	apply_changes (GTH_FILE_TOOL_ADJUST_COLORS (user_data));
}


static void
preview_checkbutton_toggled_cb (GtkToggleButton *togglebutton,
				gpointer         user_data)
{
	GthFileToolAdjustColors *self = user_data;

	if (gtk_toggle_button_get_active (togglebutton))
		gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->destination);
	else
		gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
}


static GtkWidget *
gth_file_tool_adjust_colors_get_options (GthFileTool *base)
{
	GthFileToolAdjustColors *self;
	GthViewerPage           *viewer_page;
	GtkWidget               *viewer;
	cairo_surface_t         *source;
	GtkWidget               *options;
	int                      width, height;
	GtkAllocation            allocation;

	self = (GthFileToolAdjustColors *) base;

	viewer_page = gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (viewer_page == NULL)
		return NULL;

	_cairo_clear_surface (&self->priv->destination);
	_cairo_clear_surface (&self->priv->preview);

	source = gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (source == NULL)
		return NULL;

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gtk_widget_get_allocation (GTK_WIDGET (viewer), &allocation);
	if (scale_keeping_ratio (&width, &height, PREVIEW_SIZE * allocation.width, PREVIEW_SIZE * allocation.height, FALSE))
		self->priv->preview = _cairo_image_surface_scale_fast (source, width, height);
	else
		self->priv->preview = cairo_surface_reference (source);

	self->priv->destination = cairo_surface_reference (self->priv->preview);
	self->priv->apply_to_original = FALSE;
	self->priv->closing = FALSE;

	self->priv->builder = _gtk_builder_new_from_file ("adjust-colors-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	self->priv->histogram_view = gth_histogram_view_new (self->priv->histogram);
	gtk_widget_show (self->priv->histogram_view);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("histogram_hbox")), self->priv->histogram_view, TRUE, TRUE, 0);

	self->priv->brightness_adj    = gth_color_scale_label_new (GET_WIDGET ("brightness_hbox"),
								   GTK_LABEL (GET_WIDGET ("brightness_label")),
								   GTH_COLOR_SCALE_BLACK_WHITE,
								   0.0, -99.0, 99.0, 1.0, 1.0, "%+.0f");
	self->priv->contrast_adj      = gth_color_scale_label_new (GET_WIDGET ("contrast_hbox"),
								   GTK_LABEL (GET_WIDGET ("contrast_label")),
								   GTH_COLOR_SCALE_GRAY_BLACK,
								   0.0, -99.0, 99.0, 1.0, 1.0, "%+.0f");
	self->priv->gamma_adj         = gth_color_scale_label_new (GET_WIDGET ("gamma_hbox"),
								   GTK_LABEL (GET_WIDGET ("gamma_label")),
								   GTH_COLOR_SCALE_WHITE_BLACK,
								   0.0, -99.0, 99.0, 1.0, 1.0, "%+.0f");
	self->priv->saturation_adj    = gth_color_scale_label_new (GET_WIDGET ("saturation_hbox"),
								   GTK_LABEL (GET_WIDGET ("saturation_label")),
								   GTH_COLOR_SCALE_GRAY_WHITE,
								   0.0, -99.0, 99.0, 1.0, 1.0, "%+.0f");
	self->priv->cyan_red_adj      = gth_color_scale_label_new (GET_WIDGET ("cyan_red_hbox"),
								   GTK_LABEL (GET_WIDGET ("cyan_red_label")),
								   GTH_COLOR_SCALE_CYAN_RED,
								   0.0, -99.0, 99.0, 1.0, 1.0, "%+.0f");
	self->priv->magenta_green_adj = gth_color_scale_label_new (GET_WIDGET ("magenta_green_hbox"),
								   GTK_LABEL (GET_WIDGET ("magenta_green_label")),
								   GTH_COLOR_SCALE_MAGENTA_GREEN,
								   0.0, -99.0, 99.0, 1.0, 1.0, "%+.0f");
	self->priv->yellow_blue_adj   = gth_color_scale_label_new (GET_WIDGET ("yellow_blue_hbox"),
								   GTK_LABEL (GET_WIDGET ("yellow_blue_label")),
								   GTH_COLOR_SCALE_YELLOW_BLUE,
								   0.0, -99.0, 99.0, 1.0, 1.0, "%+.0f");

	g_signal_connect (G_OBJECT (self->priv->brightness_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->contrast_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->gamma_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->saturation_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->cyan_red_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->magenta_green_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->yellow_blue_adj),
			  "value-changed",
			  G_CALLBACK (value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("preview_checkbutton"),
			  "toggled",
			  G_CALLBACK (preview_checkbutton_toggled_cb),
			  self);

	self->priv->preview_tool = gth_preview_tool_new ();
	gth_preview_tool_set_image (GTH_PREVIEW_TOOL (self->priv->preview_tool), self->priv->preview);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->preview_tool);
	gth_histogram_calculate_for_image (self->priv->histogram, self->priv->preview);

	return options;
}


static void
gth_file_tool_adjust_colors_destroy_options (GthFileTool *base)
{
	GthFileToolAdjustColors *self;
	GthViewerPage           *viewer_page;

	self = (GthFileToolAdjustColors *) base;

	if (self->priv->apply_event != 0) {
		g_source_remove (self->priv->apply_event);
		self->priv->apply_event = 0;
	}

	viewer_page = gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	gth_image_viewer_page_reset_viewer_tool (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_viewer_page_update_sensitivity (viewer_page);

	_cairo_clear_surface (&self->priv->preview);
	_cairo_clear_surface (&self->priv->destination);
	_g_clear_object (&self->priv->builder);
}


static void
gth_file_tool_adjust_colors_apply_options (GthFileTool *base)
{
	GthFileToolAdjustColors *self;

	self = (GthFileToolAdjustColors *) base;
	self->priv->apply_to_original = TRUE;
	apply_changes (self);
}


static void
gth_file_tool_adjust_colors_populate_headerbar (GthFileTool *base,
						GthBrowser  *browser)
{
	GthFileToolAdjustColors *self;
	GtkWidget               *button;

	self = (GthFileToolAdjustColors *) base;

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
	GthFileToolAdjustColors *self = (GthFileToolAdjustColors *) base;

	if (self->priv->image_task != NULL) {
		self->priv->closing = TRUE;
		gth_task_cancel (self->priv->image_task);
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
gth_file_tool_adjust_colors_init (GthFileToolAdjustColors *self)
{
	self->priv = gth_file_tool_adjust_colors_get_instance_private (self);
	self->priv->histogram = gth_histogram_new ();
	self->priv->preview = NULL;
	self->priv->destination = NULL;
	self->priv->builder = NULL;
	self->priv->image_task = NULL;

	gth_file_tool_construct (GTH_FILE_TOOL (self), "image-adjust-colors-symbolic", _("Adjust Colors"), GTH_TOOLBOX_SECTION_COLORS);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Change brightness, contrast, saturation and gamma level of the image"));
}


static void
gth_file_tool_adjust_colors_finalize (GObject *object)
{
	GthFileToolAdjustColors *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_ADJUST_COLORS (object));

	self = (GthFileToolAdjustColors *) object;

	cairo_surface_destroy (self->priv->preview);
	cairo_surface_destroy (self->priv->destination);
	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->histogram);

	G_OBJECT_CLASS (gth_file_tool_adjust_colors_parent_class)->finalize (object);
}


static void
gth_file_tool_adjust_colors_class_init (GthFileToolAdjustColorsClass *klass)
{
	GObjectClass                *gobject_class;
	GthFileToolClass            *file_tool_class;
	GthImageViewerPageToolClass *image_viewer_page_tool_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_adjust_colors_finalize;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->get_options = gth_file_tool_adjust_colors_get_options;
	file_tool_class->destroy_options = gth_file_tool_adjust_colors_destroy_options;
	file_tool_class->apply_options = gth_file_tool_adjust_colors_apply_options;
	file_tool_class->populate_headerbar = gth_file_tool_adjust_colors_populate_headerbar;

	image_viewer_page_tool_class = (GthImageViewerPageToolClass *) klass;
	image_viewer_page_tool_class->reset_image = gth_file_tool_sharpen_reset_image;
}
