/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include <extensions/image_viewer/preferences.h>
#include "gth-file-tool-resize.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define IMAGE_RATIO_POSITION 2
#define SCREEN_RATIO_POSITION 3
#define PIXELS_UNIT_POSITION 0


G_DEFINE_TYPE (GthFileToolResize, gth_file_tool_resize, GTH_TYPE_FILE_TOOL)


struct _GthFileToolResizePrivate {
	GSettings       *settings;
	cairo_surface_t *original_image;
	cairo_surface_t *new_image;
	GtkBuilder      *builder;
	GtkWidget       *ratio_combobox;
	int              original_width;
	int              original_height;
	int              screen_width;
	int              screen_height;
	gboolean         fixed_aspect_ratio;
	double           aspect_ratio;
	int              new_width;
	int              new_height;
	gboolean         high_quality;
	GthUnit          unit;
	GthTask         *resize_task;
	gboolean         closing;
	gboolean         final_resize;
	guint            update_size_id;
};


static void
gth_file_tool_resize_update_sensitivity (GthFileTool *base)
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


static void update_pixbuf_size (GthFileToolResize *self);


static void
resize_button_clicked_cb (GtkButton       *button,
			GthFileToolResize *self)
{
	self->priv->final_resize = TRUE;
	update_pixbuf_size (self);
}


static void
update_dimensione_info_label (GthFileToolResize *self,
			      const char        *id,
			      double             x,
			      double             y,
			      gboolean           as_int)
{
	char *s;

	if (as_int)
		s = g_strdup_printf ("%d×%d", (int) x, (int) y);
	else
		s = g_strdup_printf ("%.2f×%.2f", x, y);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET (id)), s);

	g_free (s);
}


static gpointer
resize_task_exec (GthAsyncTask *task,
		  gpointer      user_data)
{
	GthFileToolResize *self = user_data;
	cairo_surface_t   *destination;

	destination = _cairo_image_surface_scale (self->priv->original_image,
						  self->priv->new_width,
						  self->priv->new_height,
						  (self->priv->high_quality ? SCALE_FILTER_BEST : SCALE_FILTER_FAST),
						  task);

	if (destination != NULL) {
		GthImage *destination_image;

		destination_image = gth_image_new_for_surface (destination);
		gth_image_task_set_destination (GTH_IMAGE_TASK (task), destination_image);

		_g_object_unref (destination_image);
		cairo_surface_destroy (destination);
	}

	return NULL;
}


static void gth_file_tool_resize_cancel (GthFileTool *base);


static void
resize_task_completed_cb (GthTask  *task,
			  GError   *error,
			  gpointer  user_data)
{
	GthFileToolResize *self = user_data;
	GthImage          *destination_image;
	GtkWidget         *window;
	GtkWidget         *viewer_page;

	self->priv->resize_task = NULL;

	if (self->priv->closing) {
		g_object_unref (task);
		gth_file_tool_resize_cancel (GTH_FILE_TOOL (self));
		return;
	}

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			update_pixbuf_size (self);
		g_object_unref (task);
		return;
	}

	destination_image = gth_image_task_get_destination (GTH_IMAGE_TASK (task));
	if (destination_image == NULL) {
		g_object_unref (task);
		return;
	}

	_cairo_clear_surface (&self->priv->new_image);
	self->priv->new_image = gth_image_get_cairo_surface (destination_image);

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));

	gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->new_image, FALSE);

	if (self->priv->final_resize) {
		gth_image_history_add_image (gth_image_viewer_page_get_history (GTH_IMAGE_VIEWER_PAGE (viewer_page)),
					     self->priv->new_image,
					     TRUE);
		gth_viewer_page_focus (GTH_VIEWER_PAGE (viewer_page));
		gth_file_tool_hide_options (GTH_FILE_TOOL (self));
	}
	else {
		update_dimensione_info_label (self,
					      "new_dimensions_label",
					      self->priv->new_width,
					      self->priv->new_height,
					      TRUE);
		update_dimensione_info_label (self,
					      "scale_factor_label",
					      (double) self->priv->new_width / self->priv->original_width,
					      (double) self->priv->new_height / self->priv->original_height,
					      FALSE);
	}

	g_object_unref (task);
}


