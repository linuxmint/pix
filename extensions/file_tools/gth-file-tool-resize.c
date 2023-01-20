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
#include <extensions/image_viewer/image-viewer.h>
#include "gth-file-tool-resize.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define IMAGE_RATIO_POSITION 2
#define SCREEN_RATIO_POSITION 3
#define PIXELS_UNIT_POSITION 0


struct _GthFileToolResizePrivate {
	GSettings       *settings;
	cairo_surface_t *preview;
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
	gboolean         apply_to_original;
	guint            update_size_id;
};


G_DEFINE_TYPE_WITH_CODE (GthFileToolResize,
			 gth_file_tool_resize,
			 GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL,
			 G_ADD_PRIVATE (GthFileToolResize))


static void update_image_size (GthFileToolResize *self);


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


static void
resize_task_completed_cb (GthTask  *task,
			  GError   *error,
			  gpointer  user_data)
{
	GthFileToolResize *self = user_data;
	GtkWidget         *window;
	GthViewerPage     *viewer_page;

	self->priv->resize_task = NULL;

	if (self->priv->closing) {
		g_object_unref (task);
		gth_image_viewer_page_tool_reset_image (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
		return;
	}

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			update_image_size (self);
		g_object_unref (task);
		return;
	}

	_cairo_clear_surface (&self->priv->new_image);
	self->priv->new_image = gth_image_task_get_destination_surface (GTH_IMAGE_TASK (task));
	if (self->priv->new_image == NULL) {
		g_object_unref (task);
		return;
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->new_image, FALSE);

	if (self->priv->apply_to_original) {
		gth_image_history_add_surface (gth_image_viewer_page_get_history (GTH_IMAGE_VIEWER_PAGE (viewer_page)),
					       self->priv->new_image,
					       -1,
					       TRUE);
		gth_viewer_page_focus (viewer_page);
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


static gpointer
resize_task_exec (GthAsyncTask *task,
		  gpointer      user_data)
{
	GthFileToolResize *self = user_data;
	cairo_surface_t   *source;
	cairo_surface_t   *destination;

	source = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	destination = _cairo_image_surface_scale (source,
						  self->priv->new_width,
						  self->priv->new_height,
						  (self->priv->high_quality ? SCALE_FILTER_BEST : SCALE_FILTER_FAST),
						  task);
	if (destination != NULL) {
		_cairo_image_surface_clear_metadata (destination);
		gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), destination);
		cairo_surface_destroy (destination);
	}

	cairo_surface_destroy (source);

	return NULL;
}


static gboolean
update_image_size_cb (gpointer user_data)
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
	if (self->priv->apply_to_original)
		gth_image_task_set_source_surface (GTH_IMAGE_TASK (self->priv->resize_task), gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self)));
	else
		gth_image_task_set_source_surface (GTH_IMAGE_TASK (self->priv->resize_task), self->priv->preview);

	g_signal_connect (self->priv->resize_task,
			  "completed",
			  G_CALLBACK (resize_task_completed_cb),
			  self);
	gth_browser_exec_task (GTH_BROWSER (gth_file_tool_get_window (GTH_FILE_TOOL (self))),
			       self->priv->resize_task,
			       GTH_TASK_FLAGS_DEFAULT);

	return FALSE;
}


static void
update_image_size (GthFileToolResize *self)
{
	if (self->priv->update_size_id != 0)
		g_source_remove (self->priv->update_size_id);
	self->priv->update_size_id = g_timeout_add (100, update_image_size_cb, self);
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
		_g_signal_handlers_block_by_data (GET_WIDGET ("resize_height_spinbutton"), self);
		self->priv->new_height = MAX ((int) round ((double) self->priv->new_width / self->priv->aspect_ratio), 1);
		if (self->priv->unit == GTH_UNIT_PIXELS)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), self->priv->new_height);
		else if (self->priv->unit == GTH_UNIT_PERCENTAGE)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), ((double) self->priv->new_height) / self->priv->original_height * 100.0);
		_g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_height_spinbutton"), self);
	}

	update_image_size (self);
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
		_g_signal_handlers_block_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
		self->priv->new_width = MAX ((int) round ((double) self->priv->new_height * self->priv->aspect_ratio), 1);
		if (self->priv->unit == GTH_UNIT_PIXELS)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self->priv->new_width);
		else if (self->priv->unit == GTH_UNIT_PERCENTAGE)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), ((double) self->priv->new_width) / self->priv->original_width * 100.0);
		_g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	}

	update_image_size (self);
}


static void
high_quality_checkbutton_toggled_cb (GtkToggleButton   *button,
				     GthFileToolResize *self)
{
	self->priv->high_quality = gtk_toggle_button_get_active (button);
	update_image_size (self);
}


