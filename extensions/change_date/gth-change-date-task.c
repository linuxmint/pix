/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "gth-change-date-task.h"


struct _GthChangeDateTaskPrivate {
	GFile           *location;
	GList           *files; /* GFile */
	GList           *file_list; /* GthFileData */
	GthChangeFields  fields;
	GthChangeType    change_type;
	GthDateTime     *date_time;
	int              time_offset;
	int              n_files;
	int              n_current;
	GList           *current;
};


G_DEFINE_TYPE_WITH_CODE (GthChangeDateTask,
			 gth_change_date_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthChangeDateTask))


static void
gth_change_date_task_finalize (GObject *object)
{
	GthChangeDateTask *self;

	self = GTH_CHANGE_DATE_TASK (object);

	_g_object_unref (self->priv->location);
	_g_object_list_unref (self->priv->file_list);
	_g_object_list_unref (self->priv->files);
	gth_datetime_free (self->priv->date_time);

	G_OBJECT_CLASS (gth_change_date_task_parent_class)->finalize (object);
}


static void
set_date_time_from_change_type (GthChangeDateTask *self,
				GthDateTime       *date_time,
				GthChangeType      change_type,
				GthFileData       *file_data)
{
	if (change_type == GTH_CHANGE_TO_FOLLOWING_DATE) {
		gth_datetime_copy (self->priv->date_time, date_time);
	}
	else if (change_type == GTH_CHANGE_TO_FILE_MODIFIED_DATE) {
		gth_datetime_from_timeval (date_time, gth_file_data_get_modification_time (file_data));
	}
	else if (change_type == GTH_CHANGE_TO_FILE_CREATION_DATE) {
		gth_datetime_from_timeval (date_time, gth_file_data_get_creation_time (file_data));
	}
	else if (change_type == GTH_CHANGE_TO_PHOTO_ORIGINAL_DATE) {
		GTimeVal time_val;

		if (gth_file_data_get_digitalization_time (file_data, &time_val))
			gth_datetime_from_timeval (date_time, &time_val);
	}
}


static void
update_modification_time (GthChangeDateTask *self)
{

	GList      *scan;
	GError     *error = NULL;
	GthMonitor *monitor;
	GList      *files;

	if ((self->priv->fields & GTH_CHANGE_LAST_MODIFIED_DATE) == GTH_CHANGE_LAST_MODIFIED_DATE) {
		GthDateTime *date_time;

		date_time = gth_datetime_new ();
		for (scan = self->priv->file_list; scan; scan = scan->next) {
			GthFileData *file_data = scan->data;
			GTimeVal     timeval;

			gth_datetime_clear (date_time);
			if (self->priv->change_type == GTH_CHANGE_ADJUST_TIME)
				set_date_time_from_change_type (self, date_time, GTH_CHANGE_TO_FILE_MODIFIED_DATE, file_data);
			else
				set_date_time_from_change_type (self, date_time, self->priv->change_type, file_data);

			/* Read the time from the attribute if it's not valid in
			 * date_time. */

			if (! gth_time_valid (date_time->time)) {
				GTimeVal    *original_modification_time;
				GthDateTime *original_date_time;

				original_modification_time = gth_file_data_get_modification_time (file_data);
				original_date_time = gth_datetime_new ();
				gth_datetime_from_timeval (original_date_time, original_modification_time);
				*date_time->time = *original_date_time->time;

				gth_datetime_free (original_date_time);
			}

			if (! gth_datetime_valid (date_time))
				continue;

			if (gth_datetime_to_timeval (date_time, &timeval)) {
				if (self->priv->change_type == GTH_CHANGE_ADJUST_TIME)
					timeval.tv_sec += self->priv->time_offset;
				if (! _g_file_set_modification_time (file_data->file,
								     &timeval,
								     gth_task_get_cancellable (GTH_TASK (self)),
								     &error))
				{
					break;
				}
			}
		}

		gth_datetime_free (date_time);
	}

	monitor = gth_main_get_default_monitor ();
	files = gth_file_data_list_to_file_list (self->priv->file_list);
	gth_monitor_folder_changed (monitor,
				    self->priv->location,
				    files,
				    GTH_MONITOR_EVENT_CHANGED);

	gth_task_completed (GTH_TASK (self), error);

	_g_object_list_unref (files);
}


