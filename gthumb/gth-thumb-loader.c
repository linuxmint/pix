/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2010 Free Software Foundation, Inc.
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <glib/gstdio.h>
#define GDK_PIXBUF_ENABLE_BACKEND
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gio-utils.h"
#include "glib-utils.h"
#define GNOME_DESKTOP_USE_UNSTABLE_API
#include "gnome-desktop-thumbnail.h"
#include "gth-error.h"
#include "gth-image-loader.h"
#include "gth-image-utils.h"
#include "gth-main.h"
#include "gth-thumb-loader.h"
#include "pixbuf-io.h"
#include "pixbuf-utils.h"
#include "typedefs.h"

#define THUMBNAIL_LARGE_SIZE	  256
#define THUMBNAIL_NORMAL_SIZE	  128
#define THUMBNAIL_DIR_PERMISSIONS 0700
#define MAX_THUMBNAILER_LIFETIME  4000   /* kill the thumbnailer after this amount of time*/
#define CHECK_CANCELLABLE_DELAY   200

struct _GthThumbLoaderPrivate
{
	GthImageLoader   *iloader;
	GthImageLoader   *tloader;
	guint             use_cache : 1;
	guint             save_thumbnails : 1;
	int               requested_size;
	int               cache_max_size;
	goffset           max_file_size;         /* If the file size is greater
					    	  * than this the thumbnail
					    	  * will not be created, for
					    	  * functionality reasons. */
	GnomeDesktopThumbnailSize
			  thumb_size;
	GnomeDesktopThumbnailFactory
			 *thumb_factory;
};


enum {
	READY,
	LAST_SIGNAL
};


G_DEFINE_TYPE (GthThumbLoader, gth_thumb_loader, G_TYPE_OBJECT)


static void
gth_thumb_loader_finalize (GObject *object)
{
	GthThumbLoader *self;

	self = GTH_THUMB_LOADER (object);
	_g_object_unref (self->priv->iloader);
	_g_object_unref (self->priv->tloader);

	G_OBJECT_CLASS (gth_thumb_loader_parent_class)->finalize (object);
}


static void
gth_thumb_loader_class_init (GthThumbLoaderClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (GthThumbLoaderPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_thumb_loader_finalize;
}


static void
gth_thumb_loader_init (GthThumbLoader *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_THUMB_LOADER, GthThumbLoaderPrivate);
	self->priv->use_cache = TRUE;
	self->priv->save_thumbnails = TRUE;
	self->priv->max_file_size = 0;
}


static GthImage *
generate_thumbnail (GInputStream  *istream,
		    GthFileData   *file_data,
		    int            requested_size,
		    int           *original_width,
		    int           *original_height,
		    gpointer       user_data,
		    GCancellable  *cancellable,
		    GError       **error)
{
	GthThumbLoader *self = user_data;
	GdkPixbuf      *pixbuf = NULL;
	GthImage       *image = NULL;
	char           *uri;
	const char     *mime_type;

	if (file_data == NULL) {
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME, "Could not load file");
		return NULL;
	}

	if (original_width != NULL)
		*original_width = -1;

	if (original_height != NULL)
		*original_height = -1;

	mime_type = _g_content_type_get_from_stream (istream, file_data->file, cancellable, error);
	if (mime_type == NULL) {
		if ((error != NULL) && (*error == NULL))
			*error = g_error_new_literal (GTH_ERROR, 0, "Cannot generate the thumbnail: unknown file type");
		return NULL;
	}

	uri = g_file_get_uri (file_data->file);
	pixbuf = gnome_desktop_thumbnail_factory_generate_no_script (self->priv->thumb_factory,
								     uri,
								     mime_type,
								     cancellable);

	if (g_cancellable_is_cancelled (cancellable)) {
		_g_object_unref (pixbuf);
		g_free (uri);
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, "");
		return NULL;
	}

	if (pixbuf != NULL) {
		image = gth_image_new_for_pixbuf (pixbuf);
		if (original_width != NULL)
			*original_width = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (pixbuf), "gnome-original-width"));
		if (original_height != NULL)
			*original_height = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (pixbuf), "gnome-original-height"));
		if (error != NULL)
			g_clear_error (error);

		g_object_unref (pixbuf);
	}
	else {
		GthImageFormat     preferred_format;
		GthImageLoaderFunc thumbnailer;

		/* Prefer the internal loader for jpeg images to load rotated
		 * images correctly. */
		if (strcmp (mime_type, "image/jpeg") == 0)
			preferred_format = GTH_IMAGE_FORMAT_CAIRO_SURFACE;
		else
			preferred_format = GTH_IMAGE_FORMAT_GDK_PIXBUF;
		thumbnailer = gth_main_get_image_loader_func (mime_type, preferred_format);
		if (thumbnailer != NULL)
			image = thumbnailer (istream,
					     file_data,
					     self->priv->cache_max_size,
					     original_width,
					     original_height,
					     NULL,
					     cancellable,
					     error);
	}

	if ((image == NULL) && (error != NULL))
		*error = g_error_new_literal (GTH_ERROR, 0, "Could not generate the thumbnail");

	g_free (uri);

	return image;
}