static void
update_size_spin_buttons_from_unit_value (GthFileToolResize *self)
{
	_g_signal_handlers_block_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	_g_signal_handlers_block_by_data (GET_WIDGET ("resize_height_spinbutton"), self);

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

	_g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	_g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_height_spinbutton"), self);
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
	_g_signal_handlers_block_by_data (G_OBJECT (spin), self);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), x);
	_g_signal_handlers_unblock_by_data (G_OBJECT (spin), self);
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

	gtk_widget_set_visible (GET_WIDGET ("custom_ratio_box"), idx == GTH_ASPECT_RATIO_CUSTOM);
	gtk_widget_set_sensitive (GET_WIDGET ("invert_ratio_checkbutton"), use_ratio);
	set_spin_value (self, ratio_w_spinbutton, w);
	set_spin_value (self, ratio_h_spinbutton, h);

	self->priv->fixed_aspect_ratio = use_ratio;
	self->priv->aspect_ratio = (double) w / h;
	selection_width_value_changed_cb (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), self);
	if (! use_ratio)
		selection_height_value_changed_cb (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), self);
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

	_g_signal_handlers_block_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	_g_signal_handlers_block_by_data (GET_WIDGET ("resize_height_spinbutton"), self);
	_g_signal_handlers_block_by_data (GET_WIDGET ("unit_combobox"), self);
	_g_signal_handlers_block_by_data (self->priv->ratio_combobox, self);
	_g_signal_handlers_block_by_data (GET_WIDGET ("invert_ratio_checkbutton"), self);
	_g_signal_handlers_block_by_data (GET_WIDGET ("ratio_w_spinbutton"), self);
	_g_signal_handlers_block_by_data (GET_WIDGET ("ratio_h_spinbutton"), self);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")), PIXELS_UNIT_POSITION);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->ratio_combobox), ratio);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton")), FALSE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_width_spinbutton")), w);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("resize_height_spinbutton")), h);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_w_spinbutton")), w);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_h_spinbutton")), h);

	_g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_width_spinbutton"), self);
	_g_signal_handlers_unblock_by_data (GET_WIDGET ("resize_height_spinbutton"), self);
	_g_signal_handlers_unblock_by_data (GET_WIDGET ("unit_combobox"), self);
	_g_signal_handlers_unblock_by_data (self->priv->ratio_combobox, self);
	_g_signal_handlers_unblock_by_data (GET_WIDGET ("invert_ratio_checkbutton"), self);
	_g_signal_handlers_unblock_by_data (GET_WIDGET ("ratio_w_spinbutton"), self);
	_g_signal_handlers_unblock_by_data (GET_WIDGET ("ratio_h_spinbutton"), self);

	update_image_size (self);
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


static void
options_button_clicked_cb (GtkButton         *button,
			   GthFileToolResize *self)
{
	GtkWidget *dialog;

	dialog = GET_WIDGET ("options_dialog");
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (gth_file_tool_get_window (GTH_FILE_TOOL (self))));
	gtk_widget_show (dialog);
}


static GtkWidget *
gth_file_tool_resize_get_options (GthFileTool *base)
{
	GthFileToolResize *self = (GthFileToolResize *) base;
	cairo_surface_t   *source;
	GtkWidget         *window;
	GthViewerPage     *viewer_page;
	GtkWidget         *viewer;
	GtkAllocation      allocation;
	int                preview_width;
	int                preview_height;
	GtkWidget         *options;
	char              *text;

	source = gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (source == NULL)
		return NULL;

	if (self->priv->settings == NULL)
		self->priv->settings = g_settings_new (GTHUMB_RESIZE_SCHEMA);

	self->priv->original_width = cairo_image_surface_get_width (source);
	self->priv->original_height = cairo_image_surface_get_height (source);

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));

	gtk_widget_get_allocation (GTK_WIDGET (viewer), &allocation);
	preview_width = self->priv->original_width;
	preview_height = self->priv->original_height;
	if (scale_keeping_ratio (&preview_width, &preview_height, allocation.width, allocation.height, FALSE))
		self->priv->preview = _cairo_image_surface_scale_fast (source, preview_width, preview_height);
	else
		self->priv->preview = cairo_surface_reference (source);

	_gtk_widget_get_screen_size (window, &self->priv->screen_width, &self->priv->screen_height);
	self->priv->new_image = NULL;
	self->priv->new_width = self->priv->original_width;
	self->priv->new_height = self->priv->original_height;
	self->priv->high_quality = g_settings_get_boolean (self->priv->settings, PREF_RESIZE_HIGH_QUALITY);
	self->priv->unit = g_settings_get_enum (self->priv->settings, PREF_RESIZE_UNIT);
	self->priv->builder = _gtk_builder_new_from_file ("resize-options.ui", "file_tools");
	self->priv->apply_to_original = FALSE;

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
	text = g_strdup_printf (_("%d × %d (Image)"), self->priv->original_width, self->priv->original_height);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("image_size_label")), text);
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->ratio_combobox), text);
	g_free (text);
	text = g_strdup_printf (_("%d × %d (Screen)"), self->priv->screen_width, self->priv->screen_height);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("screen_size_label")), text);
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->ratio_combobox), text);
	g_free (text);
	_gtk_combo_box_append_texts (GTK_COMBO_BOX_TEXT (self->priv->ratio_combobox),
				     _("5∶4"),
				     _("4∶3 (DVD, Book)"),
				     _("7∶5"),
				     _("3∶2 (Postcard)"),
				     _("16∶10"),
				     _("16∶9 (DVD)"),
				     _("1.85∶1"),
				     _("2.39∶1"),
				     _("Other…"),
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

	g_signal_connect_swapped (GET_WIDGET ("options_close_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_hide),
				  GET_WIDGET ("options_dialog"));
	g_signal_connect (GET_WIDGET ("options_dialog"),
			  "delete-event",
			  G_CALLBACK (gtk_widget_hide_on_delete),
			  NULL);
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

	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (viewer), GTH_ZOOM_QUALITY_HIGH);

	return options;
}


