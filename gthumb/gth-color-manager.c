/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2017 Free Software Foundation, Inc.
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
#include <glib.h>
#ifdef HAVE_LCMS2
#include <lcms2.h>
#endif /* HAVE_LCMS2 */
#ifdef HAVE_COLORD
#include <colord.h>
#endif /* HAVE_COLORD */
#include "gio-utils.h"
#include "glib-utils.h"
#include "gth-color-manager.h"


typedef struct {
	GthICCProfile   *in_profile;
	GthICCProfile   *out_profile;
	GthICCTransform *transform;
} TransformData;


static TransformData *
transform_data_new (void)
{
	TransformData *data;

	data = g_new (TransformData, 1);
	data->in_profile = NULL;
	data->out_profile = NULL;
	data->transform = NULL;

	return data;
}


static void
transform_data_free (TransformData *data)
{
	_g_object_unref (data->in_profile);
	_g_object_unref (data->out_profile);
	_g_object_unref (data->transform);
	g_free (data);
}


/* Signals */
enum {
	CHANGED,
	LAST_SIGNAL
};


struct _GthColorManagerPrivate {
	GHashTable *profile_cache;
	GHashTable *transform_cache;
#if HAVE_COLORD
	CdClient   *cd_client;
#else
	GObject    *cd_client;
#endif
};


static guint gth_color_manager_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthColorManager,
			 gth_color_manager,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthColorManager))


static void
gth_color_manager_finalize (GObject *object)
{
	GthColorManager *self;

	self = GTH_COLOR_MANAGER (object);

	g_hash_table_unref (self->priv->profile_cache);
	g_hash_table_unref (self->priv->transform_cache);
	_g_object_unref (self->priv->cd_client);

	G_OBJECT_CLASS (gth_color_manager_parent_class)->finalize (object);
}


static void
gth_color_manager_class_init (GthColorManagerClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_color_manager_finalize;

	/* signals */

	gth_color_manager_signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthColorManagerClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}


static void
gth_color_manager_init (GthColorManager *self)
{
	self->priv = gth_color_manager_get_instance_private (self);
	self->priv->profile_cache = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
	self->priv->transform_cache = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) transform_data_free);
	self->priv->cd_client = NULL;
}


GthColorManager *
gth_color_manager_new (void)
{
	return (GthColorManager*) g_object_new (GTH_TYPE_COLOR_MANAGER, NULL);
}



#if HAVE_LCMS2 && HAVE_COLORD


static void
_gth_color_manager_add_profile (GthColorManager	 *self,
			        const char       *id,
			        GthICCProfile    *profile)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (id != NULL);
	g_return_if_fail (profile != NULL);

	g_hash_table_insert (self->priv->profile_cache, g_strdup (id), g_object_ref (profile));
}


typedef struct {
	GthColorManager *color_manager;
	char            *monitor_name;
	char            *cache_id;
	GFile           *icc_file;
} ProfilesData;


static void
profiles_data_free (ProfilesData *data)
{
	_g_object_unref (data->color_manager);
	_g_object_unref (data->icc_file);
	g_free (data->monitor_name);
	g_free (data->cache_id);
	g_slice_free (ProfilesData, data);
}


static void
profile_buffer_ready_cb (void     **buffer,
			 gsize      count,
			 GError    *error,
			 gpointer   user_data)
{
	GTask *task = user_data;

	if (error != NULL) {
		g_task_return_error (task, g_error_copy (error));
	}
	else {
		ProfilesData  *data = g_task_get_task_data (task);
		char          *uri;
		GthICCProfile *profile;

		uri = g_file_get_uri (data->icc_file);
		profile = gth_icc_profile_new (uri, (GthCMSProfile) cmsOpenProfileFromMem (*buffer, count));
		_gth_color_manager_add_profile (data->color_manager, data->cache_id, profile);
		g_task_return_pointer (task, g_object_ref (profile), g_object_unref);

		g_object_unref (profile);
		g_free (uri);
	}

	g_object_unref (task);
}


