/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "gio-utils.h"
#include "glib-utils.h"
#include "gth-error.h"
#include "gth-hook.h"
#include "gth-main.h"
#include "gth-image-saver.h"


G_DEFINE_TYPE (GthImageSaver, gth_image_saver, G_TYPE_OBJECT)


static GtkWidget *
base_get_control (GthImageSaver *self)
{
	return gtk_label_new (_("No options available for this file type"));
}


static void
base_save_options (GthImageSaver *self)
{
	/* void */
}


static gboolean
base_can_save (GthImageSaver *self,
	       const char    *mime_type)
{
	return FALSE;
}


static gboolean
base_save_image (GthImageSaver  *self,
		 GthImage       *image,
		 char          **buffer,
		 gsize          *buffer_size,
		 const char     *mime_type,
		 GCancellable   *cancellable,
		 GError        **error)
{
	return FALSE;
}


static void
gth_image_saver_class_init (GthImageSaverClass *klass)
{
	klass->id = "";
	klass->display_name = "";
	klass->get_control = base_get_control;
	klass->save_options = base_save_options;
	klass->can_save = base_can_save;
	klass->save_image = base_save_image;
}


static void
gth_image_saver_init (GthImageSaver *self)
{
	/* void */
}


const char *
gth_image_saver_get_id (GthImageSaver *self)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->id;
}


const char *
gth_image_saver_get_display_name (GthImageSaver *self)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->display_name;
}


const char *
gth_image_saver_get_mime_type (GthImageSaver *self)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->mime_type;
}


const char *
gth_image_saver_get_extensions (GthImageSaver *self)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->extensions;
}


const char *
gth_image_saver_get_default_ext (GthImageSaver *self)
{
	if (GTH_IMAGE_SAVER_GET_CLASS (self)->get_default_ext != NULL)
		return GTH_IMAGE_SAVER_GET_CLASS (self)->get_default_ext (self);
	else
		return gth_image_saver_get_extensions (self);
}


GtkWidget *
gth_image_saver_get_control (GthImageSaver *self)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->get_control (self);
}


void
gth_image_saver_save_options (GthImageSaver *self)
{
	GTH_IMAGE_SAVER_GET_CLASS (self)->save_options (self);
}


gboolean
gth_image_saver_can_save (GthImageSaver *self,
			  const char    *mime_type)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->can_save (self, mime_type);
}


static gboolean
gth_image_saver_save_image (GthImageSaver  *self,
			    GthImage       *image,
			    char          **buffer,
			    gsize          *buffer_size,
			    const char     *mime_type,
			    GCancellable   *cancellable,
			    GError        **error)
{
	return GTH_IMAGE_SAVER_GET_CLASS (self)->save_image (self,
							     image,
							     buffer,
							     buffer_size,
							     mime_type,
							     cancellable,
							     error);
}


static GthImageSaveData *
_gth_image_save_to_buffer_common (GthImage      *image,
				  const char    *mime_type,
				  GthFileData   *file_data,
				  GCancellable  *cancellable,
				  GError       **p_error)
{
	GthImageSaver    *saver;
	char             *buffer;
	gsize             buffer_size;
	GError           *error = NULL;
	GthImageSaveData *save_data = NULL;

	saver = gth_main_get_image_saver (mime_type);
	if (saver == NULL) {
		if (p_error != NULL)
			*p_error = g_error_new (GTH_ERROR, GTH_ERROR_GENERIC, _("Could not find a suitable module to save the image as “%s”"), mime_type);
		return NULL;
	}

	if (gth_image_saver_save_image (saver,
					image,
					&buffer,
					&buffer_size,
					mime_type,
					cancellable,
					&error))
	{
		save_data = g_new0 (GthImageSaveData, 1);
		save_data->file_data = _g_object_ref (file_data);
		save_data->image = gth_image_copy (image);
		save_data->mime_type = mime_type;
		save_data->buffer = buffer;
		save_data->buffer_size = buffer_size;
		save_data->files = NULL;
		save_data->error = NULL;
		save_data->cancellable = _g_object_ref (cancellable);

		if (save_data->file_data != NULL)
			gth_hook_invoke ("save-image", save_data);

		if ((save_data->error != NULL) && (p_error != NULL))
			*p_error = g_error_copy (*save_data->error);
	}
	else {
		if (p_error != NULL)
			*p_error = error;
		else
			_g_error_free (error);
	}

	g_object_unref (saver);

	return save_data;
}