static GthImage *
load_cached_thumbnail (GInputStream  *istream,
		       GthFileData   *file_data,
		       int            requested_size,
		       int           *original_width,
		       int           *original_height,
		       gpointer       user_data,
		       GCancellable  *cancellable,
		       GError       **error)
{
	GthImage  *image = NULL;
	char      *filename;
	GdkPixbuf *pixbuf;

	if (file_data == NULL) {
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME, "Could not load file");
		return NULL;
	}

	filename = g_file_get_path (file_data->file);
	pixbuf = gdk_pixbuf_new_from_file (filename, error);
	if (pixbuf != NULL) {
		image = gth_image_new_for_pixbuf (pixbuf);
		g_object_unref (pixbuf);
	}

	g_free (filename);

	return image;
}


static void
gth_thumb_loader_construct (GthThumbLoader *self,
			    int             requested_size)
{
	gth_thumb_loader_set_requested_size (self, requested_size);
	self->priv->tloader = gth_image_loader_new (generate_thumbnail, self);
	self->priv->iloader = gth_image_loader_new (load_cached_thumbnail, NULL);
}


GthThumbLoader *
gth_thumb_loader_new (int requested_size)
{
	GthThumbLoader *self;

	self = g_object_new (GTH_TYPE_THUMB_LOADER, NULL);
	gth_thumb_loader_construct (self, requested_size);

	return self;
}


void
gth_thumb_loader_set_loader_func (GthThumbLoader     *self,
				  GthImageLoaderFunc  loader_func)
{
	gth_image_loader_set_loader_func (self->priv->tloader,
					  (loader_func != NULL) ? loader_func : generate_thumbnail,
					  self);
}


void
gth_thumb_loader_set_requested_size (GthThumbLoader *self,
				     int             size)
{
	GnomeDesktopThumbnailSize thumb_size;

	self->priv->requested_size = size;
	if (self->priv->requested_size <= THUMBNAIL_NORMAL_SIZE) {
		self->priv->cache_max_size = THUMBNAIL_NORMAL_SIZE;
		thumb_size = GNOME_DESKTOP_THUMBNAIL_SIZE_NORMAL;
	}
	else {
		self->priv->cache_max_size = THUMBNAIL_LARGE_SIZE;
		thumb_size = GNOME_DESKTOP_THUMBNAIL_SIZE_LARGE;
	}

	if ((self->priv->thumb_size != thumb_size) || (self->priv->thumb_factory == NULL)) {
		self->priv->thumb_size = thumb_size;
		_g_object_unref (self->priv->thumb_factory);
		self->priv->thumb_factory = gnome_desktop_thumbnail_factory_new (self->priv->thumb_size);
	}
}


int
gth_thumb_loader_get_requested_size (GthThumbLoader *self)
{
	return self->priv->requested_size;
}


void
gth_thumb_loader_set_use_cache (GthThumbLoader *self,
			        gboolean        use)
{
	g_return_if_fail (self != NULL);
	self->priv->use_cache = use;
}


void
gth_thumb_loader_set_save_thumbnails (GthThumbLoader *self,
				      gboolean        save)
{
	g_return_if_fail (self != NULL);
	self->priv->save_thumbnails = save;
}


void
gth_thumb_loader_set_max_file_size (GthThumbLoader *self,
				    goffset         size)
{
	g_return_if_fail (self != NULL);
	self->priv->max_file_size = size;
}