static void
cd_profile_connected_cb (GObject      *source_object,
		         GAsyncResult *res,
		         gpointer      user_data)
{
	CdProfile    *profile = CD_PROFILE (source_object);
	GTask        *task = user_data;
	ProfilesData *data = g_task_get_task_data (task);

	if (! cd_profile_connect_finish (profile, res, NULL)) {
		g_task_return_pointer (task, NULL, NULL);
		g_object_unref (task);
	}
	else {
		data->icc_file = g_file_new_for_path (cd_profile_get_filename (profile));
		_g_file_load_async (data->icc_file,
				    G_PRIORITY_DEFAULT,
				    g_task_get_cancellable (task),
				    profile_buffer_ready_cb,
				    task);
	}

	g_object_unref (profile);
}


static void
cd_device_connected_cb (GObject      *source_object,
		        GAsyncResult *res,
		        gpointer      user_data)
{
	CdDevice *device = CD_DEVICE (source_object);
	GTask    *task = user_data;

	if (! cd_device_connect_finish (device, res, NULL)) {
		g_task_return_pointer (task, NULL, NULL);
		g_object_unref (task);
	}
	else {
		CdProfile *profile = cd_device_get_default_profile (device);
		if (profile != NULL) {
			cd_profile_connect (profile,
					    g_task_get_cancellable (task),
					    cd_profile_connected_cb,
					    task);
		}
		else {
			g_task_return_pointer (task, NULL, NULL);
			g_object_unref (task);
		}
	}

	g_object_unref (device);
}


static void
find_device_by_kind_cb (GObject      *source_object,
			GAsyncResult *res,
			gpointer      user_data)
{
	CdClient  *cd_client = CD_CLIENT (source_object);
	GTask     *task = user_data;
	GPtrArray *devices;
	CdDevice  *device;

	devices = cd_client_get_devices_by_kind_finish (cd_client, res, NULL);
	if ((devices == NULL) || (devices->len == 0)) {
		g_task_return_pointer (task, NULL, NULL);
		g_object_unref (task);
		return;
	}

	device = g_object_ref (g_ptr_array_index (devices, 0));
	cd_device_connect (device,
			   g_task_get_cancellable (task),
			   cd_device_connected_cb,
			   task);

	g_ptr_array_unref (devices);
}


static void
find_device_by_xrandr_name_cb (GObject      *source_object,
			       GAsyncResult *res,
			       gpointer      user_data)
{
	CdClient *cd_client = CD_CLIENT (source_object);
	GTask    *task = user_data;
	CdDevice *device;

	device = cd_client_find_device_by_property_finish (cd_client, res, NULL);
	if (device == NULL) {
		cd_client_get_devices_by_kind (cd_client,
					       CD_DEVICE_KIND_DISPLAY,
					       g_task_get_cancellable (task),
					       find_device_by_kind_cb,
					       task);
		return;
	}

	cd_device_connect (device,
			   g_task_get_cancellable (task),
			   cd_device_connected_cb,
			   task);
}


static void
find_device_by_model_cb (GObject      *source_object,
			 GAsyncResult *res,
			 gpointer      user_data)
{
	CdClient *cd_client = CD_CLIENT (source_object);
	GTask    *task = user_data;
	CdDevice *device;

	device = cd_client_find_device_by_property_finish (cd_client, res, NULL);
	if (device == NULL) {
		ProfilesData *data = g_task_get_task_data (task);
		cd_client_find_device_by_property (cd_client,
						   CD_DEVICE_METADATA_XRANDR_NAME,
						   data->monitor_name,
						   g_task_get_cancellable (task),
						   find_device_by_xrandr_name_cb,
						   task);
		return;
	}

	cd_device_connect (device,
			   g_task_get_cancellable (task),
			   cd_device_connected_cb,
			   task);
}


static void
_cd_client_find_device_for_task (CdClient *cd_client,
		                 GTask    *task)
{
	ProfilesData *data = g_task_get_task_data (task);

	cd_client_find_device_by_property (cd_client,
					   CD_DEVICE_PROPERTY_MODEL,
					   data->monitor_name,
					   g_task_get_cancellable (task),
					   find_device_by_model_cb,
					   task);
}


static void
cd_client_connected_cb (GObject      *source_object,
			GAsyncResult *res,
			gpointer      user_data)
{
	CdClient *cd_client = CD_CLIENT (source_object);
	GTask    *task = user_data;

	if (! cd_client_connect_finish (cd_client, res, NULL)) {
		g_task_return_pointer (task, NULL, NULL);
		g_object_unref (task);
		return;
	}

	_cd_client_find_device_for_task (cd_client, task);
}


#endif /* HAVE_LCMS2 && HAVE_COLORD */


