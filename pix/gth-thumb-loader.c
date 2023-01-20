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
#include "cairo-scale.h"
#include "cairo-utils.h"
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


enum {
	READY,
	LAST_SIGNAL
};


struct _GthThumbLoaderPrivate {
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


G_DEFINE_TYPE_WITH_CODE (GthThumbLoader,
			 gth_thumb_loader,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthThumbLoader))


static void
gth_thumb_loader_finalize (GObject *object)
{
	GthThumbLoader *self;

	self = GTH_THUMB_LOADER (object);
	_g_object_unref (self->priv->iloader);
	_g_object_unref (self->priv->tloader);
	_g_object_unref (self->priv->thumb_factory);

	G_OBJECT_CLASS (gth_thumb_loader_parent_class)->finalize (object);
}


static void
gth_thumb_loader_class_init (GthThumbLoaderClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_thumb_loader_finalize;
}


static void
gth_thumb_loader_init (GthThumbLoader *self)
{
	self->priv = gth_thumb_loader_get_instance_private (self);
	self->priv->iloader = NULL;
	self->priv->tloader = NULL;
	self->priv->use_cache = TRUE;
	self->priv->save_thumbnails = TRUE;
	self->priv->requested_size = 0;
	self->priv->cache_max_size = 0;
	self->priv->max_file_size = 0;
	self->priv->thumb_size = GNOME_DESKTOP_THUMBNAIL_SIZE_NORMAL;
	self->priv->thumb_factory = NULL;
}


static GthImage *
generate_thumbnail (GInputStream  *istream,
		    GthFileData   *file_data,
		    int            requested_size,
		    int           *original_width,
		    int           *original_height,
		    gboolean      *loaded_original,
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
		GthImageLoaderFunc thumbnailer;

		/* prefer the GTH_IMAGE_FORMAT_CAIRO_SURFACE format to give
		 * priority to the internal loaders. */

		thumbnailer = gth_main_get_image_loader_func (mime_type, GTH_IMAGE_FORMAT_CAIRO_SURFACE);
		if (thumbnailer != NULL)
			image = thumbnailer (istream,
					     file_data,
					     self->priv->cache_max_size,
					     original_width,
					     original_height,
					     NULL,
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
		       gboolean      *loaded_original,
		       gpointer       user_data,
		       GCancellable  *cancellable,
		       GError       **error)
{
	if (file_data == NULL) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME, "Could not load file");
		return NULL;
	}

	return gth_image_new_from_stream (istream,
				          requested_size,
				          original_width,
				          original_height,
				          cancellable,
				          error);
}


