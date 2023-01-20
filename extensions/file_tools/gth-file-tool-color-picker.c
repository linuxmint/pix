/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2017 Free Software Foundation, Inc.
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
#include <locale.h>
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "gth-file-tool-color-picker.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


struct _GthFileToolColorPickerPrivate {
	GtkBuilder       *builder;
	GthImageSelector *selector;
};


G_DEFINE_TYPE_WITH_CODE (GthFileToolColorPicker,
			 gth_file_tool_color_picker,
			 GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL,
			 G_ADD_PRIVATE (GthFileToolColorPicker))


static void
_gth_file_tool_color_picker_show_color (GthFileToolColorPicker *self,
					int                     x,
					int                     y)
{
	cairo_surface_t *source;
	unsigned char   *p_source;
	int              temp;
	guchar           r, g, b, a;
	double           h, s, l;
	double           r100, g100, b100;
	GdkRGBA          color;
	char            *description;

	source = gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if ((source == NULL) || (x < 0) || (y < 0) || (x >= cairo_image_surface_get_width (source)) || (y >= cairo_image_surface_get_height (source))) {
		gtk_widget_set_sensitive (GET_WIDGET ("color_section"), FALSE);
		return;
	}

	gtk_widget_set_sensitive (GET_WIDGET ("color_section"), TRUE);

	p_source = _cairo_image_surface_flush_and_get_data (source) + (y * cairo_image_surface_get_stride (source)) + (x * 4);
	CAIRO_GET_RGBA (p_source, r, g, b, a);

	color.red   = (double ) r / 255.0;
	color.green = (double ) g / 255.0;
	color.blue  = (double ) b / 255.0;
	color.alpha = (double ) a / 255.0;

	rgb_to_hsl (r, g, b, &h, &s, &l);
	if (h < 0)
		h = 255 + h;
	h = round (h / 255.0 * 360.0);
	s = round (s / 255.0 * 100.0);
	l = round (l / 255.0 * 100.0);

	r100 = round (color.red * 100.0);
	g100 = round (color.green * 100.0);
	b100 = round (color.blue * 100.0);

	gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (GET_WIDGET ("color_chooser")), &color);

	setlocale (LC_NUMERIC, "C"); // use always the dot as decimal separator
	if (color.alpha == 1.0) {
		description = g_strdup_printf ("#%02x%02x%02x", r, g, b);
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("hex_color")), description);
		g_free (description);

		description = g_strdup_printf ("rgb(%u, %u, %u)", r, g, b);
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("rgb_color")), description);
		g_free (description);

		description = g_strdup_printf ("rgb(%.0f%%, %.0f%%, %.0f%%)", r100, g100, b100);
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("rgb_100_color")), description);
		g_free (description);

		description = g_strdup_printf ("hsl(%.0f, %.0f%%, %.0f%%)", h, s, l);
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("hsl_color")), description);
		g_free (description);
	}
	else {
		description = g_strdup_printf ("#%02x%02x%02x%02x", r, g, b, a);
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("hex_color")), description);
		g_free (description);

		description = g_strdup_printf ("rgba(%u, %u, %u, %.2f)", r, g, b, color.alpha);
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("rgb_color")), description);
		g_free (description);

		description = g_strdup_printf ("rgba(%.0f%%, %.0f%%, %.0f%%, %.2f)", r100, g100, b100, color.alpha);
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("rgb_100_color")), description);
		g_free (description);

		description = g_strdup_printf ("hsla(%.0f, %.0f%%, %.0f%%, %.2f)", h, s, l, color.alpha);
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("hsl_color")), description);
		g_free (description);
	}
	setlocale (LC_NUMERIC, "");
}


static void
selector_selected_cb (GthImageSelector  *selector,
		      int                x,
		      int                y,
		      gpointer           user_data)
{
	GthFileToolColorPicker *self = user_data;

	_gth_file_tool_color_picker_show_color (self, x, y);
}


static void
selector_motion_notify_cb (GthImageSelector *selector,
		           int               x,
		           int               y,
		           gpointer          user_data)
{
	GthFileToolColorPicker *self = user_data;

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("x_spinbutton")), (double) x);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("y_spinbutton")), (double) y);
}


