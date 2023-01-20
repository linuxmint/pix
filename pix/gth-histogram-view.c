/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2010 The Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <math.h>
#include <cairo/cairo.h>
#include <glib/gi18n.h>
#include "glib-utils.h"
#include "gth-histogram-view.h"
#include "gth-enum-types.h"
#include "gtk-utils.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


/* Properties */
enum {
	PROP_0,
	PROP_HISTOGRAM,
	PROP_CURRENT_CHANNEL,
	PROP_DISPLAY_MODE,
	PROP_SCALE_TYPE
};

enum {
	CHANNEL_COLUMN_NAME,
	CHANNEL_COLUMN_SENSITIVE
};


struct _GthHistogramViewPrivate {
	GthHistogram        *histogram;
	gulong               histogram_changed_event;
	GthHistogramMode     display_mode;
	GthHistogramScale    scale_type;
	GthHistogramChannel  current_channel;
	guchar               selection_start;
	guchar               selection_end;
	GtkWidget           *view;
	GtkWidget           *linear_histogram_button;
	GtkWidget           *logarithmic_histogram_button;
	GtkWidget           *channel_combo_box;
	GtkBuilder          *builder;
	gboolean             selecting;
	guchar               tmp_selection_start;
	guchar               tmp_selection_end;
};


G_DEFINE_TYPE_WITH_CODE (GthHistogramView,
			 gth_histogram_view,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthHistogramView))


static void
gth_histogram_set_property (GObject      *object,
			    guint         property_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GthHistogramView *self;

        self = GTH_HISTOGRAM_VIEW (object);

	switch (property_id) {
	case PROP_HISTOGRAM:
		gth_histogram_view_set_histogram (self, g_value_get_object (value));
		break;
	case PROP_CURRENT_CHANNEL:
		gth_histogram_view_set_current_channel (self, g_value_get_enum (value));
		break;
	case PROP_DISPLAY_MODE:
		gth_histogram_view_set_display_mode (self, g_value_get_enum (value));
		break;
	case PROP_SCALE_TYPE:
		gth_histogram_view_set_scale_type (self, g_value_get_enum (value));
		break;
	default:
		break;
	}
}


