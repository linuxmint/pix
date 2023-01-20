/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include <stdlib.h>
#include "glib-utils.h"
#include "gth-duplicable.h"
#include "gth-metadata.h"
#include "gth-file-data.h"
#include "gth-string-list.h"


const char *FileDataDigitalizationTags[] = {
	"Exif::Photo::DateTimeOriginal",
	"Xmp::exif::DateTimeOriginal",
	"Exif::Photo::DateTimeDigitized",
	"Xmp::exif::DateTimeDigitized",
	"Xmp::xmp::CreateDate",
	"Xmp::photoshop::DateCreated",
	"Xmp::xmp::ModifyDate",
	"Xmp::xmp::MetadataDate",
	NULL
};


struct _GthFileDataPrivate {
	GTimeVal  ctime;   /* creation time */
	GTimeVal  mtime;   /* modification time */
	GTimeVal  dtime;   /* digitalization time */
	char     *sort_key;
};


static void gth_file_data_gth_duplicable_interface_init (GthDuplicableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthFileData,
			 gth_file_data,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthFileData)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_DUPLICABLE,
						gth_file_data_gth_duplicable_interface_init))


static void
gth_file_data_finalize (GObject *obj)
{
	GthFileData *self = GTH_FILE_DATA (obj);

	_g_object_unref (self->file);
	_g_object_unref (self->info);
	g_free (self->priv->sort_key);

	G_OBJECT_CLASS (gth_file_data_parent_class)->finalize (obj);
}


static void
gth_file_data_class_init (GthFileDataClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_file_data_finalize;
}


static GObject *
gth_file_data_real_duplicate (GthDuplicable *base)
{
	return (GObject *) gth_file_data_dup ((GthFileData*) base);
}


static void
gth_file_data_gth_duplicable_interface_init (GthDuplicableInterface *iface)
{
	iface->duplicate = gth_file_data_real_duplicate;
}


static void
gth_file_data_init (GthFileData *self)
{
	self->priv = gth_file_data_get_instance_private (self);
	self->priv->dtime.tv_sec = 0;
	self->priv->sort_key = NULL;
	self->file = NULL;
	self->info = NULL;
}


GthFileData *
gth_file_data_new (GFile     *file,
		   GFileInfo *info)
{
	GthFileData *self;

	self = g_object_new (GTH_TYPE_FILE_DATA, NULL);
	gth_file_data_set_file (self, file);
	gth_file_data_set_info (self, info);

	return self;
}


GthFileData *
gth_file_data_new_for_uri (const char *uri,
			   const char *mime_type)
{
	GFile       *file;
	GthFileData *file_data;

	file = g_file_new_for_uri (uri);
	file_data = gth_file_data_new (file, NULL);
	gth_file_data_set_mime_type (file_data, mime_type);

	g_object_unref (file);

	return file_data;
}


GthFileData *
gth_file_data_dup (GthFileData *self)
{
	GthFileData *file;

	if (self == NULL)
		return NULL;

	file = g_object_new (GTH_TYPE_FILE_DATA, NULL);
	file->file = g_file_dup (self->file);
	file->info = g_file_info_dup (self->info);

	return file;
}


void
gth_file_data_set_file (GthFileData *self,
			GFile       *file)
{
	if (file != NULL)
		g_object_ref (file);

	if (self->file != NULL) {
		g_object_unref (self->file);
		self->file = NULL;
	}

	if (file != NULL)
		self->file = file;
}


void
gth_file_data_set_info (GthFileData *self,
			GFileInfo   *info)
{
	if (info != NULL)
		g_object_ref (info);

	if (self->info != NULL)
		g_object_unref (self->info);

	if (info != NULL)
		self->info = info;
	else
		self->info = g_file_info_new ();

	self->priv->dtime.tv_sec = 0;
}


