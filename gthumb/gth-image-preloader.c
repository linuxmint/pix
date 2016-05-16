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
#include "glib-utils.h"
#include "gth-image-preloader.h"
#include "gth-marshal.h"


#undef DEBUG_PRELOADER
#define GTH_IMAGE_PRELOADER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_IMAGE_PRELOADER, GthImagePreloaderPrivate))
#define REQUESTED_INTERVAL 100
#define SECOND_STEP_INTERVAL 100
#define NOT_REQUESTED_INTERVAL 400


enum {
	REQUESTED_READY,
	ORIGINAL_SIZE_READY,
	LAST_SIGNAL
};


typedef struct {
	int                 ref;
	GthImagePreloader  *self;
	GthFileData        *file_data;
	int                 requested_size;
	gboolean            loaded;
	gboolean            error;
	gboolean	    finalized;
	GthImageLoader     *loader;
	GthImage           *image;
	int                 original_width;
	int                 original_height;
	guint               token;
} Preloader;


typedef struct {
	GthImagePreloader  *self;
	GthFileData        *requested;
	int                 requested_size;
	GthFileData       **files;
	int                 n_files;
	guint               token;
} LoadData;


struct _GthImagePreloaderPrivate {
	GthLoadPolicy load_policy;
	int           n_preloaders;
	Preloader   **loader;                  /* Array of loaders, each loader
					        * will load an image. */
	int           requested;               /* This is the loader with the
					        * requested image.  The
					        * requested image is the image
					        * the user has expressly
					        * requested to view, when this
					        * image is loaded a
					        * requested_ready signal is
					        * emitted.
					        * Other images do not trigger
					        * any signal. */
	GFile        *requested_file;
	int           requested_size;
	int           current;                 /* This is the loader that has
					        * a loading underway. */
	guint         load_id;
	GCancellable *cancellable;
	guint         token;
	LoadData     *next_load_data;
};


static guint gth_image_preloader_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (GthImagePreloader, gth_image_preloader, G_TYPE_OBJECT)


/* -- Preloader -- */


static Preloader *
preloader_new (GthImagePreloader *self)
{
	Preloader *preloader;

	preloader = g_new0 (Preloader, 1);
	preloader->ref = 1;
	preloader->self = self;
	preloader->file_data = NULL;
	preloader->loaded = FALSE;
	preloader->error = FALSE;
	preloader->finalized = FALSE;
	preloader->loader = gth_image_loader_new (NULL, NULL);
	gth_image_loader_set_preferred_format (preloader->loader, GTH_IMAGE_FORMAT_CAIRO_SURFACE);
	preloader->image = NULL;
	preloader->original_width = -1;
	preloader->original_height = -1;

	return preloader;
}


static Preloader *
preloader_ref (Preloader *preloader)
{
	preloader->ref++;
	return preloader;
}


static void
preloader_unref (Preloader *preloader)
{
	if (preloader == NULL)
		return;
	if (--preloader->ref > 0)
		return;
	_g_object_unref (preloader->image);
	_g_object_unref (preloader->loader);
	_g_object_unref (preloader->file_data);
	g_free (preloader);
}


static void
preloader_set_file_data (Preloader    *preloader,
			 GthFileData  *file_data)
{
	g_return_if_fail (preloader != NULL);

	if (preloader->file_data != file_data) {
		_g_object_unref (preloader->file_data);
		preloader->file_data = NULL;
	}

	if (file_data != NULL)
		preloader->file_data = g_object_ref (file_data);

	preloader->loaded = FALSE;
	preloader->error = FALSE;
}


static gboolean
preloader_has_valid_content_for_file (Preloader   *preloader,
				      GthFileData *file_data)
{
	return ((preloader->file_data != NULL)
		&& preloader->loaded
		&& ! preloader->error
		&& (preloader->image != NULL)
	        && g_file_equal (preloader->file_data->file, file_data->file)
	        && (_g_time_val_cmp (gth_file_data_get_modification_time (file_data),
	        		     gth_file_data_get_modification_time (preloader->file_data)) == 0));
}