static void
gth_histogram_get_property (GObject    *object,
		            guint       property_id,
		            GValue     *value,
		            GParamSpec *pspec)
{
	GthHistogramView *self;

        self = GTH_HISTOGRAM_VIEW (object);

	switch (property_id) {
	case PROP_HISTOGRAM:
		g_value_set_object (value, self->priv->histogram);
		break;
	case PROP_CURRENT_CHANNEL:
		g_value_set_int (value, self->priv->current_channel);
		break;
	case PROP_DISPLAY_MODE:
		g_value_set_enum (value, self->priv->display_mode);
		break;
	case PROP_SCALE_TYPE:
		g_value_set_enum (value, self->priv->scale_type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_histogram_view_finalize (GObject *obj)
{
	GthHistogramView *self;

	self = GTH_HISTOGRAM_VIEW (obj);

	if (self->priv->histogram_changed_event != 0)
		g_signal_handler_disconnect (self->priv->histogram, self->priv->histogram_changed_event);
	_g_object_unref (self->priv->histogram);
	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (gth_histogram_view_parent_class)->finalize (obj);
}


static void
gth_histogram_view_class_init (GthHistogramViewClass *klass)
{
	GObjectClass   *object_class;

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_histogram_set_property;
	object_class->get_property = gth_histogram_get_property;
	object_class->finalize = gth_histogram_view_finalize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_HISTOGRAM,
					 g_param_spec_object ("histogram",
							      "Histogram",
							      "The histogram to display",
							      GTH_TYPE_HISTOGRAM,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_CURRENT_CHANNEL,
					 g_param_spec_enum ("current-channel",
							    "Channel",
							    "The channel to display",
							    GTH_TYPE_HISTOGRAM_CHANNEL,
							    GTH_HISTOGRAM_CHANNEL_VALUE,
							    G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_DISPLAY_MODE,
					 g_param_spec_enum ("display-mode",
							    "Display mode",
							    "Whether to display a single channel or all the channels at the same time",
							    GTH_TYPE_HISTOGRAM_MODE,
							    GTH_HISTOGRAM_MODE_ONE_CHANNEL,
							    G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_SCALE_TYPE,
					 g_param_spec_enum ("scale-type",
							    "Scale",
							    "The scale type",
							    GTH_TYPE_HISTOGRAM_SCALE,
							    GTH_HISTOGRAM_SCALE_LOGARITHMIC,
							    G_PARAM_READWRITE));
}


static void
_gth_histogram_view_update_info (GthHistogramView *self)
{
	GthHistogramChannel  first_channel;
	GthHistogramChannel  last_channel;
	GthHistogramChannel  channel;
	guchar               start;
	guchar               end;
	goffset              total;
	goffset              pixels;
	double               mean;
	double               std_dev;
	double               sum;
	int                  median;
	double               max;
	int                  i;
	char                *s;

	if ((self->priv->histogram == NULL)
	    || ((int) self->priv->current_channel > gth_histogram_get_nchannels (self->priv->histogram)))
	{
		gtk_widget_set_sensitive (GET_WIDGET ("histogram_info"), FALSE);
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("mean_label")), "");
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("std_dev_label")), "");
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("median_label")), "");
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("total_label")), "");
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("selected_label")), "");
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("max_label")), "");

		return;
	}

	gtk_widget_set_sensitive (GET_WIDGET ("histogram_info"), TRUE);

	if (gth_histogram_view_get_display_mode (self) == GTH_HISTOGRAM_MODE_ALL_CHANNELS) {
		first_channel = GTH_HISTOGRAM_CHANNEL_RED;
		last_channel = GTH_HISTOGRAM_CHANNEL_BLUE;
	}
	else
		first_channel = last_channel = gth_histogram_view_get_current_channel (self);

	start = self->priv->selection_start;
	end = self->priv->selection_end;

	/* total */

	total = 0;
	for (channel = first_channel; channel <= last_channel; channel++)
		for (i = 0; i <= 255; i++)
			total += gth_histogram_get_value (self->priv->histogram, channel, i);

	/* mean / pixels */

	pixels = 0;
	mean = 0.0;
	for (channel = first_channel; channel <= last_channel; channel++) {
		for (i = start; i <= end; i++) {
			double v = gth_histogram_get_value (self->priv->histogram, channel, i);
			pixels += v;
			mean += v * i;
		}
	}
	mean = mean / pixels;

	/* std_dev */

	std_dev = 0.0;
	for (channel = first_channel; channel <= last_channel; channel++) {
		for (i = start; i <= end; i++) {
			double v = gth_histogram_get_value (self->priv->histogram, channel, i);
			std_dev += v * SQR (i - mean);
		}
	}
	std_dev = sqrt (std_dev / pixels);

	/* median */

	median = 0;
	sum = 0.0;
	for (i = start; i <= end; i++) {
		for (channel = first_channel; channel <= last_channel; channel++)
			sum += gth_histogram_get_value (self->priv->histogram, channel, i);
		if (sum * 2 > pixels) {
			median = i;
			break;
		}
	}

	if (gth_histogram_view_get_display_mode (self) == GTH_HISTOGRAM_MODE_ALL_CHANNELS) {
		max = MAX (gth_histogram_get_channel_max (self->priv->histogram, 1), gth_histogram_get_channel_max (self->priv->histogram, 2));
		max = MAX (max, gth_histogram_get_channel_max (self->priv->histogram, 3));
	}
	else
		max = gth_histogram_get_channel_max (self->priv->histogram, first_channel);

	s = g_strdup_printf ("%.1f", mean);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("mean_label")), s);
	g_free (s);

	s = g_strdup_printf ("%.1f", std_dev);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("std_dev_label")), s);
	g_free (s);

	s = g_strdup_printf ("%d", median);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("median_label")), s);
	g_free (s);

	s = g_strdup_printf ("%'" G_GOFFSET_FORMAT, total);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("total_label")), s);
	g_free (s);

	s = g_strdup_printf ("%.1f%%", ((double) pixels / total) * 100.0);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("selected_label")), s);
	g_free (s);

	s = g_strdup_printf ("%.1f%%", ((double) max / total) * 100.0);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("max_label")), s);
	g_free (s);
}


#define convert_to_scale(scale_type, value) (((scale_type) == GTH_HISTOGRAM_SCALE_LOGARITHMIC) ? log (value) : (value))


static void
_cairo_set_source_color_from_channel (cairo_t *cr,
				      int      channel)
{
	switch (channel) {
	case GTH_HISTOGRAM_CHANNEL_VALUE:
	default:
		cairo_set_source_rgba (cr, 0.8, 0.8, 0.8, 1.0);
		break;
	case GTH_HISTOGRAM_CHANNEL_RED:
		cairo_set_source_rgba (cr, 0.68, 0.18, 0.19, 1.0); /* #af2e31 */
		break;
	case GTH_HISTOGRAM_CHANNEL_GREEN:
		cairo_set_source_rgba (cr, 0.33, 0.78, 0.30, 1.0); /* #55c74d */
		break;
	case GTH_HISTOGRAM_CHANNEL_BLUE:
		cairo_set_source_rgba (cr, 0.13, 0.54, 0.8, 1.0); /* #238acc */
		break;
	case GTH_HISTOGRAM_CHANNEL_ALPHA:
		cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 1.0);
		break;
	}
}


