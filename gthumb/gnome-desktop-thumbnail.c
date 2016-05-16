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
 * gnome-thumbnail.c: Utilities for handling thumbnails
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

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>
#include <png.h>
#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#define GNOME_DESKTOP_USE_UNSTABLE_API
#include "gnome-desktop-thumbnail.h"
#include "gth-hook.h"
#include "pixbuf-utils.h"

#define SECONDS_BETWEEN_STATS 10

struct _GnomeDesktopThumbnailFactoryPrivate {
  GnomeDesktopThumbnailSize size;
  gboolean use_xdg_dir;

  GMutex lock;

  GList *thumbnailers;
  GHashTable *mime_types_map;
  GList *monitors;

  GSettings *settings;
  gboolean loaded : 1;
  gboolean disabled : 1;
  gchar **disabled_types;
};

static const char *appname = "gnome-thumbnail-factory";

static void gnome_desktop_thumbnail_factory_init          (GnomeDesktopThumbnailFactory      *factory);
static void gnome_desktop_thumbnail_factory_class_init    (GnomeDesktopThumbnailFactoryClass *class);

G_DEFINE_TYPE (GnomeDesktopThumbnailFactory,
	       gnome_desktop_thumbnail_factory,
	       G_TYPE_OBJECT)
#define parent_class gnome_desktop_thumbnail_factory_parent_class

#define GNOME_DESKTOP_THUMBNAIL_FACTORY_GET_PRIVATE(object) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object), GNOME_DESKTOP_TYPE_THUMBNAIL_FACTORY, GnomeDesktopThumbnailFactoryPrivate))

typedef struct {
    gint width;
    gint height;
    gint input_width;
    gint input_height;
    gboolean preserve_aspect_ratio;
} SizePrepareContext;

#define LOAD_BUFFER_SIZE 4096

static void
size_prepared_cb (GdkPixbufLoader *loader,
		  int              width,
		  int              height,
		  gpointer         data)
{
  SizePrepareContext *info = data;

  g_return_if_fail (width > 0 && height > 0);

  info->input_width = width;
  info->input_height = height;

  if (width < info->width && height < info->height) return;

  if (info->preserve_aspect_ratio &&
      (info->width > 0 || info->height > 0)) {
    if (info->width < 0)
      {
	width = width * (double)info->height/(double)height;
	height = info->height;
      }
    else if (info->height < 0)
      {
	height = height * (double)info->width/(double)width;
	width = info->width;
      }
    else if ((double)height * (double)info->width >
	     (double)width * (double)info->height) {
      width = 0.5 + (double)width * (double)info->height / (double)height;
      height = info->height;
    } else {
      height = 0.5 + (double)height * (double)info->width / (double)width;
      width = info->width;
    }
  } else {
    if (info->width > 0)
      width = info->width;
    if (info->height > 0)
      height = info->height;
  }

  gdk_pixbuf_loader_set_size (loader, width, height);
}

static GdkPixbuf *
_gdk_pixbuf_new_from_uri_at_scale (const char   *uri,
				   gint          width,
				   gint          height,
				   gboolean      preserve_aspect_ratio,
				   gboolean      load_from_preview_icon,
				   GCancellable *cancellable)
{
	gboolean                result;
	char                    buffer[LOAD_BUFFER_SIZE];
	gsize                   bytes_read;
	GdkPixbufLoader        *loader;
	GdkPixbuf              *pixbuf;
	GdkPixbufAnimation     *animation;
	GdkPixbufAnimationIter *iter;
	gboolean                has_frame;
	SizePrepareContext      info;
	GFile                  *file;
	GInputStream           *input_stream;

	g_return_val_if_fail (uri != NULL, NULL);

	input_stream = NULL;

	file = g_file_new_for_uri (uri);

	if (load_from_preview_icon) { /* see if we can get an input stream via preview::icon  */
		GFileInfo *file_info;

		file_info = g_file_query_info (file,
					       G_FILE_ATTRIBUTE_PREVIEW_ICON,
					       G_FILE_QUERY_INFO_NONE,
					       cancellable,  /* GCancellable */
					       NULL); /* return location for GError */

		if (file_info != NULL) {
			GObject *object;

			object = g_file_info_get_attribute_object (file_info, G_FILE_ATTRIBUTE_PREVIEW_ICON);
			if ((object != NULL) && G_IS_LOADABLE_ICON (object))
				input_stream = g_loadable_icon_load (G_LOADABLE_ICON (object),
								     0,     /* size */
								     NULL,  /* return location for type */
								     cancellable,  /* GCancellable */
								     NULL); /* return location for GError */

			g_object_unref (file_info);
		}
	}
	else
		input_stream = G_INPUT_STREAM (g_file_read (file, NULL, NULL));

	if (input_stream == NULL) {
		g_object_unref (file);
		return NULL;
	}

	loader = gdk_pixbuf_loader_new ();
	if (1 <= width || 1 <= height) {
		info.width = width;
		info.height = height;
		info.input_width = info.input_height = 0;
		info.preserve_aspect_ratio = preserve_aspect_ratio;
		g_signal_connect (loader, "size-prepared", G_CALLBACK (size_prepared_cb), &info);
	}

	has_frame = FALSE;

	result = FALSE;
	while (!has_frame) {
		bytes_read = g_input_stream_read (input_stream,
						  buffer,
						  sizeof (buffer),
						  cancellable,
						  NULL);
		if (bytes_read == -1)
			break;

		result = TRUE;
		if (bytes_read == 0)
			break;

		if (!gdk_pixbuf_loader_write (loader,
					      (unsigned char *)buffer,
					      bytes_read,
					      NULL)) {
			result = FALSE;
			break;
		}

		animation = gdk_pixbuf_loader_get_animation (loader);
		if (animation) {
			iter = gdk_pixbuf_animation_get_iter (animation, NULL);
			if (!gdk_pixbuf_animation_iter_on_currently_loading_frame (iter))
				has_frame = TRUE;

			g_object_unref (iter);
		}
	}

	gdk_pixbuf_loader_close (loader, NULL);

	if (!result) {
		g_object_unref (G_OBJECT (loader));
		g_input_stream_close (input_stream, NULL, NULL);
		g_object_unref (input_stream);
		g_object_unref (file);
		return NULL;
	}

	g_input_stream_close (input_stream, NULL, NULL);
	g_object_unref (input_stream);
	g_object_unref (file);

	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	if (pixbuf != NULL) {
		g_object_ref (G_OBJECT (pixbuf));
		g_object_set_data (G_OBJECT (pixbuf), "gnome-original-width",
				   GINT_TO_POINTER (info.input_width));
		g_object_set_data (G_OBJECT (pixbuf), "gnome-original-height",
				   GINT_TO_POINTER (info.input_height));
	}
	g_object_unref (G_OBJECT (loader));

	return pixbuf;
}