static gboolean
preloader_needs_to_load (Preloader *preloader)
{
	return ((preloader->token == preloader->self->priv->token)
		 && (preloader->file_data != NULL)
		 && ! preloader->error
		 && ! preloader->loaded);
}


static gboolean
preloader_needs_second_step (Preloader *preloader)
{
	return ((preloader->token == preloader->self->priv->token)
		&& ! preloader->error
		&& (preloader->requested_size != -1)
		&& ((preloader->original_width > preloader->requested_size) || (preloader->original_height > preloader->requested_size))
		&& (preloader->image != NULL)
		&& ! gth_image_is_animation (preloader->image));
}


static int
preloader_signal_to_emit (Preloader *preloader)
{
	int signal = -1;

	switch (preloader->self->priv->load_policy) {
	case GTH_LOAD_POLICY_ONE_STEP:
		signal = REQUESTED_READY;
		break;

	case GTH_LOAD_POLICY_TWO_STEPS:
		if (preloader->self->priv->requested_size == -1)
			signal = ORIGINAL_SIZE_READY;
		else
			signal = REQUESTED_READY;
		break;
	}

	g_assert (signal != -1);

	return signal;
}


/* -- GthImagePreloader -- */


static void load_data_free (LoadData *load_data);


static void
gth_image_preloader_finalize (GObject *object)
{
	GthImagePreloader *self;
	int                i;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_PRELOADER (object));

	self = GTH_IMAGE_PRELOADER (object);

	if (self->priv->load_id != 0) {
		g_source_remove (self->priv->load_id);
		self->priv->load_id = 0;
	}

	if (self->priv->next_load_data != NULL) {
		load_data_free (self->priv->next_load_data);
		self->priv->next_load_data = NULL;
	}

	for (i = 0; i < self->priv->n_preloaders; i++) {
		self->priv->loader[i]->finalized = TRUE;
		preloader_unref (self->priv->loader[i]);
		self->priv->loader[i] = NULL;
	}
	g_free (self->priv->loader);
	_g_object_unref (self->priv->requested_file);
	g_object_unref (self->priv->cancellable);

	G_OBJECT_CLASS (gth_image_preloader_parent_class)->finalize (object);
}


static void
gth_image_preloader_class_init (GthImagePreloaderClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (GthImagePreloaderPrivate));

	gth_image_preloader_signals[REQUESTED_READY] =
		g_signal_new ("requested_ready",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImagePreloaderClass, requested_ready),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_OBJECT_INT_INT_POINTER,
			      G_TYPE_NONE,
			      5,
			      G_TYPE_OBJECT,
			      G_TYPE_OBJECT,
			      G_TYPE_INT,
			      G_TYPE_INT,
			      G_TYPE_POINTER);
	gth_image_preloader_signals[ORIGINAL_SIZE_READY] =
		g_signal_new ("original_size_ready",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImagePreloaderClass, original_size_ready),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_OBJECT_INT_INT_POINTER,
			      G_TYPE_NONE,
			      5,
			      G_TYPE_OBJECT,
			      G_TYPE_OBJECT,
			      G_TYPE_INT,
			      G_TYPE_INT,
			      G_TYPE_POINTER);

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_image_preloader_finalize;
}


static void
gth_image_preloader_init (GthImagePreloader *self)
{
	self->priv = GTH_IMAGE_PRELOADER_GET_PRIVATE (self);
	self->priv->loader = NULL;
	self->priv->requested = -1;
	self->priv->requested_file = NULL;
	self->priv->current = -1;
	self->priv->load_policy = GTH_LOAD_POLICY_ONE_STEP;
	self->priv->cancellable = g_cancellable_new ();
	self->priv->token = 0;
}


GthImagePreloader *
gth_image_preloader_new (GthLoadPolicy load_policy,
			 int           n_preloaders)
{
	GthImagePreloader *self;
	int                i;

	g_return_val_if_fail (n_preloaders > 0, NULL);

	self = g_object_new (GTH_TYPE_IMAGE_PRELOADER, NULL);

	self->priv->n_preloaders = n_preloaders;
	self->priv->load_policy = load_policy;
	self->priv->loader = g_new0 (Preloader *, self->priv->n_preloaders);
	for (i = 0; i < self->priv->n_preloaders; i++)
		self->priv->loader[i] = preloader_new (self);

	return self;
}


