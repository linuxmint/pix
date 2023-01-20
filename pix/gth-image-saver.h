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

#ifndef GTH_IMAGE_SAVER_H
#define GTH_IMAGE_SAVER_H

#include <gtk/gtk.h>
#include "gth-file-data.h"
#include "gth-image.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_SAVER              (gth_image_saver_get_type ())
#define GTH_IMAGE_SAVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_SAVER, GthImageSaver))
#define GTH_IMAGE_SAVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_SAVER, GthImageSaverClass))
#define GTH_IS_IMAGE_SAVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_SAVER))
#define GTH_IS_IMAGE_SAVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_SAVER))
#define GTH_IMAGE_SAVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_SAVER, GthImageSaverClass))

typedef struct _GthImageSaver         GthImageSaver;
typedef struct _GthImageSaverClass    GthImageSaverClass;
typedef struct _GthImageSaverPrivate  GthImageSaverPrivate;

struct _GthImageSaver
{
	GObject __parent;
	GthImageSaverPrivate *priv;
};

struct _GthImageSaverClass
{
	GObjectClass __parent_class;

	/*< class attributes >*/

	const char *id;
	const char *display_name;
	const char *mime_type;
	const char *extensions;

	/*< virtual functions >*/

	const char * (*get_default_ext) (GthImageSaver   *self);
	GtkWidget *  (*get_control)     (GthImageSaver   *self);
	void         (*save_options)    (GthImageSaver   *self);
	gboolean     (*can_save)        (GthImageSaver   *self,
				         const char      *mime_type);
	gboolean     (*save_image)      (GthImageSaver   *self,
					 GthImage        *image,
				         char           **buffer,
				         gsize           *buffer_size,
				         const char      *mime_type,
				         GCancellable    *cancellable,
				         GError         **error);
};

typedef struct {
	GFile *file;
	void  *buffer;
	gsize  buffer_size;
} GthImageSaveFile;


typedef struct {
	GthImage     *image;
	GthFileData  *file_data;
	const char   *mime_type;
	gboolean      replace;
	GCancellable *cancellable;
	void         *buffer;
	gsize         buffer_size;
	GList        *files; 		/* GthImageSaveFile list */
	GError      **error;
} GthImageSaveData;


GType         gth_image_saver_get_type          (void);
const char *  gth_image_saver_get_id            (GthImageSaver    *self);
const char *  gth_image_saver_get_display_name  (GthImageSaver    *self);
const char *  gth_image_saver_get_mime_type     (GthImageSaver    *self);
const char *  gth_image_saver_get_extensions    (GthImageSaver    *self);
const char *  gth_image_saver_get_default_ext   (GthImageSaver    *self);
GtkWidget *   gth_image_saver_get_control       (GthImageSaver    *self);
void          gth_image_saver_save_options      (GthImageSaver    *self);
gboolean      gth_image_saver_can_save          (GthImageSaver    *self,
					         const char       *mime_type);
gboolean      gth_image_save_to_buffer          (GthImage         *image,
						 const char       *mime_type,
						 GthFileData      *file_data,
						 char            **buffer,
						 gsize            *buffer_size,
						 GCancellable     *cancellable,
						 GError          **error);
void          gth_image_save_to_file            (GthImage         *image,
						 const char       *mime_type,
						 GthFileData      *file_data,
						 gboolean          replace,
						 GCancellable     *cancellable,
						 GthFileDataFunc   ready_func,
						 gpointer          user_data);

G_END_DECLS

#endif /* GTH_IMAGE_SAVER_H */