static void
gth_histogram_paint_channel (GthHistogramView *self,
			     GtkStyleContext  *style_context,
			     cairo_t          *cr,
			     int               channel,
			     GtkAllocation    *allocation)
{
	double max;
	double step;
	int    i;

	if (channel > gth_histogram_get_nchannels (self->priv->histogram))
		return;

	cairo_save (cr);

	_cairo_set_source_color_from_channel (cr, channel);

	if (self->priv->display_mode == GTH_HISTOGRAM_MODE_ALL_CHANNELS)
		cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
	else
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	max = gth_histogram_get_channel_max (self->priv->histogram, channel);
	if (max > 0.0)
		max = convert_to_scale (self->priv->scale_type, max);
	else
		max = 1.0;

	step = (double) allocation->width / 256.0;
	cairo_set_line_width (cr, 0.5);
	for (i = 0; i <= 255; i++) {
		double value;
		int    y;

		value = gth_histogram_get_value (self->priv->histogram, channel, i);
		y = CLAMP ((int) (allocation->height * convert_to_scale (self->priv->scale_type, value)) / max, 0, allocation->height);

		cairo_rectangle (cr,
				 allocation->x + (i * step) + 0.5,
				 allocation->y + allocation->height - y + 0.5,
				 step,
				 y);
	}
	cairo_fill (cr);
	cairo_restore (cr);
}


static void
gth_histogram_paint_rgb (GthHistogramView *self,
			 GtkStyleContext  *style_context,
			 cairo_t          *cr,
			 GtkAllocation    *allocation)
{
	double max;
	double step;
	int    i;

	cairo_save (cr);

	max = MAX (gth_histogram_get_channel_max (self->priv->histogram, 1), gth_histogram_get_channel_max (self->priv->histogram, 2));
	max = MAX (max, gth_histogram_get_channel_max (self->priv->histogram, 3));
	if (max > 0.0)
		max = convert_to_scale (self->priv->scale_type, max);
	else
		max = 1.0;

	step = (double) allocation->width / 256.0;
	cairo_set_line_width (cr, 0.5);
	for (i = 0; i <= 255; i++) {
		double   value_r;
		double   value_g;
		double   value_b;
		int      min_c;
		int      mid_c;
		int      max_c;
		int      y;
		double   value;

		value_r = gth_histogram_get_value (self->priv->histogram, 1, i);
		value_g = gth_histogram_get_value (self->priv->histogram, 2, i);
		value_b = gth_histogram_get_value (self->priv->histogram, 3, i);

		/* find the channel with minimum value */

		if ((value_r <= value_g) && (value_r <= value_b))
			min_c = 1;
		else if ((value_g <= value_r) && (value_g <= value_b))
			min_c = 2;
		else
			min_c = 3;

		/* find the channel with maximum value */

		if ((value_r >= value_g) && (value_r >= value_b))
			max_c = 1;
		else if ((value_g >= value_r) && (value_g >= value_b))
			max_c = 2;
		else
			max_c = 3;

		/* find the channel with middle value */

		if (abs (max_c - min_c) == 1) {
			if ((max_c == 1) || (min_c == 1))
				mid_c = 3;
			else
				mid_c = 1;
		}
		else
			mid_c = 2;

		/* use the OVER operator for the maximum value */

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		_cairo_set_source_color_from_channel (cr, max_c);
		value = gth_histogram_get_value (self->priv->histogram, max_c, i);
		y = CLAMP ((int) (allocation->height * convert_to_scale (self->priv->scale_type, value)) / max, 0, allocation->height);
		cairo_rectangle (cr,
				 allocation->x + (i * step) + 0.5,
				 allocation->y + allocation->height - y + 0.5,
				 step,
				 y);
		cairo_fill (cr);

		/* use the ADD operator for the middle value */

		cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
		_cairo_set_source_color_from_channel (cr, mid_c);
		value = gth_histogram_get_value (self->priv->histogram, mid_c, i);
		y = CLAMP ((int) (allocation->height * convert_to_scale (self->priv->scale_type, value)) / max, 0, allocation->height);
		cairo_rectangle (cr,
				 allocation->x + (i * step) + 0.5,
				 allocation->y + allocation->height - y + 0.5,
				 step,
				 y);
		cairo_fill (cr);

		/* the minimum value is shared by all the channels and is
		 * painted in white if inside the selection, otherwise in black. */

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		if (((self->priv->selection_start > 0) || (self->priv->selection_end < 255))
		    && (i >= self->priv->selection_start) && (i <= self->priv->selection_end))
		{
			cairo_set_source_rgb (cr, 0.3, 0.3, 0.3);
		}
		else
			cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		value = gth_histogram_get_value (self->priv->histogram, min_c, i);
		y = CLAMP ((int) (allocation->height * convert_to_scale (self->priv->scale_type, value)) / max, 0, allocation->height);
		cairo_rectangle (cr,
				 allocation->x + (i * step) + 0.5,
				 allocation->y + allocation->height - y + 0.5,
				 step,
				 y);
		cairo_fill (cr);
	}

	cairo_restore (cr);
}