typedef struct {
	GthThumbLoader     *thumb_loader;
	GthFileData        *file_data;
	int                 requested_size;
	GSimpleAsyncResult *simple;
	GCancellable       *cancellable;
	char               *thumbnailer_tmpfile;
	GPid                thumbnailer_pid;
	guint               thumbnailer_watch;
	guint               thumbnailer_timeout;
	guint               cancellable_watch;
	gboolean            script_cancelled;
} LoadData;


static LoadData *
load_data_new (GthFileData *file_data,
	       int          requested_size)
{
	LoadData *load_data;

	load_data = g_new0 (LoadData, 1);
	load_data->file_data = g_object_ref (file_data);
	load_data->requested_size = requested_size;

	return load_data;
}


static void
load_data_unref (LoadData *load_data)
{
	g_object_unref (load_data->thumb_loader);
	g_object_unref (load_data->file_data);
	_g_object_unref (load_data->simple);
	_g_object_unref (load_data->cancellable);
	g_free (load_data->thumbnailer_tmpfile);
	g_free (load_data);
}


typedef struct {
	GthFileData *file_data;
	GdkPixbuf   *pixbuf;
} LoadResult;


static void
load_result_unref (LoadResult *load_result)
{
	g_object_unref (load_result->file_data);
	_g_object_unref (load_result->pixbuf);
	g_free (load_result);
}


static void
original_image_ready_cb (GObject      *source_object,
		         GAsyncResult *res,
		         gpointer      user_data);


static int
normalize_thumb (int *width,
		 int *height,
		 int  max_size,
		 int  cache_max_size)
{
	gboolean modified;
	float    max_w = max_size;
	float    max_h = max_size;
	float    w = *width;
	float    h = *height;
	float    factor;
	int      new_width, new_height;

	if (max_size > cache_max_size) {
		if ((*width < cache_max_size - 1) && (*height < cache_max_size - 1))
			return FALSE;
	}
	else if ((*width < max_size - 1) && (*height < max_size - 1))
		return FALSE;

	factor = MIN (max_w / w, max_h / h);
	new_width  = MAX ((int) (w * factor), 1);
	new_height = MAX ((int) (h * factor), 1);

	modified = (new_width != *width) || (new_height != *height);

	*width = new_width;
	*height = new_height;

	return modified;
}


static void
cache_image_ready_cb (GObject      *source_object,
		      GAsyncResult *res,
		      gpointer      user_data)
{
	LoadData       *load_data = user_data;
	GthThumbLoader *self = load_data->thumb_loader;
	GthImage       *image = NULL;
	GdkPixbuf      *pixbuf;
	int             width;
	int             height;
	gboolean        modified;
	LoadResult     *load_result;

	if (! gth_image_loader_load_finish (GTH_IMAGE_LOADER (source_object),
					    res,
					    &image,
					    NULL,
					    NULL,
					    NULL))
	{
		/* error loading the thumbnail from the cache, try to generate
		 * the thumbnail loading the original image. */

		gth_image_loader_load (self->priv->tloader,
				       load_data->file_data,
				       load_data->requested_size,
				       G_PRIORITY_LOW,
				       load_data->cancellable,
				       original_image_ready_cb,
				       load_data);

		return;
	}

	/* Thumbnail correctly loaded from the cache. Scale if the user wants
	 * a different size. */

	pixbuf = gth_image_get_pixbuf (image);

	g_return_if_fail (pixbuf != NULL);

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	modified = normalize_thumb (&width,
				    &height,
				    self->priv->requested_size,
				    self->priv->cache_max_size);
	if (modified) {
		GdkPixbuf *tmp = pixbuf;
		pixbuf = _gdk_pixbuf_scale_simple_safe (tmp, width, height, GDK_INTERP_BILINEAR);
		g_object_unref (tmp);
	}

	load_result = g_new0 (LoadResult, 1);
	load_result->file_data = g_object_ref (load_data->file_data);
	load_result->pixbuf = pixbuf;
	g_simple_async_result_set_op_res_gpointer (load_data->simple, load_result, (GDestroyNotify) load_result_unref);
	g_simple_async_result_complete_in_idle (load_data->simple);

	load_data_unref (load_data);
	g_object_unref (image);
}


static gboolean
is_a_cache_file (const char *uri)
{
	char     *filename;
	char     *cache_dir_1;
	char     *cache_dir_2;
	gboolean  result;

	filename = g_filename_from_uri (uri, NULL, NULL);
	if (filename == NULL)
		return FALSE;

	cache_dir_1 = g_build_filename (g_get_home_dir (), ".thumbnails", NULL);
	cache_dir_2 = g_build_filename (g_get_user_cache_dir (), "thumbnails", NULL);
	result = _g_uri_parent_of_uri (cache_dir_1, filename) || _g_uri_parent_of_uri (cache_dir_2, filename);

	g_free (cache_dir_1);
	g_free (cache_dir_2);

	return result;
}


