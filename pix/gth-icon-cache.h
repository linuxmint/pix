/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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

#ifndef GTH_ICON_CACHE_H
#define GTH_ICON_CACHE_H

#include <gtk/gtk.h>
#include <cairo.h>

G_BEGIN_DECLS

typedef struct _GthIconCache GthIconCache;

GthIconCache *		gth_icon_cache_new		(GtkIconTheme *icon_theme,
							 int           icon_size);
GthIconCache *		gth_icon_cache_new_for_widget	(GtkWidget    *widget,
							 GtkIconSize   icon_size);
void			gth_icon_cache_set_fallback	(GthIconCache *icon_cache,
							 GIcon        *icon);
void			gth_icon_cache_free		(GthIconCache *icon_cache);
void			gth_icon_cache_clear		(GthIconCache *icon_cache);
GdkPixbuf *		gth_icon_cache_get_pixbuf	(GthIconCache *icon_cache,
							 GIcon        *icon);
cairo_surface_t *	gth_icon_cache_get_surface	(GthIconCache *icon_cache,
							 GIcon        *icon);

G_END_DECLS

#endif /* GTH_ICON_CACHE_H */