static void
gth_histogram_paint_grid (GthHistogramView *self,
			  GtkStyleContext  *style_context,
			  cairo_t          *cr,
			  GtkAllocation    *allocation)
{
	GdkRGBA color;
	double  grid_step;
	int     i;

	cairo_save (cr);
	gtk_style_context_get_border_color (style_context,
					    gtk_widget_get_state_flags (GTK_WIDGET (self)),
					    &color);
	gdk_cairo_set_source_rgba (cr, &color);
	cairo_set_line_width (cr, 0.5);

	grid_step = 256.0 / 5;
	for (i = 1; i < 5; i++) {
		int x;

		x = (i * grid_step) * ((double) allocation->width / 256.0);
		cairo_move_to (cr, allocation->x + x + 0.5, allocation->y);
		cairo_line_to (cr, allocation->x + x + 0.5, allocation->y + allocation->height);
	}

	cairo_stroke (cr);
	cairo_restore (cr);
}


static void
gth_histogram_paint_selection (GthHistogramView *self,
			       GtkStyleContext  *style_context,
			       cairo_t          *cr,
			       GtkAllocation    *allocation)
{
	GdkRGBA color;
	double  step;

	cairo_save (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_line_width (cr, 0.5);

	gtk_style_context_get_background_color (style_context, GTK_STATE_SELECTED | GTK_STATE_FOCUSED, &color);
	gdk_cairo_set_source_rgba (cr, &color);

	step = (double) allocation->width / 256.0;
	cairo_rectangle (cr,
			 allocation->x + self->priv->selection_start * step,
			 allocation->y,
			 (self->priv->selection_end - self->priv->selection_start) * step,
			 allocation->height);
	cairo_fill (cr);
	cairo_restore (cr);
}


static gboolean
histogram_view_draw_cb (GtkWidget *widget,
			cairo_t   *cr,
			gpointer   user_data)
{
	GthHistogramView *self = user_data;
	GtkAllocation     allocation;
	GtkStyleContext  *style_context;

	style_context = gtk_widget_get_style_context (widget);
	gtk_style_context_save (style_context);
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_VIEW);
	gtk_style_context_add_class (style_context, "histogram");

	gtk_widget_get_allocation (widget, &allocation);
	gtk_render_background (style_context, cr, 0, 0, allocation.width, allocation.height);

	if ((self->priv->histogram != NULL)
	    && ((int) self->priv->current_channel <= gth_histogram_get_nchannels (self->priv->histogram)))
	{
		GtkBorder     padding;
		GtkAllocation inner_allocation;

		cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);

		gtk_style_context_get_padding (style_context, gtk_widget_get_state_flags (widget), &padding);

		inner_allocation.x = padding.left;
		inner_allocation.y = padding.top;
		inner_allocation.width = allocation.width - (padding.right + padding.left);
		inner_allocation.height = allocation.height - (padding.top + padding.bottom);

		if ((self->priv->selection_start > 0) || (self->priv->selection_end < 255))
			gth_histogram_paint_selection (self, style_context, cr, &inner_allocation);

		gth_histogram_paint_grid (self, style_context, cr, &inner_allocation);

		if (self->priv->display_mode == GTH_HISTOGRAM_MODE_ALL_CHANNELS)
			gth_histogram_paint_rgb (self, style_context, cr, &inner_allocation);
		else
			gth_histogram_paint_channel (self, style_context, cr, self->priv->current_channel, &inner_allocation);
	}

	gtk_style_context_restore (style_context);

	return TRUE;
}


static gboolean
histogram_view_scroll_event_cb (GtkWidget      *widget,
				GdkEventScroll *event,
				gpointer        user_data)
{
	GthHistogramView *self = user_data;
	int               channel = 0;

	if (self->priv->histogram == NULL)
		return FALSE;

	if (event->direction == GDK_SCROLL_UP) {
		if (gth_histogram_view_get_display_mode (self)  == GTH_HISTOGRAM_MODE_ALL_CHANNELS)
			channel = gth_histogram_get_nchannels (self->priv->histogram);
		else
			channel = self->priv->current_channel - 1;
	}
	else if (event->direction == GDK_SCROLL_DOWN) {
		if (gth_histogram_view_get_display_mode (self)  == GTH_HISTOGRAM_MODE_ALL_CHANNELS)
			return TRUE;
		else
			channel = self->priv->current_channel + 1;
	}

	if (channel <= gth_histogram_get_nchannels (self->priv->histogram)) {
		gth_histogram_view_set_current_channel (self, CLAMP (channel, 0, GTH_HISTOGRAM_N_CHANNELS - 1));
		gth_histogram_view_set_display_mode (self, GTH_HISTOGRAM_MODE_ONE_CHANNEL);
	}
	else
		gth_histogram_view_set_display_mode (self, GTH_HISTOGRAM_MODE_ALL_CHANNELS);

	return TRUE;
}


