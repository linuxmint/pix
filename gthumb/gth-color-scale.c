/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include "cairo-utils.h"
#include "gth-color-scale.h"
#include "gth-enum-types.h"


#define _GTK_STYLE_CLASS_COLOR "color"


enum {
        PROP_0,
        PROP_SCALE_TYPE
};


struct _GthColorScalePrivate {
	GthColorScaleType      scale_type;
	cairo_surface_t       *surface;
	int                    width;
	int                    height;
	cairo_rectangle_int_t  slider;
	gboolean               update_slider;
	gboolean               mouse_on_slider;
	gboolean               dragging_slider;
	GtkAdjustment         *adj;
};


G_DEFINE_TYPE (GthColorScale, gth_color_scale, GTK_TYPE_SCALE)


static void
gth_color_scale_finalize (GObject *object)
{
	GthColorScale *self;

	g_return_if_fail (GTH_IS_COLOR_SCALE (object));

	self = GTH_COLOR_SCALE (object);
	cairo_surface_destroy (self->priv->surface);
	if (self->priv->adj != NULL)
		g_object_unref (self->priv->adj);

	G_OBJECT_CLASS (gth_color_scale_parent_class)->finalize (object);
}


static void
gth_color_scale_set_property (GObject      *object,
			      guint         property_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GthColorScale *self;

	self = GTH_COLOR_SCALE (object);

	switch (property_id) {
	case PROP_SCALE_TYPE:
		self->priv->scale_type = g_value_get_enum (value);
		break;
	default:
		break;
	}
}


static void
gth_color_scale_get_property (GObject    *object,
			      guint       property_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	GthColorScale *self;

	self = GTH_COLOR_SCALE (object);

	switch (property_id) {
	case PROP_SCALE_TYPE:
		g_value_set_enum (value, self->priv->scale_type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
_gth_color_scale_get_surface_size (GthColorScale *self,
				   int           *x_out,
				   int           *y_out,
				   int           *width_out,
				   int           *height_out,
				   int           *slider_width_out,
				   int           *slider_length_out,
				   int           *slider_spacing_out)
{
	GtkWidget             *widget = GTK_WIDGET (self);
	int                    focus_line_width;
	int                    focus_padding;
	int                    slider_width;
	int                    slider_length;
	cairo_rectangle_int_t  bounding_box;
	cairo_rectangle_int_t  trough_rect;

	gtk_widget_style_get (widget,
			      "focus-line-width", &focus_line_width,
			      "focus-padding", &focus_padding,
			      "slider-width", &slider_width,
			      "slider-length", &slider_length,
			      NULL);

	bounding_box.width = gtk_widget_get_allocated_width (widget) - 2 * (focus_line_width + focus_padding);
	bounding_box.height = gtk_widget_get_allocated_height (widget) - 2 * (focus_line_width + focus_padding);
	bounding_box.x = focus_line_width + focus_padding;
	bounding_box.y = focus_line_width + focus_padding;

	if (gtk_orientable_get_orientation (GTK_ORIENTABLE (widget)) == GTK_ORIENTATION_HORIZONTAL) {
		trough_rect.x = bounding_box.x;
		trough_rect.width = bounding_box.width;
		trough_rect.height = bounding_box.height / 2;
		trough_rect.y = bounding_box.y;
	}
	else {
		trough_rect.y = bounding_box.y;
		trough_rect.height = bounding_box.height;
		trough_rect.width = bounding_box.width / 2;
		trough_rect.x = bounding_box.x;
	}

	if (x_out) *x_out = trough_rect.x;
	if (y_out) *y_out = trough_rect.y;
	if (width_out) *width_out = trough_rect.width;
	if (height_out) *height_out = trough_rect.height;
	if (slider_width_out) *slider_width_out = slider_width;
	if (slider_length_out) *slider_length_out = slider_length;
	if (slider_spacing_out) *slider_spacing_out = focus_line_width + focus_padding;
}


static void
_gth_color_scale_update_surface (GthColorScale *self)
{
	int              width;
	int              height;
	cairo_pattern_t *pattern;
	cairo_t         *cr;

	if (! gtk_widget_get_realized (GTK_WIDGET (self)))
		return;

	if (self->priv->scale_type == GTH_COLOR_SCALE_DEFAULT)
		return;

	_gth_color_scale_get_surface_size (self, NULL, NULL, &width, &height, NULL, NULL, NULL);

	if ((self->priv->surface != NULL)
	    && (self->priv->width == width)
	    && (self->priv->height == height))
	{
		return;
	}

	cairo_surface_destroy (self->priv->surface);
	self->priv->surface = NULL;

	pattern = cairo_pattern_create_linear (0.0, 0.0, width, 0.0);

	switch (self->priv->scale_type) {
	case GTH_COLOR_SCALE_DEFAULT:
		g_assert_not_reached ();
		break;

	case GTH_COLOR_SCALE_WHITE_BLACK:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 1.0, 1.0, 1.0);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 0.0, 0.0, 0.0);
		break;

	case GTH_COLOR_SCALE_BLACK_WHITE:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 0.0, 0.0, 0.0);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 1.0, 1.0, 1.0);
		break;

	case GTH_COLOR_SCALE_GRAY_BLACK:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 0.5, 0.5, 0.5);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 0.0, 0.0, 0.0);
		break;

	case GTH_COLOR_SCALE_GRAY_WHITE:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 0.5, 0.5, 0.5);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 1.0, 1.0, 1.0);
		break;

	case GTH_COLOR_SCALE_CYAN_RED:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 0.0, 1.0, 1.0);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 1.0, 0.0, 0.0);
		break;

	case GTH_COLOR_SCALE_MAGENTA_GREEN:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 1.0, 0.0, 1.0);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 0.0, 1.0, 0.0);
		break;

	case GTH_COLOR_SCALE_YELLOW_BLUE:
		cairo_pattern_add_color_stop_rgb (pattern, 0.0, 1.0, 1.0, 0.0);
		cairo_pattern_add_color_stop_rgb (pattern, 1.0, 0.0, 0.0, 1.0);
		break;
	}

	self->priv->surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height);
	cr = cairo_create (self->priv->surface);
	cairo_set_source (cr, pattern);
	cairo_rectangle (cr, 0, 0, width, height);
	cairo_paint (cr);

	cairo_pattern_destroy (pattern);
	cairo_destroy (cr);
}