static gboolean
_gth_thumb_loader_save_to_cache (GthThumbLoader *self,
				 GthFileData    *file_data,
				 GdkPixbuf      *pixbuf)
{
	char  *uri;

	if ((self == NULL) || (pixbuf == NULL))
		return FALSE;

	uri = g_file_get_uri (file_data->file);

	/* Do not save thumbnails from the user's thumbnail directory,
	 * or an endless loop of thumbnailing may be triggered. */

	if (is_a_cache_file (uri)) {
		g_free (uri);
		return FALSE;
	}

	gnome_desktop_thumbnail_factory_save_thumbnail (self->priv->thumb_factory,
							pixbuf,
							uri,
							gth_file_data_get_mtime (file_data));

	return TRUE;
}


static void
original_image_loaded_correctly (GthThumbLoader *self,
				 LoadData        *load_data,
				 GdkPixbuf       *pixbuf)
{
	GdkPixbuf  *local_pixbuf;
	int         width;
	int         height;
	gboolean    modified;
	LoadResult *load_result;

	local_pixbuf = g_object_ref (pixbuf);

	width = gdk_pixbuf_get_width (local_pixbuf);
	height = gdk_pixbuf_get_height (local_pixbuf);

	if (self->priv->save_thumbnails) {
		gboolean modified;

		/* Thumbnails are always saved in the cache max size, then
		 * scaled a second time if the user requested a different
		 * size. */

		modified = scale_keeping_ratio (&width,
						&height,
						self->priv->cache_max_size,
						self->priv->cache_max_size,
						FALSE);
		if (modified) {
			GdkPixbuf *tmp = local_pixbuf;
			local_pixbuf = _gdk_pixbuf_scale_simple_safe (tmp, width, height, GDK_INTERP_BILINEAR);
			g_object_unref (tmp);
		}

		_gth_thumb_loader_save_to_cache (self, load_data->file_data, local_pixbuf);
	}

	/* Scale if the user wants a different size. */

	modified = normalize_thumb (&width,
				    &height,
				    self->priv->requested_size,
				    self->priv->cache_max_size);
	if (modified) {
		GdkPixbuf *tmp = local_pixbuf;
		local_pixbuf = _gdk_pixbuf_scale_simple_safe (tmp, width, height, GDK_INTERP_BILINEAR);
		g_object_unref (tmp);
	}

	load_result = g_new0 (LoadResult, 1);
	load_result->file_data = g_object_ref (load_data->file_data);
	load_result->pixbuf = g_object_ref (local_pixbuf);
	g_simple_async_result_set_op_res_gpointer (load_data->simple, load_result, (GDestroyNotify) load_result_unref);
	g_simple_async_result_complete_in_idle (load_data->simple);

	g_object_unref (local_pixbuf);
}


static void
failed_to_load_original_image (GthThumbLoader *self,
			       LoadData       *load_data)
{
	char   *uri;
	GError *error;

	uri = g_file_get_uri (load_data->file_data->file);
	gnome_desktop_thumbnail_factory_create_failed_thumbnail (self->priv->thumb_factory,
								 uri,
								 gth_file_data_get_mtime (load_data->file_data));

	error = g_error_new_literal (GTH_ERROR, 0, "failed to generate the thumbnail");
	g_simple_async_result_set_from_error (load_data->simple, error);
	g_simple_async_result_complete_in_idle (load_data->simple);

	g_error_free (error);
	g_free (uri);
}


static gboolean
kill_thumbnailer_cb (gpointer user_data)
{
	LoadData *load_data = user_data;

	if (load_data->thumbnailer_timeout != 0) {
		g_source_remove (load_data->thumbnailer_timeout);
		load_data->thumbnailer_timeout = 0;
	}

	if (load_data->cancellable_watch != 0) {
		g_source_remove (load_data->cancellable_watch);
		load_data->cancellable_watch = 0;
	}

	if (load_data->thumbnailer_pid != 0)
		kill (load_data->thumbnailer_pid, SIGTERM);

	return FALSE;
}