#define LOAD_BUFFER_SIZE 4096

#define THUMBNAILER_ENTRY_GROUP "Thumbnailer Entry"
#define THUMBNAILER_EXTENSION   ".thumbnailer"

typedef struct {
    volatile gint ref_count;

    gchar *path;

    gchar  *try_exec;
    gchar  *command;
    gchar **mime_types;
} Thumbnailer;

static Thumbnailer *
thumbnailer_ref (Thumbnailer *thumb)
{
  g_return_val_if_fail (thumb != NULL, NULL);
  g_return_val_if_fail (thumb->ref_count > 0, NULL);

  g_atomic_int_inc (&thumb->ref_count);
  return thumb;
}

static void
thumbnailer_unref (Thumbnailer *thumb)
{
  g_return_if_fail (thumb != NULL);
  g_return_if_fail (thumb->ref_count > 0);

  if (g_atomic_int_dec_and_test (&thumb->ref_count))
    {
      g_free (thumb->path);
      g_free (thumb->try_exec);
      g_free (thumb->command);
      g_strfreev (thumb->mime_types);

      g_slice_free (Thumbnailer, thumb);
    }
}

static Thumbnailer *
thumbnailer_load (Thumbnailer *thumb)
{
  GKeyFile *key_file;
  GError *error = NULL;

  key_file = g_key_file_new ();
  if (!g_key_file_load_from_file (key_file, thumb->path, 0, &error))
    {
      g_warning ("Failed to load thumbnailer from \"%s\": %s\n", thumb->path, error->message);
      g_error_free (error);
      thumbnailer_unref (thumb);
      g_key_file_free (key_file);

      return NULL;
    }

  if (!g_key_file_has_group (key_file, THUMBNAILER_ENTRY_GROUP))
    {
      g_warning ("Invalid thumbnailer: missing group \"%s\"\n", THUMBNAILER_ENTRY_GROUP);
      thumbnailer_unref (thumb);
      g_key_file_free (key_file);

      return NULL;
    }

  thumb->command = g_key_file_get_string (key_file, THUMBNAILER_ENTRY_GROUP, "Exec", NULL);
  if (!thumb->command)
    {
      g_warning ("Invalid thumbnailer: missing Exec key\n");
      thumbnailer_unref (thumb);
      g_key_file_free (key_file);

      return NULL;
    }

  thumb->mime_types = g_key_file_get_string_list (key_file, THUMBNAILER_ENTRY_GROUP, "MimeType", NULL, NULL);
  if (!thumb->mime_types)
    {
      g_warning ("Invalid thumbnailer: missing MimeType key\n");
      thumbnailer_unref (thumb);
      g_key_file_free (key_file);

      return NULL;
    }

  thumb->try_exec = g_key_file_get_string (key_file, THUMBNAILER_ENTRY_GROUP, "TryExec", NULL);

  g_key_file_free (key_file);

  return thumb;
}

static Thumbnailer *
thumbnailer_reload (Thumbnailer *thumb)
{
  g_return_val_if_fail (thumb != NULL, NULL);

  g_free (thumb->command);
  thumb->command = NULL;
  g_strfreev (thumb->mime_types);
  thumb->mime_types = NULL;
  g_free (thumb->try_exec);
  thumb->try_exec = NULL;

  return thumbnailer_load (thumb);
}

static Thumbnailer *
thumbnailer_new (const gchar *path)
{
  Thumbnailer *thumb;

  thumb = g_slice_new0 (Thumbnailer);
  thumb->ref_count = 1;
  thumb->path = g_strdup (path);

  return thumbnailer_load (thumb);
}


static gpointer
init_thumbnailers_dirs (gpointer data)
{
  const gchar * const *data_dirs;
  gchar **thumbs_dirs;
  guint i, length;

  data_dirs = g_get_system_data_dirs ();
  length = g_strv_length ((char **) data_dirs);

  thumbs_dirs = g_new (gchar *, length + 2);
  thumbs_dirs[0] = g_build_filename (g_get_user_data_dir (), "thumbnailers", NULL);
  for (i = 0; i < length; i++)
    thumbs_dirs[i + 1] = g_build_filename (data_dirs[i], "thumbnailers", NULL);
  thumbs_dirs[length + 1] = NULL;

  return thumbs_dirs;
}

static const gchar * const *
get_thumbnailers_dirs (void)
{
  static GOnce once_init = G_ONCE_INIT;
  return g_once (&once_init, init_thumbnailers_dirs, NULL);
}


/* These should be called with the lock held */
static void
gnome_desktop_thumbnail_factory_register_mime_types (GnomeDesktopThumbnailFactory *factory,
                                                     Thumbnailer                  *thumb)
{
  GnomeDesktopThumbnailFactoryPrivate *priv = factory->priv;
  gint i;

  for (i = 0; thumb->mime_types[i]; i++)
    {
      if (!g_hash_table_lookup (priv->mime_types_map, thumb->mime_types[i]))
        g_hash_table_insert (priv->mime_types_map,
                             g_strdup (thumb->mime_types[i]),
                             thumbnailer_ref (thumb));
    }
}

