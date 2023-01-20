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
#include <gthumb.h>
#include <extensions/image_viewer/image-viewer.h>
#include "file-tools-enum-types.h"
#include "gth-file-tool-crop.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


struct _GthFileToolCropPrivate {
	GSettings        *settings;
	GtkBuilder       *builder;
	int               original_width;
	int               original_height;
	int               screen_width;
	int               screen_height;
	GthImageSelector *selector;
	GtkWidget        *ratio_combobox;
	GtkWidget        *crop_x_spinbutton;
	GtkWidget        *crop_y_spinbutton;
	GtkWidget        *crop_width_spinbutton;
	GtkWidget        *crop_height_spinbutton;
	GtkWidget	 *grid_type_combobox;
};


G_DEFINE_TYPE_WITH_CODE (GthFileToolCrop,
			 gth_file_tool_crop,
			 GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL,
			 G_ADD_PRIVATE (GthFileToolCrop))


static gpointer
crop_exec (GthAsyncTask *task,
	   gpointer      user_data)
{
	GthFileToolCrop       *self = user_data;
	cairo_rectangle_int_t  selection;
	cairo_surface_t       *source;
	cairo_surface_t       *destination;

	gth_image_selector_get_selection (self->priv->selector, &selection);
	if ((selection.width == 0) || (selection.height == 0))
		return NULL;

	source = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	destination = _cairo_image_surface_copy_subsurface (source,
					       	            selection.x,
					       	            selection.y,
					       	            selection.width,
					       	            selection.height);
	gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), destination);

	cairo_surface_destroy (destination);
	cairo_surface_destroy (source);

	return NULL;
}


static void
image_task_completed_cb (GthTask  *task,
			 GError   *error,
			 gpointer  user_data)
{
	GthFileToolCrop *self = user_data;
	cairo_surface_t *destination;
	GthViewerPage   *viewer_page;

	if (error != NULL) {
		g_object_unref (task);
		return;
	}

	destination = gth_image_task_get_destination_surface (GTH_IMAGE_TASK (task));
	if (destination == NULL) {
		g_object_unref (task);
		return;
	}

	viewer_page = gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), destination, TRUE);
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));

	cairo_surface_destroy (destination);
	g_object_unref (task);
}


static void
selection_x_value_changed_cb (GtkSpinButton   *spin,
			      GthFileToolCrop *self)
{
	gth_image_selector_set_selection_x (self->priv->selector, gtk_spin_button_get_value_as_int (spin));
}


static void
selection_y_value_changed_cb (GtkSpinButton   *spin,
			      GthFileToolCrop *self)
{
	gth_image_selector_set_selection_y (self->priv->selector, gtk_spin_button_get_value_as_int (spin));
}


static void
selection_width_value_changed_cb (GtkSpinButton   *spin,
				  GthFileToolCrop *self)
{
	gth_image_selector_set_selection_width (self->priv->selector, gtk_spin_button_get_value_as_int (spin));
}


static void
selection_height_value_changed_cb (GtkSpinButton   *spin,
				   GthFileToolCrop *self)
{
	gth_image_selector_set_selection_height (self->priv->selector, gtk_spin_button_get_value_as_int (spin));
}


static void
set_spin_range_value (GthFileToolCrop *self,
		      GtkWidget       *spin,
		      int              min,
		      int              max,
		      int              x)
{
	_g_signal_handlers_block_by_data (G_OBJECT (spin), self);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (spin), min, max);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), x);
	_g_signal_handlers_unblock_by_data (G_OBJECT (spin), self);
}


static void
selector_selection_changed_cb (GthImageSelector *selector,
			       GthFileToolCrop  *self)
{
	cairo_rectangle_int_t selection;
	int                   min, max;

	gth_image_selector_get_selection (selector, &selection);

	min = 0;
	max = self->priv->original_width - selection.width;
	set_spin_range_value (self, self->priv->crop_x_spinbutton, min, max, selection.x);

	min = 0;
	max = self->priv->original_height - selection.height;
	set_spin_range_value (self, self->priv->crop_y_spinbutton, min, max, selection.y);

	min = 0;
	max = self->priv->original_width - selection.x;
	set_spin_range_value (self, self->priv->crop_width_spinbutton, min, max, selection.width);

	min = 0;
	max = self->priv->original_height - selection.y;
	set_spin_range_value (self, self->priv->crop_height_spinbutton, min, max, selection.height);

	gth_image_selector_set_mask_visible (selector, (selection.width != 0 || selection.height != 0));
}


static void
set_spin_value (GthFileToolCrop *self,
		GtkWidget       *spin,
		int              x)
{
	_g_signal_handlers_block_by_data (G_OBJECT (spin), self);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), x);
	_g_signal_handlers_unblock_by_data (G_OBJECT (spin), self);
}


