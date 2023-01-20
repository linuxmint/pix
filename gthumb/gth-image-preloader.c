/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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
#include <string.h>
#include <glib.h>
#include "cairo-scale.h"
#include "cairo-utils.h"
#include "glib-utils.h"
#include "gth-image.h"
#include "gth-image-preloader.h"
#include "gth-image-utils.h"
#include "gth-marshal.h"


#undef DEBUG_PRELOADER
#undef RESIZE_TO_REQUESTED_SIZE
#define LOAD_NEXT_FILE_DELAY 100
#define CACHE_MAX_SIZE 10


enum {
	REQUESTED_READY,
	ORIGINAL_SIZE_READY,
	LAST_SIGNAL
};


typedef struct {
	int			 ref;
	GthFileData		*file_data;
	GthImage		*image;
	int			 original_width;
	int			 original_height;
	int			 requested_size;
	gboolean		 loaded_original;
	GError			*error;
} CacheData;


typedef struct {
	int			 ref;
	gboolean                 loading;
	gboolean                 finalized;
	GthImagePreloader	*preloader;
	GList			*files;			/* List of GthFileData */
	GList			*current_file;
	GList                   *requested_file;
	int			 requested_size;
	GTask			*task;
	GCancellable            *cancellable;
	GCancellable            *extern_cancellable;
	gulong                   cancellable_id;
} LoadRequest;


struct _GthImagePreloaderPrivate {
	GList			*requests;		/* List of queued LoadRequest */
	LoadRequest		*current_request;
	LoadRequest             *last_request;
	GthImageLoader		*loader;
	GQueue			*cache;
	guint                    load_next_id;
	GthICCProfile           *out_profile;
};


G_DEFINE_TYPE_WITH_CODE (GthImagePreloader,
			 gth_image_preloader,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthImagePreloader))


/* -- CacheData -- */


static CacheData *
cache_data_new (void)
{
	CacheData *cache_data;

	cache_data = g_new0 (CacheData, 1);
	cache_data->ref = 1;
	cache_data->file_data = NULL;
	cache_data->image = NULL;
	cache_data->original_width = -1;
	cache_data->original_height = -1;
	cache_data->requested_size = -1;
	cache_data->error = NULL;
	cache_data->loaded_original = FALSE;

	return cache_data;
}


static CacheData *
cache_data_ref (CacheData *cache_data)
{
	cache_data->ref++;
	return cache_data;
}


static void
cache_data_unref (CacheData *cache_data)
{
	if (cache_data == NULL)
		return;
	if (--cache_data->ref > 0)
		return;

	g_clear_error (&cache_data->error);
	_g_object_unref (cache_data->image);
	_g_object_unref (cache_data->file_data);
	g_free (cache_data);
}


static inline gboolean
cache_data_file_matches (CacheData   *cache_data,
			 GthFileData *file_data)
{
	if ((file_data == GTH_MODIFIED_IMAGE) && (cache_data->file_data == GTH_MODIFIED_IMAGE))
		return TRUE;

	if ((file_data == GTH_MODIFIED_IMAGE) || (cache_data->file_data == GTH_MODIFIED_IMAGE))
		return FALSE;

	if (g_file_equal (cache_data->file_data->file, file_data->file)
            && (_g_time_val_cmp (gth_file_data_get_modification_time (file_data),
        		         gth_file_data_get_modification_time (cache_data->file_data)) == 0))
	{
		return TRUE;
	}

	return FALSE;
}


static gboolean
cache_data_is_valid_for_request (CacheData   *cache_data,
				 GthFileData *file_data,
				 int          requested_size)
{
	if (! cache_data_file_matches (cache_data, file_data))
		return FALSE;

	return cache_data->requested_size == requested_size;
}


static gboolean
cache_data_has_better_quality_for_request (CacheData   *cache_data,
					   GthFileData *file_data,
					   int          requested_size)
{
	if (! cache_data_file_matches (cache_data, file_data))
		return FALSE;

	if (requested_size == GTH_ORIGINAL_SIZE)
		return FALSE;

	return (cache_data->requested_size > requested_size) || (cache_data->requested_size == GTH_ORIGINAL_SIZE);
}


/* -- LoadRequest -- */


