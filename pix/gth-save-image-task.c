/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012, 2013 Free Software Foundation, Inc.
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
#include "glib-utils.h"
#include "gth-image-saver.h"
#include "gth-save-image-task.h"


struct _GthSaveImageTaskPrivate {
	char                 *mime_type;
	GthFileData          *file_data;
	GthOverwriteResponse  overwrite_mode;
};


G_DEFINE_TYPE_WITH_CODE (GthSaveImageTask,
			 gth_save_image_task,
			 GTH_TYPE_IMAGE_TASK,
			 G_ADD_PRIVATE (GthSaveImageTask))


static void
gth_save_image_task_finalize (GObject *object)
{
	GthSaveImageTask *self;

	self = GTH_SAVE_IMAGE_TASK (object);

	g_free (self->priv->mime_type);
	_g_object_unref (self->priv->file_data);

	G_OBJECT_CLASS (gth_save_image_task_parent_class)->finalize (object);
}


static void save_image (GthSaveImageTask *self);


static void
overwrite_dialog_respnse_cb (GtkDialog *dialog,
			     int        response,
			     gpointer   user_data)
{
	GthSaveImageTask *self = user_data;

	if (response != GTK_RESPONSE_OK)
		response = GTH_OVERWRITE_RESPONSE_CANCEL;
	else
		response = gth_overwrite_dialog_get_response (GTH_OVERWRITE_DIALOG (dialog));

	if (response == GTH_OVERWRITE_RESPONSE_RENAME)
		gtk_widget_hide (GTK_WIDGET (dialog));
	else
		gtk_widget_destroy (GTK_WIDGET (dialog));
	gth_task_dialog (GTH_TASK (self), FALSE, NULL);

	switch (response) {
	case GTH_OVERWRITE_RESPONSE_YES:
		self->priv->overwrite_mode = GTH_OVERWRITE_RESPONSE_YES;
		save_image (self);
		break;

	case GTH_OVERWRITE_RESPONSE_NO:
		gth_task_completed (GTH_TASK (self), NULL);
		break;

	case GTH_OVERWRITE_RESPONSE_RENAME:
		{
			GFile *parent;
			GFile *new_file;

			parent = g_file_get_parent (self->priv->file_data->file);
			new_file = g_file_get_child_for_display_name (parent, gth_overwrite_dialog_get_filename (GTH_OVERWRITE_DIALOG (dialog)), NULL);
			gth_file_data_set_file (self->priv->file_data, new_file);
			save_image (self);

			gtk_widget_destroy (GTK_WIDGET (dialog));
			g_object_unref (new_file);
			g_object_unref (parent);
		}
		break;

	case GTH_OVERWRITE_RESPONSE_CANCEL:
		{
			GError *error;

			error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, "");
			gth_task_completed (GTH_TASK (self), error);
		}
		break;

	default:
		g_assert_not_reached ();
		break;
	}
}


static void
save_to_file_ready_cb (GthFileData *file_data,
		       GError      *error,
		       gpointer     user_data)
{
	GthSaveImageTask *self = user_data;

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
			GtkWidget *d;

			d = gth_overwrite_dialog_new (NULL,
						      gth_image_task_get_source (GTH_IMAGE_TASK (self)),
						      self->priv->file_data->file,
						      self->priv->overwrite_mode,
						      TRUE);
			g_signal_connect (d,
					  "response",
					  G_CALLBACK (overwrite_dialog_respnse_cb),
					  self);
			gtk_widget_show (d);

			gth_task_dialog (GTH_TASK (self), TRUE, d);
		}
		else
			gth_task_completed (GTH_TASK (self), error);

		return;
	}

	gth_task_completed (GTH_TASK (self), NULL);
}


static void
save_image (GthSaveImageTask *self)
{
	char *filename;
	char *description;

	filename = g_file_get_parse_name (self->priv->file_data->file);
	/* Translators: %s is a filename */
	description = g_strdup_printf (_("Saving “%s”"), filename);
	gth_task_progress (GTH_TASK (self), description, NULL, TRUE, 0.0);

	gth_image_save_to_file (gth_image_task_get_source (GTH_IMAGE_TASK (self)),
			        self->priv->mime_type,
			        self->priv->file_data,
			        ((self->priv->overwrite_mode == GTH_OVERWRITE_RESPONSE_YES)
			         || (self->priv->overwrite_mode == GTH_OVERWRITE_RESPONSE_ALWAYS_YES)),
			        gth_task_get_cancellable (GTH_TASK (self)),
			        save_to_file_ready_cb,
			        self);

	g_free (description);
	g_free (filename);
}


static void
gth_save_image_task_exec (GthTask *task)
{
	gth_image_task_set_destination (GTH_IMAGE_TASK (task), gth_image_task_get_source (GTH_IMAGE_TASK (task)));
	save_image (GTH_SAVE_IMAGE_TASK (task));
}


static void
gth_save_image_task_class_init (GthSaveImageTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_save_image_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_save_image_task_exec;
}


static void
gth_save_image_task_init (GthSaveImageTask *self)
{
	self->priv = gth_save_image_task_get_instance_private (self);
	self->priv->mime_type = NULL;
	self->priv->file_data = NULL;
}


GthTask *
gth_save_image_task_new (GthImage             *image,
			 const char           *mime_type,
			 GthFileData          *file_data,
			 GthOverwriteResponse  overwrite_mode)
{
	GthSaveImageTask *self;

	g_return_val_if_fail (file_data != NULL, NULL);

	self = (GthSaveImageTask *) g_object_new (GTH_TYPE_SAVE_IMAGE_TASK, NULL);
	self->priv->mime_type = g_strdup (mime_type);
	self->priv->file_data = gth_file_data_dup (file_data);
	self->priv->overwrite_mode = overwrite_mode;
	gth_image_task_set_source (GTH_IMAGE_TASK (self), image);

	return (GthTask *) self;
}