static gboolean
update_pixbuf_size_cb (gpointer user_data)
{
	GthFileToolResize *self = user_data;

	self->priv->update_size_id = 0;

	if (self->priv->resize_task != NULL) {
		gth_task_cancel (self->priv->resize_task);
		return FALSE;
	}

	self->priv->resize_task = gth_image_task_new (_("Resizing images"),
						      NULL,
						      resize_task_exec,
						      NULL,
						      self,
						      NULL);
	g_signal_connect (self->priv->resize_task,
			  "completed",
			  G_CALLBACK (resize_task_completed_cb),
			  self);
	gth_browser_exec_task (GTH_BROWSER (gth_file_tool_get_window (GTH_FILE_TOOL (self))),
			       self->priv->resize_task,
			       FALSE);

	return FALSE;
}


static void
update_pixbuf_size (GthFileToolResize *self)
{
	if (self->priv->update_size_id != 0)
		g_source_remove (self->priv->update_size_id);
	self->priv->update_size_id = g_timeout_add (100, update_pixbuf_size_cb, self);
}


static void
selection_width_value_changed_cb (GtkSpinButton     *spin,
				  GthFileToolResize *self)
{
	if (self->priv->unit == GTH_UNIT_PIXELS)
		self->priv->new_width = MAX (gtk_spin_button_get_value_as_int (spin), 1);
	else if (self->priv->unit == GTH_UNIT_PERCENTAGE)
		self->priv->new_width = MAX ((int) round ((gtk_spin_button_get_value (spin) / 100.0) * self->priv->original_width), 1);

	if (self->priv->fixed_aspect_ratio) {
		g_signal_handlers_block_by_data (GET_WIDGET ("resize_height_spinbutton"), self);
		self->priv->new_height = MAX ((int) round ((double) self->priv->new_width / self->priv->aspect_ratio), 1);
		if (self->priv->unit == GTH_UNIT_PIXELS)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), self->priv->new_height);
		else if (self->priv->unit == GTH_UNIT_PERCENTAGE)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), ((double) self->priv->new_height) / self->priv->original_height * 100.0);
		g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_height_spinbutton"), self);
	}

	update_pixbuf_size (self);
}


static void
selection_height_value_changed_cb (GtkSpinButton     *spin,
				   GthFileToolResize *self)
{
	if (self->priv->unit == GTH_UNIT_PIXELS)
		self->priv->new_height = MAX (gtk_spin_button_get_value_as_int (spin), 1);
	else if (self->priv->unit == GTH_UNIT_PERCENTAGE)
		self->priv->new_height = MAX ((int) round ((gtk_spin_button_get_value (spin) / 100.0) * self->priv->original_height), 1);

	if (self->priv->fixed_aspect_ratio) {
		g_signal_handlers_block_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
		self->priv->new_width = MAX ((int) round ((double) self->priv->new_height * self->priv->aspect_ratio), 1);
		if (self->priv->unit == GTH_UNIT_PIXELS)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self->priv->new_width);
		else if (self->priv->unit == GTH_UNIT_PERCENTAGE)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), ((double) self->priv->new_width) / self->priv->original_width * 100.0);
		g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	}

	update_pixbuf_size (self);
}


static void
high_quality_checkbutton_toggled_cb (GtkToggleButton   *button,
				     GthFileToolResize *self)
{
	self->priv->high_quality = gtk_toggle_button_get_active (button);
	update_pixbuf_size (self);
}


static void
update_size_spin_buttons_from_unit_value (GthFileToolResize *self)
{
	g_signal_handlers_block_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	g_signal_handlers_block_by_data (GET_WIDGET ("resize_height_spinbutton"), self);

	if (self->priv->unit == GTH_UNIT_PERCENTAGE) {
		double p;

		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), 2);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), 2);

		p = ((double) self->priv->new_width) / self->priv->original_width * 100.0;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), p);
		p = ((double) self->priv->new_height) / self->priv->original_height * 100.0;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), p);
	}
	else if (self->priv->unit == GTH_UNIT_PIXELS) {
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), 0);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), 0);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self->priv->new_width);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), self->priv->new_height);
	}

	g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_height_spinbutton"), self);
}


