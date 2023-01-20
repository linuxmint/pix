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

#include <math.h>
#include <config.h>
#include <gthumb.h>
#include <extensions/image_viewer/image-viewer.h>
#include "cairo-rotate.h"
#include "file-tools-enum-types.h"
#include "gth-file-tool-rotate.h"
#include "gth-image-line-tool.h"
#include "gth-image-rotator.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


struct _GthFileToolRotatePrivate {
	GSettings             *settings;
	cairo_surface_t       *image;
	gboolean               has_alpha;
	GtkBuilder            *builder;
	GtkWidget             *crop_grid;
	GtkAdjustment         *rotation_angle_adj;
	GtkAdjustment         *crop_p1_adj;
	GtkAdjustment         *crop_p2_adj;
	gboolean               crop_enabled;
	double                 crop_p1_plus_p2;
	cairo_rectangle_int_t  crop_region;
	GthImageViewerTool    *alignment;
	GthImageViewerTool    *rotator;
};


G_DEFINE_TYPE_WITH_CODE (GthFileToolRotate,
			 gth_file_tool_rotate,
			 GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL,
			 G_ADD_PRIVATE (GthFileToolRotate))


static void
update_crop_parameters (GthFileToolRotate *self)
{
	GthTransformResize resize;
	double             rotation_angle;
	double             crop_p1;
	double             crop_p_min;

	resize = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("resize_combobox")));
	self->priv->crop_enabled = (resize == GTH_TRANSFORM_RESIZE_CROP);

	if (self->priv->crop_enabled) {
		gtk_widget_set_sensitive (GET_WIDGET ("crop_options_table"), TRUE);

		rotation_angle = gtk_adjustment_get_value (self->priv->rotation_angle_adj);

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("keep_aspect_ratio")))) {
			gtk_widget_set_sensitive (GET_WIDGET ("crop_p2_label"), FALSE);
			gtk_widget_set_sensitive (GET_WIDGET ("crop_p2_hbox"), FALSE);

			_cairo_image_surface_rotate_get_cropping_parameters (self->priv->image,
								    	     rotation_angle,
								    	     &self->priv->crop_p1_plus_p2,
								    	     &crop_p_min);

			/* This centers the cropping region in the middle of the rotated image */

			crop_p1 = self->priv->crop_p1_plus_p2 / 2.0;

			gtk_adjustment_set_lower (self->priv->crop_p1_adj, MAX (crop_p_min, 0.0));
			gtk_adjustment_set_lower (self->priv->crop_p2_adj, MAX (crop_p_min, 0.0));

			gtk_adjustment_set_upper (self->priv->crop_p1_adj, MIN (self->priv->crop_p1_plus_p2 - crop_p_min, 1.0));
			gtk_adjustment_set_upper (self->priv->crop_p2_adj, MIN (self->priv->crop_p1_plus_p2 - crop_p_min, 1.0));

			gtk_adjustment_set_value (self->priv->crop_p1_adj, crop_p1);
		}
		else {
			self->priv->crop_p1_plus_p2 = 0;

			gtk_widget_set_sensitive (GET_WIDGET ("crop_p2_label"), TRUE);
			gtk_widget_set_sensitive (GET_WIDGET ("crop_p2_hbox"), TRUE);

			gtk_adjustment_set_lower (self->priv->crop_p1_adj, 0.0);
			gtk_adjustment_set_lower (self->priv->crop_p2_adj, 0.0);

			gtk_adjustment_set_upper (self->priv->crop_p1_adj, 1.0);
			gtk_adjustment_set_upper (self->priv->crop_p2_adj, 1.0);
		}
	}
	else
		gtk_widget_set_sensitive (GET_WIDGET ("crop_options_table"), FALSE);

	gth_image_rotator_set_resize (GTH_IMAGE_ROTATOR (self->priv->rotator), resize);
}


