/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2014 The Free Software Foundation, Inc.
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
#include "gth-curve.h"
#include "gth-curve-editor.h"
#include "gth-enum-types.h"
#include "gth-points.h"


/* Properties */
enum {
	PROP_0,
	PROP_HISTOGRAM,
	PROP_CURRENT_CHANNEL,
	PROP_SCALE_TYPE
};

enum {
	CHANNEL_COLUMN_NAME,
	CHANNEL_COLUMN_SENSITIVE
};


/* Signals */
enum {
	CHANGED,
	LAST_SIGNAL
};


struct _GthCurveEditorPrivate {
	GthHistogram        *histogram;
	gulong               histogram_changed_event;
	GthHistogramScale    scale_type;
	GthHistogramChannel  current_channel;
	GtkWidget           *view;
	GtkWidget           *linear_histogram_button;
	GtkWidget           *logarithmic_histogram_button;
	GtkWidget           *channel_combo_box;
	GthCurve            *curve[GTH_HISTOGRAM_N_CHANNELS];
	GthPoint            *active_point;
	int                  active_point_lower_limit;
	int                  active_point_upper_limit;
	GthPoint             cursor;
	gboolean             dragging;
	gboolean             paint_position;
};


static guint gth_curve_editor_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthCurveEditor,
			 gth_curve_editor,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthCurveEditor))


static void
gth_curve_editor_set_property (GObject      *object,
			       guint         property_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
	GthCurveEditor *self;

        self = GTH_CURVE_EDITOR (object);

	switch (property_id) {
	case PROP_HISTOGRAM:
		gth_curve_editor_set_histogram (self, g_value_get_object (value));
		break;
	case PROP_CURRENT_CHANNEL:
		gth_curve_editor_set_current_channel (self, g_value_get_enum (value));
		break;
	case PROP_SCALE_TYPE:
		gth_curve_editor_set_scale_type (self, g_value_get_enum (value));
		break;
	default:
		break;
	}
}


static void
gth_curve_editor_get_property (GObject    *object,
			       guint       property_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
	GthCurveEditor *self;

        self = GTH_CURVE_EDITOR (object);

	switch (property_id) {
	case PROP_HISTOGRAM:
		g_value_set_object (value, self->priv->histogram);
		break;
	case PROP_CURRENT_CHANNEL:
		g_value_set_int (value, self->priv->current_channel);
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
gth_curve_editor_finalize (GObject *obj)
{
	GthCurveEditor *self;
	int             c;

	self = GTH_CURVE_EDITOR (obj);

	if (self->priv->histogram_changed_event != 0)
		g_signal_handler_disconnect (self->priv->histogram, self->priv->histogram_changed_event);
	_g_object_unref (self->priv->histogram);

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		_g_object_unref (self->priv->curve[c]);

	G_OBJECT_CLASS (gth_curve_editor_parent_class)->finalize (obj);
}


static void
gth_curve_editor_class_init (GthCurveEditorClass *klass)
{
	GObjectClass   *object_class;

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_curve_editor_set_property;
	object_class->get_property = gth_curve_editor_get_property;
	object_class->finalize = gth_curve_editor_finalize;

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
					 PROP_SCALE_TYPE,
					 g_param_spec_enum ("scale-type",
							    "Scale",
							    "The scale type",
							    GTH_TYPE_HISTOGRAM_SCALE,
							    GTH_HISTOGRAM_SCALE_LOGARITHMIC,
							    G_PARAM_READWRITE));

	/* signals */

	gth_curve_editor_signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthCurveEditorClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}


#define convert_to_scale(scale_type, value) (((scale_type) == GTH_HISTOGRAM_SCALE_LOGARITHMIC) ? log (value) : (value))
#define HISTOGRAM_TRANSPARENCY 0.20


static void
_cairo_set_source_color_from_channel (cairo_t *cr,
				      int      channel,
				      double   alpha)
{
	switch (channel) {
	case GTH_HISTOGRAM_CHANNEL_VALUE:
	default:
		cairo_set_source_rgba (cr, 0.8, 0.8, 0.8, alpha);
		break;
	case GTH_HISTOGRAM_CHANNEL_RED:
		cairo_set_source_rgba (cr, 0.68, 0.18, 0.19, alpha); /* #af2e31 */
		break;
	case GTH_HISTOGRAM_CHANNEL_GREEN:
		cairo_set_source_rgba (cr, 0.33, 0.78, 0.30, alpha); /* #55c74d */
		break;
	case GTH_HISTOGRAM_CHANNEL_BLUE:
		cairo_set_source_rgba (cr, 0.13, 0.54, 0.8, alpha); /* #238acc */
		break;
	case GTH_HISTOGRAM_CHANNEL_ALPHA:
		cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, alpha);
		break;
	}
}