static void
gnome_desktop_thumbnail_factory_add_thumbnailer (GnomeDesktopThumbnailFactory *factory,
                                                 Thumbnailer                  *thumb)
{
  GnomeDesktopThumbnailFactoryPrivate *priv = factory->priv;

  gnome_desktop_thumbnail_factory_register_mime_types (factory, thumb);
  priv->thumbnailers = g_list_prepend (priv->thumbnailers, thumb);
}


static gboolean
gnome_desktop_thumbnail_factory_is_disabled (GnomeDesktopThumbnailFactory *factory,
                                             const gchar                  *mime_type)
{
  GnomeDesktopThumbnailFactoryPrivate *priv = factory->priv;
  guint i;

  if (priv->disabled)
    return TRUE;

  if (!priv->disabled_types)
    return FALSE;

  for (i = 0; priv->disabled_types[i]; i++)
    {
      if (g_strcmp0 (priv->disabled_types[i], mime_type) == 0)
        return TRUE;
    }

  return FALSE;
}


static gboolean
remove_thumbnailer_from_mime_type_map (gchar       *key,
                                       Thumbnailer *value,
                                       gchar       *path)
{
  return (strcmp (value->path, path) == 0);
}


static void
update_or_create_thumbnailer (GnomeDesktopThumbnailFactory *factory,
                              const gchar                  *path)
{
  GnomeDesktopThumbnailFactoryPrivate *priv = factory->priv;
  GList *l;
  Thumbnailer *thumb;
  gboolean found = FALSE;

  g_mutex_lock (&priv->lock);

  for (l = priv->thumbnailers; l && !found; l = g_list_next (l))
    {
      thumb = (Thumbnailer *)l->data;

      if (strcmp (thumb->path, path) == 0)
        {
          found = TRUE;

          /* First remove the mime_types associated to this thumbnailer */
          g_hash_table_foreach_remove (priv->mime_types_map,
                                       (GHRFunc)remove_thumbnailer_from_mime_type_map,
                                       (gpointer)path);
          if (!thumbnailer_reload (thumb))
              priv->thumbnailers = g_list_delete_link (priv->thumbnailers, l);
          else
              gnome_desktop_thumbnail_factory_register_mime_types (factory, thumb);
        }
    }

  if (!found)
    {
      thumb = thumbnailer_new (path);
      if (thumb)
        gnome_desktop_thumbnail_factory_add_thumbnailer (factory, thumb);
    }

  g_mutex_unlock (&priv->lock);
}


static void
remove_thumbnailer (GnomeDesktopThumbnailFactory *factory,
                    const gchar                  *path)
{
  GnomeDesktopThumbnailFactoryPrivate *priv = factory->priv;
  GList *l;
  Thumbnailer *thumb;

  g_mutex_lock (&priv->lock);

  for (l = priv->thumbnailers; l; l = g_list_next (l))
    {
      thumb = (Thumbnailer *)l->data;

      if (strcmp (thumb->path, path) == 0)
        {
          priv->thumbnailers = g_list_delete_link (priv->thumbnailers, l);
          g_hash_table_foreach_remove (priv->mime_types_map,
                                       (GHRFunc)remove_thumbnailer_from_mime_type_map,
                                       (gpointer)path);
          thumbnailer_unref (thumb);

          break;
        }
    }

  g_mutex_unlock (&priv->lock);
}


static void
thumbnailers_directory_changed (GFileMonitor                 *monitor,
                                GFile                        *file,
                                GFile                        *other_file,
                                GFileMonitorEvent             event_type,
                                GnomeDesktopThumbnailFactory *factory)
{
  gchar *path;

  switch (event_type)
    {
    case G_FILE_MONITOR_EVENT_CREATED:
    case G_FILE_MONITOR_EVENT_CHANGED:
    case G_FILE_MONITOR_EVENT_DELETED:
      path = g_file_get_path (file);
      if (!g_str_has_suffix (path, THUMBNAILER_EXTENSION))
        {
          g_free (path);
          return;
        }

      if (event_type == G_FILE_MONITOR_EVENT_DELETED)
        remove_thumbnailer (factory, path);
      else
        update_or_create_thumbnailer (factory, path);

      g_free (path);
      break;
    default:
      break;
    }
}


static void
gnome_desktop_thumbnail_factory_load_thumbnailers (GnomeDesktopThumbnailFactory *factory)
{
  GnomeDesktopThumbnailFactoryPrivate *priv = factory->priv;
  const gchar * const *dirs;
  guint i;

  if (priv->loaded)
    return;

  dirs = get_thumbnailers_dirs ();
  for (i = 0; dirs[i]; i++)
    {
      const gchar *path = dirs[i];
      GDir *dir;
      GFile *dir_file;
      GFileMonitor *monitor;
      const gchar *dirent;

      dir = g_dir_open (path, 0, NULL);
      if (!dir)
        continue;

      /* Monitor dir */
      dir_file = g_file_new_for_path (path);
      monitor = g_file_monitor_directory (dir_file,
                                          G_FILE_MONITOR_NONE,
                                          NULL, NULL);
      if (monitor)
        {
          g_signal_connect (monitor, "changed",
                            G_CALLBACK (thumbnailers_directory_changed),
                            factory);
          priv->monitors = g_list_prepend (priv->monitors, monitor);
        }
      g_object_unref (dir_file);

      while ((dirent = g_dir_read_name (dir)))
        {
          Thumbnailer *thumb;
          gchar       *filename;

          if (!g_str_has_suffix (dirent, THUMBNAILER_EXTENSION))
            continue;

          filename = g_build_filename (path, dirent, NULL);
          thumb = thumbnailer_new (filename);
          g_free (filename);

          if (thumb)
            gnome_desktop_thumbnail_factory_add_thumbnailer (factory, thumb);
        }

      g_dir_close (dir);
    }

  priv->loaded = TRUE;
}


