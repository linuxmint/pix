/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 The Free Software Foundation, Inc.
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
#include "gth-location-bar.h"
#include "gth-location-chooser.h"


#define LOCATION_BAR_MARGIN 3


struct _GthLocationBarPrivate {
	GtkWidget *location_chooser;
	GtkWidget *action_area;
};


G_DEFINE_TYPE_WITH_CODE (GthLocationBar,
			 gth_location_bar,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthLocationBar))


static void
gth_location_bar_class_init (GthLocationBarClass *klass)
{
	/* void */
}


static void
gth_location_bar_init (GthLocationBar *self)
{
	GtkWidget *box;

	gtk_widget_set_can_focus (GTK_WIDGET (self), FALSE);

	self->priv = gth_location_bar_get_instance_private (self);

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_margin_top (GTK_WIDGET (self), LOCATION_BAR_MARGIN);
	gtk_widget_set_margin_bottom (GTK_WIDGET (self), LOCATION_BAR_MARGIN);
	gtk_widget_set_margin_end (GTK_WIDGET (self), LOCATION_BAR_MARGIN);
	gtk_widget_set_margin_start (GTK_WIDGET (self), LOCATION_BAR_MARGIN);

	/* location chooser */

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (box);
	self->priv->location_chooser = g_object_new (GTH_TYPE_LOCATION_CHOOSER,
						     "show-entry-points", FALSE,
						     "show-other", FALSE,
						     "relief", GTK_RELIEF_NONE,
						     NULL);
	gtk_widget_show (self->priv->location_chooser);
	gtk_box_pack_start (GTK_BOX (box), self->priv->location_chooser, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (self), box, TRUE, TRUE, 0);

	/* action area */

	self->priv->action_area = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (self->priv->action_area);
	gtk_box_pack_end (GTK_BOX (self), self->priv->action_area, FALSE, FALSE, 0);
}


GtkWidget *
gth_location_bar_new (void)
{
	return g_object_new (GTH_TYPE_LOCATION_BAR, NULL);
}


GtkWidget *
gth_location_bar_get_chooser (GthLocationBar *self)
{
	return self->priv->location_chooser;
}


GtkWidget *
gth_location_bar_get_action_area (GthLocationBar *self)
{
	return self->priv->action_area;
}