static void
update_crop_region (GthFileToolRotate *self)
{
	if (self->priv->crop_enabled) {
		double rotation_angle;
		double crop_p1;
		double crop_p2;

		rotation_angle = gtk_adjustment_get_value (self->priv->rotation_angle_adj);
		crop_p1 = gtk_adjustment_get_value (self->priv->crop_p1_adj);
		crop_p2 = gtk_adjustment_get_value (self->priv->crop_p2_adj);
		_cairo_image_surface_rotate_get_cropping_region (self->priv->image,
								 rotation_angle,
								 crop_p1,
								 crop_p2,
								 &self->priv->crop_region);
		gth_image_rotator_set_crop_region (GTH_IMAGE_ROTATOR (self->priv->rotator), &self->priv->crop_region);
	}
	else
		gth_image_rotator_set_crop_region (GTH_IMAGE_ROTATOR (self->priv->rotator), NULL);
}


static void
update_crop_grid (GthFileToolRotate *self)
{
	gth_image_rotator_set_grid_type (GTH_IMAGE_ROTATOR (self->priv->rotator), gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->crop_grid)));
}


static void
alignment_changed_cb (GthImageLineTool  *line_tool,
		      GthFileToolRotate *self)
{
	GtkWidget     *window;
	GthViewerPage *viewer_page;
	GtkWidget     *viewer;
	GdkPoint       p1;
	GdkPoint       p2;
	double         angle;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("options_notebook")), 0);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->rotator);

	gth_image_line_tool_get_points (line_tool, &p1, &p2);
	angle = _cairo_image_surface_rotate_get_align_angle (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("alignment_parallel_radiobutton"))), &p1, &p2);
	gtk_adjustment_set_value (self->priv->rotation_angle_adj, angle);
}


static void
apply_changes (GthFileToolRotate *self)
{
	gth_image_rotator_set_angle (GTH_IMAGE_ROTATOR (self->priv->rotator), gtk_adjustment_get_value (self->priv->rotation_angle_adj));
	update_crop_parameters (self);
	update_crop_region (self);
}


static void
alignment_cancel_button_clicked_cb (GtkButton         *button,
				    GthFileToolRotate *self)
{
	GtkWidget     *window;
	GthViewerPage *viewer_page;
	GtkWidget     *viewer;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("options_notebook")), 0);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->rotator);
}


static void
align_button_clicked_cb (GtkButton         *button,
			 GthFileToolRotate *self)
{
	GtkWidget     *window;
	GthViewerPage *viewer_page;
	GtkWidget     *viewer;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));

	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("options_notebook")), 1);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->alignment);
}


static void
reset_button_clicked_cb (GtkButton         *button,
			 GthFileToolRotate *self)
{
	gth_image_rotator_set_center (GTH_IMAGE_ROTATOR (self->priv->rotator),
				      cairo_image_surface_get_width (self->priv->image) / 2,
				      cairo_image_surface_get_height (self->priv->image) / 2);
	gtk_adjustment_set_value (self->priv->rotation_angle_adj, 0.0);
}


static void
options_button_clicked_cb (GtkButton         *button,
			       GthFileToolRotate *self)
{
	GtkWidget *dialog;

	dialog = GET_WIDGET ("options_dialog");
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (gth_file_tool_get_window (GTH_FILE_TOOL (self))));
	gtk_widget_show (dialog);
}


static gpointer
rotate_exec (GthAsyncTask *task,
	     gpointer      user_data)
{
	GthImageViewerTool *rotator = user_data;
	cairo_surface_t    *source;
	cairo_surface_t    *destination;

	source = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	destination = gth_image_rotator_get_result (GTH_IMAGE_ROTATOR (rotator), source, task);
	gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), destination);

	cairo_surface_destroy (destination);
	cairo_surface_destroy (source);

	return NULL;
}


static void
crop_settings_changed_cb (GtkAdjustment     *adj,
		          GthFileToolRotate *self)
{
	update_crop_parameters (self);
	update_crop_region (self);
}


static void
crop_parameters_changed_cb (GtkAdjustment     *adj,
		            GthFileToolRotate *self)
{
	if ((adj == self->priv->crop_p1_adj) && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("keep_aspect_ratio"))))
		gtk_adjustment_set_value (self->priv->crop_p2_adj,
					  self->priv->crop_p1_plus_p2 - gtk_adjustment_get_value (adj));
	else
		update_crop_region (self);
}


static void
crop_grid_changed_cb (GtkAdjustment     *adj,
		      GthFileToolRotate *self)
{
	update_crop_grid (self);
}