static void
unit_combobox_changed_cb (GtkComboBox       *combobox,
			  GthFileToolResize *self)
{
	self->priv->unit = gtk_combo_box_get_active (combobox);
	update_size_spin_buttons_from_unit_value (self);

	selection_width_value_changed_cb (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self);
}


static void
set_spin_value (GthFileToolResize *self,
		GtkWidget         *spin,
		int                x)
{
	g_signal_handlers_block_by_data (G_OBJECT (spin), self);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), x);
	g_signal_handlers_unblock_by_data (G_OBJECT (spin), self);
}


static void
ratio_combobox_changed_cb (GtkComboBox       *combobox,
			   GthFileToolResize *self)
{
	GtkWidget *ratio_w_spinbutton;
	GtkWidget *ratio_h_spinbutton;
	int        idx;
	int        w, h;
	gboolean   use_ratio;

	ratio_w_spinbutton = GET_WIDGET ("ratio_w_spinbutton");
	ratio_h_spinbutton = GET_WIDGET ("ratio_h_spinbutton");
	w = h = 1;
	use_ratio = TRUE;
	idx = gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->ratio_combobox));

	switch (idx) {
	case GTH_ASPECT_RATIO_NONE:
		use_ratio = FALSE;
		break;
	case GTH_ASPECT_RATIO_SQUARE:
		w = h = 1;
		break;
	case GTH_ASPECT_RATIO_IMAGE:
		w = self->priv->original_width;
		h = self->priv->original_height;
		break;
	case GTH_ASPECT_RATIO_DISPLAY:
		w = self->priv->screen_width;
		h = self->priv->screen_height;
		break;
	case GTH_ASPECT_RATIO_5x4:
		w = 5;
		h = 4;
		break;
	case GTH_ASPECT_RATIO_4x3:
		w = 4;
		h = 3;
		break;
	case GTH_ASPECT_RATIO_7x5:
		w = 7;
		h = 5;
		break;
	case GTH_ASPECT_RATIO_3x2:
		w = 3;
		h = 2;
		break;
	case GTH_ASPECT_RATIO_16x10:
		w = 16;
		h = 10;
		break;
	case GTH_ASPECT_RATIO_16x9:
		w = 16;
		h = 9;
		break;
	case GTH_ASPECT_RATIO_185x100:
		w = 185;
		h = 100;
		break;
	case GTH_ASPECT_RATIO_239x100:
		w = 239;
		h = 100;
		break;
	case GTH_ASPECT_RATIO_CUSTOM:
	default:
		w = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ratio_w_spinbutton));
		h = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ratio_h_spinbutton));
		break;
	}

	gtk_widget_set_sensitive (GET_WIDGET ("custom_ratio_box"), idx == GTH_ASPECT_RATIO_CUSTOM);
	gtk_widget_set_sensitive (GET_WIDGET ("invert_ratio_checkbutton"), use_ratio);
	set_spin_value (self, ratio_w_spinbutton, w);
	set_spin_value (self, ratio_h_spinbutton, h);

	self->priv->fixed_aspect_ratio = use_ratio;
	self->priv->aspect_ratio = (double) w / h;
	selection_width_value_changed_cb (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self);
}


static void
update_ratio (GtkSpinButton     *spin,
	      GthFileToolResize *self,
	      gboolean           swap_x_and_y_to_start)
{
	int w, h;

	self->priv->fixed_aspect_ratio = gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->ratio_combobox)) != GTH_ASPECT_RATIO_NONE;
	w = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_w_spinbutton")));
	h = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_h_spinbutton")));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton"))))
		self->priv->aspect_ratio = (double) h / w;
	else
		self->priv->aspect_ratio = (double) w / h;
	selection_width_value_changed_cb (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self);
}


