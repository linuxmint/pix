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

#ifndef GTH_IMAGE_NAVIGATOR_H
#define GTH_IMAGE_NAVIGATOR_H

#include <gtk/gtk.h>
#include "gth-image-viewer.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_NAVIGATOR            (gth_image_navigator_get_type ())
#define GTH_IMAGE_NAVIGATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_NAVIGATOR, GthImageNavigator))
#define GTH_IMAGE_NAVIGATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_NAVIGATOR, GthImageNavigatorClass))
#define GTH_IS_IMAGE_NAVIGATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_NAVIGATOR))
#define GTH_IS_IMAGE_NAVIGATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_NAVIGATOR))
#define GTH_IMAGE_NAVIGATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_NAVIGATOR, GthImageNavigatorClass))

typedef struct _GthImageNavigator        GthImageNavigator;
typedef struct _GthImageNavigatorClass   GthImageNavigatorClass;
typedef struct _GthImageNavigatorPrivate GthImageNavigatorPrivate;

struct _GthImageNavigator {
	GtkContainer __parent;
	GthImageNavigatorPrivate *priv;
};

struct _GthImageNavigatorClass {
	GtkContainerClass __parent;
};

GType        gth_image_navigator_get_type                 (void);
GtkWidget *  gth_image_navigator_new                      (GthImageViewer    *viewer);
void         gth_image_navigator_set_automatic_scrollbars (GthImageNavigator *window,
						           gboolean           automatic);

G_END_DECLS

#endif /* GTH_IMAGE_NAVIGATOR_H */