void
gth_file_data_set_mime_type (GthFileData *self,
			     const char  *mime_type)
{
	if (mime_type != NULL) {
		g_file_info_set_content_type (self->info, _g_str_get_static (mime_type));
		g_file_info_set_attribute_string (self->info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, _g_str_get_static (mime_type));
		g_file_info_set_attribute_string (self->info, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE, _g_str_get_static (mime_type));
	}
}


const char *
gth_file_data_get_mime_type (GthFileData *self)
{
	const char *content_type;

	if (self->info == NULL)
		return NULL;

	content_type = g_file_info_get_attribute_string (self->info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
	if (content_type == NULL)
		content_type = g_file_info_get_attribute_string (self->info, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);

	if (content_type == NULL) {
		char *filename;

		if (self->file == NULL)
			return NULL;

		filename = g_file_get_basename (self->file);
		if (filename == NULL)
			return NULL;

		content_type = _g_content_type_guess_from_name (filename);
		g_file_info_set_attribute_string (self->info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, content_type);

		g_free (filename);
	}

	return _g_str_get_static (content_type);
}


const char *
gth_file_data_get_mime_type_from_content (GthFileData  *self,
					  GCancellable *cancellable)
{
	const char *content_type;

	if (self->info == NULL)
		return NULL;

	content_type = g_file_info_get_attribute_string (self->info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
	if (content_type == NULL) {
		GInputStream *istream;
		GError       *error = NULL;

		if (self->file == NULL)
			return NULL;

		istream = (GInputStream *) g_file_read (self->file, cancellable, &error);
		if (istream == NULL) {
			g_warning ("%s", error->message);
			g_clear_error (&error);
			return NULL;
		}

		content_type = _g_content_type_get_from_stream (istream, self->file, cancellable, &error);
		g_file_info_set_attribute_string (self->info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, content_type);

		g_object_unref (istream);
	}

	return _g_str_get_static (content_type);
}


const char *
gth_file_data_get_filename_sort_key (GthFileData *self)
{
	if (self->info == NULL)
		return NULL;

	if (self->priv->sort_key == NULL)
		self->priv->sort_key = g_utf8_collate_key_for_filename (g_file_info_get_display_name (self->info), -1);

	return self->priv->sort_key;
}


time_t
gth_file_data_get_mtime (GthFileData *self)
{
	g_file_info_get_modification_time (self->info, &self->priv->mtime);
	return (time_t) self->priv->mtime.tv_sec;
}


GTimeVal *
gth_file_data_get_modification_time (GthFileData *self)
{
	g_file_info_get_modification_time (self->info, &self->priv->mtime);
	return &self->priv->mtime;
}


GTimeVal *
gth_file_data_get_creation_time (GthFileData *self)
{
	self->priv->ctime.tv_sec = g_file_info_get_attribute_uint64 (self->info, G_FILE_ATTRIBUTE_TIME_CREATED);
	self->priv->ctime.tv_usec = g_file_info_get_attribute_uint32 (self->info, G_FILE_ATTRIBUTE_TIME_CREATED_USEC);
	return &self->priv->ctime;
}


gboolean
gth_file_data_get_digitalization_time (GthFileData *self,
				       GTimeVal    *_time)
{
	int i;

	if (self->priv->dtime.tv_sec != 0) {
		*_time = self->priv->dtime;
		return TRUE;
	}

	for (i = 0; FileDataDigitalizationTags[i] != NULL; i++) {
		GthMetadata *m;

		m = (GthMetadata *) g_file_info_get_attribute_object (self->info, FileDataDigitalizationTags[i]);
		if (m == NULL)
			continue;

		if (! _g_time_val_from_exif_date (gth_metadata_get_raw (m), &self->priv->dtime))
			continue;

		*_time = self->priv->dtime;
		return TRUE;
	}

	return FALSE;
}


gboolean
gth_file_data_is_readable (GthFileData *self)
{
	return g_file_info_get_attribute_boolean (self->info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
}


void
gth_file_data_update_info (GthFileData *fd,
			   const char  *attributes)
{
	if (attributes == NULL)
		attributes = GFILE_STANDARD_ATTRIBUTES;
	if (fd->info != NULL)
		g_object_unref (fd->info);

	fd->info = g_file_query_info (fd->file, attributes, G_FILE_QUERY_INFO_NONE, NULL, NULL);

	if (fd->info == NULL)
		fd->info = g_file_info_new ();

	fd->priv->dtime.tv_sec = 0;
}


void
gth_file_data_update_mime_type (GthFileData *fd,
				gboolean     fast)
{
	gth_file_data_set_mime_type (fd, _g_file_query_mime_type (fd->file, fast || ! g_file_is_native (fd->file)));
}


void
gth_file_data_update_all (GthFileData *fd,
			  gboolean     fast)
{
	gth_file_data_update_info (fd, NULL);
	gth_file_data_update_mime_type (fd, fast);
}


GList*
gth_file_data_list_dup (GList *list)
{
	GList *result = NULL;
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		result = g_list_prepend (result, gth_file_data_dup (file_data));
	}

	return g_list_reverse (result);
}


GList*
gth_file_data_list_from_uri_list (GList *list)
{
	GList *result = NULL;
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		char  *uri = scan->data;
		GFile *file;

		file = g_file_new_for_uri (uri);
		result = g_list_prepend (result, gth_file_data_new (file, NULL));
		g_object_unref (file);
	}

	return g_list_reverse (result);
}


GList*
gth_file_data_list_to_uri_list (GList *list)
{
	GList *result = NULL;
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		GthFileData *file = scan->data;
		result = g_list_prepend (result, g_file_get_uri (file->file));
	}

	return g_list_reverse (result);
}