static void
external_thumbnailers_disabled_all_changed_cb (GSettings                    *settings,
                                               const gchar                  *key,
                                               GnomeDesktopThumbnailFactory *factory)
{
	g_mutex_lock (&factory->priv->lock);

	factory->priv->disabled = g_settings_get_boolean (factory->priv->settings, "disable-all");
	if (factory->priv->disabled) {
		g_strfreev (factory->priv->disabled_types);
		factory->priv->disabled_types = NULL;
	}
	else {
		factory->priv->disabled_types = g_settings_get_strv (factory->priv->settings, "disable");
		gnome_desktop_thumbnail_factory_load_thumbnailers (factory);
	}

	g_mutex_unlock (&factory->priv->lock);
}

static void
external_thumbnailers_disabled_changed_cb (GSettings                    *settings,
                                           const gchar                  *key,
                                           GnomeDesktopThumbnailFactory *factory)
{
	g_mutex_lock (&factory->priv->lock);

	if (factory->priv->disabled)
		return;
	g_strfreev (factory->priv->disabled_types);
	factory->priv->disabled_types = g_settings_get_strv (factory->priv->settings, "disable");

	g_mutex_unlock (&factory->priv->lock);
}


static void
gnome_desktop_thumbnail_factory_init_scripts (GnomeDesktopThumbnailFactory *factory)
{
	factory->priv->mime_types_map = g_hash_table_new_full (g_str_hash,
							       g_str_equal,
							       (GDestroyNotify)g_free,
							       (GDestroyNotify)thumbnailer_unref);
	factory->priv->settings = g_settings_new ("org.gnome.desktop.thumbnailers");
	factory->priv->disabled = g_settings_get_boolean (factory->priv->settings, "disable-all");
	if (! factory->priv->disabled)
		factory->priv->disabled_types = g_settings_get_strv (factory->priv->settings, "disable");
	g_signal_connect (factory->priv->settings,
			  "changed::disable-all",
			  G_CALLBACK (external_thumbnailers_disabled_all_changed_cb),
			  factory);
	g_signal_connect (factory->priv->settings,
			  "changed::disable",
			  G_CALLBACK (external_thumbnailers_disabled_changed_cb),
			  factory);

	if (! factory->priv->disabled)
		gnome_desktop_thumbnail_factory_load_thumbnailers (factory);
}


static void
gnome_desktop_thumbnail_factory_finalize_scripts (GnomeDesktopThumbnailFactory *factory)
{
	if (factory->priv->thumbnailers) {
		g_list_free_full (factory->priv->thumbnailers, (GDestroyNotify)thumbnailer_unref);
		factory->priv->thumbnailers = NULL;
	}

	if (factory->priv->mime_types_map) {
		g_hash_table_destroy (factory->priv->mime_types_map);
		factory->priv->mime_types_map = NULL;
	}

	if (factory->priv->monitors) {
		g_list_free_full (factory->priv->monitors, (GDestroyNotify)g_object_unref);
		factory->priv->monitors = NULL;
	}

	if (factory->priv->disabled_types) {
		g_strfreev (factory->priv->disabled_types);
		factory->priv->disabled_types = NULL;
	}

	if (factory->priv->settings) {
		g_object_unref (factory->priv->settings);
		factory->priv->settings = NULL;
	}
}


static char *
gnome_desktop_thumbnail_factory_get_script (GnomeDesktopThumbnailFactory *factory,
					    const char                   *mime_type)
{
	char *script = NULL;

	if (!gnome_desktop_thumbnail_factory_is_disabled (factory, mime_type)) {
		Thumbnailer *thumb;

		thumb = g_hash_table_lookup (factory->priv->mime_types_map, mime_type);
		if (thumb)
			script = g_strdup (thumb->command);
	}

	return script;
}


static void
gnome_desktop_thumbnail_factory_finalize (GObject *object)
{
	GnomeDesktopThumbnailFactory *factory;

	factory = GNOME_DESKTOP_THUMBNAIL_FACTORY (object);

	gnome_desktop_thumbnail_factory_finalize_scripts (factory);

	g_mutex_clear (&factory->priv->lock);

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


static void
gnome_desktop_thumbnail_factory_init (GnomeDesktopThumbnailFactory *factory)
{
	factory->priv = GNOME_DESKTOP_THUMBNAIL_FACTORY_GET_PRIVATE (factory);
	factory->priv->size = GNOME_DESKTOP_THUMBNAIL_SIZE_NORMAL;
	g_mutex_init (&factory->priv->lock);

	gnome_desktop_thumbnail_factory_init_scripts (factory);

}

static void
gnome_desktop_thumbnail_factory_class_init (GnomeDesktopThumbnailFactoryClass *class)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = gnome_desktop_thumbnail_factory_finalize;

  g_type_class_add_private (class, sizeof (GnomeDesktopThumbnailFactoryPrivate));
}

/**
 * gnome_desktop_thumbnail_factory_new:
 * @size: The thumbnail size to use
 *
 * Creates a new #GnomeDesktopThumbnailFactory.
 *
 * This function must be called on the main thread.
 *
 * Return value: a new #GnomeDesktopThumbnailFactory
 *
 * Since: 2.2
 **/
GnomeDesktopThumbnailFactory *
gnome_desktop_thumbnail_factory_new (GnomeDesktopThumbnailSize size)
{
  GnomeDesktopThumbnailFactory *factory;

  factory = g_object_new (GNOME_DESKTOP_TYPE_THUMBNAIL_FACTORY, NULL);

  factory->priv->size = size;

#if FORCE_XDG_THUMBNAIL_DIR

  factory->priv->use_xdg_dir = TRUE;

#else

  /* use the xdg directory only if it already exists */

  char *xdg_directory = g_build_filename (g_get_user_cache_dir (), "thumbnails", NULL);
  factory->priv->use_xdg_dir = g_file_test (xdg_directory, G_FILE_TEST_EXISTS);
  g_free (xdg_directory);

#endif

  return factory;
}


