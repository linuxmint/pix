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

#ifndef GTH_FILE_DATA_H
#define GTH_FILE_DATA_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include "gth-string-list.h"

G_BEGIN_DECLS

extern const char *FileDataDigitalizationTags[];

#define GTH_TYPE_FILE_DATA (gth_file_data_get_type ())
#define GTH_FILE_DATA(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_DATA, GthFileData))
#define GTH_FILE_DATA_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FILE_DATA, GthFileDataClass))
#define GTH_IS_FILE_DATA(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_DATA))
#define GTH_IS_FILE_DATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FILE_DATA))
#define GTH_FILE_DATA_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_FILE_DATA, GthFileDataClass))

typedef struct _GthFileData GthFileData;
typedef struct _GthFileDataClass GthFileDataClass;
typedef struct _GthFileDataPrivate GthFileDataPrivate;

struct _GthFileData {
	GObject    parent_instance;
	GFile     *file;
	GFileInfo *info;
	GthFileDataPrivate *priv;
};

struct _GthFileDataClass {
	GObjectClass parent_class;
};

typedef int  (*GthFileDataCompFunc) (GthFileData *a, GthFileData *b);
typedef void (*GthFileDataFunc)     (GthFileData *a, GError *error, gpointer data);

typedef struct {
	const char          *name;
	const char          *display_name;
	const char          *required_attributes;
	GthFileDataCompFunc  cmp_func;
} GthFileDataSort;

GType         gth_file_data_get_type                (void);
GthFileData * gth_file_data_new                     (GFile          *file,
						     GFileInfo      *info);
GthFileData * gth_file_data_new_for_uri             (const char     *uri,
						     const char     *mime_type);
GthFileData * gth_file_data_dup                     (GthFileData    *self);
void          gth_file_data_set_file                (GthFileData    *self,
						     GFile          *file);
void          gth_file_data_set_info                (GthFileData    *self,
						     GFileInfo      *info);
void          gth_file_data_set_mime_type           (GthFileData    *self,
						     const char     *mime_type);
const char *  gth_file_data_get_mime_type           (GthFileData    *self);
const char *  gth_file_data_get_mime_type_from_content
						    (GthFileData    *self,
						     GCancellable   *cancellable);
const char *  gth_file_data_get_filename_sort_key   (GthFileData    *self);
time_t        gth_file_data_get_mtime               (GthFileData    *self);
GTimeVal *    gth_file_data_get_modification_time   (GthFileData    *self);
GTimeVal *    gth_file_data_get_creation_time       (GthFileData    *self);
gboolean      gth_file_data_get_digitalization_time (GthFileData    *self,
				                     GTimeVal       *_time);
gboolean      gth_file_data_is_readable             (GthFileData    *self);
void          gth_file_data_update_info             (GthFileData    *self,
						     const char     *attributes);
void          gth_file_data_update_mime_type        (GthFileData    *self,
						     gboolean        fast);
void          gth_file_data_update_all              (GthFileData    *self,
						     gboolean        fast);

GList*        gth_file_data_list_dup                (GList          *list);
GList*        gth_file_data_list_from_uri_list      (GList          *list);
GList*        gth_file_data_list_to_uri_list        (GList          *list);
GList*        gth_file_data_list_to_file_list       (GList          *list);
GList*        gth_file_data_list_find_file          (GList          *list,
						     GFile          *file);
GList*        gth_file_data_list_find_uri           (GList          *list,
						     const char     *uri);

void          gth_file_data_ready_with_error        (GthFileData    *file_data,
						     GthFileDataFunc ready_func,
						     gpointer        ready_data,
						     GError         *error);
char *        gth_file_data_get_attribute_as_string (GthFileData    *file_data,
				                     const char     *id);
GFileInfo *   gth_file_data_list_get_common_info    (GList          *file_data_list,
						     const char     *attribtues);
gboolean      gth_file_data_attribute_equal         (GthFileData    *file_data,
						     const char     *attribute,
						     const char     *value);
gboolean      gth_file_data_attribute_equal_int     (GthFileData    *file_data,
				   	   	     const char     *attribute,
				   	   	     const char     *value);
gboolean      gth_file_data_attribute_equal_string_list
						    (GthFileData    *file_data,
						     const char     *attribute,
						     GthStringList  *value);

G_END_DECLS

#endif /* GTH_FILE_DATA_H */