static GthICCProfile *
_gth_color_manager_get_profile (GthColorManager	 *self,
			        const char       *id)
{
	GthICCProfile *profile;

	g_return_val_if_fail (self != NULL, NULL);
	g_return_val_if_fail (id != NULL, NULL);

	profile = g_hash_table_lookup (self->priv->profile_cache, id);
	return _g_object_ref (profile);
}


void
gth_color_manager_get_profile_async (GthColorManager	 *self,
				     char                *monitor_name,
		     	     	     GCancellable	 *cancellable,
				     GAsyncReadyCallback  callback,
				     gpointer		  user_data)
{
	GTask *task;

	task = g_task_new (self, cancellable, callback, user_data);

#if HAVE_LCMS2

	{
		char          *id;
		GthICCProfile *profile;

		id = g_strdup_printf ("monitor://%s", monitor_name);
		profile = _gth_color_manager_get_profile (self, id);
		if (profile != NULL) {
			g_task_return_pointer (task, g_object_ref (profile), g_object_unref);
			g_object_unref (task);
			g_object_unref (profile);
			g_free (id);
			return;
		}

#if HAVE_COLORD

		{
			ProfilesData  *data;

			data = g_slice_new (ProfilesData);
			data->color_manager = g_object_ref (self);
			data->monitor_name = g_strdup (monitor_name);
			data->cache_id = g_strdup (id);
			data->icc_file = NULL;
			g_task_set_task_data (task, data, (GDestroyNotify) profiles_data_free);

			if (self->priv->cd_client == NULL)
				self->priv->cd_client = cd_client_new ();

			if (! cd_client_get_connected (self->priv->cd_client))
				cd_client_connect (self->priv->cd_client,
						   g_task_get_cancellable (task),
						   cd_client_connected_cb,
						   task);
			else
				_cd_client_find_device_for_task (self->priv->cd_client, task);

			g_free (id);
			return;
		}

#endif /* HAVE_COLORD */

		g_free (id);
	}

#endif /* HAVE_LCMS2 */

	g_task_return_pointer (task, NULL, NULL);
	g_object_unref (task);
}


GthICCProfile *
gth_color_manager_get_profile_finish (GthColorManager	 *self,
				      GAsyncResult	 *result,
				      GError		**error)
{
	g_return_val_if_fail (g_task_is_valid (result, self), NULL);
	return g_task_propagate_pointer (G_TASK (result), error);
}


static char *
create_transform_id_for_cache (GthICCProfile *from_profile,
			       GthICCProfile *to_profile)
{
	const char *from_id = gth_icc_profile_get_id (from_profile);
	const char *to_id = gth_icc_profile_get_id (to_profile);

	/*g_print (" -> gth_color_manager_get_transform: %s -> %s\n", from_id, to_id);*/

	if (gth_icc_profile_id_is_unknown (from_id) || gth_icc_profile_id_is_unknown (to_id))
		return NULL;

	return g_strdup_printf ("%s -> %s", from_id, to_id);
}


GthICCTransform *
gth_color_manager_get_transform (GthColorManager *self,
				 GthICCProfile   *in_profile,
				 GthICCProfile   *out_profile)
{
#if HAVE_LCMS2

	GthICCTransform *transform = NULL;
	char            *transform_id;

	g_return_val_if_fail (self != NULL, NULL);

	transform_id = create_transform_id_for_cache (in_profile, out_profile);
	if (transform_id != NULL) {
		TransformData *transform_data = g_hash_table_lookup (self->priv->transform_cache, transform_id);

		if (transform_data == NULL) {
			transform_data = transform_data_new ();
			transform_data->in_profile = g_object_ref (in_profile);
			transform_data->out_profile = g_object_ref (out_profile);
			transform_data->transform = gth_icc_transform_new_from_profiles (transform_data->in_profile, transform_data->out_profile);

			if (transform_data->transform != NULL) {
				g_hash_table_insert (self->priv->transform_cache, g_strdup (transform_id), transform_data);
			}
			else {
				transform_data_free (transform_data);
				transform_data = NULL;
			}
		}

		if (transform_data != NULL)
			transform = g_object_ref (transform_data->transform);

		g_free (transform_id);
	}
	else
		transform = gth_icc_transform_new_from_profiles (in_profile, out_profile);

	return transform;

#else

	return NULL;

#endif
}