static void
write_metadata_ready_cb (GObject      *source_object,
                	 GAsyncResult *result,
                	 gpointer      user_data)
{
	GthChangeDateTask *self = user_data;
	GError            *error = NULL;

	if (! _g_write_metadata_finish (result, &error)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	if (g_cancellable_set_error_if_cancelled (gth_task_get_cancellable (GTH_TASK (self)), &error)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	update_modification_time (self);
}


static void
set_date_metadata (GthFileData *file_data,
		   const char  *attribute,
		   GthDateTime *date_time,
		   int          time_offset)
{
	GthDateTime *new_date_time;

	new_date_time = gth_datetime_new ();
	gth_datetime_copy (date_time, new_date_time);

	if (time_offset != 0) {
		GTimeVal timeval;

		gth_datetime_to_timeval (new_date_time, &timeval);
		timeval.tv_sec += time_offset;
		gth_datetime_from_timeval (new_date_time, &timeval);
	}
	else {
		/* Read the time from the attribute if it's not valid in
		 * date_time. */

		if (! gth_time_valid (new_date_time->time)) {
			GthMetadata *original_date;

			original_date = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, attribute);
			if (original_date != NULL) {
				GthDateTime *original_date_time;

				original_date_time = gth_datetime_new ();
				gth_datetime_from_exif_date (original_date_time, gth_metadata_get_raw (original_date));
				*new_date_time->time = *original_date_time->time;

				gth_datetime_free (original_date_time);
			}
		}
	}

	if (gth_datetime_valid (new_date_time)) {
		char    *raw;
		char    *formatted;
		GObject *metadata;

		raw = gth_datetime_to_exif_date (new_date_time);
		formatted = gth_datetime_strftime (new_date_time, "%x");
		metadata = (GObject *) gth_metadata_new ();
		g_object_set (metadata,
			      "id", attribute,
			      "raw", raw,
			      "formatted", formatted,
			      NULL);
		g_file_info_set_attribute_object (file_data->info, attribute, metadata);

		g_object_unref (metadata);
		g_free (formatted);
		g_free (raw);
	}

	gth_datetime_free (new_date_time);
}


static void
set_date_time_from_field (GthChangeDateTask *self,
			  GthDateTime       *date_time,
			  GthChangeFields    field,
			  GthFileData       *file_data)
{
	if (field & GTH_CHANGE_LAST_MODIFIED_DATE) {
		gth_datetime_from_timeval (date_time, gth_file_data_get_modification_time (file_data));
	}
	else if (field & GTH_CHANGE_COMMENT_DATE) {
		GthMetadata *m;
		GTimeVal     time_val;

		m = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::datetime");
		if ((m != NULL) && _g_time_val_from_exif_date (gth_metadata_get_raw (m), &time_val))
			gth_datetime_from_timeval (date_time, &time_val);
	}
}


static void
info_ready_cb (GList    *files,
	       GError   *error,
	       gpointer  user_data)
{
	GthChangeDateTask *self = user_data;
	GthDateTime       *date_time;
	GList             *scan;
	GPtrArray         *attribute_v;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	if (g_cancellable_set_error_if_cancelled (gth_task_get_cancellable (GTH_TASK (self)), &error)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	date_time = gth_datetime_new ();
	self->priv->file_list = _g_object_list_ref (files);
	for (scan = self->priv->file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (self->priv->change_type == GTH_CHANGE_ADJUST_TIME) {
			if (self->priv->fields & GTH_CHANGE_COMMENT_DATE) {
				gth_datetime_clear (date_time);
				set_date_time_from_field (self, date_time, GTH_CHANGE_COMMENT_DATE, file_data);
				if (gth_datetime_valid (date_time))
					set_date_metadata (file_data, "general::datetime", date_time, self->priv->time_offset);
			}
		}
		else {
			gth_datetime_clear (date_time);
			set_date_time_from_change_type (self, date_time, self->priv->change_type, file_data);
			if (g_date_valid (date_time->date)) {
				if (self->priv->fields & GTH_CHANGE_COMMENT_DATE) {
					set_date_metadata (file_data, "general::datetime", date_time, 0);
				}
			}
		}
	}

	attribute_v = g_ptr_array_new ();
	if (self->priv->fields & GTH_CHANGE_COMMENT_DATE)
		g_ptr_array_add (attribute_v, "general::datetime");
	if (attribute_v->len > 0) {
		char *attributes;

		attributes = _g_string_array_join (attribute_v, ",");
		_g_write_metadata_async (self->priv->file_list,
					 GTH_METADATA_WRITE_DEFAULT,
					 attributes,
					 gth_task_get_cancellable (GTH_TASK (self)),
					 write_metadata_ready_cb,
					 self);

		g_free (attributes);
	}
	else
		update_modification_time (self);

	g_ptr_array_unref (attribute_v);
	gth_datetime_free (date_time);
}


static void
gth_change_date_task_exec (GthTask *task)
{
	GthChangeDateTask *self = GTH_CHANGE_DATE_TASK (task);

	_g_query_all_metadata_async (self->priv->files,
				     GTH_LIST_DEFAULT,
				     "*",
				     gth_task_get_cancellable (task),
				     info_ready_cb,
				     self);
}


static void
gth_change_date_task_class_init (GthChangeDateTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_change_date_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_change_date_task_exec;
}


static void
gth_change_date_task_init (GthChangeDateTask *self)
{
	self->priv = gth_change_date_task_get_instance_private (self);
	self->priv->date_time = gth_datetime_new ();
}


GthTask *
gth_change_date_task_new (GFile             *location,
			  GList             *files, /* GthFileData */
			  GthChangeFields    fields,
			  GthChangeType      change_type,
			  GthDateTime       *date_time,
			  int                time_offset)
{
	GthChangeDateTask *self;

	self = GTH_CHANGE_DATE_TASK (g_object_new (GTH_TYPE_CHANGE_DATE_TASK, NULL));
	self->priv->location = g_file_dup (location);
	self->priv->files = gth_file_data_list_to_file_list (files);
	self->priv->fields = fields;
	self->priv->change_type = change_type;
	if (date_time != NULL)
		gth_datetime_copy (date_time, self->priv->date_time);
	self->priv->time_offset = time_offset;

	return (GthTask *) self;
}
