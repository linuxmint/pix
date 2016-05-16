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
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include <extensions/image_viewer/preferences.h>
#include "enum-types.h"
#include "gth-file-tool-crop.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


G_DEFINE_TYPE (GthFileToolCrop, gth_file_tool_crop, GTH_TYPE_FILE_TOOL)


struct _GthFileToolCropPrivate {
	GSettings        *settings;
	GtkBuilder       *builder;
	int               pixbuf_width;
	int               pixbuf_height;
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


static void
gth_file_tool_crop_update_sensitivity (GthFileTool *base)
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
crop_button_clicked_cb (GtkButton       *button,
			GthFileToolCrop *self)
{
	cairo_rectangle_int_t  selection;
	GtkWidget             *window;
	GtkWidget             *viewer_page;
	GtkWidget             *viewer;
	cairo_surface_t       *old_image;
	cairo_surface_t       *new_image;

	gth_image_selector_get_selection (self->priv->selector, &selection);
	if ((selection.width == 0) || (selection.height == 0))
		return;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	old_image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer));

	new_image = _cairo_image_surface_copy_subsurface (old_image,
					       	          selection.x,
					       	          selection.y,
					       	          selection.width,
					       	          selection.height);
	if (new_image != NULL) {
		gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), new_image, TRUE);
		gth_file_tool_hide_options (GTH_FILE_TOOL (self));

		cairo_surface_destroy (new_image);
	}
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
	g_signal_handlers_block_by_data (G_OBJECT (spin), self);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (spin), min, max);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), x);
	g_signal_handlers_unblock_by_data (G_OBJECT (spin), self);
}


static void
selector_selection_changed_cb (GthImageSelector *selector,
			       GthFileToolCrop  *self)
{
	cairo_rectangle_int_t selection;
	int                   min, max;

	gth_image_selector_get_selection (selector, &selection);

	min = 0;
	max = self->priv->pixbuf_width - selection.width;
	set_spin_range_value (self, self->priv->crop_x_spinbutton, min, max, selection.x);

	min = 0;
	max = self->priv->pixbuf_height - selection.height;
	set_spin_range_value (self, self->priv->crop_y_spinbutton, min, max, selection.y);

	min = 0;
	max = self->priv->pixbuf_width - selection.x;
	set_spin_range_value (self, self->priv->crop_width_spinbutton, min, max, selection.width);

	min = 0;
	max = self->priv->pixbuf_height - selection.y;
	set_spin_range_value (self, self->priv->crop_height_spinbutton, min, max, selection.height);

	gth_image_selector_set_mask_visible (selector, (selection.width != 0 || selection.height != 0));
}


static void
set_spin_value (GthFileToolCrop *self,
		GtkWidget       *spin,
		int              x)
{
	g_signal_handlers_block_by_data (G_OBJECT (spin), self);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), x);
	g_signal_handlers_unblock_by_data (G_OBJECT (spin), self);
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
		w = self->priv->pixbuf_width;
		h = self->priv->pixbuf_height;
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

	gtk_widget_set_sensitive (GET_WIDGET ("custom_ratio_box"), idx == GTH_ASPECT_RATIO_CUSTOM);
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
	if (! gth_image_selector_set_selection_width (self->priv->selector, self->priv->pixbuf_width) || ! gth_image_selector_get_use_ratio (self->priv->selector))
		gth_image_selector_set_selection_height (self->priv->selector, self->priv->pixbuf_height);
	gth_image_selector_center (self->priv->selector);
}


static void
center_button_clicked_cb (GtkButton *button,
			  gpointer   user_data)
{
	GthFileToolCrop *self = user_data;

	gth_image_selector_center (self->priv->selector);
}