static gboolean
check_cancellable_cb (gpointer user_data)
{
	LoadData *load_data = user_data;

	if (g_cancellable_is_cancelled (load_data->cancellable)) {
		load_data->script_cancelled = TRUE;
		kill_thumbnailer_cb (user_data);
		return FALSE;
	}

	return TRUE;
}


static void
watch_thumbnailer_cb (GPid     pid,
		      int      status,
		      gpointer user_data)
{
	LoadData       *load_data = user_data;
	GthThumbLoader *self = load_data->thumb_loader;
	GdkPixbuf      *pixbuf;

	if (load_data->thumbnailer_timeout != 0) {
		g_source_remove (load_data->thumbnailer_timeout);
		load_data->thumbnailer_timeout = 0;
	}

	if (load_data->cancellable_watch != 0) {
		g_source_remove (load_data->cancellable_watch);
		load_data->cancellable_watch = 0;
	}

	g_spawn_close_pid (pid);
	load_data->thumbnailer_pid = 0;
	load_data->thumbnailer_watch = 0;

	if (load_data->script_cancelled) {
		GError *error;

		if (load_data->thumbnailer_tmpfile != NULL) {
			g_unlink (load_data->thumbnailer_tmpfile);
			g_free (load_data->thumbnailer_tmpfile);
			load_data->thumbnailer_tmpfile = NULL;
		}

		error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, "script cancelled");
		g_simple_async_result_set_from_error (load_data->simple, error);
		g_simple_async_result_complete_in_idle (load_data->simple);

		g_error_free (error);

		return;
	}

	pixbuf = NULL;
	if (status == 0)
		pixbuf = gnome_desktop_thumbnail_factory_load_from_tempfile (self->priv->thumb_factory,
									     &load_data->thumbnailer_tmpfile);

	if (pixbuf != NULL) {
		original_image_loaded_correctly (self, load_data, pixbuf);
		g_object_unref (pixbuf);
	}
	else
		failed_to_load_original_image (self, load_data);
}


static void
original_image_ready_cb (GObject      *source_object,
		         GAsyncResult *res,
		         gpointer      user_data)
{
	LoadData       *load_data = user_data;
	GthThumbLoader *self = load_data->thumb_loader;
	GthImage       *image = NULL;
	GdkPixbuf      *pixbuf = NULL;
	GError         *error = NULL;

	if (! gth_image_loader_load_finish (GTH_IMAGE_LOADER (source_object),
					    res,
					    &image,
					    NULL,
					    NULL,
					    &error))
	{
		/* error loading the original image, try with the system
		 * thumbnailer */

		char *uri;

		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_simple_async_result_set_from_error (load_data->simple, error);
			g_simple_async_result_complete_in_idle (load_data->simple);
			return;
		}

		g_clear_error (&error);

		uri = g_file_get_uri (load_data->file_data->file);
		if (gnome_desktop_thumbnail_factory_generate_from_script (self->priv->thumb_factory,
									  uri,
									  gth_file_data_get_mime_type (load_data->file_data),
									  &load_data->thumbnailer_pid,
									  &load_data->thumbnailer_tmpfile,
									  &error))
		{
			load_data->thumbnailer_watch = g_child_watch_add (load_data->thumbnailer_pid,
									  watch_thumbnailer_cb,
									  load_data);
			load_data->thumbnailer_timeout = g_timeout_add (MAX_THUMBNAILER_LIFETIME,
									kill_thumbnailer_cb,
									load_data);
			load_data->cancellable_watch = g_timeout_add (CHECK_CANCELLABLE_DELAY,
								      check_cancellable_cb,
								      load_data);
		}
		else {
			g_clear_error (&error);
			failed_to_load_original_image (self, load_data);
			load_data_unref (load_data);
		}

		g_free (uri);

		return;
	}

	pixbuf = gth_image_get_pixbuf (image);
	original_image_loaded_correctly (self, load_data, pixbuf);

	g_object_unref (pixbuf);
	g_object_unref (image);
	load_data_unref (load_data);
}