static void
gth_histogram_paint_channel (GthCurveEditor   *self,
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

	_cairo_set_source_color_from_channel (cr, channel, HISTOGRAM_TRANSPARENCY);

	cairo_save (cr);
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


#define GRID_LINES 4


static void
gth_histogram_paint_grid (GthCurveEditor  *self,
			  GtkStyleContext *style_context,
			  cairo_t         *cr,
			  GtkAllocation   *allocation)
{
	GdkRGBA color;
	double  grid_step;
	int     i;

	cairo_save (cr);
	gtk_style_context_get_border_color (style_context,
					    gtk_widget_get_state_flags (GTK_WIDGET (self)),
					    &color);
	cairo_set_line_width (cr, 0.5);

	grid_step = (double) allocation->width / GRID_LINES;
	for (i = 0; i <= GRID_LINES; i++) {
		int ofs = round (grid_step * i);

		cairo_set_source_rgba (cr, color.red, color.green, color.blue, (i == 4) ? 1.0 : 0.5);
		cairo_move_to (cr, allocation->x + ofs + 0.5, allocation->y);
		cairo_line_to (cr, allocation->x + ofs + 0.5, allocation->y + allocation->height);
		cairo_stroke (cr);
	}

	grid_step = (double) allocation->height / GRID_LINES;
	for (i = 0; i <= GRID_LINES; i++) {
		int ofs = round (grid_step * i);

		cairo_set_source_rgba (cr, color.red, color.green, color.blue, (i == 4) ? 1.0 : 0.5);
		cairo_move_to (cr, allocation->x + 0.5, allocation->y + ofs + 0.5);
		cairo_line_to (cr, allocation->x + allocation->width + 0.5, allocation->y + ofs + 0.5);
		cairo_stroke (cr);
	}

	/* diagonal */

	cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);
	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgba (cr, color.red, color.green, color.blue, 0.5);
	cairo_move_to (cr, allocation->x + 0.5, allocation->y + allocation->height + 0.5);
	cairo_line_to (cr, allocation->x + allocation->width+ 0.5, allocation->y + 0.5);
	cairo_stroke (cr);

	cairo_restore (cr);
}


static void
gth_histogram_paint_points (GthCurveEditor  *self,
			    GtkStyleContext *style_context,
			    cairo_t         *cr,
			    GthPoints       *points,
			    GtkAllocation   *allocation)
{
	double x_scale, y_scale;
	int    i;

	cairo_save (cr);
	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);

	x_scale = (double) allocation->width / 255;
	y_scale = (double) allocation->height / 255;
	for (i = 0; i < points->n; i++) {
		double x = points->p[i].x;
		double y = points->p[i].y;

		cairo_arc (cr,
			   round (allocation->x + (x * x_scale)),
			   round (allocation->y + allocation->height - (y * y_scale)),
			   3.5,
			   0.0,
			   2 * M_PI);
		if (&points->p[i] == self->priv->active_point)
			cairo_fill_preserve (cr);
		cairo_stroke (cr);
	}

	cairo_restore (cr);
}


static void
gth_histogram_paint_curve (GthCurveEditor  *self,
			   GtkStyleContext *style_context,
			   cairo_t         *cr,
			   GthCurve        *spline,
			   GtkAllocation   *allocation)
{
	double x_scale, y_scale;
	double i;

	cairo_save (cr);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);
	cairo_set_line_width (cr, 1.0);

	x_scale = (double) allocation->width / 255;
	y_scale = (double) allocation->height / 255;
	for (i = 0; i <= 256; i += 1) {
		int    j;
		double x, y;

		j = gth_curve_eval (spline, i);
		x = allocation->x + (i * x_scale);
		y = allocation->y + allocation->height - (j * y_scale);

		if (i == 0)
			cairo_move_to (cr, x, y);
		else
			cairo_line_to (cr, x, y);
	}
	cairo_stroke (cr);
	cairo_restore (cr);
}


