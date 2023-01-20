/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006-2009 The Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_HISTORY_H
#define GTH_IMAGE_HISTORY_H

#include <glib.h>
#include <glib-object.h>
#include <cairo.h>
#include "gth-image.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_HISTORY            (gth_image_history_get_type ())
#define GTH_IMAGE_HISTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_HISTORY, GthImageHistory))
#define GTH_IMAGE_HISTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_HISTORY, GthImageHistoryClass))
#define GTH_IS_IMAGE_HISTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_HISTORY))
#define GTH_IS_IMAGE_HISTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_HISTORY))
#define GTH_IMAGE_HISTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_HISTORY, GthImageHistoryClass))

typedef struct _GthImageHistory        GthImageHistory;
typedef struct _GthImageHistoryPrivate GthImageHistoryPrivate;
typedef struct _GthImageHistoryClass   GthImageHistoryClass;

typedef struct {
	int       ref;
	GthImage *image;
	int       requested_size;
	gboolean  unsaved;
} GthImageData;

struct _GthImageHistory {
	GObject __parent;
	GthImageHistoryPrivate *priv;
};

struct _GthImageHistoryClass {
	GObjectClass __parent;

	/* -- signals -- */

	void (*changed) (GthImageHistory *image_history);
};

GthImageData *    gth_image_data_new		(GthImage        *image,
						 int              requested_size,
						 gboolean         unsaved);
GthImageData *    gth_image_data_ref		(GthImageData    *idata);
void              gth_image_data_unref		(GthImageData    *idata);
void              gth_image_data_list_free	(GList           *list);

GType             gth_image_history_get_type	(void);
GthImageHistory * gth_image_history_new		(void);
void              gth_image_history_add_image	(GthImageHistory *history,
						 GthImage	 *image,
						 int              requested_size,
						 gboolean         unsaved);
void              gth_image_history_add_surface	(GthImageHistory *history,
						 cairo_surface_t *image,
						 int              requested_size,
						 gboolean         unsaved);
GthImageData *    gth_image_history_undo	(GthImageHistory *history);
GthImageData *    gth_image_history_redo	(GthImageHistory *history);
void              gth_image_history_clear	(GthImageHistory *history);
gboolean          gth_image_history_can_undo	(GthImageHistory *history);
gboolean          gth_image_history_can_redo	(GthImageHistory *history);
GthImageData *    gth_image_history_revert	(GthImageHistory *history);
GthImageData *    gth_image_history_get_last	(GthImageHistory *history);

G_END_DECLS

#endif /* GTH_IMAGE_HISTORY_H */