static char *
thumbnail_factory_build_filename (GnomeDesktopThumbnailFactory *factory,
						const char *first_element,
						...)
{
	char       *base_dir;
	GString    *path;
	va_list     var_args;
	const char *element;

	if (factory->priv->use_xdg_dir)
		base_dir = g_build_filename (g_get_user_cache_dir (),
					     "thumbnails",
					     NULL);
	else
		base_dir = g_build_filename (g_get_home_dir (),
					     ".thumbnails",
					     NULL);
	path = g_string_new (base_dir);
	g_free (base_dir);

	va_start (var_args, first_element);
	element = first_element;
  	while (element != NULL) {
  		if (element[0] != '\0') {
  			g_string_append (path, G_DIR_SEPARATOR_S);
  			g_string_append (path, element);
  		}
		element = va_arg (var_args, const char *);
	}
	va_end (var_args);

	return g_string_free (path, FALSE);
}


/**
 * gnome_desktop_thumbnail_factory_lookup:
 * @factory: a #GnomeDesktopThumbnailFactory
 * @uri: the uri of a file
 * @mtime: the mtime of the file
 *
 * Tries to locate an existing thumbnail for the file specified.
 *
 * Usage of this function is threadsafe.
 *
 * Return value: The absolute path of the thumbnail, or %NULL if none exist.
 *
 * Since: 2.2
 **/
char *
gnome_desktop_thumbnail_factory_lookup (GnomeDesktopThumbnailFactory *factory,
					const char                   *uri,
					time_t                        mtime)
{
	GChecksum *checksum;
	guint8     digest[16];
	gsize      digest_len = sizeof (digest);
	char      *file;
	char      *path;

	g_return_val_if_fail (uri != NULL, NULL);

	checksum = g_checksum_new (G_CHECKSUM_MD5);
	g_checksum_update (checksum, (const guchar *) uri, strlen (uri));
	g_checksum_get_digest (checksum, digest, &digest_len);
	g_assert (digest_len == 16);

	file = g_strconcat (g_checksum_get_string (checksum), ".png", NULL);
	path = thumbnail_factory_build_filename (factory,
							       (factory->priv->size == GNOME_DESKTOP_THUMBNAIL_SIZE_NORMAL)?"normal":"large",
							       file,
							       NULL);

	if (! gnome_desktop_thumbnail_is_valid (path, uri, mtime)) {
		g_free (path);
		path = NULL;
	}

	g_free (file);
	g_checksum_free (checksum);

	return path;
}


/**
 * gnome_desktop_thumbnail_factory_has_valid_failed_thumbnail:
 * @factory: a #GnomeDesktopThumbnailFactory
 * @uri: the uri of a file
 * @mtime: the mtime of the file
 *
 * Tries to locate an failed thumbnail for the file specified. Writing
 * and looking for failed thumbnails is important to avoid to try to
 * thumbnail e.g. broken images several times.
 *
 * Usage of this function is threadsafe.
 *
 * Return value: TRUE if there is a failed thumbnail for the file.
 *
 * Since: 2.2
 **/
gboolean
gnome_desktop_thumbnail_factory_has_valid_failed_thumbnail (GnomeDesktopThumbnailFactory *factory,
							    const char                   *uri,
							    time_t                        mtime)
{
	GChecksum *checksum;
	guint8     digest[16];
	gsize      digest_len = sizeof (digest);
	char      *path;
	char      *file;
	gboolean   res;

	checksum = g_checksum_new (G_CHECKSUM_MD5);
	g_checksum_update (checksum, (const guchar *) uri, strlen (uri));
	g_checksum_get_digest (checksum, digest, &digest_len);
	g_assert (digest_len == 16);

	file = g_strconcat (g_checksum_get_string (checksum), ".png", NULL);
	path = thumbnail_factory_build_filename (factory,
							       "fail",
							       appname,
							       file,
							       NULL);
	res = gnome_desktop_thumbnail_is_valid (path, uri, mtime);

	g_free (path);
	g_free (file);
	g_checksum_free (checksum);

	return res;
}

static char *
expand_thumbnailing_script (const char *script,
			    const int   size,
			    const char *inuri,
			    const char *outfile)
{
  GString *str;
  const char *p, *last;
  char *localfile, *quoted;
  gboolean got_in;

  str = g_string_new (NULL);

  got_in = FALSE;
  last = script;
  while ((p = strchr (last, '%')) != NULL)
    {
      g_string_append_len (str, last, p - last);
      p++;

      switch (*p) {
      case 'u':
	quoted = g_shell_quote (inuri);
	g_string_append (str, quoted);
	g_free (quoted);
	got_in = TRUE;
	p++;
	break;
      case 'i':
	localfile = g_filename_from_uri (inuri, NULL, NULL);
	if (localfile)
	  {
	    quoted = g_shell_quote (localfile);
	    g_string_append (str, quoted);
	    got_in = TRUE;
	    g_free (quoted);
	    g_free (localfile);
	  }
	p++;
	break;
      case 'o':
	quoted = g_shell_quote (outfile);
	g_string_append (str, quoted);
	g_free (quoted);
	p++;
	break;
      case 's':
	g_string_append_printf (str, "%d", size);
	p++;
	break;
      case '%':
	g_string_append_c (str, '%');
	p++;
	break;
      case 0:
      default:
	break;
      }
      last = p;
    }
  g_string_append (str, last);

  if (got_in)
    return g_string_free (str, FALSE);

  g_string_free (str, TRUE);
  return NULL;
}


GdkPixbuf *
gnome_desktop_thumbnail_factory_generate_no_script (GnomeDesktopThumbnailFactory *factory,
						    const char                   *uri,
						    const char                   *mime_type,
						    GCancellable                 *cancellable)
{
  GdkPixbuf *pixbuf, *scaled, *tmp_pixbuf;
  int width, height, size;
  int original_width = 0;
  int original_height = 0;
  char dimension[12];
  double scale;

  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);

  /* Doesn't access any volatile fields in factory, so it's threadsafe */

  size = 128;
  if (factory->priv->size == GNOME_DESKTOP_THUMBNAIL_SIZE_LARGE)
    size = 256;

  /* Check for preview::icon first */
  pixbuf = _gdk_pixbuf_new_from_uri_at_scale (uri, size, size, TRUE, TRUE, cancellable);

  /* ...then use a registered thumbnail generator (the exiv2 extension tries
   * to read the embedded thumbnail) */
  if (pixbuf == NULL)
    pixbuf = gth_hook_invoke_get ("generate-thumbnail", (char *) uri, mime_type, size);