void
gth_image_prelaoder_set_load_policy (GthImagePreloader *self,
				     GthLoadPolicy      policy)
{
	self->priv->load_policy = policy;
}


GthLoadPolicy
gth_image_prelaoder_get_load_policy  (GthImagePreloader *self)
{
	return self->priv->load_policy;
}


/* -- gth_image_preloader_load -- */


static void start_next_loader (GthImagePreloader *self);


static gboolean
load_next (gpointer data)
{
	GthImagePreloader *self = data;

	if (self->priv->load_id != 0) {
		g_source_remove (self->priv->load_id);
		self->priv->load_id = 0;
	}
	start_next_loader (self);

	return FALSE;
}


typedef struct {
	Preloader   *preloader;
	GthFileData *file_data;
	int          requested_size;
} LoadRequest;


static void
load_request_free (LoadRequest *load_request)
{
	preloader_unref (load_request->preloader);
	g_object_unref (load_request->file_data);
	g_free (load_request);
}


static void assign_loaders (LoadData *load_data);


static void
image_loader_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	LoadRequest        *load_request = user_data;
	Preloader          *preloader = load_request->preloader;
	GthImagePreloader  *self = preloader->self;
	GthImage           *image = NULL;
	int                 original_width;
	int                 original_height;
	GError             *error = NULL;
	gboolean            success;
	int                 interval;

	if (preloader->finalized) {
		load_request_free (load_request);
		return;
	}

	self->priv->current = -1;

	success = gth_image_loader_load_finish  (GTH_IMAGE_LOADER (source_object),
						 result,
						 &image,
						 &original_width,
						 &original_height,
						 &error);

	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)
	    || ! g_file_equal (load_request->file_data->file, preloader->file_data->file)
	    || (preloader->token != self->priv->token))
	{
		load_request_free (load_request);
		if (error != NULL)
			g_error_free (error);
		_g_object_unref (image);

		if (self->priv->next_load_data != NULL) {
			assign_loaders (self->priv->next_load_data);
			start_next_loader (self);

			load_data_free (self->priv->next_load_data);
			self->priv->next_load_data = NULL;
		}

		return;
	}

	interval = NOT_REQUESTED_INTERVAL;

	_g_object_unref (preloader->image);
	preloader->image = _g_object_ref (image);
	preloader->original_width = original_width;
	preloader->original_height = original_height;
	preloader->loaded = success;
	preloader->error  = ! success;
	preloader->requested_size = load_request->requested_size;

	if (_g_file_equal (load_request->file_data->file, self->priv->requested_file)) {
#if DEBUG_PRELOADER
		debug (DEBUG_INFO, "loaded [requested] %s => %s [size: %d]", (error == NULL) ? "ready" : "error", g_file_get_uri (preloader->file_data->file), preloader->requested_size);
#endif

		g_signal_emit (G_OBJECT (self),
			       gth_image_preloader_signals[preloader_signal_to_emit (preloader)],
			       0,
			       preloader->file_data,
			       preloader->image,
			       preloader->original_width,
			       preloader->original_height,
			       error);

		/* Reload only if the original size is bigger then the
		 * requested size, and if the image is not an animation. */

		if (preloader_needs_second_step (preloader)) {
			/* Reload the image at the original size */
			preloader->loaded = FALSE;
			preloader->requested_size = -1;
			interval = SECOND_STEP_INTERVAL;
		}
		else
			interval = REQUESTED_INTERVAL;
	}
#if DEBUG_PRELOADER
	else
		debug (DEBUG_INFO, "loaded [non-requested] %s => %s [size: %d]", (error == NULL) ? "ready" : "error", g_file_get_uri (preloader->file_data->file), preloader->requested_size);
#endif

	if (self->priv->load_id == 0)
		self->priv->load_id = g_timeout_add (interval, load_next, self);

	load_request_free (load_request);
	_g_object_unref (image);
}


static Preloader *
current_preloader (GthImagePreloader *self)
{
	return (self->priv->current == -1) ? NULL : self->priv->loader[self->priv->current];
}