static void
gth_thumb_loader_construct (GthThumbLoader *self,
			    int             requested_size)
{
	if (requested_size > 0)
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


GthThumbLoader *
gth_thumb_loader_copy (GthThumbLoader *self)
{
	GthThumbLoader *loader;

	loader = gth_thumb_loader_new (-1);

	loader->priv->requested_size = self->priv->requested_size;
	loader->priv->cache_max_size = self->priv->cache_max_size;
	loader->priv->thumb_size = self->priv->thumb_size;
	loader->priv->thumb_factory = _g_object_ref (self->priv->thumb_factory);

	gth_thumb_loader_set_loader_func (loader, gth_image_loader_get_loader_func (self->priv->tloader));
	gth_thumb_loader_set_use_cache (loader, self->priv->use_cache);
	gth_thumb_loader_set_save_thumbnails (loader, self->priv->save_thumbnails);
	gth_thumb_loader_set_max_file_size (loader, self->priv->max_file_size);

	return loader;
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
	GTask              *task;
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
	_g_object_unref (load_data->task);
	_g_object_unref (load_data->cancellable);
	g_free (load_data->thumbnailer_tmpfile);
	g_free (load_data);
}


typedef struct {
	GthFileData     *file_data;
	cairo_surface_t *image;
} LoadResult;


static void
load_result_unref (LoadResult *load_result)
{
	g_object_unref (load_result->file_data);
	cairo_surface_destroy (load_result->image);
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


static cairo_surface_t *
_cairo_image_surface_scale_for_thumbnail (cairo_surface_t *image,
					  int              new_width,
					  int              new_height)
{
	cairo_surface_t *scaled;

	scaled = _cairo_image_surface_scale (image, new_width, new_height, SCALE_FILTER_GOOD, NULL);
	if (scaled != NULL)
		_cairo_image_surface_copy_metadata (image, scaled);

	return scaled;
}


static void
cache_image_ready_cb (GObject      *source_object,
		      GAsyncResult *res,
		      gpointer      user_data)
{
	LoadData        *load_data = user_data;
	GthThumbLoader  *self = load_data->thumb_loader;
	GthImage        *image = NULL;
	cairo_surface_t *surface;
	int              width;
	int              height;
	gboolean         modified;
	LoadResult      *load_result;

	if (! gth_image_loader_load_finish (GTH_IMAGE_LOADER (source_object),
					    res,
					    &image,
					    NULL,
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

	surface = gth_image_get_cairo_surface (image);

	g_return_if_fail (surface != NULL);

	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);
	modified = normalize_thumb (&width,
				    &height,
				    self->priv->requested_size,
				    self->priv->cache_max_size);
	if (modified) {
		cairo_surface_t *tmp = surface;
		surface = _cairo_image_surface_scale_for_thumbnail (tmp, width, height);
		cairo_surface_destroy (tmp);
	}

	load_result = g_new0 (LoadResult, 1);
	load_result->file_data = g_object_ref (load_data->file_data);
	load_result->image = surface;
	g_task_return_pointer (load_data->task, load_result, (GDestroyNotify) load_result_unref);

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
	result = _g_path_is_parent (cache_dir_1, filename) || _g_path_is_parent (cache_dir_2, filename);

	g_free (cache_dir_1);
	g_free (cache_dir_2);
	g_free (filename);

	return result;
}


static gboolean
_gth_thumb_loader_save_to_cache (GthThumbLoader  *self,
				 GthFileData     *file_data,
				 cairo_surface_t *image,
				 int              original_width,
				 int              original_height)
{
	char                     *uri;
	cairo_surface_metadata_t *metadata;
	GdkPixbuf                *pixbuf;

	if ((self == NULL) || (image == NULL))
		return FALSE;

	uri = g_file_get_uri (file_data->file);

	/* Do not save thumbnails from the user's thumbnail directory,
	 * or an endless loop of thumbnailing may be triggered. */

	if (is_a_cache_file (uri)) {
		g_free (uri);
		return FALSE;
	}

	if ((original_width > 0) && (original_height > 0)) {
		metadata = _cairo_image_surface_get_metadata (image);
		metadata->thumbnail.image_width = original_width;
		metadata->thumbnail.image_height = original_height;
	}
	pixbuf = _gdk_pixbuf_new_from_cairo_surface (image);
	if (pixbuf == NULL)
		return FALSE;

	gnome_desktop_thumbnail_factory_save_thumbnail (self->priv->thumb_factory,
							pixbuf,
							uri,
							gth_file_data_get_mtime (file_data));

	g_object_unref (pixbuf);

	return TRUE;
}


static void
original_image_loaded_correctly (GthThumbLoader *self,
				 LoadData        *load_data,
				 cairo_surface_t *image,
				 int              original_width,
				 int              original_height)
{
	cairo_surface_t *local_image;
	int              width;
	int              height;
	gboolean         modified;
	LoadResult      *load_result;

	local_image = cairo_surface_reference (image);

	width = cairo_image_surface_get_width (local_image);
	height = cairo_image_surface_get_height (local_image);

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
			cairo_surface_t *tmp = local_image;
			local_image = _cairo_image_surface_scale_for_thumbnail (tmp, width, height);
			cairo_surface_destroy (tmp);
		}

		_gth_thumb_loader_save_to_cache (self,
						 load_data->file_data,
						 local_image,
						 original_width,
						 original_height);
	}

	/* Scale if the user wants a different size. */

	modified = normalize_thumb (&width,
				    &height,
				    self->priv->requested_size,
				    self->priv->cache_max_size);
	if (modified) {
		cairo_surface_t *tmp = local_image;
		local_image = _cairo_image_surface_scale_for_thumbnail (tmp, width, height);
		cairo_surface_destroy (tmp);
	}

	load_result = g_new0 (LoadResult, 1);
	load_result->file_data = g_object_ref (load_data->file_data);
	load_result->image = cairo_surface_reference (local_image);
	g_task_return_pointer (load_data->task, load_result, (GDestroyNotify) load_result_unref);

	cairo_surface_destroy (local_image);
}


static void
failed_to_load_original_image (GthThumbLoader *self,
			       LoadData       *load_data)
{
	char *uri;

	uri = g_file_get_uri (load_data->file_data->file);
	gnome_desktop_thumbnail_factory_create_failed_thumbnail (self->priv->thumb_factory,
								 uri,
								 gth_file_data_get_mtime (load_data->file_data));
	g_task_return_error (load_data->task, g_error_new_literal (GTH_ERROR, 0, "failed to generate the thumbnail"));

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
		if (load_data->thumbnailer_tmpfile != NULL) {
			g_unlink (load_data->thumbnailer_tmpfile);
			g_free (load_data->thumbnailer_tmpfile);
			load_data->thumbnailer_tmpfile = NULL;
		}

		g_task_return_error (load_data->task, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, "script cancelled"));
		load_data_unref (load_data);
		return;
	}

	pixbuf = NULL;
	if (status == 0)
		pixbuf = gnome_desktop_thumbnail_factory_load_from_tempfile (self->priv->thumb_factory,
									     &load_data->thumbnailer_tmpfile);

	if (pixbuf != NULL) {
		cairo_surface_t *surface;

		surface = _cairo_image_surface_create_from_pixbuf (pixbuf);
		original_image_loaded_correctly (self, load_data, surface, 0, 0);

		cairo_surface_destroy (surface);
		g_object_unref (pixbuf);
	}
	else
		failed_to_load_original_image (self, load_data);

	load_data_unref (load_data);
}