#if 0
  /* ...lastly try the whole file */
  if (pixbuf == NULL)
    pixbuf = _gdk_pixbuf_new_from_uri_at_scale (uri, size, size, TRUE, FALSE, cancellable);
#endif

  if (pixbuf == NULL)
    return NULL;

  if (pixbuf != NULL)
    {
      original_width = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (pixbuf),
                                                           "gnome-original-width"));
      original_height = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (pixbuf),
                                                            "gnome-original-height"));
    }

  /* The pixbuf loader may attach an "orientation" option to the pixbuf,
     if the tiff or exif jpeg file had an orientation tag. Rotate/flip
     the pixbuf as specified by this tag, if present. */
  tmp_pixbuf = gdk_pixbuf_apply_embedded_orientation (pixbuf);
  g_object_unref (pixbuf);
  pixbuf = tmp_pixbuf;

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  if (width > size || height > size)
    {
      const gchar *orig_width, *orig_height;
      scale = (double)size / MAX (width, height);

      /* if the scale factor is small, use bilinear interpolation for better quality */
      if ((scale >= 0.5) && (scale <= 2))
	      scaled = _gdk_pixbuf_scale_simple_safe (pixbuf,
						      floor (width * scale + 0.5),
						      floor (height * scale + 0.5),
						      GDK_INTERP_BILINEAR);
      else
	      scaled = gnome_desktop_thumbnail_scale_down_pixbuf (pixbuf,
							          floor (width * scale + 0.5),
							          floor (height * scale + 0.5));

      orig_width = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Image::Width");
      orig_height = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Image::Height");

      if (orig_width != NULL) {
	      gdk_pixbuf_set_option (scaled, "tEXt::Thumb::Image::Width", orig_width);
      }
      if (orig_height != NULL) {
	      gdk_pixbuf_set_option (scaled, "tEXt::Thumb::Image::Height", orig_height);
      }

      g_object_unref (pixbuf);
      pixbuf = scaled;
    }

  if (original_width > 0)
    {
      g_snprintf (dimension, sizeof (dimension), "%i", original_width);
      gdk_pixbuf_set_option (pixbuf, "tEXt::Thumb::Image::Width", dimension);
      g_object_set_data (G_OBJECT (pixbuf), "gnome-original-width", GINT_TO_POINTER (original_width));
    }
  if (original_height > 0)
    {
      g_snprintf (dimension, sizeof (dimension), "%i", original_height);
      gdk_pixbuf_set_option (pixbuf, "tEXt::Thumb::Image::Height", dimension);
      g_object_set_data (G_OBJECT (pixbuf), "gnome-original-height", GINT_TO_POINTER (original_height));
    }

  return pixbuf;
}


gboolean
gnome_desktop_thumbnail_factory_generate_from_script (GnomeDesktopThumbnailFactory  *factory,
						      const char                    *uri,
						      const char                    *mime_type,
						      GPid                          *pid,
						      char                         **tmpname,
						      GError                       **error)
{
	gboolean   retval = FALSE;
	int        size;
	char      *script;
	int        fd;
	char      *expanded_script;
	int        argc;
	char     **argv;

	g_return_val_if_fail (uri != NULL, FALSE);
	g_return_val_if_fail (mime_type != NULL, FALSE);

	size = 128;
	if (factory->priv->size == GNOME_DESKTOP_THUMBNAIL_SIZE_LARGE)
		size = 256;

	script = NULL;
	g_mutex_lock (&factory->priv->lock);
	script = gnome_desktop_thumbnail_factory_get_script (factory, mime_type);
	g_mutex_unlock (&factory->priv->lock);

	if (script == NULL) {
		*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, "No script");
		return FALSE;
	}

	fd = g_file_open_tmp (".gnome_desktop_thumbnail.XXXXXX", tmpname, error);
	if (fd == -1)
		return FALSE;
	close (fd);

	expanded_script = expand_thumbnailing_script (script, size, uri, *tmpname);
	if (g_shell_parse_argv (expanded_script, &argc, &argv, error))
		if (g_spawn_async (g_get_tmp_dir (),
				   argv,
				   NULL,
				   G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
				   NULL,
				   NULL,
				   pid,
				   error))
		{
			retval = TRUE;
		}

	g_free (expanded_script);
	g_free (script);

	return retval;
}


GdkPixbuf *
gnome_desktop_thumbnail_factory_load_from_tempfile (GnomeDesktopThumbnailFactory  *factory,
						    char                         **tmpname)
{
	GdkPixbuf *pixbuf;
	GdkPixbuf *tmp_pixbuf;

	pixbuf = gdk_pixbuf_new_from_file (*tmpname, NULL);
	g_unlink (*tmpname);
	g_free (*tmpname);
	*tmpname = NULL;

	if (pixbuf == NULL)
		return NULL;

	tmp_pixbuf = gdk_pixbuf_apply_embedded_orientation (pixbuf);
	g_object_unref (pixbuf);
	pixbuf = tmp_pixbuf;

	return pixbuf;
}


static gboolean
make_thumbnail_dirs (GnomeDesktopThumbnailFactory *factory)
{
  char *thumbnail_dir;
  char *image_dir;
  gboolean res;

  res = FALSE;

  thumbnail_dir = thumbnail_factory_build_filename (factory, NULL);
  if (!g_file_test (thumbnail_dir, G_FILE_TEST_IS_DIR))
    {
      g_mkdir (thumbnail_dir, 0700);
      res = TRUE;
    }

  image_dir = g_build_filename (thumbnail_dir,
				(factory->priv->size == GNOME_DESKTOP_THUMBNAIL_SIZE_NORMAL)?"normal":"large",
				NULL);
  if (!g_file_test (image_dir, G_FILE_TEST_IS_DIR))
    {
      g_mkdir (image_dir, 0700);
      res = TRUE;
    }

  g_free (thumbnail_dir);
  g_free (image_dir);

  return res;
}

