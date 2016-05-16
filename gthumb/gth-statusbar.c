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
#include <glib/gi18n.h>
#include "gth-statusbar.h"


struct _GthStatusbarPrivate {
	guint      list_info_cid;
	GtkWidget *primary_text;
	GtkWidget *primary_text_frame;
	GtkWidget *secondary_text;
	GtkWidget *secondary_text_frame;
	GtkWidget *progress_box;
	GtkWidget *progress_bar;
	GtkWidget *progress_label;
	/*GtkWidget *stop_button;*/
};


G_DEFINE_TYPE (GthStatusbar, gth_statusbar, GTK_TYPE_STATUSBAR)


static void
gth_statusbar_class_init (GthStatusbarClass *klass)
{
	g_type_class_add_private (klass, sizeof (GthStatusbarPrivate));
}


static void
gth_statusbar_init (GthStatusbar *statusbar)
{
	GtkWidget *separator;
	GtkWidget *hbox;
	GtkWidget *vbox;
	/*GtkWidget *image;*/

	gtk_box_set_spacing (GTK_BOX (statusbar), 0);

	statusbar->priv = G_TYPE_INSTANCE_GET_PRIVATE (statusbar, GTH_TYPE_STATUSBAR, GthStatusbarPrivate);
	statusbar->priv->list_info_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), "gth_list_info");

	/* Progress info */

	statusbar->priv->progress_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_hide (statusbar->priv->progress_box);
	gtk_box_pack_start (GTK_BOX (statusbar), statusbar->priv->progress_box, FALSE, FALSE, 0);

	separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show (separator);
	gtk_box_pack_start (GTK_BOX (statusbar->priv->progress_box), separator, FALSE, FALSE, 0);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (statusbar->priv->progress_box), hbox, FALSE, FALSE, 0);

	statusbar->priv->progress_label = gtk_label_new (NULL);
	gtk_widget_show (statusbar->priv->progress_label);
	gtk_box_pack_start (GTK_BOX (hbox), statusbar->priv->progress_label, TRUE, TRUE, 0);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_set_homogeneous (GTK_BOX (vbox), TRUE);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

	statusbar->priv->progress_bar = gtk_progress_bar_new ();
	gtk_widget_set_size_request (statusbar->priv->progress_bar, 60, 12);
	gtk_widget_show (statusbar->priv->progress_bar);
	gtk_box_pack_start (GTK_BOX (vbox), statusbar->priv->progress_bar, FALSE, FALSE, 0);

	/*
	statusbar->priv->stop_button = gtk_button_new ();
	gtk_widget_set_size_request (statusbar->priv->stop_button, 26, 26);
	gtk_button_set_relief (GTK_BUTTON (statusbar->priv->stop_button), GTK_RELIEF_NONE);
	gtk_widget_show (statusbar->priv->stop_button);
	gtk_box_pack_start (GTK_BOX (hbox), statusbar->priv->stop_button, FALSE, FALSE, 0);

	image = gtk_image_new_from_stock (GTK_STOCK_STOP, GTK_ICON_SIZE_MENU);
	gtk_widget_show (image);
	gtk_container_add (GTK_CONTAINER (statusbar->priv->stop_button), image);
	*/

	/* Secondary text */

	statusbar->priv->secondary_text = gtk_label_new (NULL);
	gtk_widget_show (statusbar->priv->secondary_text);

	statusbar->priv->secondary_text_frame = gtk_frame_new (NULL);
	gtk_widget_show (statusbar->priv->secondary_text_frame);
	gtk_frame_set_shadow_type (GTK_FRAME (statusbar->priv->secondary_text_frame), GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (statusbar->priv->secondary_text_frame), statusbar->priv->secondary_text);
	gtk_box_pack_start (GTK_BOX (statusbar), statusbar->priv->secondary_text_frame, FALSE, FALSE, 0);

	/* Primary text */

	statusbar->priv->primary_text = gtk_label_new (NULL);
	gtk_widget_show (statusbar->priv->primary_text);

	statusbar->priv->primary_text_frame = gtk_frame_new (NULL);
	gtk_widget_show (statusbar->priv->primary_text_frame);

	gtk_frame_set_shadow_type (GTK_FRAME (statusbar->priv->primary_text_frame), GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (statusbar->priv->primary_text_frame), statusbar->priv->primary_text);
	gtk_box_pack_start (GTK_BOX (statusbar), statusbar->priv->primary_text_frame, FALSE, FALSE, 0);
}


GtkWidget *
gth_statusbar_new (void)
{
	return g_object_new (GTH_TYPE_STATUSBAR, NULL);
}


void
gth_statusbar_set_list_info (GthStatusbar *statusbar,
			     const char   *text)
{
	gtk_statusbar_pop (GTK_STATUSBAR (statusbar), statusbar->priv->list_info_cid);
	gtk_statusbar_push (GTK_STATUSBAR (statusbar), statusbar->priv->list_info_cid, text);
}


void
gth_statusbar_set_primary_text (GthStatusbar *statusbar,
				const char   *text)
{
	if (text != NULL) {
		gtk_label_set_text (GTK_LABEL (statusbar->priv->primary_text), text);
		gtk_widget_show (statusbar->priv->primary_text_frame);
	}
	else
		gtk_widget_hide (statusbar->priv->primary_text_frame);
}


void
gth_statusbar_set_secondary_text (GthStatusbar *statusbar,
				  const char   *text)
{
	if (text != NULL) {
		gtk_label_set_text (GTK_LABEL (statusbar->priv->secondary_text), text);
		gtk_widget_show (statusbar->priv->secondary_text_frame);
	}
	else
		gtk_widget_hide (statusbar->priv->secondary_text_frame);
}


void
gth_statusbar_set_progress (GthStatusbar *statusbar,
			    const char   *text,
			    gboolean      pulse,
			    double        fraction)
{
	if (text == NULL) {
		gtk_widget_hide (statusbar->priv->progress_box);
		return;
	}

	gtk_widget_show (statusbar->priv->progress_box);
	gtk_label_set_text (GTK_LABEL (statusbar->priv->progress_label), text);
	if (pulse)
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (statusbar->priv->progress_bar));
	else
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (statusbar->priv->progress_bar), fraction);
}
