/* -*- Mode: CPP; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2009 Free Software Foundation, Inc.
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

/*
 * gnome-thumbnail.h: Utilities for handling thumbnails
 *
 * Copyright (C) 2002 Red Hat, Inc.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef GNOME_DESKTOP_THUMBNAIL_H
#define GNOME_DESKTOP_THUMBNAIL_H

#include <glib.h>
#include <glib-object.h>
#include <time.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

typedef enum {
  GNOME_DESKTOP_THUMBNAIL_SIZE_NORMAL,
  GNOME_DESKTOP_THUMBNAIL_SIZE_LARGE
} GnomeDesktopThumbnailSize;

#define GNOME_DESKTOP_TYPE_THUMBNAIL_FACTORY		(gnome_desktop_thumbnail_factory_get_type ())
#define GNOME_DESKTOP_THUMBNAIL_FACTORY(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_DESKTOP_TYPE_THUMBNAIL_FACTORY, GnomeDesktopThumbnailFactory))
#define GNOME_DESKTOP_THUMBNAIL_FACTORY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_DESKTOP_TYPE_THUMBNAIL_FACTORY, GnomeDesktopThumbnailFactoryClass))
#define GNOME_DESKTOP_IS_THUMBNAIL_FACTORY(obj)		(G_TYPE_INSTANCE_CHECK_TYPE ((obj), GNOME_DESKTOP_TYPE_THUMBNAIL_FACTORY))
#define GNOME_DESKTOP_IS_THUMBNAIL_FACTORY_CLASS(klass)	(G_TYPE_CLASS_CHECK_CLASS_TYPE ((klass), GNOME_DESKTOP_TYPE_THUMBNAIL_FACTORY))

typedef struct _GnomeDesktopThumbnailFactory        GnomeDesktopThumbnailFactory;
typedef struct _GnomeDesktopThumbnailFactoryClass   GnomeDesktopThumbnailFactoryClass;
typedef struct _GnomeDesktopThumbnailFactoryPrivate GnomeDesktopThumbnailFactoryPrivate;

struct _GnomeDesktopThumbnailFactory {
	GObject parent;

	GnomeDesktopThumbnailFactoryPrivate *priv;
};

struct _GnomeDesktopThumbnailFactoryClass {
	GObjectClass parent;
};

GType                  gnome_desktop_thumbnail_factory_get_type (void);
GnomeDesktopThumbnailFactory *gnome_desktop_thumbnail_factory_new      (GnomeDesktopThumbnailSize     size);

char *                 gnome_desktop_thumbnail_factory_lookup   (GnomeDesktopThumbnailFactory *factory,
								 const char            *uri,
								 time_t                 mtime);

gboolean               gnome_desktop_thumbnail_factory_has_valid_failed_thumbnail (GnomeDesktopThumbnailFactory *factory,
										   const char            *uri,
										   time_t                 mtime);
GdkPixbuf *            gnome_desktop_thumbnail_factory_generate_no_script (GnomeDesktopThumbnailFactory *factory,
									   const char                   *uri,
									   const char                   *mime_type,
									   GCancellable                 *cancellable);
gboolean               gnome_desktop_thumbnail_factory_generate_from_script (GnomeDesktopThumbnailFactory  *factory,
									     const char                    *uri,
									     const char                    *mime_type,
									     GPid                          *pid,
									     char                         **tmpname,
									     GError                       **error);
GdkPixbuf *            gnome_desktop_thumbnail_factory_load_from_tempfile (GnomeDesktopThumbnailFactory  *factory,
									   char                         **tmpname);
void                   gnome_desktop_thumbnail_factory_save_thumbnail (GnomeDesktopThumbnailFactory *factory,
								       GdkPixbuf             *thumbnail,
								       const char            *uri,
								       time_t                 original_mtime);
void                   gnome_desktop_thumbnail_factory_create_failed_thumbnail (GnomeDesktopThumbnailFactory *factory,
										const char            *uri,
										time_t                 mtime);

/* Thumbnailing utils: */

char *     gnome_desktop_thumbnail_md5      (const char *uri);
gboolean   gnome_desktop_thumbnail_is_valid (const char *thumbnail_filename,
					     const char *uri,
					     time_t      mtime);

/* Pixbuf utils */

GdkPixbuf *gnome_desktop_thumbnail_scale_down_pixbuf (GdkPixbuf          *pixbuf,
						      int                 dest_width,
						      int                 dest_height);

G_END_DECLS

#endif /* GNOME_DESKTOP_THUMBNAIL_H */
