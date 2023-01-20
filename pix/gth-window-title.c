/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 Free Software Foundation, Inc.
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
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "gth-window-title.h"
#include "gtk-utils.h"


struct _GthWindowTitlePrivate {
	GtkWidget     *title;
	GtkWidget     *emblems;
};


G_DEFINE_TYPE_WITH_CODE (GthWindowTitle,
			 gth_window_title,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthWindowTitle))


static void
gth_window_title_class_init (GthWindowTitleClass *klass)
{
	/* void */
}


static void
gth_window_title_init (GthWindowTitle *self)
{
	self->priv = gth_window_title_get_instance_private (self);

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
	gtk_box_set_spacing (GTK_BOX (self), 10);

	self->priv->title = gtk_label_new ("");
	gtk_label_set_line_wrap (GTK_LABEL (self->priv->title), FALSE);
	gtk_label_set_single_line_mode (GTK_LABEL (self->priv->title), TRUE);
	gtk_label_set_ellipsize (GTK_LABEL (self->priv->title), PANGO_ELLIPSIZE_END);
	gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->title), "title");
	gtk_box_pack_start (GTK_BOX (self), self->priv->title, FALSE, FALSE, 0);

	self->priv->emblems = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start (GTK_BOX (self), self->priv->emblems, FALSE, FALSE, 0);
}


GtkWidget *
gth_window_title_new (void)
{
	return (GtkWidget *) g_object_new (GTH_TYPE_WINDOW_TITLE, NULL);
}


void
gth_window_title_set_title (GthWindowTitle	*self,
			    const char		*title)
{
	gtk_label_set_text (GTK_LABEL (self->priv->title), (title != NULL) ? title : "");
	gtk_widget_set_visible (self->priv->title, title != NULL);
}


void
gth_window_title_set_emblems (GthWindowTitle	*self,
			      GList		*emblems)
{
	GList *scan;

	_gtk_container_remove_children (GTK_CONTAINER (self->priv->emblems), NULL, NULL);
	for (scan = emblems; scan; scan = scan->next) {
		char      *emblem = scan->data;
		GtkWidget *icon;

		icon = gtk_image_new_from_icon_name (emblem, GTK_ICON_SIZE_MENU);
		gtk_widget_show (icon);
		gtk_box_pack_start (GTK_BOX (self->priv->emblems), icon, FALSE, FALSE, 0);
	}

	gtk_widget_set_visible (self->priv->emblems, emblems != NULL);
}