static void
ratio_combobox_changed_cb (GtkComboBox     *combobox,
			   GthFileToolCrop *self)
{
	GtkWidget *ratio_w_spinbutton;
	GtkWidget *ratio_h_spinbutton;
	int        idx;
	int        w, h;
	gboolean   use_ratio;
	double     ratio;

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

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton"))))
		ratio = (double) h / w;
	else
		ratio = (double) w / h;

	gtk_widget_set_visible (GET_WIDGET ("custom_ratio_box"), idx == GTH_ASPECT_RATIO_CUSTOM);
	gtk_widget_set_sensitive (GET_WIDGET ("invert_ratio_checkbutton"), use_ratio);
	set_spin_value (self, ratio_w_spinbutton, w);
	set_spin_value (self, ratio_h_spinbutton, h);
	gth_image_selector_set_ratio (GTH_IMAGE_SELECTOR (self->priv->selector),
				      use_ratio,
				      ratio,
				      FALSE);
}


static void
update_ratio (GtkSpinButton   *spin,
	      GthFileToolCrop *self,
	      gboolean         swap_x_and_y_to_start)
{
	gboolean use_ratio;
	int      w, h;
	double   ratio;

	use_ratio = gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->ratio_combobox)) != GTH_ASPECT_RATIO_NONE;
	w = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_w_spinbutton")));
	h = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_h_spinbutton")));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton"))))
		ratio = (double) h / w;
	else
		ratio = (double) w / h;
	gth_image_selector_set_ratio (self->priv->selector,
				      use_ratio,
				      ratio,
				      swap_x_and_y_to_start);
}


static void
ratio_value_changed_cb (GtkSpinButton   *spin,
			GthFileToolCrop *self)
{
	update_ratio (spin, self, FALSE);
}


static void
invert_ratio_changed_cb (GtkSpinButton   *spin,
			 GthFileToolCrop *self)
{
	update_ratio (spin, self, TRUE);
}


static void
grid_type_changed_cb (GtkComboBox     *combobox,
		      GthFileToolCrop *self)
{
	gth_image_selector_set_grid_type (self->priv->selector, gtk_combo_box_get_active (combobox));
}


static void
update_sensitivity (GthFileToolCrop *self)
{
	gtk_widget_set_sensitive (GET_WIDGET ("bind_factor_spinbutton"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET("bind_dimensions_checkbutton"))));
}


static void
bind_dimensions_checkbutton_toggled_cb (GtkToggleButton *togglebutton,
					 gpointer         user_data)
{
	GthFileToolCrop *self = user_data;

	gth_image_selector_bind_dimensions (self->priv->selector,
					    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET("bind_dimensions_checkbutton"))),
					    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("bind_factor_spinbutton"))));
	update_sensitivity (self);
}


static void
bind_factor_spinbutton_value_changed_cb (GtkSpinButton *spinbutton,
					 gpointer       user_data)
{
	GthFileToolCrop *self = user_data;

	gth_image_selector_bind_dimensions (self->priv->selector,
					    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET("bind_dimensions_checkbutton"))),
					    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("bind_factor_spinbutton"))));
	update_sensitivity (self);
}


static void
maximize_button_clicked_cb (GtkButton *button,
			    gpointer   user_data)
{
	GthFileToolCrop *self = user_data;

	gth_image_selector_set_selection_pos (self->priv->selector, 0, 0);
	if (! gth_image_selector_set_selection_width (self->priv->selector, self->priv->original_width) || ! gth_image_selector_get_use_ratio (self->priv->selector))
		gth_image_selector_set_selection_height (self->priv->selector, self->priv->original_height);
	gth_image_selector_center (self->priv->selector);
}


static void
center_button_clicked_cb (GtkButton *button,
			  gpointer   user_data)
{
	GthFileToolCrop *self = user_data;

	gth_image_selector_center (self->priv->selector);
}


static void
options_button_clicked_cb (GtkButton       *button,
			   GthFileToolCrop *self)
{
	GtkWidget *dialog;

	dialog = GET_WIDGET ("options_dialog");
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (gth_file_tool_get_window (GTH_FILE_TOOL (self))));
	gtk_widget_show (dialog);
}


