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

#ifndef GTH_IMAGE_PRINT_JOB_H
#define GTH_IMAGE_PRINT_JOB_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_PRINT_JOB            (gth_image_print_job_get_type ())
#define GTH_IMAGE_PRINT_JOB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_PRINT_JOB, GthImagePrintJob))
#define GTH_IMAGE_PRINT_JOB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_PRINT_JOB, GthImagePrintJobClass))
#define GTH_IS_IMAGE_PRINT_JOB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_PRINT_JOB))
#define GTH_IS_IMAGE_PRINT_JOB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_PRINT_JOB))
#define GTH_IMAGE_PRINT_JOB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_IMAGE_PRINT_JOB, GthImagePrintJobClass))

typedef struct _GthImagePrintJob        GthImagePrintJob;
typedef struct _GthImagePrintJobClass   GthImagePrintJobClass;
typedef struct _GthImagePrintJobPrivate GthImagePrintJobPrivate;

struct _GthImagePrintJob {
	GObject parent_instance;
	GthImagePrintJobPrivate *priv;
};

struct _GthImagePrintJobClass {
	GObjectClass parent_class;
};

GType              gth_image_print_job_get_type (void);
GthImagePrintJob * gth_image_print_job_new      (GList                    *file_data_list,
						 GthFileData              *current,
						 cairo_surface_t          *current_image,
						 const char               *event_name,
						 GError                  **error);
void               gth_image_print_job_run      (GthImagePrintJob         *self,
						 GtkPrintOperationAction   action,
						 GthBrowser               *browser);

G_END_DECLS

#endif /* GTH_IMAGE_PRINT_JOB_H */
