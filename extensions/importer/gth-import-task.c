/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009-2010 Free Software Foundation, Inc.
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
#include <extensions/catalogs/gth-catalog.h>
#ifdef HAVE_EXIV2
#include <extensions/exiv2_tools/exiv2-utils.h>
#endif
#include <extensions/image_rotation/rotation-utils.h>
#include "gth-import-task.h"
#include "preferences.h"
#include "utils.h"


#define IMPORTED_KEY "imported"


struct _GthImportTaskPrivate {
	GthBrowser          *browser;
	GList               *files;
	GFile               *destination;
	GHashTable          *destinations;
	char                *subfolder_template;;
	char                *event_name;
	char               **tags;
	GTimeVal             import_start_time;
	gboolean             delete_imported;
	gboolean             overwrite_files;
	gboolean             adjust_orientation;

	GHashTable          *catalogs;
	gsize                tot_size;
	gsize                copied_size;
	gsize                current_file_size;
	GList               *current;
	GthFileData         *destination_file;
	GFile               *imported_catalog;
	gboolean             delete_not_supported;
	int                  n_imported;
	GthOverwriteResponse default_response;
	void                *buffer;
	gsize                buffer_size;
};


G_DEFINE_TYPE_WITH_CODE (GthImportTask,
			 gth_import_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthImportTask))


static void
gth_import_task_finalize (GObject *object)
{
	GthImportTask *self;

	self = GTH_IMPORT_TASK (object);

	if (gth_browser_get_close_with_task (self->priv->browser))
		gtk_window_present (GTK_WINDOW (self->priv->browser));

	g_free (self->priv->buffer);
	g_hash_table_unref (self->priv->destinations);
	_g_object_list_unref (self->priv->files);
	g_object_unref (self->priv->destination);
	_g_object_unref (self->priv->destination_file);
	g_free (self->priv->subfolder_template);
	g_free (self->priv->event_name);
	if (self->priv->tags != NULL)
		g_strfreev (self->priv->tags);
	g_hash_table_destroy (self->priv->catalogs);
	_g_object_unref (self->priv->imported_catalog);
	g_object_unref (self->priv->browser);

	G_OBJECT_CLASS (gth_import_task_parent_class)->finalize (object);
}


static void import_current_file (GthImportTask *self);


static void
import_next_file (GthImportTask *self)
{
	self->priv->copied_size += self->priv->current_file_size;
	self->priv->current = self->priv->current->next;
	import_current_file (self);
}


static void
save_catalog (gpointer key,
	      gpointer value,
	      gpointer user_data)
{
	GthCatalog *catalog = value;

	gth_catalog_save (catalog);
}


static void
save_catalogs (GthImportTask *self)
{
	g_hash_table_foreach (self->priv->catalogs, save_catalog, self);
}


static void
catalog_imported_file (GthImportTask *self)
{
	char       *key;
	GObject    *metadata;
	GTimeVal    timeval;
	GthCatalog *catalog;

	self->priv->n_imported++;

	if (! gth_main_extension_is_active ("catalogs")) {
		import_next_file (self);
		return;
	}

	key = NULL;
	metadata = g_file_info_get_attribute_object (self->priv->destination_file->info, "Embedded::Photo::DateTimeOriginal");
	if (metadata != NULL) {
		if (_g_time_val_from_exif_date (gth_metadata_get_raw (GTH_METADATA (metadata)), &timeval))
			key = _g_time_val_strftime (&timeval, "%Y.%m.%d");
	}

	if (key == NULL) {
		g_get_current_time (&timeval);
		key = _g_time_val_strftime (&timeval, "%Y.%m.%d");
	}

	catalog = g_hash_table_lookup (self->priv->catalogs, key);
	if (catalog == NULL) {
		GthDateTime *date_time;
		GFile       *catalog_file;

		date_time = gth_datetime_new ();
		gth_datetime_from_timeval (date_time, &timeval);

		catalog_file = gth_catalog_get_file_for_date (date_time, ".catalog");
		catalog = gth_catalog_load_from_file (catalog_file);
		if (catalog == NULL)
			catalog = gth_catalog_new ();
		gth_catalog_set_date (catalog, date_time);
		gth_catalog_set_file (catalog, catalog_file);

		g_hash_table_insert (self->priv->catalogs, g_strdup (key), catalog);

		g_object_unref (catalog_file);
		gth_datetime_free (date_time);
	}
	gth_catalog_insert_file (catalog, self->priv->destination_file->file, -1);

	catalog = g_hash_table_lookup (self->priv->catalogs, IMPORTED_KEY);
	if (catalog != NULL)
		gth_catalog_insert_file (catalog, self->priv->destination_file->file, -1);

	import_next_file (self);

	g_free (key);
}


