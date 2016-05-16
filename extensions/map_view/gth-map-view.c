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
#include <math.h>
#include <glib/gi18n.h>
#include <champlain/champlain.h>
#include <champlain-gtk/champlain-gtk.h>
#include <clutter-gtk/clutter-gtk.h>
#include <gthumb.h>
#include "gth-map-view.h"

#define GTH_MAP_VIEW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_MAP_VIEW, GthMapViewPrivate))
#define LABEL_MAX_WIDTH 200


static void gth_map_view_gth_multipage_child_interface_init (GthMultipageChildInterface *iface);
static void gth_map_view_gth_property_view_interface_init (GthPropertyViewInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GthMapView,
			 gth_map_view,
			 GTK_TYPE_BOX,
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_MULTIPAGE_CHILD,
					        gth_map_view_gth_multipage_child_interface_init)
		         G_IMPLEMENT_INTERFACE (GTH_TYPE_PROPERTY_VIEW,
		        		        gth_map_view_gth_property_view_interface_init))


struct _GthMapViewPrivate {
	GtkWidget            *no_gps_label;
	GtkWidget            *embed;
	ChamplainView        *map_view;
	ChamplainMarkerLayer *marker_layer;
	ClutterActor         *marker;
};


/* Exif format: %d/%d %d/%d %d/%d */
static double
exif_coordinate_to_decimal (const char *raw)
{
	double   value;
	double   factor;
	char   **raw_values;
	int      i;

	value = 0.0;
	factor = 1.0;
	raw_values = g_strsplit (raw, " ", 3);
	for (i = 0; i < 3; i++) {
		int numerator;
		int denominator;

		if (raw_values[i] == NULL)
			break; /* Error */

		sscanf (raw_values[i], "%d/%d", &numerator, &denominator);
		if ((numerator != 0) && (denominator != 0))
			value += ((double) numerator / denominator) / factor;

		factor *= 60.0;
	}

	g_strfreev (raw_values);

	return value;
}


static char *
decimal_to_string (double value)
{
	GString *s;
	double   part;

	s = g_string_new ("");
	if (value < 0.0)
		value = - value;

	/* degree */

	part = floor (value);
	g_string_append_printf (s, "%.0f°", part);
	value = (value - part) * 60.0;

	/* minutes */

	part = floor (value);
	g_string_append_printf (s, " %.0fʹ", part);
	value = (value - part) * 60.0;

	/* seconds */

	g_string_append_printf (s, " %02.3fʺ", value);

	return g_string_free (s, FALSE);
}


static char *
decimal_coordinates_to_string (double latitude,
			       double longitude)
{
	char *latitude_s;
	char *longitude_s;
	char *s;

	latitude_s = decimal_to_string (latitude);
	longitude_s = decimal_to_string (longitude);
	s = g_strdup_printf ("%s %s\n%s %s",
			     latitude_s,
			     ((latitude < 0.0) ? C_("Cardinal point", "S") : C_("Cardinal point", "N")),
			     longitude_s,
			     ((longitude < 0.0) ? C_("Cardinal point", "W") : C_("Cardinal point", "E")));

	g_free (longitude_s);
	g_free (latitude_s);

	return s;
}


void
gth_map_view_real_set_file (GthPropertyView *base,
		 	    GthFileData     *file_data)
{
	GthMapView *self = GTH_MAP_VIEW (base);
	int         coordinates_available;
	double      latitude;
	double      longitude;

	coordinates_available = 0;

	if (file_data != NULL) {
		GthMetadata *metadata;

		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Exif::GPSInfo::GPSLatitude");
		if (metadata != NULL) {
			latitude = exif_coordinate_to_decimal (gth_metadata_get_raw (metadata));

			metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Exif::GPSInfo::GPSLatitudeRef");
			if (metadata != NULL) {
				if (g_strcmp0 (gth_metadata_get_raw (metadata), "S") == 0)
					latitude = - latitude;
			}

			coordinates_available++;
		}

		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Exif::GPSInfo::GPSLongitude");
		if (metadata != NULL) {
			longitude = exif_coordinate_to_decimal (gth_metadata_get_raw (metadata));

			metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Exif::GPSInfo::GPSLongitudeRef");
			if (metadata != NULL) {
				if (g_strcmp0 (gth_metadata_get_raw (metadata), "W") == 0)
					longitude = - longitude;
			}

			coordinates_available++;
		}
	}

	if (coordinates_available == 2) {
		char *position;

		gtk_widget_hide (self->priv->no_gps_label);
		gtk_widget_show (self->priv->embed);

		position = decimal_coordinates_to_string (latitude, longitude);
		champlain_label_set_text (CHAMPLAIN_LABEL (self->priv->marker), position);
		g_free (position);

		champlain_location_set_location (CHAMPLAIN_LOCATION (self->priv->marker), latitude, longitude);
		champlain_view_center_on (CHAMPLAIN_VIEW (self->priv->map_view), latitude, longitude);
	}
	else {
		gtk_widget_hide (self->priv->embed);
		gtk_widget_show (self->priv->no_gps_label);
	}
}