static GtkWidget *
gth_file_tool_crop_get_options (GthFileTool *base)
{
	GthFileToolCrop *self;
	GtkWidget       *window;
	GtkWidget       *viewer_page;
	GtkWidget       *viewer;
	cairo_surface_t *image;
	GtkWidget       *options;
	char            *text;

	self = (GthFileToolCrop *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (viewer));
	if (image == NULL)
		return NULL;

	self->priv->pixbuf_width = cairo_image_surface_get_width (image);
	self->priv->pixbuf_height = cairo_image_surface_get_height (image);
	_gtk_widget_get_screen_size (window, &self->priv->screen_width, &self->priv->screen_height);

	self->priv->builder = _gtk_builder_new_from_file ("crop-options.ui", "file_tools");

	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);
	self->priv->crop_x_spinbutton = _gtk_builder_get_widget (self->priv->builder, "crop_x_spinbutton");
	self->priv->crop_y_spinbutton = _gtk_builder_get_widget (self->priv->builder, "crop_y_spinbutton");
	self->priv->crop_width_spinbutton = _gtk_builder_get_widget (self->priv->builder, "crop_width_spinbutton");
	self->priv->crop_height_spinbutton = _gtk_builder_get_widget (self->priv->builder, "crop_height_spinbutton");

	self->priv->ratio_combobox = _gtk_combo_box_new_with_texts (_("None"), _("Square"), NULL);
	text = g_strdup_printf (_("%d x %d (Image)"), self->priv->pixbuf_width, self->priv->pixbuf_height);
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->ratio_combobox), text);
	g_free (text);
	text = g_strdup_printf (_("%d x %d (Screen)"), self->priv->screen_width, self->priv->screen_height);
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

	gtk_widget_set_vexpand (GET_WIDGET ("options_box"), FALSE);

	g_signal_connect (GET_WIDGET ("crop_button"),
			  "clicked",
			  G_CALLBACK (crop_button_clicked_cb),
			  self);
	g_signal_connect_swapped (GET_WIDGET ("cancel_button"),
				  "clicked",
				  G_CALLBACK (gth_file_tool_cancel),
				  self);
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

	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), (GthImageViewerTool *) self->priv->selector);
	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (viewer), GTH_ZOOM_QUALITY_LOW);
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (viewer), GTH_FIT_SIZE_IF_LARGER);
	ratio_combobox_changed_cb (NULL, self);

	if (! gth_image_selector_set_selection_width (self->priv->selector, self->priv->pixbuf_width * 2 / 3) || ! gth_image_selector_get_use_ratio (self->priv->selector))
		gth_image_selector_set_selection_height (self->priv->selector, self->priv->pixbuf_height * 2 / 3);
	gth_image_selector_center (self->priv->selector);

	update_sensitivity (self);

	return options;
}


static void
gth_file_tool_crop_destroy_options (GthFileTool *base)
{
	GthFileToolCrop *self;
	GtkWidget       *window;
	GtkWidget       *viewer_page;
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
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), NULL);

	/* restore the zoom quality */
	viewer_settings = g_settings_new (GTHUMB_IMAGE_VIEWER_SCHEMA);
	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (viewer),
					   g_settings_get_enum (viewer_settings, PREF_IMAGE_VIEWER_ZOOM_QUALITY));

	g_object_unref (viewer_settings);
}


static void
gth_file_tool_crop_activate (GthFileTool *base)
{
	gth_file_tool_show_options (base);
}


static void
gth_file_tool_crop_init (GthFileToolCrop *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_TOOL_CROP, GthFileToolCropPrivate);
	self->priv->settings = g_settings_new (GTHUMB_CROP_SCHEMA);
	gth_file_tool_construct (GTH_FILE_TOOL (self), "tool-crop", _("Crop..."), _("Crop"), FALSE);
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
gth_file_tool_crop_class_init (GthFileToolCropClass *class)
{
	GObjectClass     *gobject_class;
	GthFileToolClass *file_tool_class;

	g_type_class_add_private (class, sizeof (GthFileToolCropPrivate));

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_file_tool_crop_finalize;

	file_tool_class = (GthFileToolClass *) class;
	file_tool_class->update_sensitivity = gth_file_tool_crop_update_sensitivity;
	file_tool_class->activate = gth_file_tool_crop_activate;
	file_tool_class->get_options = gth_file_tool_crop_get_options;
	file_tool_class->destroy_options = gth_file_tool_crop_destroy_options;
}