static LoadRequest *
load_request_new (GthImagePreloader *preloader)
{
	LoadRequest *request;

	request = g_new0 (LoadRequest, 1);
	request->ref = 1;
	request->loading = FALSE;
	request->finalized = FALSE;
	request->preloader = preloader;
	request->files = NULL;
	request->current_file = NULL;
	request->requested_size = GTH_ORIGINAL_SIZE;
	request->task = NULL;
	request->cancellable = NULL;
	request->extern_cancellable = NULL;
	request->cancellable_id = 0;

	return request;
}


static LoadRequest *
load_request_ref (LoadRequest *request)
{
	request->ref++;
	return request;
}


static void
load_request_unref (LoadRequest *request)
{
	if (request == NULL)
		return;
	if (--request->ref > 0)
		return;

	if (request->cancellable_id != 0)
		g_signal_handler_disconnect (request->extern_cancellable, request->cancellable_id);
	_g_object_unref (request->extern_cancellable);
	_g_object_unref (request->cancellable);
	_g_object_unref (request->task);
	_g_object_list_unref (request->files);
	g_free (request);
}


static void
load_request_completed_with_error (LoadRequest *request,
				   GQuark       domain,
				   int          code)
{
	g_task_return_error (request->task, g_error_new_literal (domain, code, ""));
}


/* -- GthImagePreloader -- */


static void
gth_image_preloader_finalize (GObject *object)
{
	GthImagePreloader *self;
	GList             *scan;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_PRELOADER (object));

	self = GTH_IMAGE_PRELOADER (object);

	if (self->priv->load_next_id != 0)
		g_source_remove (self->priv->load_next_id);
	load_request_unref (self->priv->last_request);
	load_request_unref (self->priv->current_request);

	for (scan = self->priv->requests; scan; scan = scan->next) {
		LoadRequest *request = scan->data;

		request->finalized = TRUE;
		load_request_unref (request);
	}
	g_list_free (self->priv->requests);

	g_object_unref (self->priv->loader);
	_g_object_unref (self->priv->out_profile);
	g_queue_free_full (self->priv->cache, (GDestroyNotify) cache_data_unref);

	G_OBJECT_CLASS (gth_image_preloader_parent_class)->finalize (object);
}


static void
gth_image_preloader_class_init (GthImagePreloaderClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_image_preloader_finalize;
}


static void
gth_image_preloader_init (GthImagePreloader *self)
{
	self->priv = gth_image_preloader_get_instance_private (self);
	self->priv->requests = NULL;
	self->priv->current_request = NULL;
	self->priv->last_request = NULL;
	self->priv->loader = gth_image_loader_new (NULL, NULL);
	self->priv->cache = g_queue_new ();
	self->priv->load_next_id = 0;
	self->priv->out_profile = NULL;
}


GthImagePreloader *
gth_image_preloader_new (void)
{
	return (GthImagePreloader *) g_object_new (GTH_TYPE_IMAGE_PRELOADER, NULL);
}


void
gth_image_preloader_set_out_profile (GthImagePreloader *self,
				     GthICCProfile     *out_profile)
{
	g_return_if_fail (self != NULL);

	_g_object_ref (out_profile);
	_g_object_unref (self->priv->out_profile);
	self->priv->out_profile = out_profile;
}


/* -- gth_image_preloader_load -- */


static CacheData *
_gth_image_preloader_lookup_same_size (GthImagePreloader	*self,
				       GthFileData		*requested_file,
				       int			 requested_size)
{
	GList *scan;

	for (scan = self->priv->cache->head; scan; scan = scan->next) {
		CacheData *cache_data = scan->data;
		if (cache_data_is_valid_for_request (cache_data, requested_file, requested_size))
			return cache_data;
	}

	return NULL;
}


static CacheData *
_gth_image_preloader_lookup_bigger_size (GthImagePreloader	*self,
					 GthFileData		*requested_file,
					 int			 requested_size)
{
	GList *scan;

	if (requested_file != GTH_MODIFIED_IMAGE)
		return NULL;

	if (requested_size == GTH_ORIGINAL_SIZE)
		return NULL;

	for (scan = self->priv->cache->head; scan; scan = scan->next) {
		CacheData *cache_data = scan->data;
		if (cache_data_has_better_quality_for_request (cache_data, requested_file, requested_size))
			return cache_data;
	}

	return NULL;
}


