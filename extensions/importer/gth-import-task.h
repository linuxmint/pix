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

#ifndef GTH_IMPORT_TASK_H
#define GTH_IMPORT_TASK_H

#include <glib.h>
#include <gthumb.h>
#include "utils.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMPORT_TASK            (gth_import_task_get_type ())
#define GTH_IMPORT_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMPORT_TASK, GthImportTask))
#define GTH_IMPORT_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMPORT_TASK, GthImportTaskClass))
#define GTH_IS_IMPORT_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMPORT_TASK))
#define GTH_IS_IMPORT_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMPORT_TASK))
#define GTH_IMPORT_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMPORT_TASK, GthImportTaskClass))

typedef struct _GthImportTask        GthImportTask;
typedef struct _GthImportTaskClass   GthImportTaskClass;
typedef struct _GthImportTaskPrivate GthImportTaskPrivate;

struct _GthImportTask {
	GthTask __parent;
	GthImportTaskPrivate *priv;
};

struct _GthImportTaskClass {
	GthTaskClass __parent;
};

GType       gth_import_task_get_type         (void);
GthTask *   gth_import_task_new              (GthBrowser         *browser,
					      GList              *files, /* GthFileData list */
					      GFile              *destination,
					      GthSubfolderType    subfolder_type,
					      GthSubfolderFormat  subfolder_format,
					      gboolean            single_subfolder,
					      const char         *custom_format,
					      const char         *event_name,
					      char              **tags,
					      gboolean            delete_imported,
					      gboolean            overwrite_files,
					      gboolean            adjust_orientation);
gboolean    gth_import_task_check_free_space (GFile              *destination,
					      GList              *files, /* GthFileData list */
					      GError            **error);

G_END_DECLS

#endif /* GTH_IMPORT_TASK_H */