static void
gth_histogram_paint_point_position (GthCurveEditor  *self,
				    GtkStyleContext *style_context,
				    cairo_t         *cr,
				    GthPoint        *point,
				    GtkAllocation   *allocation)
{
	char                 *text;
	cairo_text_extents_t  extents;
	int                   top = 9, left = 9, padding = 3;

	if (point->x < 0 || point->y < 0)
		return;

	cairo_save (cr);

	/* Translators: the first number is converted to the second number */
	text = g_strdup_printf (_("%d → %d"), (int) point->x, (int) point->y);
	cairo_text_extents (cr, text, &extents);

	cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.5);
	cairo_rectangle (cr, left - padding, top - padding, extents.width + 2*padding, extents.height + 2*padding);
	cairo_fill (cr);

	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
	cairo_move_to (cr, left - extents.x_bearing, top - extents.y_bearing);
	cairo_show_text (cr, text);
	g_free (text);

	cairo_restore (cr);
}


static void
gth_curve_editor_get_graph_area (GthCurveEditor *self,
			         GtkAllocation  *area)
{
	GtkAllocation allocation;
	GtkBorder     padding;

	gtk_widget_get_allocation (GTK_WIDGET (self->priv->view), &allocation);

	padding.left = 5;
	padding.right = 5;
	padding.top = 5;
	padding.bottom = 5;

	area->x = padding.left;
	area->y = padding.top;
	area->width = allocation.width - (padding.right + padding.left) - 1;
	area->height = allocation.height - (padding.top + padding.bottom) - 1;

}


static gboolean
curve_editor_draw_cb (GtkWidget *widget,
		      cairo_t   *cr,
		      gpointer   user_data)
{
	GthCurveEditor  *self = user_data;
	GtkAllocation    allocation;
	GtkStyleContext *style_context;

	style_context = gtk_widget_get_style_context (widget);
	gtk_style_context_save (style_context);
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_VIEW);
	gtk_style_context_add_class (style_context, "histogram");

	gtk_widget_get_allocation (widget, &allocation);
	gtk_render_background (style_context, cr, 0, 0, allocation.width, allocation.height);

	if ((self->priv->histogram != NULL)
	    && ((int) self->priv->current_channel <= gth_histogram_get_nchannels (self->priv->histogram)))
	{
		GtkAllocation  area;
		int            c;

		cairo_save (cr);
		cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
		gth_curve_editor_get_graph_area (self, &area);
		gth_histogram_paint_channel (self, style_context, cr, self->priv->current_channel, &area);
		gth_histogram_paint_grid (self, style_context, cr, &area);

		cairo_save (cr);
		for (c = GTH_HISTOGRAM_CHANNEL_VALUE; c <= GTH_HISTOGRAM_CHANNEL_BLUE; c++) {
			GthCurve  *curve;
			GthPoints *points;
			GthPoint  *p;

			if (c == self->priv->current_channel)
				continue;

			curve = self->priv->curve[c];
			points = gth_curve_get_points (curve);
			p = points->p;

			/* do not paint unchanged curves */
			if ((points->n == 2) && (p[0].x == 0 && p[0].y == 0) && (p[1].x == 255 && p[1].y == 255))
				continue;

			_cairo_set_source_color_from_channel (cr, c, 0.25);
			gth_histogram_paint_curve (self, style_context, cr, curve, &area);
		}
		_cairo_set_source_color_from_channel (cr, self->priv->current_channel, 1.0);
		gth_histogram_paint_curve (self, style_context, cr, self->priv->curve[self->priv->current_channel], &area);
		cairo_restore (cr);

		gth_histogram_paint_points (self, style_context, cr, gth_curve_get_points (self->priv->curve[self->priv->current_channel]), &area);

		if (self->priv->paint_position) {
			if (self->priv->active_point != NULL)
				gth_histogram_paint_point_position (self, style_context, cr, self->priv->active_point, &area);
			else
				gth_histogram_paint_point_position (self, style_context, cr, &self->priv->cursor, &area);
		}
		cairo_restore (cr);
	}

	gtk_style_context_restore (style_context);

	return TRUE;
}