static void
_gth_color_scale_update_slider_position (GthColorScale *self)
{
	int focus_line_width;
	int focus_padding;
	int slider_width;
	int slider_length;
	int slider_start;
	int slider_end;

	if (! self->priv->update_slider)
		return;

	gtk_widget_style_get (GTK_WIDGET (self),
			      "focus-line-width", &focus_line_width,
			      "focus-padding", &focus_padding,
			      "slider-width", &slider_width,
			      "slider-length", &slider_length,
			      NULL);
	gtk_range_get_slider_range (GTK_RANGE (self), &slider_start, &slider_end);

	if (gtk_orientable_get_orientation (GTK_ORIENTABLE (self)) == GTK_ORIENTATION_HORIZONTAL) {
		self->priv->slider.x = slider_start;
		self->priv->slider.y = focus_line_width + focus_padding;
		self->priv->slider.width = slider_length;
		self->priv->slider.height = slider_width;
	}

	self->priv->update_slider = FALSE;
}


static gboolean
gth_color_scale_draw (GtkWidget *widget,
		      cairo_t   *cr)
{
	GthColorScale         *self;
	cairo_rectangle_int_t  surface_rect;
	cairo_pattern_t       *pattern;
	GtkStyleContext       *context;
	GtkOrientation         orientation;
	GdkRectangle           range_rect;
	int                    slider_width;
	int                    slider_length;
	int                    slider_start;
	int                    slider_end;
	int                    slider_spacing;
	GtkStateFlags          widget_state;
	GtkStateFlags          slider_state;

	self = GTH_COLOR_SCALE (widget);

	if (self->priv->scale_type == GTH_COLOR_SCALE_DEFAULT) {
		GTK_WIDGET_CLASS (gth_color_scale_parent_class)->draw (widget, cr);
		return FALSE;
	}

	context = gtk_widget_get_style_context (widget);
	orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (widget));
	widget_state = gtk_widget_get_state_flags (widget);
	gtk_range_get_range_rect (GTK_RANGE (self), &range_rect);
	gtk_range_get_slider_range (GTK_RANGE (self), &slider_start, &slider_end);

	/* background */

	_gth_color_scale_update_surface (self);
	_gth_color_scale_get_surface_size (self,
					   &surface_rect.x,
					   &surface_rect.y,
					   &surface_rect.width,
					   &surface_rect.height,
					   &slider_width,
					   &slider_length,
					   &slider_spacing);

	cairo_save (cr);
	cairo_translate (cr, surface_rect.x, surface_rect.y);
	pattern = cairo_pattern_create_for_surface (self->priv->surface);
	cairo_rectangle (cr, 0.0, 0.0, surface_rect.width, surface_rect.height);
	if ((orientation == GTK_ORIENTATION_HORIZONTAL)
	    && (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL))
	{
		cairo_matrix_t matrix;

		cairo_matrix_init_scale (&matrix, -1, 1);
		cairo_matrix_translate (&matrix, -surface_rect.width, 0);
		cairo_pattern_set_matrix (pattern, &matrix);
	}
	cairo_set_source (cr, pattern);
	cairo_fill (cr);
	cairo_pattern_destroy (pattern);
	cairo_restore (cr);

	/* focus */

	if (! (widget_state & GTK_STATE_FLAG_INSENSITIVE) && gtk_widget_has_visible_focus (widget))
	          gtk_render_focus (context, cr,
	                            range_rect.x,
	                            range_rect.y,
	                            range_rect.width,
	                            range_rect.height);

	/* slider */

	slider_state = widget_state;
	slider_state &= ~(GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_ACTIVE);
	if (self->priv->mouse_on_slider && ! (slider_state & GTK_STATE_FLAG_INSENSITIVE))
		slider_state |= GTK_STATE_FLAG_PRELIGHT;
	if (self->priv->dragging_slider)
		slider_state |= GTK_STATE_FLAG_ACTIVE;

	_gth_color_scale_update_slider_position (self);

	gtk_style_context_save (context);
	gtk_style_context_add_class (context, GTK_STYLE_CLASS_SLIDER);
	gtk_style_context_set_state (context, slider_state);
	gtk_render_slider (context,
			   cr,
			   self->priv->slider.x,
			   self->priv->slider.y,
			   self->priv->slider.width,
			   self->priv->slider.height,
			   orientation);
	gtk_style_context_restore (context);

	return FALSE;
}


