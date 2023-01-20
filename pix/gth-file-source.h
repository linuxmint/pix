/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2009 Free Software Foundation, Inc.
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

#ifndef GTH_FILE_SOURCE_H
#define GTH_FILE_SOURCE_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gio-utils.h"
#include "gth-file-data.h"
#include "typedefs.h"

G_BEGIN_DECLS

#define GTH_TYPE_FILE_SOURCE         (gth_file_source_get_type ())
#define GTH_FILE_SOURCE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_FILE_SOURCE, GthFileSource))
#define GTH_FILE_SOURCE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_FILE_SOURCE, GthFileSourceClass))
#define GTH_IS_FILE_SOURCE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_FILE_SOURCE))
#define GTH_IS_FILE_SOURCE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_FILE_SOURCE))
#define GTH_FILE_SOURCE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_FILE_SOURCE, GthFileSourceClass))

typedef struct _GthFileSource         GthFileSource;
typedef struct _GthFileSourcePrivate  GthFileSourcePrivate;
typedef struct _GthFileSourceClass    GthFileSourceClass;

typedef void (*ListReady)          (GthFileSource *file_source,
				    GList         *files,
				    GError        *error,
				    gpointer       data);
typedef void (*SpaceReadyCallback) (GthFileSource *file_source,
				    guint64        total_size,
				    guint64        free_space,
				    GError        *error,
				    gpointer       data);

struct _GthFileSource
{
	GObject __parent;
	GthFileSourcePrivate *priv;

	/*< protected >*/

	GList *folders; /* list of GthFileData */
	GList *files;   /* list of GthFileData */
};

struct _GthFileSourceClass
{
	GObjectClass __parent_class;

	/*< virtual functions >*/

	GList *      (*get_entry_points)      (GthFileSource        *file_source);
	GList *      (*get_current_list)      (GthFileSource        *file_source,
					       GFile                *file);
	GFile *      (*to_gio_file)           (GthFileSource        *file_source,
					       GFile                *file);
	GFileInfo *  (*get_file_info)         (GthFileSource        *file_source,
					       GFile                *file,
					       const char           *attributes);
	GthFileData *(*get_file_data)         (GthFileSource        *file_source,
					       GFile                *file,
					       GFileInfo            *info);
	void         (*write_metadata)        (GthFileSource        *file_source,
					       GthFileData          *file_data,
					       const char           *attributes,
					       ReadyCallback         callback,
       					       gpointer              data);
	void         (*read_metadata)         (GthFileSource        *file_source,
					       GthFileData          *file_data,
					       const char           *attributes,
					       ReadyCallback         callback,
					       gpointer              data);
	void         (*for_each_child)        (GthFileSource        *file_source,
					       GFile                *parent,
					       gboolean              recursive,
					       const char           *attributes,
					       StartDirCallback      dir_func,
					       ForEachChildCallback  child_func,
					       ReadyCallback         ready_func,
					       gpointer              data);
	void         (*rename)                (GthFileSource        *file_source,
					       GFile                *file,
					       const char           *edit_name,
					       ReadyCallback         callback,
					       gpointer              data);
	void         (*copy)                  (GthFileSource        *file_source,
					       GthFileData          *destination,
					       GList                *file_list, /* GFile list */
					       gboolean              move,
					       int                   destination_position,
					       ProgressCallback      progress_callback,
					       DialogCallback        dialog_callback,
					       ReadyCallback         callback,
					       gpointer              data);
	gboolean     (*can_cut)               (GthFileSource        *file_source);
	void         (*monitor_entry_points)  (GthFileSource        *file_source);
	void         (*monitor_directory)     (GthFileSource        *file_source,
					       GFile                *file,
					       gboolean              activate);
	gboolean     (*is_reorderable)        (GthFileSource        *file_source);
	void         (*reorder)               (GthFileSource        *file_source,
					       GthFileData          *destination,
					       GList                *visible_files, /* GFile list */
					       GList                *files_to_move, /* GFile list */
					       int                   dest_pos,
					       ReadyCallback         callback,
					       gpointer              data);
	void         (*remove)                (GthFileSource        *file_source,
					       GthFileData          *location,
					       GList                *file_list, /* GthFileData list */
					       gboolean              permanently,
					       GtkWindow            *parent);
	void         (*deleted_from_disk)     (GthFileSource        *file_source,
					       GthFileData          *location,
					       GList                *files /* GFile list */);
	void         (*get_free_space)        (GthFileSource        *file_source,
					       GFile                *location,
					       SpaceReadyCallback    callback,
					       gpointer              data);
	gboolean     (*shows_extra_widget)    (GthFileSource        *file_source);
	GdkDragAction(*get_drop_actions)      (GthFileSource        *file_source,
					       GFile                *destination,
					       GFile                *file);
};

