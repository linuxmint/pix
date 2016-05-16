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

#include <config.h>
#include "glib-utils.h"
#include "gth-icon-cache.h"
#include "gtk-utils.h"


struct _GthIconCache {
	GtkIconTheme *icon_theme;
	int           icon_size;
	GHashTable   *cache;
	GIcon        *fallback_icon;
};


GthIconCache *
gth_icon_cache_new (GtkIconTheme *icon_theme,
		    int           icon_size)
{
	GthIconCache *icon_cache;

	g_return_val_if_fail (icon_theme != NULL, NULL);

	icon_cache = g_new0 (GthIconCache, 1);
	icon_cache->icon_theme = icon_theme;
	icon_cache->icon_size = icon_size;
	icon_cache->cache = g_hash_table_new_full (g_icon_hash, (GEqualFunc) g_icon_equal, g_object_unref, g_object_unref);

	return icon_cache;
}


GthIconCache *
gth_icon_cache_new_for_widget (GtkWidget   *widget,
	                       GtkIconSize  icon_size)
{
	GtkIconTheme *icon_theme;
	int           pixel_size;

	icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));
	pixel_size = _gtk_widget_lookup_for_size (widget, icon_size);

	return gth_icon_cache_new (icon_theme, pixel_size);
}


void
gth_icon_cache_set_fallback (GthIconCache *icon_cache,
			     GIcon        *icon)
{
	if (icon_cache->fallback_icon != NULL)
		g_object_unref (icon_cache->fallback_icon);
	icon_cache->fallback_icon = icon;
}


void
gth_icon_cache_free (GthIconCache *icon_cache)
{
	if (icon_cache == NULL)
		return;
	g_hash_table_destroy (icon_cache->cache);
	if (icon_cache->fallback_icon != NULL)
		g_object_unref (icon_cache->fallback_icon);
	g_free (icon_cache);
}


void
gth_icon_cache_clear (GthIconCache *icon_cache)
{
	if (icon_cache != NULL)
		g_hash_table_remove_all (icon_cache->cache);
}


GdkPixbuf *
gth_icon_cache_get_pixbuf (GthIconCache *icon_cache,
			   GIcon        *icon)
{
	GdkPixbuf *pixbuf = NULL;

	if (icon == NULL)
		icon = icon_cache->fallback_icon;

	if (icon != NULL)
		pixbuf = g_hash_table_lookup (icon_cache->cache, icon);

	if (pixbuf != NULL)
		return g_object_ref (pixbuf);

	if (icon != NULL)
		pixbuf = _g_icon_get_pixbuf (icon, icon_cache->icon_size, icon_cache->icon_theme);

	if ((pixbuf == NULL) && (icon_cache->fallback_icon != NULL))
		pixbuf = _g_icon_get_pixbuf (icon_cache->fallback_icon, icon_cache->icon_size, icon_cache->icon_theme);

	if ((icon != NULL) && (pixbuf != NULL))
		g_hash_table_insert (icon_cache->cache, g_object_ref (icon), g_object_ref (pixbuf));

	return pixbuf;
}