static gboolean
curve_editor_scroll_event_cb (GtkWidget      *widget,
			      GdkEventScroll *event,
			      gpointer        user_data)
{
	GthCurveEditor *self = user_data;
	int               channel = 0;

	if (self->priv->histogram == NULL)
		return FALSE;

	if (event->direction == GDK_SCROLL_UP)
		channel = self->priv->current_channel - 1;
	else if (event->direction == GDK_SCROLL_DOWN)
		channel = self->priv->current_channel + 1;

	if (channel <= gth_histogram_get_nchannels (self->priv->histogram))
		gth_curve_editor_set_current_channel (self, CLAMP (channel, 0, GTH_HISTOGRAM_N_CHANNELS - 1));

	return TRUE;
}


static void
gth_curve_editor_get_point_from_event (GthCurveEditor *self,
				       GthPoint       *p,
				       double          x,
				       double          y)
{
	GtkAllocation area;

	gth_curve_editor_get_graph_area (self, &area);
	p->x = x - area.x;
	p->y = area.height - (y - area.y);

	p->x *= 255.0 / area.width;
	p->y *= 255.0 / area.height;

	p->x = round (p->x);
	p->y = round (p->y);
}


#define POINT_ATTRACTION_THRESHOLD 10


static void
gth_curve_editor_get_nearest_point (GthCurveEditor  *self,
				    GthPoint        *p,
				    int             *n)
{
	double     min = 0;
	GthPoints *points;
	int        i;

	*n = -1;
	points = gth_curve_get_points (self->priv->curve[self->priv->current_channel]);
	for (i = 0; i < points->n; i++) {
		GthPoint *q = &points->p[i];
		double    d;

		d = q->x - p->x;
		d = ABS (d);
		if ((d < POINT_ATTRACTION_THRESHOLD) && ((*n == -1) || (d < min))) {
			min = d;
			*n = i;
		}
	}
}


static void
gth_curve_editor_set_active_point (GthCurveEditor *self,
				   int             n)
{
	GthCurve  *curve;
	GthPoints *points;

	curve = self->priv->curve[self->priv->current_channel];
	points = gth_curve_get_points (curve);
	if (n >= points->n)
		n = -1;

	if (n >= 0) {
		self->priv->active_point = points->p + n;
		self->priv->active_point_lower_limit = (n > 0) ? points->p[n-1].x + 1 : 0;
		self->priv->active_point_upper_limit = (n < points->n-1) ? points->p[n+1].x - 1 : 255;
	}
	else
		self->priv->active_point = NULL;
}


static void
gth_curve_editor_changed (GthCurveEditor *self)
{
	g_signal_emit (self, gth_curve_editor_signals[CHANGED], 0);
}


static gboolean
curve_editor_button_press_event_cb (GtkWidget      *widget,
				    GdkEventButton *event,
				    gpointer        user_data)
{
	GthCurveEditor *self = user_data;
	GthPoint        p;
	int             n_active_point;

	gth_curve_editor_get_point_from_event (self, &p, event->x, event->y);
	gth_curve_editor_get_nearest_point (self, &p, &n_active_point);

	if (event->button == 1) {
		if (n_active_point < 0) {
			GthCurve  *curve;
			GthPoints *points;

			curve = self->priv->curve[self->priv->current_channel];
			points = gth_curve_get_points (curve);
			n_active_point = gth_points_add_point (points, p.x, p.y);
			gth_curve_setup (curve);
			gth_curve_editor_changed (self);
		}

		if (n_active_point >= 0) {
			GdkCursor *cursor;

			self->priv->dragging = TRUE;

			cursor = _gdk_cursor_new_for_widget (self->priv->view, GDK_BLANK_CURSOR);
			gdk_window_set_cursor (gtk_widget_get_window (self->priv->view), cursor);
			g_object_unref (cursor);
		}
	}
	else if (event->button == 3) {
		if (n_active_point >= 0) {
			GthCurve  *curve;
			GthPoints *points;

			curve = self->priv->curve[self->priv->current_channel];
			points = gth_curve_get_points (curve);
			if (points->n > 2) {
				gth_points_delete_point (points, n_active_point);
				n_active_point = -1;
				gth_curve_setup (curve);
				gth_curve_editor_changed (self);
			}
		}
	}

	gth_curve_editor_set_active_point (self, n_active_point);

	gtk_widget_queue_draw (self->priv->view);

	return TRUE;
}