GType          gth_file_source_get_type              (void) G_GNUC_CONST;

/*< public >*/

void           gth_file_source_set_cancellable       (GthFileSource        *file_source,
						      GCancellable         *cancellable);
GList *        gth_file_source_get_entry_points      (GthFileSource        *file_source); /* returns GthFileData list */
GList *        gth_file_source_get_current_list      (GthFileSource        *file_source,  /* GFile list */
						      GFile                *file);
GFile *        gth_file_source_to_gio_file           (GthFileSource        *file_source,
						      GFile                *file);
GList *        gth_file_source_to_gio_file_list      (GthFileSource        *file_source,
						      GList                *files);
GFileInfo *    gth_file_source_get_file_info         (GthFileSource        *file_source,
						      GFile                *file,
						      const char           *attributes);
GthFileData *  gth_file_source_get_file_data         (GthFileSource        *file_source,
					              GFile                *file,
					              GFileInfo            *info);
gboolean       gth_file_source_is_active             (GthFileSource        *file_source);
void           gth_file_source_cancel                (GthFileSource        *file_source);
void           gth_file_source_write_metadata        (GthFileSource        *file_source,
						      GthFileData          *file_data,
						      const char           *attributes,
						      ReadyCallback         callback,
						      gpointer              data);
void           gth_file_source_read_metadata         (GthFileSource        *file_source,
						      GthFileData          *file_data,
						      const char           *attributes,
						      ReadyCallback         callback,
						      gpointer              data);
void           gth_file_source_list                  (GthFileSource        *file_source,
						      GFile                *folder,
						      const char           *attributes,
						      ListReady             func,
						      gpointer              data);
void           gth_file_source_for_each_child        (GthFileSource        *file_source,
						      GFile                *parent,
						      gboolean              recursive,
						      const char           *attributes,
						      StartDirCallback      dir_func,
						      ForEachChildCallback  child_func,
						      ReadyCallback         ready_func,
						      gpointer              data);
void           gth_file_source_read_attributes       (GthFileSource        *file_source,
						      GList                *files,
						      const char           *attributes,
						      ListReady             func,
						      gpointer              data);
void           gth_file_source_rename                (GthFileSource        *file_source,
						      GFile                *file,
						      const char           *edit_name,
						      ReadyCallback         ready_callback,
						      gpointer              data);
void           gth_file_source_copy                  (GthFileSource        *file_source,
						      GthFileData          *destination,
						      GList                *file_list, /* GFile list */
						      gboolean              move,
						      int                   destination_position,
						      ProgressCallback      progress_callback,
						      DialogCallback        dialog_callback,
						      ReadyCallback         ready_callback,
						      gpointer              data);
gboolean       gth_file_source_can_cut               (GthFileSource        *file_source);
void           gth_file_source_monitor_entry_points  (GthFileSource        *file_source);
void           gth_file_source_monitor_directory     (GthFileSource        *file_source,
						      GFile                *file,
						      gboolean              activate);
gboolean       gth_file_source_is_reorderable        (GthFileSource        *file_source);
void           gth_file_source_reorder               (GthFileSource        *file_source,
						      GthFileData          *destination,
						      GList                *visible_files, /* GFile list */
						      GList                *files_to_move, /* GFile list */
						      int                   dest_pos,
						      ReadyCallback         callback,
						      gpointer              data);
void           gth_file_source_remove                (GthFileSource        *file_source,
						      GthFileData          *location,
		       	       	       	       	      GList                *file_list /* GthFileData list */,
		       	       	       	       	      gboolean              permanently,
		       	       	       	       	      GtkWindow            *parent);
void           gth_file_source_deleted_from_disk     (GthFileSource        *file_source,
						      GthFileData          *location,
						      GList                *file_list /* GFile list */);
void           gth_file_source_get_free_space        (GthFileSource        *file_source,
						      GFile                *location,
						      SpaceReadyCallback    callback,
						      gpointer              data);
gboolean       gth_file_source_shows_extra_widget    (GthFileSource        *file_source);
GdkDragAction  gth_file_source_get_drop_actions      (GthFileSource        *file_source,
						      GFile                *destination,
						      GFile                *file);

/*< protected >*/

void           gth_file_source_add_scheme            (GthFileSource        *file_source,
						      const char           *scheme);
gboolean       gth_file_source_supports_scheme       (GthFileSource        *file_source,
						      const char           *uri);
void           gth_file_source_set_active            (GthFileSource        *file_source,
						      gboolean              pending);
GCancellable * gth_file_source_get_cancellable       (GthFileSource        *file_source);

G_END_DECLS

#endif /* GTH_FILE_SOURCE_H */