static void
_gth_image_preloader_request_finished (GthImagePreloader *self,
				       LoadRequest       *load_request)
{
	if (self->priv->last_request == load_request)
		self->priv->last_request = NULL;
	if (self->priv->current_request == load_request) {
		load_request_unref (self->priv->current_request);
		self->priv->current_request = NULL;
	}
	self->priv->requests = g_list_remove (self->priv->requests, load_request);
	load_request_unref (load_request);
}

static void
_gth_image_preloader_cancel_request (GthImagePreloader *self,
				     LoadRequest       *request);

static void
_gth_image_preloader_load_current_file (GthImagePreloader *self,
					LoadRequest       *request);


static gboolean
load_current_file (gpointer user_data)
{
	LoadRequest       *request = user_data;
	GthImagePreloader *self = request->preloader;

	g_return_val_if_fail (request->current_file != NULL, FALSE);

	if (self->priv->last_request != request) {
		_gth_image_preloader_cancel_request (self, request);
	}
	else {
		if (self->priv->load_next_id > 0) {
			g_source_remove (self->priv->load_next_id);
			self->priv->load_next_id = 0;
		}
		_gth_image_preloader_load_current_file (self, request);
	}

	return FALSE;
}


static void
_gth_image_preloader_request_completed (GthImagePreloader *self,
					LoadRequest       *request,
					CacheData	  *cache_data)
{
	if (request->current_file == request->requested_file) {
		if (cache_data != NULL) {
#ifdef DEBUG_PRELOADER
			{
				cairo_surface_t	*image;
				int		 w, h;

				image = NULL;
				if (cache_data->image != NULL)
					image = gth_image_get_cairo_surface (cache_data->image);
				if (image != NULL) {
					w = cairo_image_surface_get_width (image);
					h = cairo_image_surface_get_height (image);
				}
				else {
					w = 0;
					h = 0;
				}

				g_print (" --> done @%d [%dx%d]\n",
						cache_data->requested_size,
						w,
						h);
			}
#endif

			g_task_return_pointer (request->task,
					       cache_data_ref (cache_data),
					       (GDestroyNotify) cache_data_unref);
		}
		else
			load_request_completed_with_error (request, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
	}

	/* queue the next file */

	g_return_if_fail (request->current_file != NULL);

	cache_data = NULL;
	do {
		GthFileData *requested_file;

		request->current_file = request->current_file->next;
		if (request->current_file == NULL) {
			_gth_image_preloader_request_finished (self, request);
			return;
		}

		requested_file = (GthFileData *) request->current_file->data;
		cache_data = _gth_image_preloader_lookup_same_size (self,
								    requested_file,
								    request->requested_size);
	}
	while (cache_data != NULL);

	if (self->priv->load_next_id > 0)
		g_source_remove (self->priv->load_next_id);
	self->priv->load_next_id = g_timeout_add (LOAD_NEXT_FILE_DELAY,
						  load_current_file,
						  request);
}


static void
_gth_image_preloader_start_last_request (GthImagePreloader *self);


static void
_gth_image_preloader_add_to_cache (GthImagePreloader *self,
				   CacheData         *cache_data)
{
	while (g_queue_get_length (self->priv->cache) >= CACHE_MAX_SIZE) {
		CacheData *oldest = g_queue_pop_tail (self->priv->cache);
		cache_data_unref (oldest);
	}
	g_queue_push_head (self->priv->cache, cache_data);
}


typedef struct {
	LoadRequest *request;
	gboolean     resize_to_requested_size;
} LoadData;


static LoadData *
load_data_new (LoadRequest *request,
	       gboolean     resize_image)
{
	LoadData *load_data;

	load_data = g_new0 (LoadData, 1);
	load_data->request = load_request_ref (request);
	load_data->resize_to_requested_size = resize_image;

	return load_data;
}


static void
load_data_free (LoadData *load_data)
{
	load_request_unref (load_data->request);
	g_free (load_data);
}


#ifdef RESIZE_TO_REQUESTED_SIZE

static void
image_scale_ready_cb (GObject      *source_object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	LoadData          *load_data = user_data;
	LoadRequest       *request = load_data->request;
	GthImagePreloader *self = request->preloader;
	cairo_surface_t   *surface;
	GError            *error = NULL;
	int                original_width;
	int                original_height;
	CacheData         *cache_data;

	if (request->finalized) {
		load_data_free (load_data);
		return;
	}

	surface = _cairo_image_surface_scale_finish (result, &error);

	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)
	    || (self->priv->last_request != request))
	{
		load_data_free (load_data);
		if (error != NULL)
			g_error_free (error);
		cairo_surface_destroy (surface);
		_gth_image_preloader_request_finished (self, request);

		_gth_image_preloader_start_last_request (self);

		return;
	}

	if (! _cairo_image_surface_get_original_size (surface, &original_width, &original_height)) {
		original_width = cairo_image_surface_get_width (surface);
		original_height = cairo_image_surface_get_height (surface);
	}
	cache_data = cache_data_new ();
	cache_data->file_data = _g_object_ref (request->current_file->data);
	cache_data->image = (surface != NULL) ? gth_image_new_for_surface (surface) : NULL;
	cache_data->original_width = (surface != NULL) ? original_width : -1;
	cache_data->original_height = (surface != NULL) ? original_height : -1;
	cache_data->requested_size = request->requested_size;
	cache_data->error = error;
	_gth_image_preloader_add_to_cache (self, cache_data);
	_gth_image_preloader_request_completed (self, request, cache_data);

	cairo_surface_destroy (surface);
	load_data_free (load_data);
}


