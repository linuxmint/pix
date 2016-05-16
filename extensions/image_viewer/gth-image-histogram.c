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


#define MIN_HISTOGRAM_HEIGHT 280


static void gth_image_histogram_gth_multipage_child_interface_init (GthMultipageChildInterface *iface);
static void gth_image_histogram_gth_property_view_interface_init (GthPropertyViewInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthImageHistogram,
			 gth_image_histogram,
			 GTK_TYPE_BOX,
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_MULTIPAGE_CHILD,
					        gth_image_histogram_gth_multipage_child_interface_init)
		         G_IMPLEMENT_INTERFACE (GTH_TYPE_PROPERTY_VIEW,
		        		        gth_image_histogram_gth_property_view_interface_init))


struct _GthImageHistogramPrivate {
	GthHistogram *histogram;
	GtkWidget    *histogram_view;
};


void
gth_image_histogram_real_set_file (GthPropertyView *base,
		 		   GthFileData     *file_data)
{
	GthImageHistogram *self = GTH_IMAGE_HISTOGRAM (base);
	GthBrowser        *browser;
	GtkWidget         *viewer_page;

	if (file_data == NULL) {
		gth_histogram_calculate_for_image (self->priv->histogram, NULL);
		return;
	}

	browser = (GthBrowser *) gtk_widget_get_toplevel (GTK_WIDGET (base));
	if (! gtk_widget_is_toplevel (GTK_WIDGET (browser))) {
		gth_histogram_calculate_for_image (self->priv->histogram, NULL);
		return;
	}

	viewer_page = gth_browser_get_viewer_page (browser);
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page)) {
		gth_histogram_calculate_for_image (self->priv->histogram, NULL);
		return;
	}

	gth_histogram_calculate_for_image (self->priv->histogram, gth_image_viewer_page_get_image (GTH_IMAGE_VIEWER_PAGE (viewer_page)));
}


const char *
gth_image_histogram_real_get_name (GthMultipageChild *self)
{
	return _("Histogram");
}


const char *
gth_image_histogram_real_get_icon (GthMultipageChild *self)
{
	return "histogram";
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
	g_type_class_add_private (klass, sizeof (GthImageHistogramPrivate));
	G_OBJECT_CLASS (klass)->finalize = gth_image_histogram_finalize;
}


static void
gth_image_histogram_gth_multipage_child_interface_init (GthMultipageChildInterface *iface)
{
	iface->get_name = gth_image_histogram_real_get_name;
	iface->get_icon = gth_image_histogram_real_get_icon;
}


static void
gth_image_histogram_gth_property_view_interface_init (GthPropertyViewInterface *iface)
{
	iface->set_file = gth_image_histogram_real_set_file;
}


static void
gth_image_histogram_init (GthImageHistogram *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMAGE_HISTOGRAM, GthImageHistogramPrivate);
	self->priv->histogram = gth_histogram_new ();

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
	gtk_box_set_spacing (GTK_BOX (self), 6);
	gtk_container_set_border_width (GTK_CONTAINER (self), 2);

	self->priv->histogram_view = gth_histogram_view_new (self->priv->histogram);
	gth_histogram_view_show_info (GTH_HISTOGRAM_VIEW (self->priv->histogram_view), TRUE);
	gtk_widget_set_size_request (self->priv->histogram_view, -1, MIN_HISTOGRAM_HEIGHT);
	gtk_widget_show (self->priv->histogram_view);
	gtk_box_pack_start (GTK_BOX (self), self->priv->histogram_view, FALSE, FALSE, 0);
}