static gboolean
histogram_view_button_press_event_cb (GtkWidget      *widget,
				      GdkEventButton *event,
				      gpointer        user_data)
{
	GthHistogramView *self = user_data;
	GtkAllocation     allocation;
	int               value;

	if (! gtk_widget_get_visible (GET_WIDGET ("histogram_info")))
		return TRUE;

	gtk_widget_get_allocation (self->priv->view, &allocation);
	value = CLAMP (event->x / allocation.width * 256 + .5, 0, 255);
	self->priv->selecting = TRUE;
	self->priv->tmp_selection_start = value;
	self->priv->tmp_selection_end = self->priv->tmp_selection_start;
	self->priv->selection_start = MIN (self->priv->tmp_selection_start, self->priv->tmp_selection_end);
	self->priv->selection_end = MAX (self->priv->tmp_selection_start, self->priv->tmp_selection_end);
	gtk_widget_queue_draw (self->priv->view);

	return TRUE;
}


static gboolean
histogram_view_button_release_event_cb (GtkWidget      *widget,
					GdkEventButton *event,
					gpointer        user_data)
{
	GthHistogramView *self = user_data;

	if (! self->priv->selecting)
		return TRUE;

	self->priv->selecting = FALSE;

	if (self->priv->selection_start == self->priv->selection_end) {
		self->priv->selection_start = 0;
		self->priv->selection_end = 255;
		gtk_widget_queue_draw (self->priv->view);
	}

	if (gtk_widget_get_visible (GET_WIDGET ("histogram_info")))
		_gth_histogram_view_update_info (self);

	return TRUE;
}


static gboolean
histogram_view_motion_notify_event_cb (GtkWidget      *widget,
                		       GdkEventMotion *event,
                		       gpointer        user_data)
{
	GthHistogramView *self = user_data;
	GtkAllocation     allocation;
	int               value;

	if (! self->priv->selecting)
		return TRUE;

	gtk_widget_get_allocation (self->priv->view, &allocation);
	value = CLAMP (event->x / allocation.width * 256 + .5, 0, 255);
	self->priv->tmp_selection_end = value;
	self->priv->selection_start = MIN (self->priv->tmp_selection_start, self->priv->tmp_selection_end);
	self->priv->selection_end = MAX (self->priv->tmp_selection_start, self->priv->tmp_selection_end);
	gtk_widget_queue_draw (self->priv->view);

	return TRUE;
}


static void
linear_histogram_button_toggled_cb (GtkToggleButton *button,
				    gpointer   user_data)
{
	GthHistogramView *self = user_data;

	if (gtk_toggle_button_get_active (button))
		gth_histogram_view_set_scale_type (GTH_HISTOGRAM_VIEW (self), GTH_HISTOGRAM_SCALE_LINEAR);
}


static void
logarithmic_histogram_button_toggled_cb (GtkToggleButton *button,
					 gpointer         user_data)
{
	GthHistogramView *self = user_data;

	if (gtk_toggle_button_get_active (button))
		gth_histogram_view_set_scale_type (GTH_HISTOGRAM_VIEW (self), GTH_HISTOGRAM_SCALE_LOGARITHMIC);
}


static void
channel_combo_box_changed_cb (GtkComboBox *combo_box,
			      gpointer     user_data)
{
	GthHistogramView *self = user_data;
	int               n_channel;

	n_channel = gtk_combo_box_get_active (combo_box);
	if (n_channel < GTH_HISTOGRAM_N_CHANNELS) {
		gth_histogram_view_set_current_channel (GTH_HISTOGRAM_VIEW (self), n_channel);
		gth_histogram_view_set_display_mode (GTH_HISTOGRAM_VIEW (self), GTH_HISTOGRAM_MODE_ONE_CHANNEL);
	}
	else
		gth_histogram_view_set_display_mode (GTH_HISTOGRAM_VIEW (self), GTH_HISTOGRAM_MODE_ALL_CHANNELS);
}


static void
self_notify_current_channel_cb (GObject    *gobject,
				GParamSpec *pspec,
				gpointer    user_data)
{
	GthHistogramView *self = user_data;

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->channel_combo_box), self->priv->current_channel);
}


