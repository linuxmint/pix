/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 The Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "gth-paned.h"


enum {
	PROP_0,
	PROP_POSITION2
};


struct _GthPanedPrivate {
	int child2_size;
	gboolean position2_set;
};


G_DEFINE_TYPE_WITH_CODE (GthPaned,
			 gth_paned,
			 GTK_TYPE_PANED,
			 G_ADD_PRIVATE (GthPaned))


static void
gth_paned_size_allocate (GtkWidget     *widget,
			 GtkAllocation *allocation)
{
	GthPaned *self;

	self = GTH_PANED (widget);

	if (self->priv->position2_set) {
		self->priv->position2_set = FALSE;
		switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (widget))) {
		case GTK_ORIENTATION_HORIZONTAL:
			gtk_paned_set_position (GTK_PANED (self), allocation->width - self->priv->child2_size);
			break;
		case GTK_ORIENTATION_VERTICAL:
			gtk_paned_set_position (GTK_PANED (self), allocation->height - self->priv->child2_size);
			break;
		}
	}

	GTK_WIDGET_CLASS (gth_paned_parent_class)->size_allocate (widget, allocation);
}


static void
gth_paned_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	GthPaned *self;

	self = GTH_PANED (object);

	switch (prop_id) {
	case PROP_POSITION2:
		gth_paned_set_position2 (self, g_value_get_int (value));
		break;
	default:
		break;
	}
}


static void
gth_paned_get_property (GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GthPaned *self;

	self = GTH_PANED (object);

	switch (prop_id) {
	case PROP_POSITION2:
		g_value_set_int (value, self->priv->child2_size);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
gth_paned_class_init (GthPanedClass *paned_class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;

	gobject_class = (GObjectClass*) paned_class;
	gobject_class->set_property = gth_paned_set_property;
	gobject_class->get_property = gth_paned_get_property;

	widget_class = (GtkWidgetClass*) paned_class;
	widget_class->size_allocate = gth_paned_size_allocate;

	/* Properties */

	g_object_class_install_property (gobject_class,
					 PROP_POSITION2,
					 g_param_spec_int ("position2",
							   "Position 2",
							   "Position starting from the end",
							   0,
							   G_MAXINT32,
							   0,
							   G_PARAM_READWRITE));
}


static void
gth_paned_init (GthPaned *self)
{
	gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);

	self->priv = gth_paned_get_instance_private (self);
	self->priv->position2_set = FALSE;
	self->priv->child2_size = 0;
}


GtkWidget *
gth_paned_new (GtkOrientation orientation)
{
	return g_object_new (GTH_TYPE_PANED, "orientation", orientation, NULL);
}


void
gth_paned_set_position2 (GthPaned *self,
			 int       position)
{
	g_return_if_fail (GTH_IS_PANED (self));

	if (position >= 0) {
		self->priv->child2_size = position;
		self->priv->position2_set = TRUE;
	}
	else
		self->priv->position2_set = FALSE;

	g_object_notify (G_OBJECT (self), "position2");
	gtk_widget_queue_resize_no_redraw (GTK_WIDGET (self));
}


int
gth_paned_get_position2 (GthPaned *self)
{
	g_return_val_if_fail (GTH_IS_PANED (self), 0);

	return self->priv->child2_size;
}