GList *
gth_file_data_list_to_file_list (GList *list)
{
	GList *result = NULL;
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		GthFileData *file = scan->data;
		result = g_list_prepend (result, g_file_dup (file->file));
	}

	return g_list_reverse (result);
}


GList *
gth_file_data_list_find_file (GList *list,
			      GFile *file)
{
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (g_file_equal (file_data->file, file))
			return scan;
	}

	return NULL;
}


GList *
gth_file_data_list_find_uri (GList      *list,
			     const char *uri)
{
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		GthFileData *file = scan->data;
		char    *file_uri;

		file_uri = g_file_get_uri (file->file);
		if (strcmp (file_uri, uri) == 0) {
			g_free (file_uri);
			return scan;
		}
		g_free (file_uri);
	}

	return NULL;
}


typedef struct {
	GthFileData     *file_data;
	GthFileDataFunc  ready_func;
	gpointer         user_data;
	GError          *error;
	guint            id;
} ReadyData;


static gboolean
exec_ready_func (gpointer user_data)
{
	ReadyData *data = user_data;

	g_source_remove (data->id);
	data->ready_func (data->file_data, data->error, data->user_data);

	_g_object_unref (data->file_data);
	g_free (data);

	return FALSE;
}


void
gth_file_data_ready_with_error (GthFileData     *file_data,
				GthFileDataFunc  ready_func,
				gpointer         user_data,
				GError          *error)
{
	ReadyData *data;

	data = g_new0 (ReadyData, 1);
	if (file_data != NULL)
		data->file_data = g_object_ref (file_data);
	data->ready_func = ready_func;
	data->user_data = user_data;
	data->error = error;
	data->id = g_idle_add (exec_ready_func, data);
}