static GtkWidget *
gth_file_tool_crop_get_options (GthFileTool *base)
{
	GthFileToolCrop *self;
	GtkWidget       *window;
	GthViewerPage   *viewer_page;
	GtkWidget       *viewer;
	cairo_surface_t *source;
	GtkWidget       *options;
	char            *text;

	self = (GthFileToolCrop *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	source = gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (source == NULL)
		return NULL;

	gth_image_viewer_get_original_size (GTH_IMAGE_VIEWER (viewer), &self->priv->original_width, &self->priv->original_height);
	_gtk_widget_get_screen_size (window, &self->priv->screen_width, &self->priv->screen_height);

	if (self->priv->settings == NULL)
		self->priv->settings = g_settings_new (GTHUMB_CROP_SCHEMA);
	self->priv->builder = _gtk_builder_new_from_file ("crop-options.ui", "file_tools");

	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);
	self->priv->crop_x_spinbutton = _gtk_builder_get_widget (self->priv->builder, "crop_x_spinbutton");
	self->priv->crop_y_spinbutton = _gtk_builder_get_widget (self->priv->builder, "crop_y_spinbutton");
	self->priv->crop_width_spinbutton = _gtk_builder_get_widget (self->priv->builder, "crop_width_spinbutton");
	self->priv->crop_height_spinbutton = _gtk_builder_get_widget (self->priv->builder, "crop_height_spinbutton");

	self->priv->ratio_combobox = _gtk_combo_box_new_with_texts (_("None"), _("Square"), NULL);
	text = g_strdup_printf (_("%d × %d (Image)"), self->priv->original_width, self->priv->original_height);
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->ratio_combobox), text);
	g_free (text);
	text = g_strdup_printf (_("%d × %d (Screen)"), self->priv->screen_width, self->priv->screen_height);
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

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->ratio_combobox),
				  g_settings_get_enum (self->priv->settings, PREF_CROP_ASPECT_RATIO));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_w_spinbutton")),
				   MAX (g_settings_get_int (self->priv->settings, PREF_CROP_ASPECT_RATIO_WIDTH), 1));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_h_spinbutton")),
				   MAX (g_settings_get_int (self->priv->settings, PREF_CROP_ASPECT_RATIO_HEIGHT), 1));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton")),
				      g_settings_get_boolean (self->priv->settings, PREF_CROP_ASPECT_RATIO_INVERT));

	self->priv->grid_type_combobox = _gtk_combo_box_new_with_texts (_("None"),
									_("Rule of Thirds"),
									_("Golden Sections"),
									_("Center Lines"),
									_("Uniform"),
									NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->grid_type_combobox),
				  g_settings_get_enum (self->priv->settings, PREF_CROP_GRID_TYPE));
	gtk_widget_show (self->priv->grid_type_combobox);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("grid_type_combobox_box")),
			    self->priv->grid_type_combobox, TRUE, TRUE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("grid_label")),
				       self->priv->grid_type_combobox);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("bind_dimensions_checkbutton")),
				      g_settings_get_boolean (self->priv->settings, PREF_CROP_BIND_DIMENSIONS));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("bind_factor_spinbutton")),
				   g_settings_get_int (self->priv->settings, PREF_CROP_BIND_FACTOR));

	g_signal_connect_swapped (GET_WIDGET ("options_close_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_hide),
				  GET_WIDGET ("options_dialog"));
	g_signal_connect (GET_WIDGET ("options_dialog"),
			  "delete-event",
			  G_CALLBACK (gtk_widget_hide_on_delete),
			  NULL);
	g_signal_connect (G_OBJECT (self->priv->crop_x_spinbutton),
			  "value-changed",
			  G_CALLBACK (selection_x_value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_y_spinbutton),
			  "value-changed",
			  G_CALLBACK (selection_y_value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_width_spinbutton),
			  "value-changed",
			  G_CALLBACK (selection_width_value_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_height_spinbutton),
			  "value-changed",
			  G_CALLBACK (selection_height_value_changed_cb),
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
        g_signal_connect (self->priv->grid_type_combobox,
                          "changed",
                          G_CALLBACK (grid_type_changed_cb),
                          self);
	g_signal_connect (GET_WIDGET ("bind_dimensions_checkbutton"),
			  "toggled",
			  G_CALLBACK (bind_dimensions_checkbutton_toggled_cb),
			  self);
	g_signal_connect (GET_WIDGET ("bind_factor_spinbutton"),
			  "value-changed",
			  G_CALLBACK (bind_factor_spinbutton_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("maximize_button"),
			  "clicked",
			  G_CALLBACK (maximize_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("center_button"),
			  "clicked",
			  G_CALLBACK (center_button_clicked_cb),
			  self);

	self->priv->selector = (GthImageSelector *) gth_image_selector_new (GTH_SELECTOR_TYPE_REGION);
	gth_image_selector_set_grid_type (self->priv->selector, gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->grid_type_combobox)));
	gth_image_selector_bind_dimensions (self->priv->selector,
					    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET("bind_dimensions_checkbutton"))),
					    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("bind_factor_spinbutton"))));
	g_signal_connect (self->priv->selector,
			  "selection-changed",
			  G_CALLBACK (selector_selection_changed_cb),
			  self);

	gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), source, FALSE);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), GTH_IMAGE_VIEWER_TOOL (self->priv->selector));
	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (viewer), GTH_ZOOM_QUALITY_LOW);
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (viewer), GTH_FIT_SIZE_IF_LARGER);
	ratio_combobox_changed_cb (NULL, self);

	if (! gth_image_selector_set_selection_width (self->priv->selector, self->priv->original_width * 2 / 3) || ! gth_image_selector_get_use_ratio (self->priv->selector))
		gth_image_selector_set_selection_height (self->priv->selector, self->priv->original_height * 2 / 3);
	gth_image_selector_center (self->priv->selector);

	update_sensitivity (self);

	return options;
}