static void
write_metadata_ready_func (GObject      *source_object,
	 	 	   GAsyncResult *result,
	 	 	   gpointer      user_data)
{
	GthImportTask *self = user_data;
	GError        *error = NULL;

	if (! _g_write_metadata_finish (result, &error)
	    && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
	{
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	if (error != NULL)
		g_clear_error (&error);

	catalog_imported_file (self);
}


static void
write_file_tags (GthImportTask *self)
{
	GthStringList *tag_list;
	GthMetadata   *metadata;
	GList         *file_list;

	if ((self->priv->tags == NULL) || (self->priv->tags[0] == NULL)) {
		catalog_imported_file (self);
		return;
	}

	tag_list = gth_string_list_new_from_strv (self->priv->tags);
	metadata = gth_metadata_new_for_string_list (tag_list);
	g_file_info_set_attribute_object (self->priv->destination_file->info, "comment::categories", G_OBJECT (metadata));
	file_list = g_list_prepend (NULL, self->priv->destination_file);
	_g_write_metadata_async (file_list,
				 GTH_METADATA_WRITE_DEFAULT,
				 "comment::categories",
				 gth_task_get_cancellable (GTH_TASK (self)),
				 write_metadata_ready_func,
				 self);

	g_list_free (file_list);
	g_object_unref (metadata);
	g_object_unref (tag_list);
}


static void
write_file_to_destination (GthImportTask *self,
			   GFile         *destination_file,
			   void          *buffer,
			   gsize          buffer_size,
			   gboolean       overwrite);


static void
overwrite_dialog_response_cb (GtkDialog *dialog,
                              gint       response_id,
                              gpointer   user_data)
{
	GthImportTask *self = user_data;

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
		import_next_file (self);
		break;

	case GTH_OVERWRITE_RESPONSE_YES:
	case GTH_OVERWRITE_RESPONSE_ALWAYS_YES:
		write_file_to_destination (self,
					   self->priv->destination_file->file,
					   self->priv->buffer,
					   self->priv->buffer_size,
					   TRUE);
		break;

	case GTH_OVERWRITE_RESPONSE_RENAME:
		{
			GthFileData *file_data;
			GFile       *destination_folder;
			GFile       *new_destination;

			file_data = self->priv->current->data;
			destination_folder = gth_import_utils_get_file_destination (file_data,
										    self->priv->destination,
										    self->priv->subfolder_template,
										    self->priv->event_name,
										    self->priv->import_start_time);
			new_destination = g_file_get_child_for_display_name (destination_folder, gth_overwrite_dialog_get_filename (GTH_OVERWRITE_DIALOG (dialog)), NULL);
			write_file_to_destination (self,
						   new_destination,
						   self->priv->buffer,
						   self->priv->buffer_size,
						   FALSE);

			g_object_unref (new_destination);
			g_object_unref (destination_folder);
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
after_saving_to_destination (GthImportTask  *self,
			     void          **buffer,
			     gsize           count,
			     GError         *error)
{
	GthFileData *file_data;

	file_data = self->priv->current->data;

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
			if (self->priv->default_response != GTH_OVERWRITE_RESPONSE_ALWAYS_NO) {
				GInputStream *stream;
				GthImage     *image;
				GtkWidget    *dialog;

				/* take ownership of the buffer */

				if (buffer != NULL) {
					self->priv->buffer = *buffer;
					self->priv->buffer_size = count;
					*buffer = NULL;
				}
				else {
					self->priv->buffer = NULL;
					self->priv->buffer_size = 0;
				}

				/* show the overwrite dialog */

				if (self->priv->buffer != NULL) {
					stream = g_memory_input_stream_new_from_data (self->priv->buffer, self->priv->buffer_size, NULL);
					image = gth_image_new_from_stream (stream, 128, NULL, NULL, NULL, NULL);
				}
				else {
					stream = NULL;
					image = NULL;
				}

				dialog = gth_overwrite_dialog_new (file_data->file,
								   image,
								   self->priv->destination_file->file,
								   self->priv->default_response,
								   self->priv->files->next == NULL);
				g_signal_connect (dialog,
						  "response",
						  G_CALLBACK (overwrite_dialog_response_cb),
						  self);
				gtk_widget_show (dialog);
				gth_task_dialog (GTH_TASK (self), TRUE, dialog);

				_g_object_unref (image);
				_g_object_unref (stream);
			}
			else
				import_next_file (self);

			return;
		}

		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	if (self->priv->delete_imported) {
		GError *local_error = NULL;

		if (! g_file_delete (file_data->file,
				     gth_task_get_cancellable (GTH_TASK (self)),
				     &local_error))
		{
			if (g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED)) {
				self->priv->delete_imported = FALSE;
				self->priv->delete_not_supported = TRUE;
				local_error = NULL;
			}
			if (local_error != NULL) {
				gth_task_completed (GTH_TASK (self), local_error);
				return;
			}
		}
	}

	write_file_tags (self);
}


static void
copy_non_image_ready_cb (GObject      *source_object,
			 GAsyncResult *res,
			 gpointer      user_data)
{
	GError *error = NULL;

	g_file_copy_finish (G_FILE (source_object), res, &error);
	after_saving_to_destination (GTH_IMPORT_TASK (user_data), NULL, 0, error);
}


static void
copy_non_image_progress_cb (goffset  current_num_bytes,
			    goffset  total_num_bytes,
			    gpointer user_data)
{
	GthImportTask *self = user_data;
	GthFileData   *file_data;

	file_data = self->priv->current->data;

	gth_task_progress (GTH_TASK (self),
			   _("Importing files"),
			   g_file_info_get_display_name (file_data->info),
			   FALSE,
			   CLAMP ((double) (self->priv->copied_size + current_num_bytes) / self->priv->tot_size, 0.0, 1.0));
}


static void
write_buffer_ready_cb (void     **buffer,
		       gsize      count,
		       GError    *error,
		       gpointer   user_data)
{
	after_saving_to_destination (GTH_IMPORT_TASK (user_data), buffer, count, error);
}


static void
write_file_to_destination (GthImportTask *self,
			   GFile         *destination_file,
			   void          *buffer,
			   gsize          buffer_size,
			   gboolean       replace)
{
	GthFileData *file_data;

	file_data = self->priv->current->data;

	if ((self->priv->destination_file == NULL) || (destination_file != self->priv->destination_file->file)) {
		_g_object_unref (self->priv->destination_file);
		self->priv->destination_file = gth_file_data_new (destination_file, file_data->info);
	}

	if (buffer != NULL) {
		gth_task_progress (GTH_TASK (self),
				   _("Importing files"),
				   g_file_info_get_display_name (file_data->info),
				   FALSE,
				   (double) (self->priv->copied_size + ((double) self->priv->current_file_size / 3.0 * 2.0)) / self->priv->tot_size);

		self->priv->buffer = NULL; /* the buffer will be deallocated in _g_file_write_async */

#ifdef HAVE_LIBJPEG
		if (self->priv->adjust_orientation && gth_main_extension_is_active ("image_rotation")) {
			if (g_content_type_equals (gth_file_data_get_mime_type (self->priv->destination_file), "image/jpeg")) {
				GthTransform  orientation;
				GthMetadata  *metadata;

				orientation = GTH_TRANSFORM_NONE;
				metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->destination_file->info, "Embedded::Image::Orientation");
				if ((metadata != NULL) && (gth_metadata_get_raw (metadata) != NULL))
					orientation = strtol (gth_metadata_get_raw (metadata), (char **) NULL, 10);

				if (orientation != GTH_TRANSFORM_NONE) {
					void  *out_buffer;
					gsize  out_buffer_size;

					if (jpegtran (buffer,
						      buffer_size,
						      &out_buffer,
						      &out_buffer_size,
						      orientation,
						      JPEG_MCU_ACTION_ABORT,
						      NULL))
					{
						g_free (buffer);
						buffer = out_buffer;
						buffer_size = out_buffer_size;
					}
				}
			}
		}
#endif /* HAVE_LIBJPEG */

		_g_file_write_async (self->priv->destination_file->file,
				     buffer,
				     buffer_size,
				     replace,
				     G_PRIORITY_DEFAULT,
				     gth_task_get_cancellable (GTH_TASK (self)),
				     write_buffer_ready_cb,
				     self);
	}
	else
		g_file_copy_async (file_data->file,
				   self->priv->destination_file->file,
				   (replace ? G_FILE_COPY_OVERWRITE : G_FILE_COPY_NONE) | G_FILE_COPY_TARGET_DEFAULT_PERMS,
				   G_PRIORITY_DEFAULT,
				   gth_task_get_cancellable (GTH_TASK (self)),
				   copy_non_image_progress_cb,
				   self,
				   copy_non_image_ready_cb,
				   self);
}


static GFile *
get_destination_file (GthImportTask *self,
		      GthFileData   *file_data)
{
	GError *error = NULL;
	GFile  *destination;
	GFile  *destination_file;

	destination = gth_import_utils_get_file_destination (file_data,
							     self->priv->destination,
							     self->priv->subfolder_template,
							     self->priv->event_name,
							     self->priv->import_start_time);
	if (! g_file_make_directory_with_parents (destination, gth_task_get_cancellable (GTH_TASK (self)), &error)) {
		if (! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
			gth_task_completed (GTH_TASK (self), error);
			return NULL;
		}
	}

	/* Get the destination file avoiding to overwrite an already imported
	 * file. */

	destination_file = _g_file_get_destination (file_data->file, NULL, destination);
	while (g_hash_table_lookup (self->priv->destinations, destination_file) != NULL) {
		GFile *tmp = destination_file;
		destination_file = _g_file_get_duplicated (tmp);
		g_object_unref (tmp);
	}
	g_hash_table_insert (self->priv->destinations, g_object_ref (destination_file), GINT_TO_POINTER (1));

	g_object_unref (destination);

	return destination_file;
}


static void
file_buffer_ready_cb (void     **buffer,
		      gsize      count,
		      GError    *error,
		      gpointer   user_data)
{
	GthImportTask *self = user_data;
	GthFileData   *file_data;
	GFile         *destination_file;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	file_data = self->priv->current->data;

#ifdef HAVE_EXIV2
	if (gth_main_extension_is_active ("exiv2_tools"))
		exiv2_read_metadata_from_buffer (*buffer,
						 count,
						 file_data->info,
						 TRUE,
						 NULL);
#endif

	destination_file = get_destination_file (self, file_data);
	if (destination_file == NULL)
		return;

	write_file_to_destination (self,
				   destination_file,
				   *buffer,
				   count,
				   self->priv->default_response == GTH_OVERWRITE_RESPONSE_ALWAYS_YES);
	*buffer = NULL; /* _g_file_write_async takes ownership of the buffer */

	g_object_unref (destination_file);
}


static void
import_current_file (GthImportTask *self)
{
	GthFileData *file_data;
	gboolean     adjust_image_orientation;
	gboolean     need_image_metadata;

	g_free (self->priv->buffer);
	self->priv->buffer = NULL;

	if (self->priv->current == NULL) {
		save_catalogs (self);

		if (self->priv->n_imported == 0) {
			GtkWidget *d;

			d =  _gtk_message_dialog_new (GTK_WINDOW (self->priv->browser),
						      0,
						      _GTK_ICON_NAME_DIALOG_WARNING,
						      _("No file imported"),
						      _("The selected files are already present in the destination."),
						      _GTK_LABEL_CLOSE, GTK_RESPONSE_CANCEL,
						      NULL);
			g_signal_connect (G_OBJECT (d), "response",
					  G_CALLBACK (gtk_widget_destroy),
					  NULL);
			gtk_widget_show (d);
		}
		else {
			GSettings *settings;

			if (! _g_str_empty (self->priv->subfolder_template) && (self->priv->imported_catalog != NULL))
				gth_browser_go_to (self->priv->browser, self->priv->imported_catalog, NULL);
			else
				gth_browser_go_to (self->priv->browser, self->priv->destination, NULL);

			settings = g_settings_new (GTHUMB_IMPORTER_SCHEMA);
			if (self->priv->delete_not_supported && g_settings_get_boolean (settings, PREF_IMPORTER_WARN_DELETE_UNSUPPORTED)) {
				GtkWidget *d;

				d =  _gtk_message_dialog_new (GTK_WINDOW (self->priv->browser),
							      0,
							      _GTK_ICON_NAME_DIALOG_WARNING,
							      _("Could not delete the files"),
							      _("Delete operation not supported."),
							      _GTK_LABEL_CLOSE, GTK_RESPONSE_CANCEL,
							      NULL);
				g_signal_connect (G_OBJECT (d), "response",
						  G_CALLBACK (gtk_widget_destroy),
						  NULL);
				gtk_widget_show (d);

				g_settings_set_boolean (settings, PREF_IMPORTER_WARN_DELETE_UNSUPPORTED, FALSE);
			}

			g_object_unref (settings);
		}

		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	file_data = self->priv->current->data;
	self->priv->current_file_size = g_file_info_get_size (file_data->info);

	adjust_image_orientation = self->priv->adjust_orientation && gth_main_extension_is_active ("image_rotation");
	need_image_metadata = (_g_utf8_find_str (self->priv->subfolder_template, "%D") != NULL) || adjust_image_orientation;

	if (_g_mime_type_is_image (gth_file_data_get_mime_type (file_data)) && need_image_metadata) {
		gth_task_progress (GTH_TASK (self),
				   _("Importing files"),
				   g_file_info_get_display_name (file_data->info),
				   FALSE,
				   (double) (self->priv->copied_size + ((double) self->priv->current_file_size / 3.0)) / self->priv->tot_size);

		_g_file_load_async (file_data->file,
				    G_PRIORITY_DEFAULT,
				    gth_task_get_cancellable (GTH_TASK (self)),
				    file_buffer_ready_cb,
				    self);
	}
	else {
		GFile *destination_file;

		destination_file = get_destination_file (self, file_data);
		if (destination_file != NULL) {
			write_file_to_destination (self,
						   destination_file,
						   NULL,
						   0,
						   self->priv->default_response == GTH_OVERWRITE_RESPONSE_ALWAYS_YES);
			g_object_unref (destination_file);
		}
	}
}


static void
gth_import_task_exec (GthTask *base)
{
	GthImportTask *self = (GthImportTask *) base;
	GTimeVal       timeval;
	GList         *scan;

	self->priv->n_imported = 0;
	self->priv->tot_size = 0;
	for (scan = self->priv->files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		self->priv->tot_size += g_file_info_get_size (file_data->info);
	}
	g_get_current_time (&timeval);
	self->priv->import_start_time = timeval;
	self->priv->default_response = GTH_OVERWRITE_RESPONSE_UNSPECIFIED;

	/* create the imported files catalog */

	if (gth_main_extension_is_active ("catalogs")) {
		GthDateTime *date_time;
		char        *display_name;
		GthCatalog  *catalog = NULL;

		date_time = gth_datetime_new ();
		gth_datetime_from_timeval (date_time, &timeval);

		if ((self->priv->event_name != NULL) && ! _g_utf8_all_spaces (self->priv->event_name)) {
			display_name = g_strdup (self->priv->event_name);
			self->priv->imported_catalog = _g_file_new_for_display_name ("catalog://", display_name, ".catalog");
			/* append files to the catalog if an event name was given */
			catalog = gth_catalog_load_from_file (self->priv->imported_catalog);
		}
		else {
			display_name = g_strdup (_("Last imported"));
			self->priv->imported_catalog = _g_file_new_for_display_name ("catalog://", display_name, ".catalog");
			/* overwrite the catalog content if the generic "last imported" catalog is used. */
			catalog = NULL;
		}

		if (catalog == NULL)
			catalog = gth_catalog_new ();
		gth_catalog_set_file (catalog, self->priv->imported_catalog);
		gth_catalog_set_date (catalog, date_time);
		gth_catalog_set_name (catalog, display_name);

		g_hash_table_insert (self->priv->catalogs, g_strdup (IMPORTED_KEY), catalog);

		g_free (display_name);
		gth_datetime_free (date_time);
	}

	self->priv->buffer = NULL;
	self->priv->current = self->priv->files;
	import_current_file (self);
}


static void
gth_import_task_class_init (GthImportTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_import_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_import_task_exec;
}


static void
gth_import_task_init (GthImportTask *self)
{
	self->priv = gth_import_task_get_instance_private (self);
	self->priv->catalogs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
	self->priv->delete_not_supported = FALSE;
	self->priv->destinations = g_hash_table_new_full (g_file_hash,
							  (GEqualFunc) g_file_equal,
							  g_object_unref,
							  NULL);
	self->priv->buffer = NULL;
}


GthTask *
gth_import_task_new (GthBrowser         *browser,
		     GList              *files,
		     GFile              *destination,
		     const char         *subfolder_template,
		     const char         *event_name,
		     char              **tags,
		     gboolean            delete_imported,
		     gboolean            overwrite_files,
		     gboolean            adjust_orientation)
{
	GthImportTask *self;

	self = GTH_IMPORT_TASK (g_object_new (GTH_TYPE_IMPORT_TASK, NULL));
	self->priv->browser = g_object_ref (browser);
	self->priv->files = _g_object_list_ref (files);
	self->priv->destination = g_file_dup (destination);
	self->priv->subfolder_template = g_strdup (subfolder_template);
	self->priv->event_name = g_strdup (event_name);
	self->priv->tags = g_strdupv (tags);
	self->priv->delete_imported = delete_imported;
	self->priv->overwrite_files = overwrite_files;
	self->priv->default_response = overwrite_files ? GTH_OVERWRITE_RESPONSE_ALWAYS_YES : GTH_OVERWRITE_RESPONSE_UNSPECIFIED;
	self->priv->adjust_orientation = adjust_orientation;

	return (GthTask *) self;
}


gboolean
gth_import_task_check_free_space (GFile   *destination,
				  GList   *files, /* GthFileData list */
				  GError **error)
{
	GFileInfo *info;
	guint64    free_space;
	goffset    total_file_size;
	goffset    max_file_size;
	goffset    min_free_space;
	GList     *scan;

	if (files == NULL) {
		if (error != NULL)
			*error = g_error_new (G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "%s", _("No file specified."));
		return FALSE;
	}

	info = g_file_query_filesystem_info (destination, G_FILE_ATTRIBUTE_FILESYSTEM_FREE, NULL, error);
	if (info == NULL)
		return FALSE;

	free_space = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);

	total_file_size = 0;
	max_file_size = 0;
	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		goffset      file_size = g_file_info_get_size (file_data->info);

		total_file_size += file_size;
		if (file_size > max_file_size)
			max_file_size = file_size;
	}

	min_free_space = total_file_size +
			 max_file_size +               /* image rotation can require a temporary file */
			 (total_file_size * 5 / 100);  /* 5% of FS fragmentation */

	if ((free_space < min_free_space) && (error != NULL)) {
		char *destination_name;
		char *min_free_space_s;
		char *free_space_s;

		destination_name = g_file_get_parse_name (destination);
		min_free_space_s = g_format_size (min_free_space);
		free_space_s = g_format_size (free_space);

		*error = g_error_new (G_IO_ERROR,
				      G_IO_ERROR_NO_SPACE,
				      /* Translators: For example: Not enough free space in “/home/user/Images”.\n1.3 GB of space is required but only 300 MB is available. */
				      _("Not enough free space in “%s”.\n%s of space is required but only %s is available."),
				      destination_name,
				      min_free_space_s,
				      free_space_s);

		g_free (free_space_s);
		g_free (min_free_space_s);
		g_free (destination_name);
	}

	return free_space >= min_free_space;
}
