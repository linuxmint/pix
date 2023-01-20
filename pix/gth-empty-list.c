/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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

#include "gth-empty-list.h"

/* Properties */
enum {
	PROP_0,
	PROP_TEXT
};

struct _GthEmptyListPrivate {
	char        *text;
	PangoLayout *layout;
};


G_DEFINE_TYPE_WITH_CODE (GthEmptyList,
			 gth_empty_list,
			 GTK_TYPE_WIDGET,
			 G_ADD_PRIVATE (GthEmptyList))


static void
gth_empty_list_finalize (GObject *obj)
{
	g_free (GTH_EMPTY_LIST (obj)->priv->text);
	G_OBJECT_CLASS (gth_empty_list_parent_class)->finalize (obj);
}


static void
gth_empty_list_set_property (GObject      *object,
			     guint         property_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	GthEmptyList *self;

        self = GTH_EMPTY_LIST (object);

	switch (property_id) {
	case PROP_TEXT:
		g_free (self->priv->text);
		self->priv->text = g_value_dup_string (value);
		gtk_widget_queue_resize (GTK_WIDGET (self));
		g_object_notify (object, "text");
		break;

	default:
		break;
	}
}


static void
gth_empty_list_get_property (GObject    *object,
			     guint       property_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	GthEmptyList *self;

        self = GTH_EMPTY_LIST (object);

	switch (property_id) {
	case PROP_TEXT:
		g_value_set_string (value, self->priv->text);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_empty_list_realize (GtkWidget *widget)
{
	GthEmptyList    *self;
	GtkAllocation    allocation;
	GdkWindowAttr    attributes;
	int              attributes_mask;
	GdkWindow       *window;
	GtkStyleContext *style_context;

	g_return_if_fail (GTH_IS_EMPTY_LIST (widget));
	self = (GthEmptyList*) widget;

	gtk_widget_set_realized (widget, TRUE);

	/**/

	gtk_widget_get_allocation (widget, &allocation);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x           = allocation.x;
	attributes.y           = allocation.y;
	attributes.width       = allocation.width;
	attributes.height      = allocation.height;
	attributes.wclass      = GDK_INPUT_OUTPUT;
	attributes.visual      = gtk_widget_get_visual (widget);
	attributes.event_mask = (GDK_EXPOSURE_MASK
				 | GDK_SCROLL_MASK
				 | GDK_POINTER_MOTION_MASK
				 | GDK_ENTER_NOTIFY_MASK
				 | GDK_LEAVE_NOTIFY_MASK
				 | GDK_BUTTON_PRESS_MASK
				 | GDK_BUTTON_RELEASE_MASK
				 | gtk_widget_get_events (widget));
	attributes_mask        = (GDK_WA_X
				  | GDK_WA_Y
				  | GDK_WA_VISUAL);
	window = gdk_window_new (gtk_widget_get_parent_window (widget),
			         &attributes,
			         attributes_mask);
	gtk_widget_register_window (widget, window);
	gtk_widget_set_window (widget, window);

	/* Style */

	style_context = gtk_widget_get_style_context (widget);
	gtk_style_context_set_background (style_context, window);

	/* 'No Image' message Layout */

	if (self->priv->layout != NULL)
		g_object_unref (self->priv->layout);

	self->priv->layout = gtk_widget_create_pango_layout (widget, NULL);
	pango_layout_set_wrap (self->priv->layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_alignment (self->priv->layout, PANGO_ALIGN_CENTER);
}


static void
gth_empty_list_unrealize (GtkWidget *widget)
{
	GthEmptyList *self;

	g_return_if_fail (GTH_IS_EMPTY_LIST (widget));

	self = (GthEmptyList*) widget;

	if (self->priv->layout != NULL) {
		g_object_unref (self->priv->layout);
		self->priv->layout = NULL;
	}

	GTK_WIDGET_CLASS (gth_empty_list_parent_class)->unrealize (widget);
}


static void
gth_empty_list_map (GtkWidget *widget)
{
	gtk_widget_set_mapped (widget, TRUE);
	gdk_window_show (gtk_widget_get_window (widget));
}


static void
gth_empty_list_unmap (GtkWidget *widget)
{
	gtk_widget_set_mapped (widget, FALSE);
	gdk_window_hide (gtk_widget_get_window (widget));
}


static void
gth_empty_list_size_allocate (GtkWidget     *widget,
			      GtkAllocation *allocation)
{
	gtk_widget_set_allocation (widget, allocation);

	if (gtk_widget_get_realized (widget))
		gdk_window_move_resize (gtk_widget_get_window (widget),
					allocation->x,
					allocation->y,
					allocation->width,
					allocation->height);
}


static gboolean
gth_empty_list_draw (GtkWidget *widget,
		     cairo_t   *cr)
{
	GthEmptyList    *self = (GthEmptyList*) widget;
	GtkStyleContext *style_context;
	GtkAllocation    allocation;

	if (! gtk_cairo_should_draw_window (cr, gtk_widget_get_window (widget)))
		return FALSE;

	style_context = gtk_widget_get_style_context (widget);

	gtk_widget_get_allocation (widget, &allocation);

	gtk_render_background (style_context, cr,
			       0,
			       0,
			       allocation.width,
			       allocation.height);

	gtk_render_frame (style_context, cr,
			  0,
			  0,
			  allocation.width,
			  allocation.height);

	if (self->priv->text != NULL) {
		PangoRectangle        bounds;
		GdkRGBA               color;
		PangoFontDescription *font;

		pango_layout_set_width (self->priv->layout, allocation.width * PANGO_SCALE);
		pango_layout_set_text (self->priv->layout, self->priv->text, strlen (self->priv->text));
		pango_layout_get_pixel_extents (self->priv->layout, NULL, &bounds);
		gtk_style_context_get (style_context, gtk_widget_get_state_flags (widget), "font", &font, NULL);
		pango_layout_set_font_description (self->priv->layout, font);
		gtk_style_context_get_color (style_context, gtk_widget_get_state_flags (widget), &color);
		gdk_cairo_set_source_rgba (cr, &color);
		gtk_render_layout (style_context, cr, 0, (allocation.height - bounds.height) / 2, self->priv->layout);
		cairo_fill (cr);

		pango_font_description_free (font);
	}

	return FALSE;
}


static void
gth_empty_list_class_init (GthEmptyListClass *klass)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GObjectClass*) (klass);
	object_class->set_property = gth_empty_list_set_property;
	object_class->get_property = gth_empty_list_get_property;
	object_class->finalize = gth_empty_list_finalize;

	widget_class = (GtkWidgetClass*) klass;
	widget_class->realize = gth_empty_list_realize;
	widget_class->unrealize = gth_empty_list_unrealize;
	widget_class->map = gth_empty_list_map;
	widget_class->unmap = gth_empty_list_unmap;
	widget_class->size_allocate = gth_empty_list_size_allocate;
	widget_class->draw = gth_empty_list_draw;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_TEXT,
					 g_param_spec_string ("text",
                                                              "Text",
                                                              "The text to display",
                                                              NULL,
                                                              G_PARAM_READABLE | G_PARAM_WRITABLE));
}


static void
gth_empty_list_init (GthEmptyList *self)
{
	GtkStyleContext *style_context;

	gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);
	gtk_widget_set_can_focus (GTK_WIDGET (self), FALSE);

	self->priv = gth_empty_list_get_instance_private (self);
	self->priv->layout = NULL;
	self->priv->text = NULL;

	style_context = gtk_widget_get_style_context (GTK_WIDGET (self));
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_VIEW);
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_FRAME);
	gtk_style_context_add_class (style_context, "empty-view");
}


GtkWidget *
gth_empty_list_new (const char *text)
{
	return g_object_new (GTH_TYPE_EMPTY_LIST, "text", text, NULL);
}


void
gth_empty_list_set_text (GthEmptyList *self,
			 const char   *text)
{
	g_object_set (self, "text", text, NULL);
}