static Preloader *
requested_preloader (GthImagePreloader *self)
{
	return (self->priv->requested == -1) ? NULL : self->priv->loader[self->priv->requested];
}


static void
start_next_loader (GthImagePreloader *self)
{
	int        i;
	Preloader *preloader;

	i = -1;

	if (preloader_needs_to_load (requested_preloader (self))) {
		i = self->priv->requested;
	}
	else {
		int n = 0;

		if  (self->priv->current == -1)
			i = 0;
		else
			i = (self->priv->current + 1) % self->priv->n_preloaders;

		for (i = 0; n < self->priv->n_preloaders; i = (i + 1) % self->priv->n_preloaders) {
			if (preloader_needs_to_load (self->priv->loader[i]))
				break;
			n++;
		}

		if (n >= self->priv->n_preloaders)
			i = -1;
	}

	self->priv->current = i;
	preloader = current_preloader (self);

	if (preloader != NULL) {
		LoadRequest *load_request;

#if DEBUG_PRELOADER
		{
			char *uri;

			uri = g_file_get_uri (preloader->file_data->file);
			debug (DEBUG_INFO, "load %s [size: %d]", uri, preloader->requested_size);
			g_free (uri);
		}
#endif

		_g_object_unref (preloader->image);
		preloader->image = NULL;

		load_request = g_new0 (LoadRequest, 1);
		load_request->preloader = preloader_ref (preloader);
		load_request->file_data = g_object_ref (preloader->file_data);
		load_request->requested_size = preloader->requested_size;

		g_cancellable_reset (preloader->self->priv->cancellable);
		gth_image_loader_load (preloader->loader,
				       preloader->file_data,
				       preloader->requested_size,
				       (i == self->priv->requested) ? G_PRIORITY_HIGH : G_PRIORITY_DEFAULT,
				       preloader->self->priv->cancellable,
				       image_loader_ready_cb,
				       load_request);
	}
#if DEBUG_PRELOADER
	else
		debug (DEBUG_INFO, "done");
#endif
}


static void
load_data_free (LoadData *load_data)
{
	int i;

	if (load_data == NULL)
		return;

	for (i = 0; i < load_data->n_files; i++)
		g_object_unref (load_data->files[i]);
	g_free (load_data->files);
	g_free (load_data);
}