static void
rotation_angle_value_changed_cb (GtkAdjustment     *adj,
				 GthFileToolRotate *self)
{
	apply_changes (self);
}


static void
background_colorbutton_color_set_cb (GtkColorButton    *color_button,
		             	     GthFileToolRotate *self)
{
	GdkRGBA  background_color;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("background_transparent_checkbutton")), FALSE);

	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (color_button), &background_color);
	gth_image_rotator_set_background (GTH_IMAGE_ROTATOR (self->priv->rotator), &background_color);

	apply_changes (self);
}


static void
background_transparent_toggled_cb (GtkToggleButton   *toggle_button,
		             	   GthFileToolRotate *self)
{
	if (gtk_toggle_button_get_active (toggle_button)) {
		GdkRGBA background_color;

		background_color.red = 0.0;
		background_color.green = 0.0;
		background_color.blue = 0.0;
		background_color.alpha = 0.0;
		gth_image_rotator_set_background (GTH_IMAGE_ROTATOR (self->priv->rotator), &background_color);
	}
	else
		background_colorbutton_color_set_cb (GTK_COLOR_BUTTON (GET_WIDGET ("background_colorbutton")), self);
}


static void
resize_combobox_changed_cb (GtkComboBox       *combo_box,
			    GthFileToolRotate *self)
{
	update_crop_parameters (self);
	update_crop_region (self);
}


static void
rotator_angle_changed_cb (GthImageRotator   *rotator,
			  double             angle,
			  GthFileToolRotate *self)
{
	gtk_adjustment_set_value (self->priv->rotation_angle_adj, angle);
}


static void
rotator_center_changed_cb (GthImageRotator   *rotator,
		  	   int                x,
		  	   int                y,
		  	   GthFileToolRotate *self)
{
	gth_image_rotator_set_center (rotator, x, y);
	update_crop_parameters (self);
	update_crop_region (self);
}