static gboolean
_gth_image_preloader_resize_at_requested_size (GthImagePreloader *self,
					       LoadRequest       *request,
					       GthImage          *image)
{
	cairo_surface_t *surface;
	int              new_width;
	int              new_height;
	gboolean         scaled;

	surface = gth_image_get_cairo_surface (image);
	new_width = cairo_image_surface_get_width (surface);
	new_height = cairo_image_surface_get_height (surface);
	scaled = scale_keeping_ratio (&new_width,
				      &new_height,
				      request->requested_size,
				      request->requested_size,
				      FALSE);

	if (scaled)
		_cairo_image_surface_scale_async (surface,
						  new_width,
						  new_height,
						  SCALE_FILTER_GOOD,
						  request->cancellable,
						  image_scale_ready_cb,
						  load_data_new (request, FALSE));

	cairo_surface_destroy (surface);

	return scaled;
}

#endif


static void
_gth_image_preloader_cancel_request (GthImagePreloader *self,
				     LoadRequest       *request)
{
	if (request->current_file == request->requested_file)
		g_task_return_error (request->task, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, ""));
	_gth_image_preloader_request_finished (self, request);
}


static void
_gth_image_preloader_request_cancelled (GthImagePreloader *self,
				        LoadRequest       *request)
{
	_gth_image_preloader_cancel_request (self, request);
	_gth_image_preloader_start_last_request (self);
}


static void
image_loader_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	LoadData          *load_data = user_data;
	LoadRequest       *request = load_data->request;
	GthImagePreloader *self = request->preloader;
	GthImage          *image = NULL;
	int                original_width;
	int                original_height;
	gboolean           loaded_original;
	GError            *error = NULL;
	gboolean           success;
	CacheData         *cache_data;
	gboolean           resized;

	request->loading = FALSE;

	if (request->finalized) {
#ifdef DEBUG_PRELOADER
		g_print (" --> cancelled [0]\n");
#endif
		load_data_free (load_data);
		return;
	}

	success = gth_image_loader_load_finish  (GTH_IMAGE_LOADER (source_object),
						 result,
						 &image,
						 &original_width,
						 &original_height,
						 &loaded_original,
						 &error);

	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)
	    || (self->priv->last_request != request))
	{
#ifdef DEBUG_PRELOADER
		g_print (" --> cancelled [1] %s\n", g_file_get_uri (GTH_FILE_DATA (request->current_file->data)->file));
#endif
		load_data_free (load_data);
		if (error != NULL)
			g_error_free (error);
		_g_object_unref (image);
		_gth_image_preloader_request_cancelled (self, request);
		return;
	}

	cache_data = cache_data_new ();
	cache_data->file_data = g_object_ref (request->current_file->data);
	cache_data->image = success ? _g_object_ref (image) : NULL;
	cache_data->original_width = success ? original_width : -1;
	cache_data->original_height = success ? original_height : -1;
	cache_data->requested_size = request->requested_size;
	cache_data->loaded_original = loaded_original;
	cache_data->error = error;
	_gth_image_preloader_add_to_cache (self, cache_data);

	if ((request->requested_size > 0) && loaded_original)
		load_data->resize_to_requested_size = TRUE;

	if ((image != NULL) && (gth_image_get_is_zoomable (image) || gth_image_get_is_animation (image)))
		load_data->resize_to_requested_size = FALSE;

	resized = FALSE;
