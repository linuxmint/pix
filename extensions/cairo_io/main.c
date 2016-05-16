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
#include "cairo-image-surface-jpeg.h"
#include "cairo-image-surface-png.h"
#include "cairo-image-surface-svg.h"
#include "cairo-image-surface-webp.h"
#include "cairo-image-surface-xcf.h"
#include "gth-image-saver-jpeg.h"
#include "gth-image-saver-png.h"
#include "gth-image-saver-tga.h"
#include "gth-image-saver-tiff.h"
#include "gth-image-saver-webp.h"
#include "preferences.h"


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
#ifdef HAVE_LIBJPEG
	gth_main_register_image_loader_func (_cairo_image_surface_create_from_jpeg,
					     GTH_IMAGE_FORMAT_CAIRO_SURFACE,
					     "image/jpeg",
					     NULL);
	gth_main_register_type ("image-saver", GTH_TYPE_IMAGE_SAVER_JPEG);
#endif

	gth_main_register_image_loader_func (_cairo_image_surface_create_from_png,
					     GTH_IMAGE_FORMAT_CAIRO_SURFACE,
					     "image/png",
					     NULL);

#ifdef HAVE_LIBRSVG
	gth_main_register_image_loader_func (_cairo_image_surface_create_from_svg,
					     GTH_IMAGE_FORMAT_CAIRO_SURFACE,
					     "image/svg+xml",
					     NULL);
#endif

	gth_main_register_type ("image-saver", GTH_TYPE_IMAGE_SAVER_PNG);
	gth_main_register_type ("image-saver", GTH_TYPE_IMAGE_SAVER_TGA);
	gth_main_register_type ("image-saver", GTH_TYPE_IMAGE_SAVER_TIFF);

#ifdef HAVE_LIBWEBP
	gth_main_register_image_loader_func (_cairo_image_surface_create_from_webp,
					     GTH_IMAGE_FORMAT_CAIRO_SURFACE,
					     "image/webp",
					     NULL);
	gth_main_register_type ("image-saver", GTH_TYPE_IMAGE_SAVER_WEBP);
#endif

	gth_main_register_image_loader_func (_cairo_image_surface_create_from_xcf,
					     GTH_IMAGE_FORMAT_CAIRO_SURFACE,
					     "image/x-xcf",
					     NULL);

	gth_hook_add_callback ("dlg-preferences-construct", 30, G_CALLBACK (ci__dlg_preferences_construct_cb), NULL);
	gth_hook_add_callback ("dlg-preferences-apply", 10, G_CALLBACK (ci__dlg_preferences_apply_cb), NULL);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