static gboolean
gth_color_scale_motion_notify_event (GtkWidget      *widget,
				     GdkEventMotion *event)
{
	GthColorScale *self = GTH_COLOR_SCALE (widget);

	GTK_WIDGET_CLASS (gth_color_scale_parent_class)->motion_notify_event (widget, event);
	_gth_color_scale_update_slider_position (self);
	self->priv->mouse_on_slider = _cairo_rectangle_contains_point (&self->priv->slider, event->x, event->y);

	return FALSE;
}


static gboolean
gth_color_scale_button_press_event (GtkWidget      *widget,
				    GdkEventButton *event)
{
	GthColorScale *self = GTH_COLOR_SCALE (widget);

	GTK_WIDGET_CLASS (gth_color_scale_parent_class)->button_press_event (widget, event);
	self->priv->dragging_slider = self->priv->mouse_on_slider;

	return FALSE;
}


static gboolean
gth_color_scale_button_release_event (GtkWidget      *widget,
				      GdkEventButton *event)
{
	GthColorScale *self = GTH_COLOR_SCALE (widget);

	GTK_WIDGET_CLASS (gth_color_scale_parent_class)->button_release_event (widget, event);
	self->priv->dragging_slider = FALSE;

	return FALSE;
}


static void
gth_color_scale_size_allocate (GtkWidget     *widget,
			       GtkAllocation *allocation)
{
	GthColorScale *self = GTH_COLOR_SCALE (widget);

	GTK_WIDGET_CLASS (gth_color_scale_parent_class)->size_allocate (widget, allocation);
	self->priv->update_slider = TRUE;
}


