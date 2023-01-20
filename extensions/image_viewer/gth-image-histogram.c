/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include <math.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include "gth-image-histogram.h"
#include "gth-image-viewer-page.h"
#include "preferences.h"


#define MIN_HISTOGRAM_HEIGHT 350


static void gth_image_histogram_gth_property_view_interface_init (GthPropertyViewInterface *iface);


struct _GthImageHistogramPrivate {
	GthHistogram *histogram;
	GtkWidget    *histogram_view;
};


G_DEFINE_TYPE_WITH_CODE (GthImageHistogram,
			 gth_image_histogram,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthImageHistogram)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_PROPERTY_VIEW,
						gth_image_histogram_gth_property_view_interface_init))


static cairo_surface_t *
gth_image_histogram_get_current_image (GthImageHistogram *self)
{
	GthBrowser    *browser;
	GthViewerPage *viewer_page;

	browser = (GthBrowser *) _gtk_widget_get_toplevel_if_window (GTK_WIDGET (self));
	if (browser == NULL)
		return NULL;

	viewer_page = gth_browser_get_viewer_page (browser);
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	return gth_image_viewer_page_get_current_image (GTH_IMAGE_VIEWER_PAGE (viewer_page));
}


static gboolean
gth_image_histogram_real_can_view (GthPropertyView *base,
		 		  GthFileData     *file_data)
{
	GthImageHistogram *self = GTH_IMAGE_HISTOGRAM (base);
	return gth_image_histogram_get_current_image (self) != NULL;
}


static void
gth_image_histogram_real_set_file (GthPropertyView *base,
		 		   GthFileData     *file_data)
{
	GthImageHistogram *self = GTH_IMAGE_HISTOGRAM (base);
	cairo_surface_t   *image;

	image = gth_image_histogram_get_current_image (self);
	if (image != NULL)
		gth_histogram_calculate_for_image (self->priv->histogram, image);
}


static const char *
gth_image_histogram_real_get_name (GthPropertyView *self)
{
	return _("Histogram");
}


static const char *
gth_image_histogram_real_get_icon (GthPropertyView *self)
{
	return "histogram-symbolic";
}


static void
gth_image_histogram_finalize (GObject *base)
{
	GthImageHistogram *self = GTH_IMAGE_HISTOGRAM (base);

	g_object_unref (self->priv->histogram);
	G_OBJECT_CLASS (gth_image_histogram_parent_class)->finalize (base);
}


static void
gth_image_histogram_class_init (GthImageHistogramClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_image_histogram_finalize;
}


static void
gth_image_histogram_gth_property_view_interface_init (GthPropertyViewInterface *iface)
{
	iface->get_name = gth_image_histogram_real_get_name;
	iface->get_icon = gth_image_histogram_real_get_icon;
	iface->can_view = gth_image_histogram_real_can_view;
	iface->set_file = gth_image_histogram_real_set_file;
}


static void
histogram_view_notify_scale_type_cb (GObject    *gobject,
				     GParamSpec *pspec,
				     gpointer    user_data)
{
	GthImageHistogram *self = user_data;
	GSettings         *settings;

	settings = g_settings_new (GTHUMB_IMAGE_VIEWER_SCHEMA);
	g_settings_set_enum (settings, PREF_IMAGE_VIEWER_HISTOGRAM_SCALE, gth_histogram_view_get_scale_type (GTH_HISTOGRAM_VIEW (self->priv->histogram_view)));
	g_object_unref (settings);
}


static void
gth_image_histogram_init (GthImageHistogram *self)
{
	GSettings *settings;

	self->priv = gth_image_histogram_get_instance_private (self);
	self->priv->histogram = gth_histogram_new ();

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
	gtk_box_set_spacing (GTK_BOX (self), 6);

	settings = g_settings_new (GTHUMB_IMAGE_VIEWER_SCHEMA);

	self->priv->histogram_view = gth_histogram_view_new (self->priv->histogram);
	gth_histogram_view_show_info (GTH_HISTOGRAM_VIEW (self->priv->histogram_view), TRUE);
	gth_histogram_view_set_scale_type (GTH_HISTOGRAM_VIEW (self->priv->histogram_view), g_settings_get_enum (settings, PREF_IMAGE_VIEWER_HISTOGRAM_SCALE));
	gtk_widget_set_size_request (self->priv->histogram_view, -1, MIN_HISTOGRAM_HEIGHT);
	gtk_widget_show (self->priv->histogram_view);
	gtk_box_pack_start (GTK_BOX (self), self->priv->histogram_view, FALSE, FALSE, 0);
	g_signal_connect (self->priv->histogram_view,
			  "notify::scale-type",
			  G_CALLBACK (histogram_view_notify_scale_type_cb),
			  self);

	g_object_unref (settings);
}
