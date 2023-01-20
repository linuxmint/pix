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

#ifndef GTH_PROGRESS_DIALOG_H
#define GTH_PROGRESS_DIALOG_H

#include <gtk/gtk.h>
#include "gth-task.h"

G_BEGIN_DECLS

#define GTH_TYPE_PROGRESS_DIALOG            (gth_progress_dialog_get_type ())
#define GTH_PROGRESS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_PROGRESS_DIALOG, GthProgressDialog))
#define GTH_PROGRESS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_PROGRESS_DIALOG, GthProgressDialogClass))
#define GTH_IS_PROGRESS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_PROGRESS_DIALOG))
#define GTH_IS_PROGRESS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_PROGRESS_DIALOG))
#define GTH_PROGRESS_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_PROGRESS_DIALOG, GthProgressDialogClass))

typedef struct _GthProgressDialog GthProgressDialog;
typedef struct _GthProgressDialogClass GthProgressDialogClass;
typedef struct _GthProgressDialogPrivate GthProgressDialogPrivate;

struct _GthProgressDialog {
	GtkDialog parent_instance;
	GthProgressDialogPrivate *priv;
};

struct _GthProgressDialogClass {
	GtkDialogClass parent_class;
};

GType          gth_progress_dialog_get_type		(void);
GtkWidget *    gth_progress_dialog_new			(GtkWindow		*parent);
void           gth_progress_dialog_destroy_with_tasks	(GthProgressDialog	*dialog,
							 gboolean		 value);
void           gth_progress_dialog_add_task		(GthProgressDialog	*dialog,
							 GthTask		*task,
							 GthTaskFlags		 flags);

GtkWidget *    gth_task_progress_new			(GthTask		*task);

G_END_DECLS

#endif /* GTH_PROGRESS_DIALOG_H */
