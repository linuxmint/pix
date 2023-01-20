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

#include <config.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#ifdef HAVE_LCMS2
#include <lcms2.h>
#endif /* HAVE_LCMS2 */
#include "cairo-utils.h"
#include "glib-utils.h"
#include "gth-image.h"
#include "gth-main.h"
#include "pixbuf-utils.h"


/* -- GthImage -- */


struct _GthImagePrivate {
	GthImageFormat format;
	union {
		cairo_surface_t    *surface;
		GdkPixbuf          *pixbuf;
		GdkPixbufAnimation *pixbuf_animation;
	} data;
	GthICCProfile *icc_data;
};


G_DEFINE_TYPE_WITH_CODE (GthImage,
			 gth_image,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthImage))


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
_gth_image_free_icc_profile (GthImage *self)
{
	_g_object_unref (self->priv->icc_data);
	self->priv->icc_data = NULL;
}


static void
gth_image_finalize (GObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE (object));

	_gth_image_free_data (GTH_IMAGE (object));
	_gth_image_free_icc_profile (GTH_IMAGE (object));

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

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_image_finalize;

	klass->get_is_zoomable = base_get_is_zoomable;
	klass->set_zoom = base_set_zoom;
}


static void
gth_image_init (GthImage *self)
{
	self->priv = gth_image_get_instance_private (self);
	self->priv->format = GTH_IMAGE_FORMAT_CAIRO_SURFACE;
	self->priv->data.surface = NULL;
	self->priv->icc_data = NULL;
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
	gth_image_set_icc_profile (new_image, gth_image_get_icc_profile (image));

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
gth_image_get_original_size (GthImage *image,
			     int      *width,
			     int      *height)
{
	cairo_surface_t *surface;
	int              local_width;
	int              local_height;
	gboolean         value_set = FALSE;

	switch (image->priv->format) {
	case GTH_IMAGE_FORMAT_CAIRO_SURFACE:
		surface = gth_image_get_cairo_surface (image);
		if (surface != NULL) {
			if (! _cairo_image_surface_get_original_size (surface, &local_width, &local_height)) {
				local_width = cairo_image_surface_get_width (surface);
				local_height = cairo_image_surface_get_height (surface);
			}
			value_set = TRUE;
		}
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF:
		if (image->priv->data.pixbuf != NULL) {
			local_width = gdk_pixbuf_get_width (image->priv->data.pixbuf);
			local_height = gdk_pixbuf_get_height (image->priv->data.pixbuf);
			value_set = TRUE;
		}
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION:
		if (image->priv->data.pixbuf_animation != NULL) {
			local_width = gdk_pixbuf_animation_get_width (image->priv->data.pixbuf_animation);
			local_height = gdk_pixbuf_animation_get_width (image->priv->data.pixbuf_animation);
			value_set = TRUE;
		}
		break;

	default:
		break;
	}

	if (value_set) {
		if (width != NULL) *width = local_width;
		if (height != NULL) *height = local_height;
	}

	return value_set;
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
gth_image_get_is_null (GthImage *self)
{
	gboolean is_null = TRUE;

	switch (self->priv->format) {
	case GTH_IMAGE_FORMAT_CAIRO_SURFACE:
		is_null = self->priv->data.surface == NULL;
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF:
		is_null = self->priv->data.pixbuf == NULL;
		break;

	case GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION:
		is_null = self->priv->data.pixbuf_animation == NULL;
		break;

	default:
		break;
	}

	return is_null;
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
	image->priv->format = GTH_IMAGE_FORMAT_GDK_PIXBUF;
	image->priv->data.pixbuf = _g_object_ref (value);
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
	image->priv->format = GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION;
	image->priv->data.pixbuf_animation = _g_object_ref (value);
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
gth_image_get_is_animation (GthImage *image)
{
	if (image == NULL)
		return FALSE;

	return ((image->priv->format == GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION)
	        && (! gdk_pixbuf_animation_is_static_image (image->priv->data.pixbuf_animation)));
}


void
gth_image_set_icc_profile (GthImage   *image,
			   GthICCProfile *profile)
{
	g_return_if_fail (image != NULL);

	_g_object_ref (profile);
	_gth_image_free_icc_profile (image);
	image->priv->icc_data = profile;
}


GthICCProfile *
gth_image_get_icc_profile (GthImage *image)
{
	g_return_val_if_fail (image != NULL, NULL);
	return image->priv->icc_data;
}


/* -- gth_image_apply_icc_profile -- */


void
gth_image_apply_icc_profile (GthImage      *image,
			     GthICCProfile *out_profile,
			     GCancellable  *cancellable)
{
#if HAVE_LCMS2

	cairo_surface_t *surface;
	GthICCTransform *transform;

	g_return_if_fail (image != NULL);

	if (out_profile == NULL)
		return;

	if (image->priv->icc_data == NULL)
		return;

	if (image->priv->format != GTH_IMAGE_FORMAT_CAIRO_SURFACE)
		return;

	surface = gth_image_get_cairo_surface (image);
	if (surface == NULL)
		return;

	transform = gth_color_manager_get_transform (gth_main_get_default_color_manager (),
			      	      	      	     image->priv->icc_data,
						     out_profile);

	if (transform != NULL) {
		cmsHTRANSFORM    hTransform;
		unsigned char   *surface_row;
		int              width;
		int              height;
		int              row_stride;
		int              row;

		hTransform = (cmsHTRANSFORM) gth_icc_transform_get_transform (transform);
		surface_row = _cairo_image_surface_flush_and_get_data (surface);
		width = cairo_image_surface_get_width (surface);
		height = cairo_image_surface_get_height (surface);
		row_stride = cairo_image_surface_get_stride (surface);

		for (row = 0; row < height; row++) {
			if (g_cancellable_is_cancelled (cancellable))
				break;
			cmsDoTransform (hTransform, surface_row, surface_row, width);
			surface_row += row_stride;
		}
		cairo_surface_mark_dirty (surface);

		cairo_surface_destroy (surface);
	}

	_g_object_unref (transform);

#endif
}


/* -- gth_image_apply_icc_profile_async -- */


typedef struct {
	GthImage   *image;
	GthICCProfile *out_profile;
} ApplyProfileData;


static void
apply_profile_data_free (gpointer user_data)
{
	ApplyProfileData *apd = user_data;

	g_object_unref (apd->image);
	_g_object_unref (apd->out_profile);
	g_free (apd);
}


static void
_gth_image_apply_icc_profile_thread (GTask        *task,
				     gpointer      source_object,
				     gpointer      task_data,
				     GCancellable *cancellable)
{
	ApplyProfileData *apd;

	apd = g_task_get_task_data (task);
	gth_image_apply_icc_profile (apd->image, apd->out_profile, cancellable);

	if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
		g_task_return_error (task, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, ""));
		return;
	}

	g_task_return_boolean (task, TRUE);
}


void
gth_image_apply_icc_profile_async (GthImage		*image,
				   GthICCProfile	*out_profile,
				   GCancellable		*cancellable,
				   GAsyncReadyCallback	 callback,
				   gpointer		 user_data)
{
	GTask            *task;
	ApplyProfileData *apd;

	g_return_if_fail (image != NULL);

	task = g_task_new (NULL, cancellable, callback, user_data);

	apd = g_new (ApplyProfileData, 1);
	apd->image = g_object_ref (image);
	apd->out_profile = _g_object_ref (out_profile);
	g_task_set_task_data (task, apd, apply_profile_data_free);
	g_task_run_in_thread (task, _gth_image_apply_icc_profile_thread);

	g_object_unref (task);
}


gboolean
gth_image_apply_icc_profile_finish (GAsyncResult  *result,
				    GError	 **error)
{
	return g_task_propagate_boolean (G_TASK (result), error);
}
