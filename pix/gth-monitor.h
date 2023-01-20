/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2008 Free Software Foundation, Inc.
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

#ifndef GTH_MONITOR_H
#define GTH_MONITOR_H

#include <gio/gio.h>
#include <glib-object.h>
#include "typedefs.h"
#include "gth-file-data.h"

G_BEGIN_DECLS

typedef enum {
	GTH_MONITOR_EVENT_CREATED = 0,
	GTH_MONITOR_EVENT_DELETED,     /* used when a file or folder is deleted from disk */
	GTH_MONITOR_EVENT_CHANGED,
	GTH_MONITOR_EVENT_REMOVED      /* used when a file is removed from a catalog */
} GthMonitorEvent;

#define GTH_TYPE_MONITOR              (gth_monitor_get_type ())
#define GTH_MONITOR(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_MONITOR, GthMonitor))
#define GTH_MONITOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_MONITOR, GthMonitorClass))
#define GTH_IS_MONITOR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_MONITOR))
#define GTH_IS_MONITOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_MONITOR))
#define GTH_MONITOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_MONITOR, GthMonitorClass))

typedef struct _GthMonitor            GthMonitor;
typedef struct _GthMonitorClass       GthMonitorClass;
typedef struct _GthMonitorPrivate     GthMonitorPrivate;

struct _GthMonitor
{
	GObject __parent;
	GthMonitorPrivate *priv;
};

struct _GthMonitorClass
{
	GObjectClass __parent_class;

	/*< signals >*/

	void   (*icon_theme_changed)      (GthMonitor      *monitor);
	void   (*bookmarks_changed)       (GthMonitor      *monitor);
	void   (*shortcuts_changed)       (GthMonitor      *monitor);
	void   (*filters_changed)         (GthMonitor      *monitor);
	void   (*tags_changed)            (GthMonitor      *monitor);
	void   (*folder_changed)          (GthMonitor      *monitor,
					   GFile           *parent,
					   GList           *list /* GFile list */,
					   int              position,
					   GthMonitorEvent  event);
	void   (*file_renamed)            (GthMonitor      *monitor,
					   GFile           *file,
					   GFile           *new_file);
	void   (*metadata_changed)        (GthMonitor      *monitor,
					   GthFileData     *file_data);
	void   (*emblems_changed)         (GthMonitor      *monitor,
					   GList           *list /* GFile list */);
	void   (*entry_points_changed)    (GthMonitor      *monitor);
	void   (*order_changed)           (GthMonitor      *monitor,
					   GFile           *file,
					   int             *new_order);
};

GType         gth_monitor_get_type                   (void);
GthMonitor *  gth_monitor_new                        (void);
void          gth_monitor_pause                      (GthMonitor      *monitor,
						      GFile           *file);
void          gth_monitor_resume                     (GthMonitor      *monitor,
						      GFile           *file);
void          gth_monitor_icon_theme_changed         (GthMonitor      *monitor);
void          gth_monitor_bookmarks_changed          (GthMonitor      *monitor);
void          gth_monitor_shortcuts_changed          (GthMonitor      *monitor);
void          gth_monitor_filters_changed            (GthMonitor      *monitor);
void          gth_monitor_tags_changed               (GthMonitor      *monitor);
void          gth_monitor_folder_changed             (GthMonitor      *monitor,
						      GFile           *parent,
						      GList           *list, /* GFile list */
						      GthMonitorEvent  event);
void          gth_monitor_files_created_with_pos     (GthMonitor      *monitor,
						      GFile           *parent,
						      GList           *list, /* GFile list */
						      int              position);
void          gth_monitor_file_renamed               (GthMonitor      *monitor,
						      GFile           *file,
						      GFile           *new_file);
void          gth_monitor_files_deleted		     (GthMonitor      *monitor,
						      GList           *list /* GFile list */);
void          gth_monitor_metadata_changed           (GthMonitor      *monitor,
						      GthFileData     *file_data);
void          gth_monitor_emblems_changed            (GthMonitor      *monitor,
						      GList           *list /* GFile list */);
void          gth_monitor_entry_points_changed       (GthMonitor      *monitor);
void          gth_monitor_order_changed              (GthMonitor      *monitor,
						      GFile           *file,
						      int             *new_order);

G_END_DECLS

#endif /* GTH_MONITOR_H */