void
gth_thumb_loader_load (GthThumbLoader      *self,
		       GthFileData         *file_data,
		       GCancellable        *cancellable,
		       GAsyncReadyCallback  callback,
		       gpointer             user_data)
{
	GSimpleAsyncResult *simple;
	char               *cache_path;
	char               *uri;
	LoadData           *load_data;

	simple = g_simple_async_result_new (G_OBJECT (self),
					    callback,
					    user_data,
					    gth_thumb_loader_load);

	cache_path = NULL;

	uri = g_file_get_uri (file_data->file);

	if (is_a_cache_file (uri)) {
		cache_path = g_file_get_path (file_data->file);
	}
	else if (self->priv->use_cache) {
		time_t mtime;

		mtime = gth_file_data_get_mtime (file_data);

		if (gnome_desktop_thumbnail_factory_has_valid_failed_thumbnail (self->priv->thumb_factory, uri, mtime)) {
			GError *error;

			error = g_error_new_literal (GTH_ERROR, 0, "found a failed thumbnail");
			g_simple_async_result_set_from_error (simple, error);
			g_simple_async_result_complete_in_idle (simple);

			g_error_free (error);
			g_free (uri);
			g_object_unref (simple);

			return;
		}

		cache_path = gnome_desktop_thumbnail_factory_lookup (self->priv->thumb_factory, uri, mtime);
	}

	g_free (uri);

	if ((cache_path == NULL)
	    && (self->priv->max_file_size > 0)
	    && (g_file_info_get_size (file_data->info) > self->priv->max_file_size))
	{
		GError *error;

		error = g_error_new_literal (GTH_ERROR, 0, "file too big to generate the thumbnail");
		g_simple_async_result_set_from_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);

		g_error_free (error);
		g_object_unref (simple);

		return;
	}

	load_data = load_data_new (file_data, self->priv->requested_size);
	load_data->thumb_loader = g_object_ref (self);
	load_data->cancellable = _g_object_ref (cancellable);
	load_data->simple = simple;

	if (cache_path != NULL) {
		GFile       *cache_file;
		GthFileData *cache_file_data;

		cache_file = g_file_new_for_path (cache_path);
		cache_file_data = gth_file_data_new (cache_file, NULL);
		gth_file_data_set_mime_type (cache_file_data, "image/png");
		gth_image_loader_load (self->priv->iloader,
				       cache_file_data,
				       -1,
				       G_PRIORITY_LOW,
				       load_data->cancellable,
				       cache_image_ready_cb,
				       load_data);

		g_object_unref (cache_file_data);
		g_object_unref (cache_file);
		g_free (cache_path);
	}
	else
		gth_image_loader_load (self->priv->tloader,
				       file_data,
				       self->priv->requested_size,
				       G_PRIORITY_LOW,
				       load_data->cancellable,
				       original_image_ready_cb,
				       load_data);
}


gboolean
gth_thumb_loader_load_finish (GthThumbLoader  *self,
			      GAsyncResult    *result,
			      GdkPixbuf      **pixbuf,
			      GError         **error)
{
	GSimpleAsyncResult *simple;
	LoadResult         *load_result;

	g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (self), gth_thumb_loader_load), FALSE);

	simple = G_SIMPLE_ASYNC_RESULT (result);

	if (g_simple_async_result_propagate_error (simple, error))
		return FALSE;

	load_result = g_simple_async_result_get_op_res_gpointer (simple);
	if (pixbuf != NULL)
		*pixbuf = _g_object_ref (load_result->pixbuf);

	return TRUE;
}


gboolean
gth_thumb_loader_has_valid_thumbnail (GthThumbLoader *self,
				      GthFileData    *file_data)
{
	gboolean  valid_thumbnail = FALSE;
	char     *uri;
	time_t    mtime;
	char     *thumbnail_path;

	uri = g_file_get_uri (file_data->file);
	mtime = gth_file_data_get_mtime (file_data);
	thumbnail_path = gnome_desktop_thumbnail_factory_lookup (self->priv->thumb_factory, uri, mtime);
	if (thumbnail_path != NULL) {
		valid_thumbnail = TRUE;
		g_free (thumbnail_path);
	}

	g_free (uri);

	return valid_thumbnail;
}


gboolean
gth_thumb_loader_has_failed_thumbnail (GthThumbLoader *self,
				       GthFileData    *file_data)
{
	char     *uri;
	time_t    mtime;
	gboolean  valid_thumbnail;

	uri = g_file_get_uri (file_data->file);
	mtime = gth_file_data_get_mtime (file_data);
	valid_thumbnail = gnome_desktop_thumbnail_factory_has_valid_failed_thumbnail (self->priv->thumb_factory, uri, mtime);

	g_free (uri);

	return valid_thumbnail;
}
