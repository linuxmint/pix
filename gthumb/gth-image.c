/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 The Free Software Foundation, Inc.
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

#define GDK_PIXBUF_ENABLE_BACKEND 1

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "cairo-utils.h"
#include "glib-utils.h"
#include "gth-image.h"
#include "pixbuf-utils.h"


struct _GthImagePrivate {
	GthImageFormat format;
	union {
		cairo_surface_t    *surface;
		GdkPixbuf          *pixbuf;
		GdkPixbufAnimation *pixbuf_animation;
	} data;
};


G_DEFINE_TYPE (GthImage, gth_image, G_TYPE_OBJECT)


static void
_gth_image_free_data (GthImage *self)
{
	switch (self->priv->format) {
	case GTH_IMAGE_FORMAT_CAIRO_SURFACE:
		cairo_surface_destroy (self->priv->data.surface);
		self->priv->data.surface = NULL;
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF:
		_g_object_unref (self->priv->data.pixbuf);
		self->priv->data.pixbuf = NULL;
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION:
		_g_object_unref (self->priv->data.pixbuf_animation);
		self->priv->data.pixbuf_animation = NULL;
		break;

	default:
		break;
	}
}


static void
gth_image_finalize (GObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE (object));

	_gth_image_free_data (GTH_IMAGE (object));

	/* Chain up */
	G_OBJECT_CLASS (gth_image_parent_class)->finalize (object);
}


static gboolean
base_get_is_zoomable (GthImage *image)
{
	return FALSE;
}


static gboolean
base_set_zoom (GthImage *image,
	       double    zoom,
	       int      *original_width,
	       int      *original_height)
{
	return FALSE;
}


static void
gth_image_class_init (GthImageClass *klass)
{
	GObjectClass *gobject_class;

	g_type_class_add_private (klass, sizeof (GthImagePrivate));

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_image_finalize;

	klass->get_is_zoomable = base_get_is_zoomable;
	klass->set_zoom = base_set_zoom;
}


static void
gth_image_init (GthImage *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMAGE, GthImagePrivate);
	self->priv->format = GTH_IMAGE_FORMAT_CAIRO_SURFACE;
	self->priv->data.surface = NULL;
}


GthImage *
gth_image_new (void)
{
	return (GthImage *) g_object_new (GTH_TYPE_IMAGE, NULL);
}


GthImage *
gth_image_new_for_surface (cairo_surface_t *surface)
{
	GthImage *image;

	image = gth_image_new ();
	gth_image_set_cairo_surface (image, surface);

	return image;
}


GthImage *
gth_image_new_for_pixbuf (GdkPixbuf *value)
{
	GthImage *image;

	image = gth_image_new ();
	gth_image_set_pixbuf (image, value);

	return image;
}


GthImage *
gth_image_copy (GthImage *image)
{
	GthImage *new_image;

	new_image = gth_image_new ();

	switch (image->priv->format) {
	case GTH_IMAGE_FORMAT_CAIRO_SURFACE:
		new_image->priv->format = GTH_IMAGE_FORMAT_CAIRO_SURFACE;
		new_image->priv->data.surface = _cairo_image_surface_copy (image->priv->data.surface);
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF:
		new_image->priv->format = GTH_IMAGE_FORMAT_GDK_PIXBUF;
		new_image->priv->data.pixbuf = gdk_pixbuf_copy (image->priv->data.pixbuf);
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION:
		new_image->priv->format = GTH_IMAGE_FORMAT_GDK_PIXBUF;
		new_image->priv->data.pixbuf = gdk_pixbuf_copy (gdk_pixbuf_animation_get_static_image (image->priv->data.pixbuf_animation));
		break;

	default:
		break;
	}

	return new_image;
}