static void
gth_image_save_file_free (GthImageSaveFile *file)
{
	g_object_unref (file->file);
	g_free (file->buffer);
	g_free (file);
}


static void
gth_image_save_data_free (GthImageSaveData *data)
{
	_g_object_unref (data->cancellable);
	_g_object_unref (data->file_data);
	g_object_unref (data->image);
	g_list_foreach (data->files, (GFunc) gth_image_save_file_free, NULL);
	g_list_free (data->files);
	g_free (data);
}


gboolean
gth_image_save_to_buffer (GthImage      *image,
			  const char    *mime_type,
			  GthFileData   *file_data,
			  char         **buffer,
			  gsize         *buffer_size,
			  GCancellable  *cancellable,
			  GError       **p_error)
{
	GthImageSaveData *save_data;

	g_return_val_if_fail (image != NULL, FALSE);

	save_data = _gth_image_save_to_buffer_common (image,
						      mime_type,
						      file_data,
						      cancellable,
						      p_error);

	if (save_data != NULL) {
		*buffer = save_data->buffer;
		*buffer_size = save_data->buffer_size;
		gth_image_save_data_free (save_data);
		return TRUE;
	}

	return FALSE;
}


/* -- gth_image_save_to_buffer_async -- */


typedef struct {
	GthImage        *image;
	char            *mime_type;
	GthFileData     *file_data;
	gboolean         replace;
	GCancellable    *cancellable;
	GthFileDataFunc  ready_func;
	gpointer         user_data;
} SaveArguments;


static void
save_arguments_free (SaveArguments *arguments)
{
	_g_object_unref (arguments->image);
	g_free (arguments->mime_type);
	_g_object_unref (arguments->file_data);
	_g_object_unref (arguments->cancellable);
	g_free (arguments);
}


static void
gth_image_save_to_buffer_async (GthImage            *image,
				const char          *mime_type,
				GthFileData         *file_data,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data);


static gboolean
gth_image_save_to_buffer_finish (GAsyncResult      *result,
		       	         GthImageSaveData **save_data,
		       	         GError           **error)
{
	GthImageSaveData *data;

	data = g_task_propagate_pointer (G_TASK (result), error);
	if (data == NULL)
		return FALSE;

	if (save_data != NULL)
		*save_data = data;
	else
		gth_image_save_data_free (data);

	return TRUE;
}


static void
save_to_buffer_thread (GTask        *task,
		       gpointer      source_object,
		       gpointer      task_data,
		       GCancellable *cancellable)
{
	SaveArguments    *arguments;
	GthImageSaveData *data;
	GError           *error = NULL;

	arguments = g_task_get_task_data (task);
	data = _gth_image_save_to_buffer_common (arguments->image,
						 arguments->mime_type,
						 arguments->file_data,
						 cancellable,
						 &error);
	if (data != NULL)
		g_task_return_pointer (task, data, (GDestroyNotify) gth_image_save_data_free);
	else
		g_task_return_error (task, error);
}


static void
gth_image_save_to_buffer_async (GthImage            *image,
				const char          *mime_type,
				GthFileData         *file_data,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data)
{
	GTask         *task;
	SaveArguments *arguments;

	g_return_if_fail (image != NULL);
	g_return_if_fail (file_data != NULL);

	arguments = g_new0 (SaveArguments, 1);
	arguments->image = g_object_ref (image);
	arguments->mime_type = g_strdup (mime_type);
	arguments->file_data = g_object_ref (file_data);

	task = g_task_new (NULL, cancellable, callback, user_data);
	g_task_set_task_data (task, arguments, (GDestroyNotify) save_arguments_free);
	g_task_run_in_thread (task, save_to_buffer_thread);

	g_object_unref (task);
}