static void
assign_loaders (LoadData *load_data)
{
	GthImagePreloader *self = load_data->self;
	gboolean          *file_assigned;
	gboolean          *loader_assigned;
	int                i, j;

	if (load_data->token != self->priv->token)
		return;

	file_assigned = g_new (gboolean, self->priv->n_preloaders);
	loader_assigned = g_new (gboolean, self->priv->n_preloaders);
	for (i = 0; i < self->priv->n_preloaders; i++) {
		Preloader *preloader = self->priv->loader[i];

		loader_assigned[i] = FALSE;
		file_assigned[i] = FALSE;

		if (preloader->loaded && ! preloader->error)
			preloader->token = load_data->token;
	}

	self->priv->requested = -1;

	for (j = 0; j < load_data->n_files; j++) {
		GthFileData *file_data = load_data->files[j];

		if (file_data == NULL)
			continue;

		/* check whether the image has already been loaded. */

		for (i = 0; i < self->priv->n_preloaders; i++) {
			Preloader *preloader = self->priv->loader[i];

			if (preloader_has_valid_content_for_file (preloader, file_data)) {
				loader_assigned[i] = TRUE;
				file_assigned[j] = TRUE;

				if (_g_file_equal (file_data->file, load_data->requested->file)) {
					self->priv->requested = i;

					g_signal_emit (G_OBJECT (self),
							gth_image_preloader_signals[preloader_signal_to_emit (preloader)],
							0,
							preloader->file_data,
							preloader->image,
							preloader->original_width,
							preloader->original_height,
							NULL);

#if DEBUG_PRELOADER
					debug (DEBUG_INFO, "[requested] preloaded %s [size: %d]", g_file_get_uri (preloader->file_data->file), preloader->requested_size);
#endif

					if (preloader_needs_second_step (preloader)) {
						/* Reload the image at the original size */
						preloader->loaded = FALSE;
						preloader->requested_size = -1;
					}
				}

#if DEBUG_PRELOADER
				{
					char *uri;

					uri = g_file_get_uri (file_data->file);
					debug (DEBUG_INFO, "[=] [%d] <- %s", i, uri);
					g_free (uri);
				}
#endif

				break;
			}
		}
	}

	/* assign the remaining files */

	for (j = 0; j < load_data->n_files; j++) {
		GthFileData *file_data = load_data->files[j];
		Preloader   *preloader;
		int          k;

		if (file_data == NULL)
			continue;

		if (file_assigned[j])
			continue;

		/* find the first non-assigned loader */
		for (k = 0; (k < self->priv->n_preloaders) && loader_assigned[k]; k++)
			/* void */;

		g_return_if_fail (k < self->priv->n_preloaders);

		loader_assigned[k] = TRUE;

		preloader = self->priv->loader[k];
		preloader_set_file_data (preloader, file_data);
		preloader->requested_size = _g_file_equal (file_data->file, load_data->requested->file) ? load_data->requested_size  : -1;
		/* force the use of the single step policy if the file is not local, in order to speed-up loading. */
		if (! g_file_is_native (file_data->file))
			preloader->requested_size = -1;
		preloader->token = load_data->token;

		if (_g_file_equal (file_data->file, load_data->requested->file)) {
			self->priv->requested = k;

#if DEBUG_PRELOADER
			{
				char *uri;

				uri = g_file_get_uri (file_data->file);
				debug (DEBUG_INFO, "[requested] %s", uri);
				g_free (uri);
			}
#endif
		}

#if DEBUG_PRELOADER
		{
			char *uri;

			uri = g_file_get_uri (file_data->file);
			debug (DEBUG_INFO, "[+] [%d] <- %s", k, uri);
			g_free (uri);
		}
#endif
	}

	g_free (loader_assigned);
	g_free (file_assigned);
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


void
gth_image_preloader_load (GthImagePreloader *self,
			  GthFileData       *requested,
			  int                requested_size,
			  ...)
{
	LoadData    *load_data;
	int          n;
	va_list      args;
	GthFileData *file_data;

	self->priv->token++;

	_g_object_unref (self->priv->requested_file);
	self->priv->requested_file = g_file_dup (requested->file);

	if (self->priv->next_load_data != NULL) {
		load_data_free (self->priv->next_load_data);
		self->priv->next_load_data = NULL;
	}

	load_data = g_new0 (LoadData, 1);
	load_data->self = self;
	load_data->token = self->priv->token;
	load_data->requested = gth_file_data_dup (requested);
	load_data->requested_size = requested_size;
	load_data->files = g_new0 (GthFileData *, self->priv->n_preloaders);

	n = 0;
	load_data->files[n++] = load_data->requested;
	va_start (args, requested_size);
	while ((n < self->priv->n_preloaders) && (file_data = va_arg (args, GthFileData *)) != NULL) {
		GthFileData *checked_file_data = check_file (file_data);
		if (checked_file_data != NULL)
			load_data->files[n++] = checked_file_data;
	}
	va_end (args);
	load_data->n_files = n;

	if (self->priv->current != -1) {
		Preloader *preloader;

		preloader = current_preloader (self);
		if (preloader != NULL) {
			self->priv->next_load_data = load_data;
			g_cancellable_cancel (preloader->self->priv->cancellable);
			return;
		}
	}

	assign_loaders (load_data);
	start_next_loader (self);

	load_data_free (load_data);
}


GthImageLoader *
gth_image_preloader_get_loader (GthImagePreloader *self,
				GthFileData       *file_data)
{
	int i;

	g_return_val_if_fail (self != NULL, NULL);

	if (file_data == NULL)
		return NULL;

	for (i = 0; i < self->priv->n_preloaders; i++) {
		Preloader *preloader = self->priv->loader[i];

		if (preloader_has_valid_content_for_file (preloader, file_data))
			return preloader->loader;
	}

	return NULL;
}


GthFileData *
gth_image_preloader_get_requested (GthImagePreloader *self)
{
	Preloader *preloader;

	preloader = requested_preloader (self);
	return (preloader != NULL) ? preloader->file_data : NULL;
}
