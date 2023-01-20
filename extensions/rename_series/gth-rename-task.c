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
#include "gth-rename-task.h"


struct _GthRenameTaskPrivate {
	GList                 *old_files;
	GList                 *new_files;
	GList                 *current_old;
	GList                 *current_new;
	int                    n_files;
	int                    n_current;
	GFile                 *source;
	GFile                 *destination;
	GthOverwriteResponse   default_response;
};


G_DEFINE_TYPE_WITH_CODE (GthRenameTask,
			 gth_rename_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthRenameTask))


static void
gth_rename_task_finalize (GObject *object)
{
	GthRenameTask *self;

	self = GTH_RENAME_TASK (object);

	_g_object_unref (self->priv->source);
	_g_object_unref (self->priv->destination);
	_g_object_list_unref (self->priv->old_files);
	_g_object_list_unref (self->priv->new_files);

	G_OBJECT_CLASS (gth_rename_task_parent_class)->finalize (object);
}


static void _gth_rename_task_exec (GthRenameTask *self);


static void
_gth_rename_task_exec_next_file (GthRenameTask *self)
{
	self->priv->current_old = self->priv->current_old->next;
	self->priv->current_new = self->priv->current_new->next;
	self->priv->n_current++;

	if (self->priv->current_old == NULL)
		gth_task_completed (GTH_TASK (self), NULL);
	else
		_gth_rename_task_exec (self);
}


static void _gth_rename_task_try_rename (GthRenameTask  *self,
					 GFile          *source,
					 GFile          *destination,
					 GFileCopyFlags  copy_flags);


static void
overwrite_dialog_response_cb (GtkDialog *dialog,
                              gint       response_id,
                              gpointer   user_data)
{
	GthRenameTask *self = user_data;

	if (response_id != GTK_RESPONSE_OK)
		self->priv->default_response = GTH_OVERWRITE_RESPONSE_CANCEL;
	else
		self->priv->default_response = gth_overwrite_dialog_get_response (GTH_OVERWRITE_DIALOG (dialog));

	gtk_widget_hide (GTK_WIDGET (dialog));
	gth_task_dialog (GTH_TASK (self), FALSE, NULL);

	switch (self->priv->default_response) {
	case GTH_OVERWRITE_RESPONSE_NO:
	case GTH_OVERWRITE_RESPONSE_ALWAYS_NO:
	case GTH_OVERWRITE_RESPONSE_UNSPECIFIED:
		_gth_rename_task_exec_next_file (self);
		break;

	case GTH_OVERWRITE_RESPONSE_YES:
	case GTH_OVERWRITE_RESPONSE_ALWAYS_YES:
		_gth_rename_task_try_rename (self,
					     self->priv->source,
					     self->priv->destination,
					     G_FILE_COPY_OVERWRITE);
		break;

	case GTH_OVERWRITE_RESPONSE_RENAME:
		{
			GFile *parent;
			GFile *new_destination;

			parent = g_file_get_parent (self->priv->destination);
			new_destination = g_file_get_child_for_display_name (parent, gth_overwrite_dialog_get_filename (GTH_OVERWRITE_DIALOG (dialog)), NULL);
			_gth_rename_task_try_rename (self, self->priv->source, new_destination, 0);

			g_object_unref (new_destination);
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
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
_gth_rename_task_try_rename (GthRenameTask   *self,
			     GFile           *source,
			     GFile           *destination,
			     GFileCopyFlags   copy_flags)
{
	char   *source_name;
	char   *destination_name;
	char   *details;
	GError *error = NULL;

	if (g_cancellable_set_error_if_cancelled (gth_task_get_cancellable (GTH_TASK (self)), &error)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	g_object_ref (source);
	_g_object_unref (self->priv->source);
	self->priv->source = source;

	g_object_ref (destination);
	_g_object_unref (self->priv->destination);
	self->priv->destination = destination;

	source_name = g_file_get_parse_name (source);
	destination_name = g_file_get_parse_name (destination);
	details = g_strdup_printf ("Renaming '%s' as '%s'", source_name, destination_name);
	gth_task_progress (GTH_TASK (self),
			   _("Renaming files"),
			   details,
			   FALSE,
			   (double) self->priv->n_current / (self->priv->n_files + 1));

	g_free (destination_name);
	g_free (source_name);

	if (self->priv->default_response == GTH_OVERWRITE_RESPONSE_ALWAYS_YES)
		copy_flags = G_FILE_COPY_OVERWRITE;

	if (! _g_file_move (source,
			    destination,
			    G_FILE_COPY_ALL_METADATA | copy_flags,
			    gth_task_get_cancellable (GTH_TASK (self)),
			    NULL,
			    NULL,
			    &error))
	{
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
			if (self->priv->default_response != GTH_OVERWRITE_RESPONSE_ALWAYS_NO) {
				GtkWidget *dialog;

				dialog = gth_overwrite_dialog_new (source,
								   NULL,
								   destination,
								   self->priv->default_response,
								   self->priv->n_files == 1);
				g_signal_connect (dialog,
						  "response",
						  G_CALLBACK (overwrite_dialog_response_cb),
						  self);
				gtk_widget_show (dialog);

				gth_task_dialog (GTH_TASK (self), TRUE, dialog);

				return;
			}
		}
		else {
			gth_task_completed (GTH_TASK (self), error);
			return;
		}
	}
	else
		gth_monitor_file_renamed (gth_main_get_default_monitor (), source, destination);

	_gth_rename_task_exec_next_file (self);
}


static void
_gth_rename_task_exec (GthRenameTask *self)
{
	GFile     *source;
	GFile     *destination;

	if (self->priv->current_old == NULL) {
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	source = (GFile *) self->priv->current_old->data;
	destination = (GFile *) self->priv->current_new->data;

	_gth_rename_task_try_rename (self, source, destination, 0);
}


static void
gth_rename_task_exec (GthTask *task)
{
	_gth_rename_task_exec (GTH_RENAME_TASK (task));
}


static void
gth_rename_task_class_init (GthRenameTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_rename_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_rename_task_exec;
}


static void
gth_rename_task_init (GthRenameTask *self)
{
	self->priv = gth_rename_task_get_instance_private (self);
	self->priv->default_response = GTH_OVERWRITE_RESPONSE_UNSPECIFIED;
}


GthTask *
gth_rename_task_new (GList  *old_files,
		     GList  *new_files)
{
	GthRenameTask *self;

	self = GTH_RENAME_TASK (g_object_new (GTH_TYPE_RENAME_TASK, NULL));
	self->priv->old_files = _g_object_list_ref (old_files);
	self->priv->new_files = _g_object_list_ref (new_files);
	self->priv->current_old = self->priv->old_files;
	self->priv->current_new = self->priv->new_files;
	self->priv->n_files = g_list_length (self->priv->old_files);
	self->priv->n_current = 1;

	return (GthTask *) self;
}