/* -- gth_image_save_to_file -- */


typedef struct {
	GthImageSaveData *data;
	GthFileDataFunc   ready_func;
	gpointer          ready_data;
	GList            *current;
} SaveData;


static void
save_completed (SaveData *save_data)
{
	if (save_data->data->error != NULL)
		(*save_data->ready_func) (save_data->data->file_data, *save_data->data->error, save_data->ready_data);
	else
		(*save_data->ready_func) (save_data->data->file_data, NULL, save_data->ready_data);
	gth_image_save_data_free (save_data->data);
	g_free (save_data);
}


static void save_current_file (SaveData *save_data);


static void
file_saved_cb (void     **buffer,
	       gsize      count,
	       GError    *error,
	       gpointer   user_data)
{
	SaveData *save_data = user_data;

	*buffer = NULL; /* do not free the buffer, it's owned by file->buffer */

	if (error != NULL) {
		save_data->data->error = &error;
		save_completed (save_data);
		return;
	}

	save_data->current = save_data->current->next;
	save_current_file (save_data);
}


static void
save_current_file (SaveData *save_data)
{
	GthImageSaveFile *file;

	if (save_data->current == NULL) {
		save_completed (save_data);
		return;
	}

	file = save_data->current->data;
	_g_file_write_async (file->file,
			     file->buffer,
			     file->buffer_size,
			     (g_file_equal (save_data->data->file_data->file, file->file) ? save_data->data->replace : TRUE),
			     G_PRIORITY_DEFAULT,
			     save_data->data->cancellable,
			     file_saved_cb,
			     save_data);
}


static void
save_files (GthImageSaveData *data,
	    GthFileDataFunc   ready_func,
	    gpointer          ready_data)
{
	SaveData *save_data;

	save_data = g_new0 (SaveData, 1);
	save_data->data = data;
	save_data->ready_func = ready_func;
	save_data->ready_data = ready_data;

	save_data->current = save_data->data->files;
	save_current_file (save_data);
}


static void
save_to_buffer_ready_cb (GObject      *source_object,
			 GAsyncResult *result,
			 gpointer      user_data)
{
	SaveArguments    *arguments = user_data;
	GthImageSaveData *data = NULL;
	GError           *error = NULL;
	GthImageSaveFile *file;

	if (! gth_image_save_to_buffer_finish (result, &data, &error)) {
		gth_file_data_ready_with_error (arguments->file_data,
						arguments->ready_func,
						arguments->user_data,
						error);
		save_arguments_free (arguments);
		return;
	}

	data->replace = arguments->replace;

	file = g_new0 (GthImageSaveFile, 1);
	file->file = g_object_ref (data->file_data->file);
	file->buffer = data->buffer;
	file->buffer_size = data->buffer_size;
	data->files = g_list_prepend (data->files, file);

	save_files (data, arguments->ready_func, arguments->user_data);

	save_arguments_free (arguments);
}


void
gth_image_save_to_file (GthImage        *image,
			const char      *mime_type,
			GthFileData     *file_data,
			gboolean         replace,
			GCancellable    *cancellable,
			GthFileDataFunc  ready_func,
			gpointer         user_data)
{
	SaveArguments *arguments;

	g_return_if_fail (image != NULL);
	g_return_if_fail (file_data != NULL);

	arguments = g_new0 (SaveArguments, 1);
	arguments->file_data = g_object_ref (file_data);
	arguments->replace = replace;
	arguments->cancellable = _g_object_ref (cancellable);
	arguments->ready_func = ready_func;
	arguments->user_data = user_data;

	gth_image_save_to_buffer_async (image,
					mime_type,
					file_data,
					cancellable,
					save_to_buffer_ready_cb,
					arguments);
}