static gboolean
curve_editor_button_release_event_cb (GtkWidget      *widget,
				      GdkEventButton *event,
				      gpointer        user_data)
{
	GthCurveEditor *self = user_data;

	if (self->priv->dragging) {
		GdkCursor *cursor;

		cursor = _gdk_cursor_new_for_widget (self->priv->view, GDK_CROSSHAIR);
		gdk_window_set_cursor (gtk_widget_get_window (self->priv->view), cursor);
		g_object_unref (cursor);
	}

	self->priv->dragging = FALSE;

	return TRUE;
}


static gboolean
curve_editor_motion_notify_event_cb (GtkWidget      *widget,
                		     GdkEventMotion *event,
                		     gpointer        user_data)
{
	GthCurveEditor *self = user_data;
	GthPoint        p;

	gth_curve_editor_get_point_from_event (self, &p, event->x, event->y);
	self->priv->cursor.x = (p.x >= 0 && p.x <= 255) ? p.x : -1;
	self->priv->cursor.y = (p.y >= 0 && p.y <= 255) ? p.y : -1;

	if (self->priv->dragging) {
		g_return_val_if_fail (self->priv->active_point != NULL, TRUE);
		self->priv->active_point->x = CLAMP (p.x, self->priv->active_point_lower_limit, self->priv->active_point_upper_limit);
		self->priv->active_point->y = CLAMP (p.y, 0, 255);
		gth_curve_setup (self->priv->curve[self->priv->current_channel]);
		gth_curve_editor_changed (self);
	}
	else {
		int n_active_point;

		gth_curve_editor_get_nearest_point (self, &p, &n_active_point);
		gth_curve_editor_set_active_point (self, n_active_point);
	}

	self->priv->paint_position = TRUE;
	gtk_widget_queue_draw (self->priv->view);

	return TRUE;
}


static gboolean
curve_editor_leave_notify_event_cb (GtkWidget *widget,
				    GdkEvent  *event,
				    gpointer   user_data)
{
	GthCurveEditor *self = user_data;

	self->priv->paint_position = FALSE;
	gtk_widget_queue_draw (self->priv->view);

	return FALSE;
}


static void
curve_editor_realize_cb (GtkWidget *widget,
			 gpointer   user_data)
{
	GthCurveEditor *self = user_data;
	GdkCursor      *cursor;

	cursor = _gdk_cursor_new_for_widget (self->priv->view, GDK_CROSSHAIR);
	gdk_window_set_cursor (gtk_widget_get_window (self->priv->view), cursor);
	g_object_unref (cursor);
}


static void
linear_histogram_button_toggled_cb (GtkToggleButton *button,
				    gpointer   user_data)
{
	GthCurveEditor *self = user_data;

	if (gtk_toggle_button_get_active (button))
		gth_curve_editor_set_scale_type (GTH_CURVE_EDITOR (self), GTH_HISTOGRAM_SCALE_LINEAR);
}


static void
logarithmic_histogram_button_toggled_cb (GtkToggleButton *button,
					 gpointer         user_data)
{
	GthCurveEditor *self = user_data;

	if (gtk_toggle_button_get_active (button))
		gth_curve_editor_set_scale_type (GTH_CURVE_EDITOR (self), GTH_HISTOGRAM_SCALE_LOGARITHMIC);
}


static void
channel_combo_box_changed_cb (GtkComboBox *combo_box,
			      gpointer     user_data)
{
	GthCurveEditor *self = user_data;
	int             n_channel;

	n_channel = gtk_combo_box_get_active (combo_box);
	if (n_channel < GTH_HISTOGRAM_N_CHANNELS)
		gth_curve_editor_set_current_channel (GTH_CURVE_EDITOR (self), n_channel);
}


