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

#ifndef GTH_TASK_H
#define GTH_TASK_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TASK_ERROR gth_task_error_quark ()

typedef enum {
	GTH_TASK_FLAGS_SHOW_ERROR = 0,
	GTH_TASK_FLAGS_IGNORE_ERROR = (1 << 0),
	GTH_TASK_FLAGS_FOREGROUND = (1 << 1)
} GthTaskFlags;

#define GTH_TASK_FLAGS_DEFAULT GTH_TASK_FLAGS_SHOW_ERROR

typedef enum {
	GTH_TASK_ERROR_FAILED,
	GTH_TASK_ERROR_CANCELLED,
	GTH_TASK_ERROR_SKIP_TO_NEXT_FILE,
} GthTaskErrorEnum;

#define GTH_TYPE_TASK         (gth_task_get_type ())
#define GTH_TASK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_TASK, GthTask))
#define GTH_TASK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_TASK, GthTaskClass))
#define GTH_IS_TASK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_TASK))
#define GTH_IS_TASK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_TASK))
#define GTH_TASK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_TASK, GthTaskClass))

typedef struct _GthTask         GthTask;
typedef struct _GthTaskPrivate  GthTaskPrivate;
typedef struct _GthTaskClass    GthTaskClass;

struct _GthTask
{
	GObject __parent;
	GthTaskPrivate *priv;
};

struct _GthTaskClass
{
	GObjectClass __parent_class;

	/*< signals >*/

	void  (*completed)    (GthTask    *task,
			       GError     *error);
	void  (*progress)     (GthTask    *task,
			       const char *description,
			       const char *details,
			       gboolean    pulse,
			       double      fraction);
	void  (*dialog)       (GthTask    *task,
			       gboolean    opened,
			       GtkWidget  *dialog);

	/*< virtual functions >*/

	void  (*exec)         (GthTask    *task);
	void  (*cancelled)    (GthTask    *task);
};

GQuark          gth_task_error_quark     (void);

GType           gth_task_get_type        (void) G_GNUC_CONST;
void            gth_task_exec            (GthTask      *task,
					  GCancellable *cancellable);
gboolean        gth_task_is_running      (GthTask      *task);
void            gth_task_cancel          (GthTask      *task);
void	 	gth_task_set_cancellable (GthTask      *task,
					  GCancellable *cancellable);
GCancellable *  gth_task_get_cancellable (GthTask      *task);
void            gth_task_completed       (GthTask      *task,
					  GError       *error);
void            gth_task_dialog          (GthTask      *task,
					  gboolean      opened,
					  GtkWidget    *dialog);
void            gth_task_progress        (GthTask      *task,
					  const char   *description,
					  const char   *details,
					  gboolean      pulse,
					  double        fraction);
void		gth_task_set_for_viewer  (GthTask      *task,
					  gboolean      for_viewer);
gboolean	gth_task_get_for_viewer  (GthTask      *task);

G_END_DECLS

#endif /* GTH_TASK_H */