const char *
gth_map_view_real_get_name (GthMultipageChild *self)
{
	return _("Map");
}


const char *
gth_map_view_real_get_icon (GthMultipageChild *self)
{
	return "map";
}


static void
gth_map_view_finalize (GObject *base)
{
	G_OBJECT_CLASS (gth_map_view_parent_class)->finalize (base);
}


static void
gth_map_view_realize (GtkWidget *widget)
{
	GthMapView *self;

	self = GTH_MAP_VIEW (widget);

	GTK_WIDGET_CLASS (gth_map_view_parent_class)->realize (widget);
	if (! gtk_widget_get_visible (self->priv->no_gps_label))
		gtk_widget_show (self->priv->embed);
}


static void
gth_map_view_unrealize (GtkWidget *widget)
{
	GthMapView *self;

	self = GTH_MAP_VIEW (widget);

	GTK_WIDGET_CLASS (gth_map_view_parent_class)->unrealize (widget);
	gtk_widget_hide (self->priv->embed);
}


static void
gth_map_view_class_init (GthMapViewClass *klass)
{
	GtkWidgetClass *widget_class;

	g_type_class_add_private (klass, sizeof (GthMapViewPrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_map_view_finalize;

	widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->realize = gth_map_view_realize;
	widget_class->unrealize = gth_map_view_unrealize;
}


static void
gth_map_view_init (GthMapView *self)
{
	ClutterActor *scale;

	self->priv = GTH_MAP_VIEW_GET_PRIVATE (self);

	gtk_box_set_spacing (GTK_BOX (self), 6);
	gtk_container_set_border_width (GTK_CONTAINER (self), 2);
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

	/* No GPS label */

	self->priv->no_gps_label = gtk_label_new (_("The geographical position information is not available for this image."));
	g_object_set (G_OBJECT (self->priv->no_gps_label),
		      "wrap", TRUE,
		      "wrap-mode", PANGO_WRAP_WORD_CHAR,
		      "single-line-mode", FALSE,
		      "justify", GTK_JUSTIFY_CENTER,
		      "width-request", LABEL_MAX_WIDTH,
		      NULL);
	gtk_widget_show (self->priv->no_gps_label);
	gtk_box_pack_start (GTK_BOX (self), self->priv->no_gps_label, TRUE, TRUE, 0);

	/* The map widget */

	self->priv->embed = gtk_champlain_embed_new ();

	self->priv->map_view = gtk_champlain_embed_get_view (GTK_CHAMPLAIN_EMBED (self->priv->embed));
	g_object_set (G_OBJECT (self->priv->map_view),
		      "reactive", TRUE,
		      "zoom-level", 5,
		      "zoom-on-double-click", TRUE,
		      "kinetic-mode", TRUE,
		      NULL);

	scale = champlain_scale_new ();
	champlain_scale_connect_view (CHAMPLAIN_SCALE (scale), self->priv->map_view);
	champlain_view_bin_layout_add (self->priv->map_view,
				       scale,
				       CLUTTER_BIN_ALIGNMENT_START,
				       CLUTTER_BIN_ALIGNMENT_END);

	self->priv->marker_layer = champlain_marker_layer_new ();
	champlain_view_add_layer (self->priv->map_view, CHAMPLAIN_LAYER (self->priv->marker_layer));
	clutter_actor_show (CLUTTER_ACTOR (self->priv->marker_layer));

	self->priv->marker = champlain_label_new_with_text ("", "Sans 10", NULL, NULL);
	clutter_actor_show (self->priv->marker);
	champlain_marker_layer_add_marker (self->priv->marker_layer, CHAMPLAIN_MARKER (self->priv->marker));

	gtk_widget_show_all (self->priv->embed);
	gtk_widget_hide (self->priv->embed);

	gtk_box_pack_start (GTK_BOX (self), self->priv->embed, TRUE, TRUE, 0);
}


static void
gth_map_view_gth_multipage_child_interface_init (GthMultipageChildInterface *iface)
{
	iface->get_name = gth_map_view_real_get_name;
	iface->get_icon = gth_map_view_real_get_icon;
}


static void
gth_map_view_gth_property_view_interface_init (GthPropertyViewInterface *iface)
{
	iface->set_file = gth_map_view_real_set_file;
}