static void
ratio_value_changed_cb (GtkSpinButton     *spin,
			GthFileToolResize *self)
{
	update_ratio (spin, self, FALSE);
}


static void
invert_ratio_changed_cb (GtkSpinButton     *spin,
			 GthFileToolResize *self)
{
	update_ratio (spin, self, TRUE);
}


static void
set_image_size (GthFileToolResize *self,
		int                w,
		int                h,
		int                ratio)
{
	self->priv->fixed_aspect_ratio = TRUE;
	self->priv->aspect_ratio = (double) w / h;;
	self->priv->new_width = w;
	self->priv->new_height = h;
	self->priv->unit = GTH_UNIT_PIXELS;

	update_size_spin_buttons_from_unit_value (self);

	g_signal_handlers_block_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	g_signal_handlers_block_by_data (GET_WIDGET ("resize_height_spinbutton"), self);
	g_signal_handlers_block_by_data (GET_WIDGET ("unit_combobox"), self);
	g_signal_handlers_block_by_data (self->priv->ratio_combobox, self);
	g_signal_handlers_block_by_data (GET_WIDGET ("invert_ratio_checkbutton"), self);
	g_signal_handlers_block_by_data (GET_WIDGET ("ratio_w_spinbutton"), self);
	g_signal_handlers_block_by_data (GET_WIDGET ("ratio_h_spinbutton"), self);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")), PIXELS_UNIT_POSITION);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->ratio_combobox), ratio);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton")), FALSE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), w);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), h);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_w_spinbutton")), w);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_h_spinbutton")), h);

	g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_height_spinbutton"), self);
	g_signal_handlers_unblock_by_data (GET_WIDGET ("unit_combobox"), self);
	g_signal_handlers_unblock_by_data (self->priv->ratio_combobox, self);
	g_signal_handlers_unblock_by_data (GET_WIDGET ("invert_ratio_checkbutton"), self);
	g_signal_handlers_unblock_by_data (GET_WIDGET ("ratio_w_spinbutton"), self);
	g_signal_handlers_unblock_by_data (GET_WIDGET ("ratio_h_spinbutton"), self);

	update_pixbuf_size (self);
}


static void
image_size_button_clicked_cb (GtkButton         *button,
			      GthFileToolResize *self)
{
	set_image_size (self,
			self->priv->original_width,
			self->priv->original_height,
			IMAGE_RATIO_POSITION);
}


static void
screen_size_button_clicked_cb (GtkButton         *button,
			       GthFileToolResize *self)
{
	set_image_size (self,
			self->priv->screen_width,
			self->priv->screen_height,
			SCREEN_RATIO_POSITION);
}