#ifdef RESIZE_TO_REQUESTED_SIZE
	if (load_data->resize_to_requested_size)
		resized = _gth_image_preloader_resize_at_requested_size (self, request, cache_data->image);
#endif

	if (! resized)
		_gth_image_preloader_request_completed (self, request, cache_data);

	_g_object_unref (image);
	load_data_free (load_data);
}


static void
_gth_image_preloader_load_current_file (GthImagePreloader *self,
					LoadRequest       *request)
{
	GthFileData *requested_file;
	CacheData   *cache_data;
	gboolean     ignore_requested_size;

	g_return_if_fail (request->current_file != NULL);
	requested_file = (GthFileData *) request->current_file->data;

	/* search the file at the requested size */

	cache_data = _gth_image_preloader_lookup_same_size (self,
							    requested_file,
							    request->requested_size);
	if (cache_data != NULL) {
		_gth_image_preloader_request_completed (self, request, cache_data);
		return;
	}

	/* search the file at a bigger size */

	cache_data = _gth_image_preloader_lookup_bigger_size (self,
							      requested_file,
							      request->requested_size);
	if (cache_data != NULL) {
#ifdef RESIZE_TO_REQUESTED_SIZE
		if (! _gth_image_preloader_resize_at_requested_size (self, request, cache_data->image))
#endif
		_gth_image_preloader_request_completed (self, request, cache_data);
		return;
	}

	/* load the file at the requested size */

	if (requested_file == GTH_MODIFIED_IMAGE) {
		/* not found */
		_gth_image_preloader_request_completed (self, request, NULL);
		return;
	}

	ignore_requested_size = (request->requested_size > 0) && ! g_file_is_native (requested_file->file);

#ifdef DEBUG_PRELOADER
	g_print ("load %s @%d\n", g_file_get_uri (requested_file->file), ignore_requested_size ? -1 : request->requested_size);
#endif

	request->loading = TRUE;
	gth_image_loader_set_out_profile (self->priv->loader, self->priv->out_profile);
	gth_image_loader_load (self->priv->loader,
			       requested_file,
			       ignore_requested_size ? -1 : request->requested_size,
			       (request->current_file == request->files) ? G_PRIORITY_HIGH : G_PRIORITY_DEFAULT,
			       request->cancellable,
			       image_loader_ready_cb,
			       load_data_new (request, ignore_requested_size));
}


static void
_gth_image_preloader_start_request (GthImagePreloader *self,
				    LoadRequest       *request)
{
	if (request == NULL)
		return

	load_request_unref (self->priv->current_request);
	self->priv->current_request = load_request_ref (request);

	request->current_file = request->files;
	_gth_image_preloader_load_current_file (self, request);

}


static void
_gth_image_preloader_start_last_request (GthImagePreloader *self)
{
	GList *tmp_requests;
	GList *scan;

	tmp_requests = g_list_copy (self->priv->requests);
	for (scan = tmp_requests; scan; scan = scan->next) {
		LoadRequest *request = scan->data;
		if (request != self->priv->last_request)
			_gth_image_preloader_cancel_request (self, request);
	}
	g_list_free (tmp_requests);

	_gth_image_preloader_start_request (self, self->priv->last_request);
}


static void
_gth_image_preloader_cancel_current_request (GthImagePreloader *self)
{
	if (self->priv->current_request == NULL)
		return;

	if (self->priv->load_next_id > 0) {
		g_source_remove (self->priv->load_next_id);
		self->priv->load_next_id = 0;
	}

	if (! self->priv->current_request->loading)
		_gth_image_preloader_request_cancelled (self, self->priv->current_request);
	else
		g_cancellable_cancel (self->priv->current_request->cancellable);
}


static GthFileData *
check_file (GthFileData *file_data)
{
	if (file_data == NULL)
		return NULL;

	if (! g_file_is_native (file_data->file))
		return NULL;

	if (! _g_mime_type_is_image (gth_file_data_get_mime_type (file_data)))
		return NULL;

	return gth_file_data_dup (file_data);
}


static void
request_cancelled (GCancellable *cancellable,
		   gpointer      user_data)
{
	LoadRequest *request = user_data;
	g_cancellable_cancel (request->cancellable);
}