static gboolean
make_thumbnail_fail_dirs (GnomeDesktopThumbnailFactory *factory)
{
  char *thumbnail_dir;
  char *fail_dir;
  char *app_dir;
  gboolean res;

  res = FALSE;

  thumbnail_dir = thumbnail_factory_build_filename (factory, NULL);
  if (!g_file_test (thumbnail_dir, G_FILE_TEST_IS_DIR))
    {
      g_mkdir (thumbnail_dir, 0700);
      res = TRUE;
    }

  fail_dir = g_build_filename (thumbnail_dir,
			       "fail",
			       NULL);
  if (!g_file_test (fail_dir, G_FILE_TEST_IS_DIR))
    {
      g_mkdir (fail_dir, 0700);
      res = TRUE;
    }

  app_dir = g_build_filename (fail_dir,
			      appname,
			      NULL);
  if (!g_file_test (app_dir, G_FILE_TEST_IS_DIR))
    {
      g_mkdir (app_dir, 0700);
      res = TRUE;
    }

  g_free (thumbnail_dir);
  g_free (fail_dir);
  g_free (app_dir);

  return res;
}


/**
 * gnome_desktop_thumbnail_factory_save_thumbnail:
 * @factory: a #GnomeDesktopThumbnailFactory
 * @thumbnail: the thumbnail as a pixbuf
 * @uri: the uri of a file
 * @original_mtime: the modification time of the original file
 *
 * Saves @thumbnail at the right place. If the save fails a
 * failed thumbnail is written.
 *
 * Usage of this function is threadsafe.
 *
 * Since: 2.2
 **/
void
gnome_desktop_thumbnail_factory_save_thumbnail (GnomeDesktopThumbnailFactory *factory,
						GdkPixbuf             *thumbnail,
						const char            *uri,
						time_t                 original_mtime)
{
  GnomeDesktopThumbnailFactoryPrivate *priv = factory->priv;
  char *path, *file;
  char *tmp_path;
  const char *width, *height;
  int tmp_fd;
  char mtime_str[21];
  gboolean saved_ok;
  GChecksum *checksum;
  guint8 digest[16];
  gsize digest_len = sizeof (digest);

  checksum = g_checksum_new (G_CHECKSUM_MD5);
  g_checksum_update (checksum, (const guchar *) uri, strlen (uri));

  g_checksum_get_digest (checksum, digest, &digest_len);
  g_assert (digest_len == 16);

  file = g_strconcat (g_checksum_get_string (checksum), ".png", NULL);
  path = thumbnail_factory_build_filename (factory,
		  	  	  	   (priv->size == GNOME_DESKTOP_THUMBNAIL_SIZE_NORMAL ? "normal" : "large"),
		  	  	  	   file,
		  	  	  	   NULL);

  g_free (file);

  g_checksum_free (checksum);

  tmp_path = g_strconcat (path, ".XXXXXX", NULL);

  tmp_fd = g_mkstemp (tmp_path);
  if (tmp_fd == -1 &&
      make_thumbnail_dirs (factory))
    {
      g_free (tmp_path);
      tmp_path = g_strconcat (path, ".XXXXXX", NULL);
      tmp_fd = g_mkstemp (tmp_path);
    }

  if (tmp_fd == -1)
    {
      gnome_desktop_thumbnail_factory_create_failed_thumbnail (factory, uri, original_mtime);
      g_free (tmp_path);
      g_free (path);
      return;
    }
  close (tmp_fd);

  g_snprintf (mtime_str, 21, "%ld",  original_mtime);
  width = gdk_pixbuf_get_option (thumbnail, "tEXt::Thumb::Image::Width");
  height = gdk_pixbuf_get_option (thumbnail, "tEXt::Thumb::Image::Height");

  if (width != NULL && height != NULL)
    saved_ok  = gdk_pixbuf_save (thumbnail,
				 tmp_path,
				 "png", NULL,
				 "tEXt::Thumb::Image::Width", width,
				 "tEXt::Thumb::Image::Height", height,
				 "tEXt::Thumb::URI", uri,
				 "tEXt::Thumb::MTime", mtime_str,
				 "tEXt::Software", "GNOME::ThumbnailFactory",
				 NULL);
  else
    saved_ok  = gdk_pixbuf_save (thumbnail,
				 tmp_path,
				 "png", NULL,
				 "tEXt::Thumb::URI", uri,
				 "tEXt::Thumb::MTime", mtime_str,
				 "tEXt::Software", "GNOME::ThumbnailFactory",
				 NULL);


  if (saved_ok)
    {
      g_chmod (tmp_path, 0600);
      g_rename(tmp_path, path);
    }
  else
    {
      gnome_desktop_thumbnail_factory_create_failed_thumbnail (factory, uri, original_mtime);
    }

  g_free (path);
  g_free (tmp_path);
}

/**
 * gnome_desktop_thumbnail_factory_create_failed_thumbnail:
 * @factory: a #GnomeDesktopThumbnailFactory
 * @uri: the uri of a file
 * @mtime: the modification time of the file
 *
 * Creates a failed thumbnail for the file so that we don't try
 * to re-thumbnail the file later.
 *
 * Usage of this function is threadsafe.
 *
 * Since: 2.2
 **/