static void
gth_color_scale_class_init (GthColorScaleClass *class)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	g_type_class_add_private (class, sizeof (GthColorScalePrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = gth_color_scale_set_property;
	object_class->get_property = gth_color_scale_get_property;
	object_class->finalize = gth_color_scale_finalize;

	widget_class = GTK_WIDGET_CLASS (class);
	widget_class->draw = gth_color_scale_draw;
	widget_class->motion_notify_event = gth_color_scale_motion_notify_event;
	widget_class->button_press_event = gth_color_scale_button_press_event;
	widget_class->button_release_event = gth_color_scale_button_release_event;
	widget_class->size_allocate = gth_color_scale_size_allocate;

	g_object_class_install_property (object_class,
					 PROP_SCALE_TYPE,
					 g_param_spec_enum ("scale-type",
							    "Scale Type",
							    "The type of scale",
							    GTH_TYPE_COLOR_SCALE_TYPE,
							    GTH_COLOR_SCALE_DEFAULT,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


static void
adjustment_changed_cb (GtkRange *range,
		       gpointer  user_data)
{
	GthColorScale *self = user_data;
	self->priv->update_slider = TRUE;
}


static void
notify_adjustment_cb (GObject    *gobject,
		      GParamSpec *pspec,
		      gpointer    user_data)
{
	GthColorScale *self = user_data;
	GtkAdjustment *adj;

	if (self->priv->adj != NULL) {
		g_signal_handlers_disconnect_by_data (self->priv->adj, self);
		g_object_unref (self->priv->adj);
		self->priv->adj = NULL;
	}

	adj = gtk_range_get_adjustment (GTK_RANGE (self));
	if (adj == NULL)
		return;

	self->priv->adj = g_object_ref (adj);

	g_signal_connect (self->priv->adj,
			  "value-changed",
			  G_CALLBACK (adjustment_changed_cb),
			  self);
	g_signal_connect (self->priv->adj,
			  "changed",
			  G_CALLBACK (adjustment_changed_cb),
			  self);
}


static void
gth_color_scale_init (GthColorScale *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_COLOR_SCALE, GthColorScalePrivate);
	self->priv->surface = NULL;
	self->priv->width = -1;
	self->priv->height = -1;
	self->priv->slider.x = 0;
	self->priv->slider.y = 0;
	self->priv->slider.width = 0;
	self->priv->slider.height = 0;
	self->priv->update_slider = TRUE;
	self->priv->adj = NULL;

	g_signal_connect (self,
			  "notify::adjustment",
			  G_CALLBACK (notify_adjustment_cb),
			  self);

	if (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL)
		gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)),
					     GTK_STYLE_CLASS_SCALE_HAS_MARKS_BELOW);
	else
		gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)),
					     GTK_STYLE_CLASS_SCALE_HAS_MARKS_ABOVE);

	if (gtk_orientable_get_orientation (GTK_ORIENTABLE (self)) == GTK_ORIENTATION_HORIZONTAL)
		gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)),
					     GTK_STYLE_CLASS_HORIZONTAL);
	else
		gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)),
					     GTK_STYLE_CLASS_VERTICAL);
}


GtkWidget *
gth_color_scale_new (GtkAdjustment      *adjustment,
		     GthColorScaleType   scale_type)
{
	return g_object_new (GTH_TYPE_COLOR_SCALE,
			     "adjustment", adjustment,
			     "scale-type", scale_type,
			     NULL);
}


/* -- gth_color_scale_label_new -- */


typedef struct {
	GtkWidget *value_label;
	char      *format;
} ScaleData;


static void
scale_data_free (gpointer user_data)
{
	ScaleData *scale_data = user_data;

	g_free (scale_data->format);
	g_free (scale_data);
}


static void
scale_value_changed (GtkAdjustment *adjustment,
		     gpointer       user_data)
{
	ScaleData *scale_data = user_data;
	double     num;
	char      *value;
	char      *markup;

	num = gtk_adjustment_get_value (adjustment);
	value = g_strdup_printf (scale_data->format, num);
	if ((num == 0.0) && ((value[0] == '-') || (value[0] == '+')))
		value[0] = ' ';
	markup = g_strdup_printf ("<small>%s</small>", value);
	gtk_label_set_markup (GTK_LABEL (scale_data->value_label), markup);

	g_free (markup);
	g_free (value);
}


GtkAdjustment *
gth_color_scale_label_new (GtkWidget         *parent_box,
			   GtkLabel          *related_label,
			   GthColorScaleType  scale_type,
			   float              value,
			   float              lower,
			   float              upper,
			   float              step_increment,
			   float              page_increment,
			   const char        *format)
{
	ScaleData     *scale_data;
	GtkAdjustment *adj;
	GtkWidget     *scale;
	GtkWidget     *hbox;

	adj = gtk_adjustment_new (value, lower, upper,
				  step_increment, page_increment,
				  0.0);

	scale_data = g_new (ScaleData, 1);
	scale_data->format = g_strdup (format);
	scale_data->value_label = gtk_label_new ("0");
	g_object_set_data_full (G_OBJECT (adj), "gth-scale-data", scale_data, scale_data_free);

	gtk_label_set_width_chars (GTK_LABEL (scale_data->value_label), 3);
	gtk_misc_set_alignment (GTK_MISC (scale_data->value_label), 1.0, 0.5);
	gtk_widget_show (scale_data->value_label);
	g_signal_connect (adj,
			  "value-changed",
			  G_CALLBACK (scale_value_changed),
			  scale_data);

	scale = gth_color_scale_new (adj, scale_type);
	gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_RIGHT);
	gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
	gtk_widget_show (scale);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), scale_data->value_label, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (parent_box), hbox, TRUE, TRUE, 0);

	if (related_label != NULL)
		gtk_label_set_mnemonic_widget (related_label, scale);

	scale_value_changed (adj, scale_data);

	return adj;
}