static void
color_text_icon_press_cb (GtkEntry             *entry,
			  GtkEntryIconPosition  icon_pos,
			  GdkEvent             *event,
			  gpointer              user_data)
{
	if (icon_pos == GTK_ENTRY_ICON_SECONDARY) {
		gtk_clipboard_set_text (gtk_clipboard_get_default (gtk_widget_get_display (GTK_WIDGET (entry))),
					gtk_entry_get_text (GTK_ENTRY (entry)),
					-1);
	}
}


static GtkWidget *
gth_file_tool_color_picker_get_options (GthFileTool *base)
{
	GthFileToolColorPicker *self;
	GtkWidget              *window;
	GthViewerPage          *viewer_page;
	GtkWidget              *viewer;
	GtkWidget              *options;

	self = (GthFileToolColorPicker *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	self->priv->builder = _gtk_builder_new_from_file ("color-picker-options.ui", "file_tools");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	_gth_file_tool_color_picker_show_color (self, -1, -1);

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	self->priv->selector = (GthImageSelector *) gth_image_selector_new (GTH_SELECTOR_TYPE_POINT);
	gth_image_selector_set_mask_visible (self->priv->selector, FALSE);
	g_signal_connect (self->priv->selector,
			  "selected",
			  G_CALLBACK (selector_selected_cb),
			  self);
	g_signal_connect (self->priv->selector,
			  "motion_notify",
			  G_CALLBACK (selector_motion_notify_cb),
			  self);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), (GthImageViewerTool *) self->priv->selector);

	g_signal_connect (GET_WIDGET ("hex_color"),
			  "icon-press",
			  G_CALLBACK (color_text_icon_press_cb),
			  self);
	g_signal_connect (GET_WIDGET ("rgb_color"),
			  "icon-press",
			  G_CALLBACK (color_text_icon_press_cb),
			  self);
	g_signal_connect (GET_WIDGET ("rgb_100_color"),
			  "icon-press",
			  G_CALLBACK (color_text_icon_press_cb),
			  self);
	g_signal_connect (GET_WIDGET ("hsl_color"),
			  "icon-press",
			  G_CALLBACK (color_text_icon_press_cb),
			  self);

	return options;
}


static void
gth_file_tool_color_picker_destroy_options (GthFileTool *base)
{
	GthFileToolColorPicker *self;
	GtkWidget              *window;
	GthViewerPage          *viewer_page;

	self = (GthFileToolColorPicker *) base;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_reset_viewer_tool (GTH_IMAGE_VIEWER_PAGE (viewer_page));

	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->selector);
	self->priv->builder = NULL;
	self->priv->selector = NULL;
}


static void
gth_file_tool_color_picker_apply_options (GthFileTool *base)
{
	gth_file_tool_hide_options (GTH_FILE_TOOL (base));
}


static void
gth_file_tool_color_picker_finalize (GObject *object)
{
	GthFileToolColorPicker *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_COLOR_PICKER (object));

	self = (GthFileToolColorPicker *) object;

	_g_object_unref (self->priv->selector);
	_g_object_unref (self->priv->builder);

	/* Chain up */
	G_OBJECT_CLASS (gth_file_tool_color_picker_parent_class)->finalize (object);
}


static void
gth_file_tool_color_picker_class_init (GthFileToolColorPickerClass *klass)
{
	GObjectClass	 *gobject_class;
	GthFileToolClass *file_tool_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_color_picker_finalize;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->get_options = gth_file_tool_color_picker_get_options;
	file_tool_class->destroy_options = gth_file_tool_color_picker_destroy_options;
	file_tool_class->apply_options = gth_file_tool_color_picker_apply_options;
}


static void
gth_file_tool_color_picker_init (GthFileToolColorPicker *self)
{
	self->priv = gth_file_tool_color_picker_get_instance_private (self);
	self->priv->builder = NULL;
	self->priv->selector = NULL;
	gth_file_tool_construct (GTH_FILE_TOOL (self), "eyedropper-symbolic", _("Color Picker"), GTH_TOOLBOX_SECTION_COLORS);
	gth_file_tool_set_zoomable (GTH_FILE_TOOL (self), TRUE);
	gth_file_tool_set_changes_image (GTH_FILE_TOOL (self), FALSE);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Pick a color from the image"));
}