void
gnome_desktop_thumbnail_factory_create_failed_thumbnail (GnomeDesktopThumbnailFactory *factory,
							 const char            *uri,
							 time_t                 mtime)
{
  char *path, *file;
  char *tmp_path;
  int tmp_fd;
  char mtime_str[21];
  gboolean saved_ok;
  GdkPixbuf *pixbuf;
  GChecksum *checksum;
  guint8 digest[16];
  gsize digest_len = sizeof (digest);

  checksum = g_checksum_new (G_CHECKSUM_MD5);
  g_checksum_update (checksum, (const guchar *) uri, strlen (uri));

  g_checksum_get_digest (checksum, digest, &digest_len);
  g_assert (digest_len == 16);

  file = g_strconcat (g_checksum_get_string (checksum), ".png", NULL);
  path = thumbnail_factory_build_filename (factory,
		  	  	  	  	  	 "fail",
		  	  	  	  	  	 appname,
		  	  	  	  	  	 file,
		  	  	  	  	  	 NULL);
  g_free (file);

  g_checksum_free (checksum);

  tmp_path = g_strconcat (path, ".XXXXXX", NULL);

  tmp_fd = g_mkstemp (tmp_path);
  if (tmp_fd == -1 &&
      make_thumbnail_fail_dirs (factory))
    {
      g_free (tmp_path);
      tmp_path = g_strconcat (path, ".XXXXXX", NULL);
      tmp_fd = g_mkstemp (tmp_path);
    }

  if (tmp_fd == -1)
    {
      g_free (tmp_path);
      g_free (path);
      return;
    }
  close (tmp_fd);

  g_snprintf (mtime_str, 21, "%ld",  mtime);
  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);
  saved_ok  = gdk_pixbuf_save (pixbuf,
			       tmp_path,
			       "png", NULL,
			       "tEXt::Thumb::URI", uri,
			       "tEXt::Thumb::MTime", mtime_str,
			       "tEXt::Software", "GNOME::ThumbnailFactory",
			       NULL);
  g_object_unref (pixbuf);
  if (saved_ok)
    {
      g_chmod (tmp_path, 0600);
      g_rename(tmp_path, path);
    }

  g_free (path);
  g_free (tmp_path);
}

/**
 * gnome_desktop_thumbnail_md5:
 * @uri: an uri
 *
 * Calculates the MD5 checksum of the uri. This can be useful
 * if you want to manually handle thumbnail files.
 *
 * Return value: A string with the MD5 digest of the uri string.
 *
 * Since: 2.2
 *
 * @Deprecated: 2.22: Use #GChecksum instead
 **/
char *
gnome_desktop_thumbnail_md5 (const char *uri)
{
  return g_compute_checksum_for_data (G_CHECKSUM_MD5,
                                      (const guchar *) uri,
                                      strlen (uri));
}


/* This function taken from gdk-pixbuf:
 *
 * Copyright (C) 1999 Mark Crichton
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Mark Crichton <crichton@gimp.org>
 *          Federico Mena-Quintero <federico@gimp.org>
 *
 * Released under the LGPL version 2 of the License,
 * or (at your option) any later version.
 *
 **/
static gboolean
png_text_to_pixbuf_option (png_text   text_ptr,
                           gchar    **key,
                           gchar    **value)
{
        gboolean is_ascii = TRUE;
        int i;

        /* Avoid loading iconv if the text is plain ASCII */
        for (i = 0; i < text_ptr.text_length; i++)
                if (text_ptr.text[i] & 0x80) {
                        is_ascii = FALSE;
                        break;
                }

        if (is_ascii) {
                *value = g_strdup (text_ptr.text);
        } else {
                *value = g_convert (text_ptr.text, -1,
                                     "UTF-8", "ISO-8859-1",
                                     NULL, NULL, NULL);
        }

        if (*value) {
                *key = g_strconcat ("tEXt::", text_ptr.key, NULL);
                return TRUE;
        } else {
                g_warning ("Couldn't convert text chunk value to UTF-8.");
                *key = NULL;
                return FALSE;
        }
}


static GHashTable *
read_png_options (const char *thumbnail_filename)
{
	GHashTable  *options;
	png_structp  png_ptr;
        png_infop    info_ptr;
        FILE        *f;
        png_textp    text_ptr;
	int          num_texts;

	options = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
		return options;

	info_ptr = png_create_info_struct (png_ptr);
	if (info_ptr == NULL) {
		png_destroy_read_struct (&png_ptr, NULL, NULL);
		return options;
	}

	f = fopen (thumbnail_filename, "r");
	if (f == NULL) {
		png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
		return options;
	}

	png_init_io (png_ptr, f);
	png_read_info (png_ptr, info_ptr);

	if (png_get_text (png_ptr, info_ptr, &text_ptr, &num_texts)) {
		int i;
		for (i = 0; i < num_texts; i++) {
			char *key;
			char *value;

			if (png_text_to_pixbuf_option (text_ptr[i], &key, &value))
				g_hash_table_insert (options, key, value);
		}
	}

	png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	fclose (f);

	return options;
}


/**
 * gnome_desktop_thumbnail_is_valid:
 * @thumbnail_filename: the png file that contains the thumbnail
 * @uri: a uri
 * @mtime: the mtime
 *
 * Returns whether the thumbnail has the correct uri and mtime embedded in the
 * png options.
 *
 * Return value: TRUE if the thumbnail has the right @uri and @mtime
 *
 * Since: 2.2
 **/
gboolean
gnome_desktop_thumbnail_is_valid (const char *thumbnail_filename,
				  const char *uri,
				  time_t      mtime)
{
	gboolean    is_valid = FALSE;
	GHashTable *png_options;
	const char *thumb_uri;

	png_options = read_png_options (thumbnail_filename);
	thumb_uri = g_hash_table_lookup (png_options, "tEXt::Thumb::URI");
	if (g_strcmp0 (uri, thumb_uri) == 0) {
		const char *thumb_mtime_str;

		thumb_mtime_str = g_hash_table_lookup (png_options, "tEXt::Thumb::MTime");
		if ((thumb_mtime_str != NULL) && (mtime == atol (thumb_mtime_str)))
			is_valid = TRUE;
	}

	g_hash_table_unref (png_options);

	return is_valid;
}