char *
gth_file_data_get_attribute_as_string (GthFileData *file_data,
				       const char  *id)
{
	GObject *obj;
	char    *value = NULL;

	switch (g_file_info_get_attribute_type (file_data->info, id)) {
	case G_FILE_ATTRIBUTE_TYPE_OBJECT:
		obj = g_file_info_get_attribute_object (file_data->info, id);
		if (GTH_IS_METADATA (obj)) {
			switch (gth_metadata_get_data_type (GTH_METADATA (obj))) {
			case GTH_METADATA_TYPE_STRING:
				if (strcmp (id, "general::rating") == 0) {
					int n = atoi (gth_metadata_get_formatted (GTH_METADATA (obj)));
					if ((n >= 0) && (n <= 5)) {
						GString *str = g_string_new ("");
						int      i;
						for (i = 1; i <= n; i++)
							g_string_append (str, "⭐");
						value = g_string_free (str, FALSE);
					}
				}
				if (value == NULL)
					value = g_strdup (gth_metadata_get_formatted (GTH_METADATA (obj)));
				break;
			case GTH_METADATA_TYPE_STRING_LIST:
				value = gth_string_list_join (GTH_STRING_LIST (gth_metadata_get_string_list (GTH_METADATA (obj))), " ");
				break;
			}
		}
		else if (GTH_IS_STRING_LIST (obj))
			value = gth_string_list_join (GTH_STRING_LIST (obj), " ");
		else
			value = g_file_info_get_attribute_as_string (file_data->info, id);
		break;
	default:
		value = g_file_info_get_attribute_as_string (file_data->info, id);
		break;
	}

	return value;
}


GFileInfo *
gth_file_data_list_get_common_info (GList      *file_data_list,
				    const char *attribtues)
{
	GFileInfo  *info;
	char      **attributes_v;
	int         i;

	info = g_file_info_new ();

	if (file_data_list == NULL)
		return info;

	g_file_info_copy_into (((GthFileData *) file_data_list->data)->info, info);

	attributes_v = g_strsplit (attribtues, ",", -1);
	for (i = 0; attributes_v[i] != NULL; i++) {
		char  *attribute = attributes_v[i];
		char  *first_value;
		GList *scan;

		first_value = gth_file_data_get_attribute_as_string ((GthFileData *) file_data_list->data, attribute);
		for (scan = file_data_list->next; (first_value != NULL) && scan; scan = scan->next) {
			GthFileData *file_data = scan->data;
			char        *value;

			value = gth_file_data_get_attribute_as_string (file_data, attribute);
			if (g_strcmp0 (first_value, value) != 0) {
				g_free (first_value);
				first_value = NULL;
			}

			g_free (value);
		}

		if (first_value == NULL)
			g_file_info_remove_attribute (info, attribute);

		g_free (first_value);
	}

	g_strfreev (attributes_v);

	return info;
}


gboolean
gth_file_data_attribute_equal (GthFileData *file_data,
			       const char  *attribute,
			       const char  *value)
{
	char     *v;
	gboolean  result;

	/* a NULL pointer is equal to an empty string. */

	if (g_strcmp0 (value, "") == 0)
		value = NULL;

	v = gth_file_data_get_attribute_as_string (file_data, attribute);
	if (g_strcmp0 (v, "") == 0) {
		g_free (v);
		v = NULL;
	}

	result = g_strcmp0 (v, value) == 0;

	g_free (v);

	return result;
}


gboolean
gth_file_data_attribute_equal_int (GthFileData *file_data,
				   const char  *attribute,
				   const char  *value)
{
	char     *v;
	gboolean  result;

	/* a NULL pointer or an empty string is equal to 0. */

	if ((g_strcmp0 (value, "") == 0) || (value == NULL))
		value = "0";

	v = gth_file_data_get_attribute_as_string (file_data, attribute);
	if ((g_strcmp0 (v, "") == 0) || (v == NULL)) {
		g_free (v);
		v = g_strdup ("0");
	}

	result = g_strcmp0 (v, value) == 0;

	g_free (v);

	return result;
}


gboolean
gth_file_data_attribute_equal_string_list (GthFileData    *file_data,
					   const char     *attribute,
					   GthStringList  *value)
{
	GthStringList *list;
	GObject       *obj;

	list = NULL;
	obj = g_file_info_get_attribute_object (file_data->info, attribute);
	if (GTH_IS_METADATA (obj))
		list = gth_metadata_get_string_list (GTH_METADATA (obj));
	else if (GTH_IS_STRING_LIST (obj))
		list = (GthStringList *) obj;

	return gth_string_list_equal (list, value);
}
