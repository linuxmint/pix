/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include "gth-auto-paned.h"


struct _GthAutoPanedPrivate {
	gboolean child1_visible;
	gboolean child2_visible;
};


G_DEFINE_TYPE_WITH_CODE (GthAutoPaned,
			 gth_auto_paned,
			 GTK_TYPE_PANED,
			 G_ADD_PRIVATE (GthAutoPaned))


static gboolean
_gtk_widget_get_visible (GtkWidget *widget)
{
	return (widget != NULL) && gtk_widget_get_visible (widget);
}


static void
gth_auto_paned_size_allocate (GtkWidget     *widget,
			 GtkAllocation *allocation)
{
	GthAutoPaned  *self = GTH_AUTO_PANED (widget);
	GtkWidget *child1;
	GtkWidget *child2;
	gboolean   reset_position;

	child1 = gtk_paned_get_child1 (GTK_PANED (self));
	child2 = gtk_paned_get_child2 (GTK_PANED (self));

	reset_position = FALSE;
	if ((self->priv->child1_visible != _gtk_widget_get_visible (child1))
	    || (self->priv->child2_visible != _gtk_widget_get_visible (child2)))
	{
		reset_position = TRUE;
		self->priv->child1_visible = _gtk_widget_get_visible (child1);
		self->priv->child2_visible = _gtk_widget_get_visible (child2);
	}

	if (reset_position) {
		int position;

		switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (self))) {
		case GTK_ORIENTATION_HORIZONTAL:
			position = allocation->width / 2;
			break;
		case GTK_ORIENTATION_VERTICAL:
			position = allocation->height / 2;
			break;
		default:
			g_assert_not_reached ();
		}
		gtk_paned_set_position (GTK_PANED (self), position);
	}

	GTK_WIDGET_CLASS (gth_auto_paned_parent_class)->size_allocate (widget, allocation);
}


static void
gth_auto_paned_class_init (GthAutoPanedClass *klass)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_allocate = gth_auto_paned_size_allocate;
}


static void
gth_auto_paned_init (GthAutoPaned *self)
{
	self->priv = gth_auto_paned_get_instance_private (self);
	self->priv->child1_visible = FALSE;
	self->priv->child2_visible = FALSE;
}


GtkWidget *
gth_auto_paned_new (GtkOrientation orientation)
{
	return g_object_new (GTH_TYPE_AUTO_PANED,
			     "orientation", orientation,
			     NULL);
}