void
gth_image_preloader_load (GthImagePreloader	 *self,
			  GthFileData		 *requested,
			  int			  requested_size,
			  GCancellable		 *cancellable,
			  GAsyncReadyCallback	  callback,
			  gpointer		  user_data,
			  int			  n_files,
			  ...)
{
	LoadRequest *request;
	va_list      args;

#ifdef DEBUG_PRELOADER
	if (requested != NULL)
		g_print ("request %s @%d\n", g_file_get_uri (requested->file), requested_size);
	else
		g_print ("request modified image @%d\n", requested_size);
#endif

	request = load_request_new (self);
	request->requested_size = requested_size;
	request->files = g_list_prepend (request->files, gth_file_data_dup (requested));
	va_start (args, n_files);
	while (n_files-- > 0) {
		GthFileData *file_data;
		GthFileData *checked_file_data;

		file_data = va_arg (args, GthFileData *);
		checked_file_data = check_file (file_data);
		if (checked_file_data != NULL)
			request->files = g_list_prepend (request->files, checked_file_data);
	}
	va_end (args);
	request->files = g_list_reverse (request->files);
	request->requested_file = request->files;
	request->current_file = request->files;
	request->cancellable = g_cancellable_new ();
	request->task = g_task_new (G_OBJECT (self), request->cancellable, callback, user_data);
	if (cancellable != NULL) {
		request->extern_cancellable = g_object_ref (cancellable);
		request->cancellable_id = g_cancellable_connect (cancellable, G_CALLBACK (request_cancelled), request, NULL);
	}

	self->priv->requests = g_list_prepend (self->priv->requests, request);
	self->priv->last_request = request;

	if (self->priv->current_request != NULL)
		_gth_image_preloader_cancel_current_request (self);
	else
		_gth_image_preloader_start_last_request (self);
}


gboolean
gth_image_preloader_load_finish (GthImagePreloader	 *self,
				 GAsyncResult		 *result,
				 GthFileData		**requested,
				 GthImage		**image,
				 int			 *requested_size,
				 int			 *original_width,
				 int			 *original_height,
				 GError			**error)
{
	CacheData *cache_data;

	g_return_val_if_fail (g_task_is_valid (G_TASK (result), G_OBJECT (self)), FALSE);

	cache_data = g_task_propagate_pointer (G_TASK (result), error);
	if (cache_data == NULL)
		return FALSE;

	if (cache_data->error != NULL) {
		if (error != NULL)
			*error = g_error_copy (cache_data->error);
		cache_data_unref (cache_data);
		return FALSE;
	}

	if (requested != NULL)
		*requested = _g_object_ref (cache_data->file_data);
	if (image != NULL)
		*image = _g_object_ref (cache_data->image);
	if (requested_size != NULL)
		*requested_size = cache_data->requested_size;
	if (original_width != NULL)
		*original_width = cache_data->original_width;
	if (original_height != NULL)
		*original_height = cache_data->original_height;

	cache_data_unref (cache_data);

	return TRUE;
}


void
gth_image_preloader_set_modified_image (GthImagePreloader *self,
					GthImage	  *image)
{
	GList     *scan;
	CacheData *cache_data;

	/* delete the modified image from the cache */

	for (scan = self->priv->cache->head; scan; /* void */) {
		GList *next = scan->next;

		cache_data = scan->data;
		if (cache_data->file_data == GTH_MODIFIED_IMAGE)
			g_queue_delete_link (self->priv->cache, scan);
		scan = next;
	}

	if (image == NULL)
		return;

	/* add the modified image to the cache */

	cache_data = cache_data_new ();
	cache_data->file_data = GTH_MODIFIED_IMAGE;
	cache_data->image = g_object_ref (image);
	cache_data->original_width = -1;
	cache_data->original_height = -1;
	cache_data->requested_size = -1;
	cache_data->error = NULL;
	_gth_image_preloader_add_to_cache (self, cache_data);
}


cairo_surface_t *
gth_image_preloader_get_modified_image (GthImagePreloader *self)
{
	GList *scan;

	for (scan = self->priv->cache->head; scan; scan = scan->next) {
		CacheData *cache_data = scan->data;

		if ((cache_data->file_data == GTH_MODIFIED_IMAGE) && (cache_data->requested_size == -1))
			return gth_image_get_cairo_surface (cache_data->image);
	}

	return NULL;
}


void
gth_image_preloader_clear_cache (GthImagePreloader *self)
{
	g_queue_free_full (self->priv->cache, (GDestroyNotify) cache_data_unref);
	self->priv->cache = g_queue_new ();
}