static void
self_notify_display_mode_cb (GObject    *gobject,
			     GParamSpec *pspec,
			     gpointer    user_data)
{
	GthHistogramView *self = user_data;

	if (self->priv->display_mode == GTH_HISTOGRAM_MODE_ALL_CHANNELS)
		gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->channel_combo_box), GTH_HISTOGRAM_N_CHANNELS);
	else
		gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->channel_combo_box), self->priv->current_channel);
}


static void
self_notify_scale_type_cb (GObject    *gobject,
			   GParamSpec *pspec,
			   gpointer    user_data)
{
	GthHistogramView *self = user_data;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->linear_histogram_button), self->priv->scale_type == GTH_HISTOGRAM_SCALE_LINEAR);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->logarithmic_histogram_button), self->priv->scale_type == GTH_HISTOGRAM_SCALE_LOGARITHMIC);
}


static void
gth_histogram_view_init (GthHistogramView *self)
{
	GtkWidget       *topbar_box;
	GtkWidget       *sub_box;
	PangoAttrList   *attr_list;
	GtkWidget       *label;
	GtkWidget       *view_container;
	GtkListStore    *channel_model;
	GtkCellRenderer *renderer;
	GtkTreeIter      iter;

	self->priv = gth_histogram_view_get_instance_private (self);
	self->priv->histogram = NULL;
	self->priv->current_channel = GTH_HISTOGRAM_CHANNEL_VALUE;
	self->priv->display_mode = GTH_HISTOGRAM_MODE_ONE_CHANNEL;
	self->priv->scale_type = GTH_HISTOGRAM_SCALE_LINEAR;
	self->priv->builder = _gtk_builder_new_from_file ("histogram-info.ui", NULL);
	self->priv->selection_start = 0;
	self->priv->selection_end = 255;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
	gtk_box_set_spacing (GTK_BOX (self), 6);
	gtk_widget_set_vexpand (GTK_WIDGET (self), FALSE);

	/* topbar */

	topbar_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (topbar_box);

	/* linear / logarithmic buttons */

	sub_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (sub_box);
	gtk_box_pack_end (GTK_BOX (topbar_box), sub_box, FALSE, FALSE, 0);

	self->priv->linear_histogram_button = gtk_toggle_button_new ();
	gtk_widget_set_tooltip_text (self->priv->linear_histogram_button, _("Linear scale"));
	gtk_button_set_relief (GTK_BUTTON (self->priv->linear_histogram_button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (self->priv->linear_histogram_button), gtk_image_new_from_icon_name ("format-linear-symbolic", GTK_ICON_SIZE_MENU));
	gtk_widget_show_all (self->priv->linear_histogram_button);
	gtk_box_pack_start (GTK_BOX (sub_box), self->priv->linear_histogram_button, FALSE, FALSE, 0);

	g_signal_connect (self->priv->linear_histogram_button,
			  "toggled",
			  G_CALLBACK (linear_histogram_button_toggled_cb),
			  self);

	self->priv->logarithmic_histogram_button = gtk_toggle_button_new ();
	gtk_widget_set_tooltip_text (self->priv->logarithmic_histogram_button, _("Logarithmic scale"));
	gtk_button_set_relief (GTK_BUTTON (self->priv->logarithmic_histogram_button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (self->priv->logarithmic_histogram_button), gtk_image_new_from_icon_name ("format-logarithmic-symbolic", GTK_ICON_SIZE_MENU));
	gtk_widget_show_all (self->priv->logarithmic_histogram_button);
	gtk_box_pack_start (GTK_BOX (sub_box), self->priv->logarithmic_histogram_button, FALSE, FALSE, 0);

	g_signal_connect (self->priv->logarithmic_histogram_button,
			  "toggled",
			  G_CALLBACK (logarithmic_histogram_button_toggled_cb),
			  self);

	/* channel selector */

	sub_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (sub_box);
	gtk_box_pack_start (GTK_BOX (topbar_box), sub_box, FALSE, FALSE, 0);

	attr_list = pango_attr_list_new ();
	pango_attr_list_insert (attr_list, pango_attr_size_new (PANGO_SCALE * 8));

	label = gtk_label_new (_("Channel:"));
	gtk_label_set_attributes (GTK_LABEL (label), attr_list);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (sub_box), label, FALSE, FALSE, 0);
	channel_model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
	self->priv->channel_combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (channel_model));
	g_object_unref (channel_model);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "attributes", attr_list, NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->priv->channel_combo_box),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (self->priv->channel_combo_box),
			    	    	renderer,
			    	    	"text", CHANNEL_COLUMN_NAME,
			    	    	"sensitive", CHANNEL_COLUMN_SENSITIVE,
			    	    	NULL);

	gtk_list_store_append (channel_model, &iter);
	gtk_list_store_set (channel_model, &iter,
			    CHANNEL_COLUMN_NAME, _("Value"),
			    CHANNEL_COLUMN_SENSITIVE, TRUE,
			    -1);
	gtk_list_store_append (channel_model, &iter);
	gtk_list_store_set (channel_model, &iter,
			    CHANNEL_COLUMN_NAME, _("Red"),
			    CHANNEL_COLUMN_SENSITIVE, TRUE,
			    -1);
	gtk_list_store_append (channel_model, &iter);
	gtk_list_store_set (channel_model, &iter,
			    CHANNEL_COLUMN_NAME, _("Green"),
			    CHANNEL_COLUMN_SENSITIVE, TRUE,
			    -1);
	gtk_list_store_append (channel_model, &iter);
	gtk_list_store_set (channel_model, &iter,
			    CHANNEL_COLUMN_NAME, _("Blue"),
			    CHANNEL_COLUMN_SENSITIVE, TRUE,
			    -1);
	gtk_list_store_append (channel_model, &iter);
	gtk_list_store_set (channel_model, &iter,
			    CHANNEL_COLUMN_NAME, _("Alpha"),
			    CHANNEL_COLUMN_SENSITIVE, FALSE,
			    -1);
	gtk_list_store_append (channel_model, &iter);
	gtk_list_store_set (channel_model, &iter,
			    /* Translators: RGB is an acronym for Red Green Blue */
			    CHANNEL_COLUMN_NAME, _("RGB"),
			    CHANNEL_COLUMN_SENSITIVE, TRUE,
			    -1);

	gtk_widget_show (self->priv->channel_combo_box);
	gtk_box_pack_start (GTK_BOX (sub_box), self->priv->channel_combo_box, FALSE, FALSE, 0);

	g_signal_connect (self->priv->channel_combo_box,
			  "changed",
			  G_CALLBACK (channel_combo_box_changed_cb),
			  self);

	pango_attr_list_unref (attr_list);

	/* histogram view */

	view_container = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (view_container), GTK_SHADOW_IN);
	gtk_widget_set_vexpand (view_container, TRUE);
	gtk_widget_show (view_container);

	self->priv->view = gtk_drawing_area_new ();
	gtk_widget_add_events (self->priv->view, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_STRUCTURE_MASK);
	gtk_widget_show (self->priv->view);
	gtk_container_add (GTK_CONTAINER (view_container), self->priv->view);

	g_signal_connect (self->priv->view,
			  "draw",
			  G_CALLBACK (histogram_view_draw_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "scroll-event",
			  G_CALLBACK (histogram_view_scroll_event_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "button-press-event",
			  G_CALLBACK (histogram_view_button_press_event_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "button-release-event",
			  G_CALLBACK (histogram_view_button_release_event_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "motion-notify-event",
			  G_CALLBACK (histogram_view_motion_notify_event_cb),
			  self);

	/* histogram info */

	gtk_widget_set_vexpand (GET_WIDGET ("histogram_info"), FALSE);

	/* pack the widget */

	gtk_box_pack_start (GTK_BOX (self), topbar_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (self), view_container, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (self), GET_WIDGET ("histogram_info"), FALSE, FALSE, 0);

	/* update widgets when a property changes */

	g_signal_connect (self,
			  "notify::current-channel",
			  G_CALLBACK (self_notify_current_channel_cb),
			  self);
	g_signal_connect (self,
			  "notify::display-mode",
			  G_CALLBACK (self_notify_display_mode_cb),
			  self);
	g_signal_connect (self,
			  "notify::scale-type",
			  G_CALLBACK (self_notify_scale_type_cb),
			  self);

	/* default values */

	gth_histogram_view_set_display_mode (self, GTH_HISTOGRAM_MODE_ALL_CHANNELS);
	gth_histogram_view_set_scale_type (self, GTH_HISTOGRAM_SCALE_LINEAR);
}


GtkWidget *
gth_histogram_view_new (GthHistogram *histogram)
{
	return (GtkWidget *) g_object_new (GTH_TYPE_HISTOGRAM_VIEW, "histogram", histogram, NULL);
}


static void
update_sensitivity (GthHistogramView *self)
{
	gboolean     has_alpha;
	GtkTreePath *path;
	GtkTreeIter  iter;

	/* view */

	if ((self->priv->histogram == NULL)
	    || ((int) self->priv->current_channel > gth_histogram_get_nchannels (self->priv->histogram)))
	{
		gtk_widget_set_sensitive (self->priv->view, FALSE);
	}
	else
		gtk_widget_set_sensitive (self->priv->view, TRUE);

	/* channel combobox */

	has_alpha = (self->priv->histogram != NULL) && (gth_histogram_get_nchannels (self->priv->histogram) > 3);
	path = gtk_tree_path_new_from_indices (GTH_HISTOGRAM_CHANNEL_ALPHA, -1);
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (gtk_combo_box_get_model (GTK_COMBO_BOX (self->priv->channel_combo_box))),
				     &iter,
				     path))
	{
		gtk_list_store_set (GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (self->priv->channel_combo_box))),
				    &iter,
				    CHANNEL_COLUMN_SENSITIVE, has_alpha,
				    -1);
	}

	gtk_tree_path_free (path);
}