static void
load_with_system_thumbnailer (GthThumbLoader *self,
			      LoadData       *load_data)
{
	char   *uri;
	GError *error = NULL;

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
}


static void
original_image_ready_cb (GObject      *source_object,
		         GAsyncResult *res,
		         gpointer      user_data)
{
	LoadData        *load_data = user_data;
	GthThumbLoader  *self = load_data->thumb_loader;
	GthImage        *image = NULL;
	int              original_width;
	int              original_height;
	cairo_surface_t *surface = NULL;
	GError          *error = NULL;

	if (! gth_image_loader_load_finish (GTH_IMAGE_LOADER (source_object),
					    res,
					    &image,
					    &original_width,
					    &original_height,
					    NULL,
					    &error))
	{
		/* error loading the original image, try with the system
		 * thumbnailer */

		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_task_return_error (load_data->task, error);
			load_data_unref (load_data);
			return;
		}

		g_clear_error (&error);
		load_with_system_thumbnailer (self, load_data);
		return;
	}

	surface = gth_image_get_cairo_surface (image);
	if (surface == NULL) {
		g_object_unref (image);
		load_with_system_thumbnailer (self, load_data);
		return;
	}

	original_image_loaded_correctly (self,
					 load_data,
					 surface,
					 original_width,
					 original_height);

	cairo_surface_destroy (surface);
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
	GTask    *task;
	char     *cache_path;
	char     *uri;
	LoadData *load_data;

	task = g_task_new (G_OBJECT (self), cancellable, callback, user_data);

	cache_path = NULL;

	uri = g_file_get_uri (file_data->file);

	if (is_a_cache_file (uri)) {
		cache_path = g_file_get_path (file_data->file);
	}
	else if (self->priv->use_cache) {
		time_t mtime;

		mtime = gth_file_data_get_mtime (file_data);

		if (gnome_desktop_thumbnail_factory_has_valid_failed_thumbnail (self->priv->thumb_factory, uri, mtime)) {
			g_task_return_error (task, g_error_new_literal (GTH_ERROR, 0, "found a failed thumbnail"));

			g_free (uri);
			g_object_unref (task);

			return;
		}

		cache_path = gnome_desktop_thumbnail_factory_lookup (self->priv->thumb_factory, uri, mtime);
	}

	g_free (uri);

	if ((cache_path == NULL)
	    && (self->priv->max_file_size > 0)
	    && (g_file_info_get_size (file_data->info) > self->priv->max_file_size))
	{
		g_task_return_error (task, g_error_new_literal (GTH_ERROR, 0, "file too big to generate the thumbnail"));
		g_object_unref (task);
		return;
	}

	load_data = load_data_new (file_data, self->priv->requested_size);
	load_data->thumb_loader = g_object_ref (self);
	load_data->cancellable = _g_object_ref (cancellable);
	load_data->task = task;

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
gth_thumb_loader_load_finish (GthThumbLoader   *self,
			      GAsyncResult     *result,
			      cairo_surface_t **image,
			      GError          **error)
{
	LoadResult *load_result;

	g_return_val_if_fail (g_task_is_valid (G_TASK (result), G_OBJECT (self)), FALSE);

	load_result = g_task_propagate_pointer (G_TASK (result), error);
	if (load_result == NULL)
		return FALSE;

	if (image != NULL)
		*image = cairo_surface_reference (load_result->image);
	load_result_unref (load_result);

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