static GtkWidget *
gth_file_tool_rotate_get_options (GthFileTool *base)
{
	GthFileToolRotate *self;
	GtkWidget         *window;
	GthViewerPage     *viewer_page;
	GtkWidget         *viewer;
	char              *color_spec;
	GdkRGBA            background_color;

	self = (GthFileToolRotate *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	cairo_surface_destroy (self->priv->image);

	self->priv->image = gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	if (self->priv->image == NULL)
		return NULL;

	cairo_surface_reference (self->priv->image);

	self->priv->builder = _gtk_builder_new_from_file ("rotate-options.ui", "file_tools");

	self->priv->rotation_angle_adj = gth_color_scale_label_new (GET_WIDGET ("rotation_angle_hbox"),
								    GTK_LABEL (GET_WIDGET ("rotation_angle_label")),
								    GTH_COLOR_SCALE_DEFAULT,
								    0.0, -180.0, 180.0, 0.1, 1.0, "%+.1f°");

	self->priv->crop_p1_adj = gth_color_scale_label_new (GET_WIDGET ("crop_p1_hbox"),
							     GTK_LABEL (GET_WIDGET ("crop_p1_label")),
							     GTH_COLOR_SCALE_DEFAULT,
							     1.0, 0.0, 1.0, 0.001, 0.01, "%.3f");

	self->priv->crop_p2_adj = gth_color_scale_label_new (GET_WIDGET ("crop_p2_hbox"),
							     GTK_LABEL (GET_WIDGET ("crop_p2_label")),
							     GTH_COLOR_SCALE_DEFAULT,
							     1.0, 0.0, 1.0, 0.001, 0.01, "%.3f");

	self->priv->crop_grid = _gtk_combo_box_new_with_texts (_("None"),
							       _("Rule of Thirds"),
							       _("Golden Sections"),
							       _("Center Lines"),
							       _("Uniform"),
							       NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->crop_grid), g_settings_get_enum (self->priv->settings, PREF_ROTATE_GRID_TYPE));
	gtk_widget_show (self->priv->crop_grid);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("crop_grid_hbox")), self->priv->crop_grid, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("crop_grid_label")), self->priv->crop_grid);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("resize_combobox")), g_settings_get_enum (self->priv->settings, PREF_ROTATE_RESIZE));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("keep_aspect_ratio")), g_settings_get_boolean (self->priv->settings, PREF_ROTATE_KEEP_ASPECT_RATIO));

	self->priv->alignment = gth_image_line_tool_new ();

	self->priv->rotator = gth_image_rotator_new ();
	gth_image_rotator_set_center (GTH_IMAGE_ROTATOR (self->priv->rotator),
				      cairo_image_surface_get_width (self->priv->image) / 2,
				      cairo_image_surface_get_height (self->priv->image) / 2);

	self->priv->has_alpha = _cairo_image_surface_get_has_alpha (self->priv->image);
	if (self->priv->has_alpha) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("background_transparent_checkbutton")), TRUE);
	}
	else {
		gtk_widget_set_sensitive (GET_WIDGET ("background_transparent_checkbutton"), FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("background_transparent_checkbutton")), FALSE);
	}

	color_spec = g_settings_get_string (self->priv->settings, PREF_ROTATE_BACKGROUND_COLOR);
	if (! self->priv->has_alpha) {
		gdk_rgba_parse (&background_color, color_spec);
	}
	else {
		background_color.red = 0.0;
		background_color.green = 0.0;
		background_color.blue = 0.0;
		background_color.alpha = 1.0;
	}
	gth_image_rotator_set_background (GTH_IMAGE_ROTATOR (self->priv->rotator), &background_color);

	gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->image, FALSE);
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), self->priv->rotator);
	gth_viewer_page_update_sensitivity (viewer_page);

	self->priv->crop_enabled = TRUE;
	self->priv->crop_region.x = 0;
	self->priv->crop_region.y = 0;
	self->priv->crop_region.width = cairo_image_surface_get_width (self->priv->image);
	self->priv->crop_region.height = cairo_image_surface_get_height (self->priv->image);

	g_signal_connect_swapped (GET_WIDGET ("options_close_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_hide),
				  GET_WIDGET ("options_dialog"));
	g_signal_connect (GET_WIDGET ("options_dialog"),
			  "delete-event",
			  G_CALLBACK (gtk_widget_hide_on_delete),
			  NULL);
	g_signal_connect (GET_WIDGET ("align_button"),
			  "clicked",
			  G_CALLBACK (align_button_clicked_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->rotation_angle_adj),
			  "value-changed",
			  G_CALLBACK (rotation_angle_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("background_colorbutton"),
			  "color-set",
			  G_CALLBACK (background_colorbutton_color_set_cb),
			  self);
	g_signal_connect (GET_WIDGET ("background_transparent_checkbutton"),
			  "toggled",
			  G_CALLBACK (background_transparent_toggled_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_p1_adj),
			  "value-changed",
			  G_CALLBACK (crop_parameters_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_p2_adj),
			  "value-changed",
			  G_CALLBACK (crop_parameters_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (GET_WIDGET ("keep_aspect_ratio")),
			  "toggled",
			  G_CALLBACK (crop_settings_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->crop_grid),
			  "changed",
			  G_CALLBACK (crop_grid_changed_cb),
			  self);
	g_signal_connect (self->priv->alignment,
			  "changed",
			  G_CALLBACK (alignment_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("alignment_cancel_button"),
			  "clicked",
			  G_CALLBACK (alignment_cancel_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("resize_combobox"),
			  "changed",
			  G_CALLBACK (resize_combobox_changed_cb),
			  self);
	g_signal_connect (self->priv->rotator,
			  "angle-changed",
			  G_CALLBACK (rotator_angle_changed_cb),
			  self);
	g_signal_connect (self->priv->rotator,
			  "center-changed",
			  G_CALLBACK (rotator_center_changed_cb),
			  self);

	update_crop_parameters (self);
	update_crop_region (self);
	update_crop_grid (self);

	return GET_WIDGET ("options_notebook");
}


static void
gth_file_tool_rotate_destroy_options (GthFileTool *base)
{
	GthFileToolRotate *self;
	GtkWidget         *window;
	GthViewerPage     *viewer_page;

	self = (GthFileToolRotate *) base;

	if (self->priv->builder != NULL) {
		GdkRGBA  background_color;
		char    *color_spec;

		/* save the dialog options */

		g_settings_set_enum (self->priv->settings, PREF_ROTATE_RESIZE, gth_image_rotator_get_resize (GTH_IMAGE_ROTATOR (self->priv->rotator)));
		g_settings_set_boolean (self->priv->settings, PREF_ROTATE_KEEP_ASPECT_RATIO, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("keep_aspect_ratio"))));
		g_settings_set_enum (self->priv->settings, PREF_ROTATE_GRID_TYPE, gth_image_rotator_get_grid_type (GTH_IMAGE_ROTATOR (self->priv->rotator)));

		gth_image_rotator_get_background (GTH_IMAGE_ROTATOR (self->priv->rotator), &background_color);
		color_spec = gdk_rgba_to_string (&background_color);
		g_settings_set_string (self->priv->settings, PREF_ROTATE_BACKGROUND_COLOR, color_spec);

		g_free (color_spec);
	}

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_reset_viewer_tool (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_viewer_page_update_sensitivity (viewer_page);

	cairo_surface_destroy (self->priv->image);
	self->priv->image = NULL;
	_g_clear_object (&self->priv->builder);
	_g_clear_object (&self->priv->rotator);
	_g_clear_object (&self->priv->alignment);
}


static void
gth_file_tool_rotate_apply_options (GthFileTool *base)
{
	GthFileToolRotate *self;
	GtkWidget         *window;
	GthViewerPage     *viewer_page;
	GthTask           *task;

	self = (GthFileToolRotate *) base;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	task = gth_image_viewer_task_new (GTH_IMAGE_VIEWER_PAGE (viewer_page),
					  _("Applying changes"),
					  NULL,
					  rotate_exec,
					  NULL,
					  g_object_ref (self->priv->rotator),
					  g_object_unref);
	gth_image_viewer_task_set_load_original (GTH_IMAGE_VIEWER_TASK (task), FALSE);
	gth_image_task_set_source_surface (GTH_IMAGE_TASK (task), gth_image_viewer_page_tool_get_source (GTH_IMAGE_VIEWER_PAGE_TOOL (self)));

	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (gth_image_viewer_task_set_destination),
			  NULL);
	gth_browser_exec_task (GTH_BROWSER (window), task, GTH_TASK_FLAGS_DEFAULT);

	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
gth_file_tool_rotate_populate_headerbar (GthFileTool *base,
					 GthBrowser  *browser)
{
	GthFileToolRotate *self;
	GtkWidget         *button;

	self = (GthFileToolRotate *) base;

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
gth_file_tool_rotate_reset_image (GthImageViewerPageTool *self)
{
	gth_image_viewer_page_reset (GTH_IMAGE_VIEWER_PAGE (gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self))));
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
gth_file_tool_rotate_finalize (GObject *object)
{
	GthFileToolRotate *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_ROTATE (object));

	self = (GthFileToolRotate *) object;

	cairo_surface_destroy (self->priv->image);
	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->settings);

	/* Chain up */
	G_OBJECT_CLASS (gth_file_tool_rotate_parent_class)->finalize (object);
}


static void
gth_file_tool_rotate_class_init (GthFileToolRotateClass *klass)
{
	GObjectClass                *gobject_class;
	GthFileToolClass            *file_tool_class;
	GthImageViewerPageToolClass *image_viewer_page_tool_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_rotate_finalize;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->get_options = gth_file_tool_rotate_get_options;
	file_tool_class->destroy_options = gth_file_tool_rotate_destroy_options;
	file_tool_class->apply_options = gth_file_tool_rotate_apply_options;
	file_tool_class->populate_headerbar = gth_file_tool_rotate_populate_headerbar;

	image_viewer_page_tool_class = (GthImageViewerPageToolClass *) klass;
	image_viewer_page_tool_class->reset_image = gth_file_tool_rotate_reset_image;
}


static void
gth_file_tool_rotate_init (GthFileToolRotate *self)
{
	self->priv = gth_file_tool_rotate_get_instance_private (self);
	self->priv->settings = g_settings_new (GTHUMB_ROTATE_SCHEMA);

	gth_file_tool_construct (GTH_FILE_TOOL (self), "image-rotate-symbolic", _("Rotate"), GTH_TOOLBOX_SECTION_ROTATION);
}