static void
histogram_changed_cb (GthHistogram *histogram,
		      gpointer      user_data)
{
	GthHistogramView *self = user_data;

	update_sensitivity (self);
	gtk_widget_queue_draw (GTK_WIDGET (self));

	if (gtk_widget_get_visible (GET_WIDGET ("histogram_info")))
		_gth_histogram_view_update_info (self);
}


void
gth_histogram_view_set_histogram (GthHistogramView *self,
				  GthHistogram     *histogram)
{
	g_return_if_fail (GTH_IS_HISTOGRAM_VIEW (self));

	if (self->priv->histogram == histogram)
		return;

	if (self->priv->histogram != NULL) {
		g_signal_handler_disconnect (self->priv->histogram, self->priv->histogram_changed_event);
		_g_object_unref (self->priv->histogram);
		self->priv->histogram_changed_event = 0;
		self->priv->histogram = NULL;
	}

	if (histogram != NULL) {
		self->priv->histogram = g_object_ref (histogram);
		self->priv->histogram_changed_event = g_signal_connect (self->priv->histogram, "changed", G_CALLBACK (histogram_changed_cb), self);
	}

	g_object_notify (G_OBJECT (self), "histogram");

	update_sensitivity (self);
	if (gtk_widget_get_visible (GET_WIDGET ("histogram_info")))
		_gth_histogram_view_update_info (self);
}