static void
gth_file_tool_resize_destroy_options (GthFileTool *base)
{
	GthFileToolResize *self;
	GtkWidget         *window;
	GthViewerPage     *viewer_page;
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
		_cairo_clear_surface (&self->priv->preview);
		_g_clear_object (&self->priv->builder);
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_reset_viewer_tool (GTH_IMAGE_VIEWER_PAGE (viewer_page));

	/* restore the zoom quality */

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	viewer_settings = g_settings_new (GTHUMB_IMAGE_VIEWER_SCHEMA);
	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (viewer),
					   g_settings_get_enum (viewer_settings, PREF_IMAGE_VIEWER_ZOOM_QUALITY));

	g_object_unref (viewer_settings);
}


static void
gth_file_tool_resize_apply_options (GthFileTool *base)
{
	GthFileToolResize *self;

	self = (GthFileToolResize *) base;

	self->priv->apply_to_original = TRUE;
	update_image_size (self);
}


static void
gth_file_tool_resize_populate_headerbar (GthFileTool *base,
					 GthBrowser  *browser)
{
	GthFileToolResize *self;
	GtkWidget         *button;

	self = (GthFileToolResize *) base;

	/* preferences dialog */

	button = gth_browser_add_header_bar_button (browser,
						    GTH_BROWSER_HEADER_SECTION_EDITOR_COMMANDS,
						    "preferences-system-symbolic",
						    _("Options"),
						    NULL,
						    NULL);
	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (options_button_clicked_cb),
			  self);

}


static void
gth_file_tool_resize_reset_image (GthImageViewerPageTool *base)
{
	GthFileToolResize *self = (GthFileToolResize *) base;

	if (self->priv->resize_task != NULL) {
		self->priv->closing = TRUE;
		gth_task_cancel (self->priv->resize_task);
		return;
	}

	if (self->priv->update_size_id != 0) {
		g_source_remove (self->priv->update_size_id);
		self->priv->update_size_id = 0;
	}

	gth_image_viewer_page_reset (GTH_IMAGE_VIEWER_PAGE (gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self))));
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
gth_file_tool_resize_finalize (GObject *object)
{
	GthFileToolResize *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_RESIZE (object));

	self = (GthFileToolResize *) object;

	cairo_surface_destroy (self->priv->new_image);
	cairo_surface_destroy (self->priv->preview);
	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->settings);

	/* Chain up */
	G_OBJECT_CLASS (gth_file_tool_resize_parent_class)->finalize (object);
}


static void
gth_file_tool_resize_class_init (GthFileToolResizeClass *klass)
{
	GObjectClass                *gobject_class;
	GthFileToolClass            *file_tool_class;
	GthImageViewerPageToolClass *image_viewer_page_tool_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_resize_finalize;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->get_options = gth_file_tool_resize_get_options;
	file_tool_class->destroy_options = gth_file_tool_resize_destroy_options;
	file_tool_class->apply_options = gth_file_tool_resize_apply_options;
	file_tool_class->populate_headerbar = gth_file_tool_resize_populate_headerbar;

	image_viewer_page_tool_class = (GthImageViewerPageToolClass *) klass;
	image_viewer_page_tool_class->reset_image = gth_file_tool_resize_reset_image;
}


static void
gth_file_tool_resize_init (GthFileToolResize *self)
{
	self->priv = gth_file_tool_resize_get_instance_private (self);
	self->priv->settings = NULL;
	self->priv->builder = NULL;
	self->priv->preview = NULL;
	self->priv->new_image = NULL;
	gth_file_tool_construct (GTH_FILE_TOOL (self), "image-resize-symbolic", _("Resize"), GTH_TOOLBOX_SECTION_FORMAT);
	gth_file_tool_set_zoomable (GTH_FILE_TOOL (self), TRUE);
}