void
gth_image_set_cairo_surface (GthImage        *image,
			     cairo_surface_t *value)
{
	_gth_image_free_data (image);
	if (value == NULL)
		return;

	image->priv->format = GTH_IMAGE_FORMAT_CAIRO_SURFACE;
	image->priv->data.surface = cairo_surface_reference (value);
}


cairo_surface_t *
gth_image_get_cairo_surface (GthImage *image)
{
	cairo_surface_t *result = NULL;

	switch (image->priv->format) {
	case GTH_IMAGE_FORMAT_CAIRO_SURFACE:
		result = cairo_surface_reference (image->priv->data.surface);
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF:
		result = _cairo_image_surface_create_from_pixbuf (image->priv->data.pixbuf);
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION:
		if (image->priv->data.pixbuf_animation != NULL) {
			GdkPixbuf *static_image;

			static_image = gdk_pixbuf_animation_get_static_image (image->priv->data.pixbuf_animation);
			result = _cairo_image_surface_create_from_pixbuf (static_image);
		}
		break;

	default:
		break;
	}

	return result;
}


gboolean
gth_image_get_is_zoomable (GthImage *self)
{
	if (self == NULL)
		return FALSE;
	else
		return GTH_IMAGE_GET_CLASS (self)->get_is_zoomable (self);
}


gboolean
gth_image_set_zoom (GthImage *self,
		    double    zoom,
		    int      *original_width,
		    int      *original_height)
{
	return GTH_IMAGE_GET_CLASS (self)->set_zoom (self, zoom, original_width, original_height);
}


void
gth_image_set_pixbuf (GthImage  *image,
		      GdkPixbuf *value)
{
	_gth_image_free_data (image);
	if (value == NULL)
		return;

	image->priv->format = GTH_IMAGE_FORMAT_GDK_PIXBUF;
	image->priv->data.pixbuf = g_object_ref (value);
}


GdkPixbuf *
gth_image_get_pixbuf (GthImage *image)
{
	GdkPixbuf *result = NULL;

	if (image == NULL)
		return NULL;

	switch (image->priv->format) {
	case GTH_IMAGE_FORMAT_CAIRO_SURFACE:
		result = _gdk_pixbuf_new_from_cairo_surface (image->priv->data.surface);
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF:
		result = _g_object_ref (image->priv->data.pixbuf);
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION:
		if (image->priv->data.pixbuf_animation != NULL) {
			GdkPixbuf *static_image;

			static_image = gdk_pixbuf_animation_get_static_image (image->priv->data.pixbuf_animation);
			if (static_image != NULL)
				result = gdk_pixbuf_copy (static_image);
		}
		break;

	default:
		break;
	}

	return result;
}


void
gth_image_set_pixbuf_animation (GthImage           *image,
				GdkPixbufAnimation *value)
{
	_gth_image_free_data (image);
	if (value == NULL)
		return;

	image->priv->format = GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION;
	image->priv->data.pixbuf_animation = g_object_ref (value);
}


GdkPixbufAnimation *
gth_image_get_pixbuf_animation (GthImage *image)
{
	GdkPixbufAnimation *result = NULL;

	switch (image->priv->format) {
	case GTH_IMAGE_FORMAT_CAIRO_SURFACE:
		if (image->priv->data.surface != NULL) {
			GdkPixbuf *pixbuf;

			pixbuf = _gdk_pixbuf_new_from_cairo_surface (image->priv->data.surface);
			result = gdk_pixbuf_non_anim_new (pixbuf);

			g_object_unref (pixbuf);
		}
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF:
		if (image->priv->data.pixbuf != NULL)
			result = gdk_pixbuf_non_anim_new (image->priv->data.pixbuf);
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION:
		result = _g_object_ref (image->priv->data.pixbuf);
		break;

	default:
		break;
	}

	return result;
}


gboolean
gth_image_is_animation (GthImage *image)
{
	return ((image->priv->format == GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION)
	        && (! gdk_pixbuf_animation_is_static_image (image->priv->data.pixbuf_animation)));
}