static void
gth_curve_editor_reset_channel (GthCurveEditor      *self,
				GthHistogramChannel  c)
{
	GthCurve  *curve;
	GthPoints *points;

	curve = self->priv->curve[c];
	points = gth_curve_get_points (curve);
	gth_points_dispose (points);

	gth_points_init (points, 2);
	points->p[0].x = 0;
	points->p[0].y = 0;
	points->p[1].x = 255;
	points->p[1].y = 255;
	gth_curve_setup (curve);
}


static void
reset_current_channel_button_clicked_cb (GtkButton *button,
					 gpointer   user_data)
{
	GthCurveEditor *self = user_data;

	gth_curve_editor_reset_channel (self, self->priv->current_channel);
	gth_curve_editor_changed (self);
	gtk_widget_queue_draw (self->priv->view);
}


static void
self_notify_current_channel_cb (GObject    *gobject,
				GParamSpec *pspec,
				gpointer    user_data)
{
	GthCurveEditor *self = user_data;

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->channel_combo_box), self->priv->current_channel);
}


static void
self_notify_scale_type_cb (GObject    *gobject,
			   GParamSpec *pspec,
			   gpointer    user_data)
{
	GthCurveEditor *self = user_data;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->linear_histogram_button), self->priv->scale_type == GTH_HISTOGRAM_SCALE_LINEAR);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->logarithmic_histogram_button), self->priv->scale_type == GTH_HISTOGRAM_SCALE_LOGARITHMIC);
}