GthHistogram *
gth_histogram_view_get_histogram (GthHistogramView *self)
{
	g_return_val_if_fail (GTH_IS_HISTOGRAM_VIEW (self), NULL);
	return self->priv->histogram;
}


void
gth_histogram_view_set_current_channel (GthHistogramView *self,
					int               n_channel)
{
	g_return_if_fail (GTH_IS_HISTOGRAM_VIEW (self));

	if (n_channel == self->priv->current_channel)
		return;

	self->priv->current_channel = CLAMP (n_channel, 0, GTH_HISTOGRAM_N_CHANNELS);
	g_object_notify (G_OBJECT (self), "current-channel");

	gtk_widget_queue_draw (GTK_WIDGET (self));

	if (gtk_widget_get_visible (GET_WIDGET ("histogram_info")))
		_gth_histogram_view_update_info (self);
}


gint
gth_histogram_view_get_current_channel (GthHistogramView *self)
{
	g_return_val_if_fail (GTH_IS_HISTOGRAM_VIEW (self), 0);
	return self->priv->current_channel;
}


void
gth_histogram_view_set_display_mode (GthHistogramView *self,
				     GthHistogramMode  mode)
{
	g_return_if_fail (GTH_IS_HISTOGRAM_VIEW (self));

	if (mode == self->priv->display_mode)
		return;

	self->priv->display_mode = mode;
	g_object_notify (G_OBJECT (self), "display-mode");

	gtk_widget_queue_draw (GTK_WIDGET (self));

	if (gtk_widget_get_visible (GET_WIDGET ("histogram_info")))
		_gth_histogram_view_update_info (self);
}


GthHistogramMode
gth_histogram_view_get_display_mode (GthHistogramView *self)
{
	g_return_val_if_fail (GTH_IS_HISTOGRAM_VIEW (self), 0);
	return self->priv->display_mode;
}


void
gth_histogram_view_set_scale_type (GthHistogramView  *self,
				   GthHistogramScale  scale_type)
{
	g_return_if_fail (GTH_IS_HISTOGRAM_VIEW (self));

	self->priv->scale_type = scale_type;
	g_object_notify (G_OBJECT (self), "scale-type");

	gtk_widget_queue_draw (GTK_WIDGET (self));
}


GthHistogramScale
gth_histogram_view_get_scale_type (GthHistogramView *self)
{
	g_return_val_if_fail (GTH_IS_HISTOGRAM_VIEW (self), 0);
	return self->priv->scale_type;
}


void
gth_histogram_view_set_selection (GthHistogramView *self,
				  guchar            start,
				  guchar            end)
{
	g_return_if_fail (GTH_IS_HISTOGRAM_VIEW (self));

	self->priv->selection_start = start;
	self->priv->selection_end = end;

	if (gtk_widget_get_visible (GET_WIDGET ("histogram_info")))
		_gth_histogram_view_update_info (self);
}


void
gth_histogram_view_show_info (GthHistogramView *self,
			      gboolean          show_info)
{
	if (show_info)
		gtk_widget_show (GET_WIDGET ("histogram_info"));
	else
		gtk_widget_hide (GET_WIDGET ("histogram_info"));
}