static GtkWidget *
gth_file_tool_resize_get_options (GthFileTool *base)
{
	GthFileToolResize *self;
	GtkWidget         *window;
	GtkWidget         *viewer_page;
	GtkWidget         *viewer;
	GtkWidget         *options;
	char              *text;

	self = (GthFileToolResize *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	cairo_surface_destroy (self->priv->original_image);

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	self->priv->original_image = cairo_surface_reference (gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer)));
	if (self->priv->original_image == NULL)
		return NULL;

	self->priv->original_width = cairo_image_surface_get_width (self->priv->original_image);
	self->priv->original_height = cairo_image_surface_get_height (self->priv->original_image);
	_gtk_widget_get_screen_size (window, &self->priv->screen_width, &self->priv->screen_height);
	self->priv->new_image = NULL;
	self->priv->new_width = self->priv->original_width;
	self->priv->new_height = self->priv->original_height;
	self->priv->high_quality = g_settings_get_boolean (self->priv->settings, PREF_RESIZE_HIGH_QUALITY);
	self->priv->unit = g_settings_get_enum (self->priv->settings, PREF_RESIZE_UNIT);
	self->priv->builder = _gtk_builder_new_from_file ("resize-options.ui", "file_tools");
	self->priv->final_resize = FALSE;

	update_dimensione_info_label (self,
				      "original_dimensions_label",
				      self->priv->original_width,
				      self->priv->original_height,
				      TRUE);

	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	if (self->priv->unit == GTH_UNIT_PIXELS) {
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), 0);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), 0);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")),
					   g_settings_get_double (self->priv->settings, PREF_RESIZE_WIDTH));
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")),
					   g_settings_get_double (self->priv->settings, PREF_RESIZE_HEIGHT));
	}
	else if (self->priv->unit == GTH_UNIT_PERCENTAGE) {
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), 2);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), 2);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")),
					   g_settings_get_double (self->priv->settings, PREF_RESIZE_WIDTH));
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")),
					   g_settings_get_double (self->priv->settings, PREF_RESIZE_HEIGHT));
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")), self->priv->unit);

	self->priv->ratio_combobox = _gtk_combo_box_new_with_texts (_("None"), _("Square"), NULL);
	text = g_strdup_printf (_("%d x %d (Image)"), self->priv->original_width, self->priv->original_height);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("image_size_label")), text);
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->ratio_combobox), text);
	g_free (text);
	text = g_strdup_printf (_("%d x %d (Screen)"), self->priv->screen_width, self->priv->screen_height);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("screen_size_label")), text);
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->ratio_combobox), text);
	g_free (text);
	_gtk_combo_box_append_texts (GTK_COMBO_BOX_TEXT (self->priv->ratio_combobox),
				     _("5:4"),
				     _("4:3 (DVD, Book)"),
				     _("7:5"),
				     _("3:2 (Postcard)"),
				     _("16:10"),
				     _("16:9 (DVD)"),
				     _("1.85:1"),
				     _("2.39:1"),
				     _("Custom"),
				     NULL);
	gtk_widget_show (self->priv->ratio_combobox);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("ratio_combobox_box")), self->priv->ratio_combobox, TRUE, TRUE, 0);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("high_quality_checkbutton")),
				      self->priv->high_quality);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton")),
				      g_settings_get_boolean (self->priv->settings, PREF_RESIZE_ASPECT_RATIO_INVERT));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_w_spinbutton")),
				   MAX (g_settings_get_int (self->priv->settings, PREF_RESIZE_ASPECT_RATIO_WIDTH), 1));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_h_spinbutton")),
				   MAX (g_settings_get_int (self->priv->settings, PREF_RESIZE_ASPECT_RATIO_HEIGHT), 1));

	g_signal_connect (GET_WIDGET ("resize_button"),
			  "clicked",
			  G_CALLBACK (resize_button_clicked_cb),
			  self);
	g_signal_connect_swapped (GET_WIDGET ("cancel_button"),
				  "clicked",
				  G_CALLBACK (gth_file_tool_cancel),
				  self);
	g_signal_connect (GET_WIDGET ("resize_width_spinbutton"),
			  "value-changed",
			  G_CALLBACK (selection_width_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("resize_height_spinbutton"),
			  "value-changed",
			  G_CALLBACK (selection_height_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("high_quality_checkbutton"),
			  "toggled",
			  G_CALLBACK (high_quality_checkbutton_toggled_cb),
			  self);
	g_signal_connect (GET_WIDGET ("unit_combobox"),
			  "changed",
			  G_CALLBACK (unit_combobox_changed_cb),
			  self);
	g_signal_connect (self->priv->ratio_combobox,
			  "changed",
			  G_CALLBACK (ratio_combobox_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("ratio_w_spinbutton"),
			  "value_changed",
			  G_CALLBACK (ratio_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("ratio_h_spinbutton"),
			  "value_changed",
			  G_CALLBACK (ratio_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("invert_ratio_checkbutton"),
			  "toggled",
			  G_CALLBACK (invert_ratio_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("image_size_button"),
			  "clicked",
			  G_CALLBACK (image_size_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("screen_size_button"),
			  "clicked",
			  G_CALLBACK (screen_size_button_clicked_cb),
			  self);

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->ratio_combobox),
				  g_settings_get_enum (self->priv->settings, PREF_RESIZE_ASPECT_RATIO));

	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (viewer), GTH_ZOOM_QUALITY_LOW);

	return options;
}


static void
gth_file_tool_resize_destroy_options (GthFileTool *base)
{
	GthFileToolResize *self;
	GtkWidget         *window;
	GtkWidget         *viewer_page;
	GtkWidget         *viewer;
	GSettings         *viewer_settings;

	self = (GthFileToolResize *) base;

	if (self->priv->update_size_id != 0) {
		g_source_remove (self->priv->update_size_id);
		self->priv->update_size_id = 0;
	}

	if (self->priv->builder != NULL) {
		int unit;

		/* save the dialog options */

		unit = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")));
		g_settings_set_enum (self->priv->settings, PREF_RESIZE_UNIT, unit);
		g_settings_set_double (self->priv->settings, PREF_RESIZE_WIDTH, (float) gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton"))));
		g_settings_set_double (self->priv->settings, PREF_RESIZE_HEIGHT, (float) gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton"))));
		g_settings_set_int (self->priv->settings, PREF_RESIZE_ASPECT_RATIO_WIDTH, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_w_spinbutton"))));
		g_settings_set_int (self->priv->settings, PREF_RESIZE_ASPECT_RATIO_HEIGHT, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_h_spinbutton"))));
		g_settings_set_enum (self->priv->settings, PREF_RESIZE_ASPECT_RATIO, gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->ratio_combobox)));
		g_settings_set_boolean (self->priv->settings, PREF_RESIZE_ASPECT_RATIO_INVERT, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton"))));
		g_settings_set_boolean (self->priv->settings, PREF_RESIZE_HIGH_QUALITY, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("high_quality_checkbutton"))));

		/* destroy the options data */

		_cairo_clear_surface (&self->priv->new_image);
		_cairo_clear_surface (&self->priv->original_image);
		_g_clear_object (&self->priv->builder);
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), NULL);

	/* restore the zoom quality */

	viewer_settings = g_settings_new (GTHUMB_IMAGE_VIEWER_SCHEMA);
	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (viewer),
					   g_settings_get_enum (viewer_settings, PREF_IMAGE_VIEWER_ZOOM_QUALITY));

	g_object_unref (viewer_settings);
}


static void
gth_file_tool_resize_activate (GthFileTool *base)
{
	gth_file_tool_show_options (base);
}


static void
gth_file_tool_resize_cancel (GthFileTool *base)
{
	GthFileToolResize *self = (GthFileToolResize *) base;
	GtkWidget         *window;
	GtkWidget         *viewer_page;

	if (self->priv->resize_task != NULL) {
		self->priv->closing = TRUE;
		return;
	}

	if (self->priv->update_size_id != 0) {
		g_source_remove (self->priv->update_size_id);
		self->priv->update_size_id = 0;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_reset (GTH_IMAGE_VIEWER_PAGE (viewer_page));
}


static void
gth_file_tool_resize_finalize (GObject *object)
{
	GthFileToolResize *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_RESIZE (object));

	self = (GthFileToolResize *) object;

	cairo_surface_destroy (self->priv->new_image);
	cairo_surface_destroy (self->priv->original_image);
	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->settings);

	/* Chain up */
	G_OBJECT_CLASS (gth_file_tool_resize_parent_class)->finalize (object);
}


static void
gth_file_tool_resize_class_init (GthFileToolResizeClass *klass)
{
	GObjectClass     *gobject_class;
	GthFileToolClass *file_tool_class;

	g_type_class_add_private (klass, sizeof (GthFileToolResizePrivate));

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_resize_finalize;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->update_sensitivity = gth_file_tool_resize_update_sensitivity;
	file_tool_class->activate = gth_file_tool_resize_activate;
	file_tool_class->cancel = gth_file_tool_resize_cancel;
	file_tool_class->get_options = gth_file_tool_resize_get_options;
	file_tool_class->destroy_options = gth_file_tool_resize_destroy_options;
}


static void
gth_file_tool_resize_init (GthFileToolResize *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_TOOL_RESIZE, GthFileToolResizePrivate);
	self->priv->settings = g_settings_new (GTHUMB_RESIZE_SCHEMA);
	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-resize", _("Resize..."), _("Resize"), FALSE);
}