static void
gth_file_tool_crop_destroy_options (GthFileTool *base)
{
	GthFileToolCrop *self;
	GtkWidget       *window;
	GthViewerPage   *viewer_page;
	GtkWidget       *viewer;
	GSettings       *viewer_settings;

	self = (GthFileToolCrop *) base;

	if (self->priv->builder != NULL) {
		/* save the dialog options */

		g_settings_set_enum (self->priv->settings, PREF_CROP_GRID_TYPE, gth_image_selector_get_grid_type (self->priv->selector));
		g_settings_set_int (self->priv->settings, PREF_CROP_ASPECT_RATIO_WIDTH, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_w_spinbutton"))));
		g_settings_set_int (self->priv->settings, PREF_CROP_ASPECT_RATIO_HEIGHT, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("ratio_h_spinbutton"))));
		g_settings_set_enum (self->priv->settings, PREF_CROP_ASPECT_RATIO, gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->ratio_combobox)));
		g_settings_set_boolean (self->priv->settings, PREF_CROP_ASPECT_RATIO_INVERT, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("invert_ratio_checkbutton"))));
		g_settings_set_boolean (self->priv->settings, PREF_CROP_BIND_DIMENSIONS, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("bind_dimensions_checkbutton"))));
		g_settings_set_int (self->priv->settings, PREF_CROP_BIND_FACTOR, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("bind_factor_spinbutton"))));

		/* destroy the option data */

		_g_object_unref (self->priv->builder);
		_g_object_unref (self->priv->selector);
		self->priv->builder = NULL;
		self->priv->selector = NULL;
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
gth_file_tool_crop_apply_options (GthFileTool *base)
{
	GthFileToolCrop       *self;
	cairo_rectangle_int_t  selection;
	GthTask               *task;

	self = (GthFileToolCrop *) base;

	gth_image_selector_get_selection (self->priv->selector, &selection);
	if ((selection.width == 0) || (selection.height == 0))
		return;

	task = gth_image_task_new (_("Applying changes"),
				   NULL,
				   crop_exec,
				   NULL,
				   self,
				   NULL);
	gth_image_task_set_source_surface (GTH_IMAGE_TASK (task), gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self)));
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (image_task_completed_cb),
			  self);
	gth_browser_exec_task (GTH_BROWSER (gth_file_tool_get_window (GTH_FILE_TOOL (self))), task, GTH_TASK_FLAGS_DEFAULT);
}


static void
gth_file_tool_crop_populate_headerbar (GthFileTool *base,
				       GthBrowser  *browser)
{
	GthFileToolCrop *self;
	GtkWidget       *button;

	self = (GthFileToolCrop *) base;

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
gth_file_tool_crop_reset_image (GthImageViewerPageTool *base)
{
	GthFileToolCrop *self = (GthFileToolCrop *) base;

	gth_image_viewer_page_reset (GTH_IMAGE_VIEWER_PAGE (gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self))));
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
gth_file_tool_crop_init (GthFileToolCrop *self)
{
	self->priv = gth_file_tool_crop_get_instance_private (self);
	self->priv->settings = NULL;
	gth_file_tool_construct (GTH_FILE_TOOL (self), "image-crop-symbolic", _("Crop"), GTH_TOOLBOX_SECTION_FORMAT);
}


static void
gth_file_tool_crop_finalize (GObject *object)
{
	GthFileToolCrop *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_CROP (object));

	self = (GthFileToolCrop *) object;

	_g_object_unref (self->priv->selector);
	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->settings);

	/* Chain up */
	G_OBJECT_CLASS (gth_file_tool_crop_parent_class)->finalize (object);
}


static void
gth_file_tool_crop_class_init (GthFileToolCropClass *klass)
{
	GObjectClass                *gobject_class;
	GthFileToolClass            *file_tool_class;
	GthImageViewerPageToolClass *image_viewer_page_tool_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_crop_finalize;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->get_options = gth_file_tool_crop_get_options;
	file_tool_class->destroy_options = gth_file_tool_crop_destroy_options;
	file_tool_class->apply_options = gth_file_tool_crop_apply_options;
	file_tool_class->populate_headerbar = gth_file_tool_crop_populate_headerbar;

	image_viewer_page_tool_class = (GthImageViewerPageToolClass *) klass;
	image_viewer_page_tool_class->reset_image = gth_file_tool_crop_reset_image;
}