static void
gth_curve_editor_init (GthCurveEditor *self)
{
	GtkWidget       *topbar_box;
	GtkWidget       *sub_box;
	PangoAttrList   *attr_list;
	GtkWidget       *label;
	GtkWidget       *view_container;
	GtkListStore    *channel_model;
	GtkCellRenderer *renderer;
	GtkTreeIter      iter;
	GtkWidget       *button;
	int              c;

	self->priv = gth_curve_editor_get_instance_private (self);
	self->priv->histogram = NULL;
	self->priv->current_channel = GTH_HISTOGRAM_CHANNEL_VALUE;
	self->priv->scale_type = GTH_HISTOGRAM_SCALE_LINEAR;

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		self->priv->curve[c] = gth_curve_new (GTH_TYPE_BEZIER, NULL);
		gth_curve_editor_reset_channel (self, c);
	}

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

	gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->channel_combo_box), self->priv->current_channel);
	gtk_widget_show (self->priv->channel_combo_box);
	gtk_box_pack_start (GTK_BOX (sub_box), self->priv->channel_combo_box, FALSE, FALSE, 0);

	g_signal_connect (self->priv->channel_combo_box,
			  "changed",
			  G_CALLBACK (channel_combo_box_changed_cb),
			  self);

	pango_attr_list_unref (attr_list);

	/* reset channel button */

	button = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_icon_name ("edit-undo-symbolic", GTK_ICON_SIZE_BUTTON));
	gtk_widget_set_tooltip_text (button, _("Reset"));
	gtk_widget_show_all (button);
	gtk_box_pack_start (GTK_BOX (sub_box), button, FALSE, FALSE, 0);

	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (reset_current_channel_button_clicked_cb),
			  self);

	/* histogram view */

	view_container = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (view_container), GTK_SHADOW_IN);
	gtk_widget_set_vexpand (view_container, TRUE);
	gtk_widget_show (view_container);

	self->priv->view = gtk_drawing_area_new ();
	gtk_widget_add_events (self->priv->view,
			       (GDK_BUTTON_PRESS_MASK
				| GDK_BUTTON_RELEASE_MASK
				| GDK_POINTER_MOTION_MASK
				| GDK_POINTER_MOTION_HINT_MASK
				| GDK_ENTER_NOTIFY_MASK
				| GDK_LEAVE_NOTIFY_MASK
				| GDK_STRUCTURE_MASK));
	gtk_widget_show (self->priv->view);
	gtk_container_add (GTK_CONTAINER (view_container), self->priv->view);

	g_signal_connect (self->priv->view,
			  "draw",
			  G_CALLBACK (curve_editor_draw_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "scroll-event",
			  G_CALLBACK (curve_editor_scroll_event_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "button-press-event",
			  G_CALLBACK (curve_editor_button_press_event_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "button-release-event",
			  G_CALLBACK (curve_editor_button_release_event_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "motion-notify-event",
			  G_CALLBACK (curve_editor_motion_notify_event_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "leave-notify-event",
			  G_CALLBACK (curve_editor_leave_notify_event_cb),
			  self);
	g_signal_connect (self->priv->view,
			  "realize",
			  G_CALLBACK (curve_editor_realize_cb),
			  self);

	/* pack the widget */

	gtk_box_pack_start (GTK_BOX (self), topbar_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (self), view_container, TRUE, TRUE, 0);

	/* update widgets when a property changes */

	g_signal_connect (self,
			  "notify::current-channel",
			  G_CALLBACK (self_notify_current_channel_cb),
			  self);
	g_signal_connect (self,
			  "notify::scale-type",
			  G_CALLBACK (self_notify_scale_type_cb),
			  self);

	/* default values */

	self->priv->active_point = NULL;
	self->priv->cursor.x = -1;
	self->priv->cursor.y = -1;
	self->priv->dragging = FALSE;
	self->priv->paint_position = FALSE;

	gth_curve_editor_set_scale_type (self, GTH_HISTOGRAM_SCALE_LINEAR);
	gth_curve_editor_set_current_channel (self, GTH_HISTOGRAM_CHANNEL_VALUE);
}


GtkWidget *
gth_curve_editor_new (GthHistogram *histogram)
{
	return (GtkWidget *) g_object_new (GTH_TYPE_CURVE_EDITOR, "histogram", histogram, NULL);
}


static void
update_sensitivity (GthCurveEditor *self)
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
	GthCurveEditor *self = user_data;

	update_sensitivity (self);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}


void
gth_curve_editor_set_histogram (GthCurveEditor *self,
				  GthHistogram     *histogram)
{
	g_return_if_fail (GTH_IS_CURVE_EDITOR (self));

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
}


GthHistogram *
gth_curve_editor_get_histogram (GthCurveEditor *self)
{
	g_return_val_if_fail (GTH_IS_CURVE_EDITOR (self), NULL);
	return self->priv->histogram;
}


void
gth_curve_editor_set_current_channel (GthCurveEditor *self,
					int               n_channel)
{
	g_return_if_fail (GTH_IS_CURVE_EDITOR (self));

	if (n_channel == self->priv->current_channel)
		return;

	self->priv->current_channel = CLAMP (n_channel, 0, GTH_HISTOGRAM_N_CHANNELS);
	g_object_notify (G_OBJECT (self), "current-channel");

	gtk_widget_queue_draw (GTK_WIDGET (self));
}


gint
gth_curve_editor_get_current_channel (GthCurveEditor *self)
{
	g_return_val_if_fail (GTH_IS_CURVE_EDITOR (self), 0);
	return self->priv->current_channel;
}


void
gth_curve_editor_set_scale_type (GthCurveEditor  *self,
				   GthHistogramScale  scale_type)
{
	g_return_if_fail (GTH_IS_CURVE_EDITOR (self));

	self->priv->scale_type = scale_type;
	g_object_notify (G_OBJECT (self), "scale-type");

	gtk_widget_queue_draw (GTK_WIDGET (self));
}


GthHistogramScale
gth_curve_editor_get_scale_type (GthCurveEditor *self)
{
	g_return_val_if_fail (GTH_IS_CURVE_EDITOR (self), 0);
	return self->priv->scale_type;
}


void
gth_curve_editor_set_points (GthCurveEditor	 *self,
			     GthPoints		 *points)
{
	int c;

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		gth_curve_set_points (self->priv->curve[c], points + c);
	gth_curve_editor_changed (self);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}


void
gth_curve_editor_get_points (GthCurveEditor  *self,
			     GthPoints	     *points)
{
	int c;

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		gth_points_dispose (points + c);
		gth_points_copy (gth_curve_get_points (self->priv->curve[c]), &points[c]);
	}
}


void
gth_curve_editor_reset (GthCurveEditor *self)
{
	int c;

	for (c = GTH_HISTOGRAM_CHANNEL_VALUE; c <= GTH_HISTOGRAM_CHANNEL_BLUE; c++)
		gth_curve_editor_reset_channel (self, c);
	gth_curve_editor_changed (self);
	gtk_widget_queue_draw (self->priv->view);
}
