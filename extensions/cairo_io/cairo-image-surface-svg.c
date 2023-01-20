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
#include <gthumb.h>
#include <librsvg/rsvg.h>
#include "cairo-image-surface-svg.h"


/* GthImageSvg (private class) */


#define GTH_IMAGE_SVG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), gth_image_svg_get_type(), GthImageSvg))


typedef struct {
	GthImage __parent;
	RsvgHandle *rsvg;
	int         original_width;
	int         original_height;
	double      last_zoom;
} GthImageSvg;


typedef GthImageClass GthImageSvgClass;

static gpointer gth_image_svg_parent_class;

GType gth_image_svg_get_type (void);

G_DEFINE_TYPE (GthImageSvg, gth_image_svg, GTH_TYPE_IMAGE)


static void
gth_image_svg_finalize (GObject *object)
{
	GthImageSvg *self;

	self = GTH_IMAGE_SVG (object);
	_g_object_unref (self->rsvg);

        G_OBJECT_CLASS (gth_image_svg_parent_class)->finalize (object);
}


static void
gth_image_svg_init (GthImageSvg *self)
{
	self->rsvg = NULL;
	self->original_width = 0;
	self->original_height = 0;
	self->last_zoom = 0.0;
}


static gboolean
gth_image_svg_get_is_zoomable (GthImage *base)
{
	return (((GthImageSvg *) base)->rsvg != NULL);
}


static gboolean
gth_image_svg_set_zoom (GthImage *base,
			double    zoom,
			int      *original_width,
			int      *original_height)
{
	GthImageSvg     *self;
	cairo_surface_t *surface;
	cairo_t         *cr;
	gboolean         changed = FALSE;

	self = GTH_IMAGE_SVG (base);
	if (self->rsvg == NULL)
		return FALSE;

	if (zoom != self->last_zoom) {
		self->last_zoom = zoom;

		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						      zoom * self->original_width,
						      zoom * self->original_height);
		cr = cairo_create (surface);
		cairo_scale (cr, zoom, zoom);
		rsvg_handle_render_cairo (self->rsvg, cr);
		gth_image_set_cairo_surface (base, surface);
		changed = TRUE;

		cairo_destroy (cr);
		cairo_surface_destroy (surface);
	}

	if (original_width != NULL)
		*original_width = self->original_width;
	if (original_height != NULL)
		*original_height = self->original_height;

	return changed;
}


static void
gth_image_svg_class_init (GthImageSvgClass *klass)
{
	GObjectClass  *object_class;
	GthImageClass *image_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_image_svg_finalize;

	image_class = GTH_IMAGE_CLASS (klass);
	image_class->get_is_zoomable = gth_image_svg_get_is_zoomable;
	image_class->set_zoom = gth_image_svg_set_zoom;
}


static GthImage *
gth_image_svg_new (void)
{
        return g_object_new (gth_image_svg_get_type (), NULL);
}


static gboolean
gth_image_svg_set_handle (GthImageSvg *self,
			  RsvgHandle  *rsvg)
{
	RsvgDimensionData dimension_data;

	if (self->rsvg == rsvg)
		return TRUE;

	rsvg_handle_get_dimensions (rsvg, &dimension_data);
	if ((dimension_data.width == 0) || (dimension_data.height == 0))
		return FALSE;

	self->rsvg = g_object_ref (rsvg);
	self->original_width = dimension_data.width;
	self->original_height = dimension_data.height;

	gth_image_svg_set_zoom (GTH_IMAGE (self), 1.0, NULL, NULL);

	return TRUE;
}


/* -- _cairo_image_surface_create_from_svg -- */


GthImage *
_cairo_image_surface_create_from_svg (GInputStream  *istream,
		       	       	      GthFileData   *file_data,
				      int            requested_size,
				      int           *original_width,
				      int           *original_height,
				      gboolean      *loaded_original,
				      gpointer       user_data,
				      GCancellable  *cancellable,
				      GError       **error)
{
	GthImage   *image;
	RsvgHandle *rsvg;

	image = gth_image_svg_new ();
	rsvg = rsvg_handle_new_from_stream_sync (istream,
						 (file_data != NULL ? file_data->file : NULL),
						 RSVG_HANDLE_FLAGS_NONE,
						 cancellable,
						 error);
	if (rsvg != NULL) {
		if (! gth_image_svg_set_handle (GTH_IMAGE_SVG (image), rsvg)) {
			g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Error");
			g_object_unref (image);
			image = NULL;
		}
		g_object_unref (rsvg);
	}

	return image;
}
